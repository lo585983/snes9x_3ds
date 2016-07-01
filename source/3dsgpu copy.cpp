#include <3ds.h>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <vector>
#include <string.h>
#include <stdio.h>


#define _3DSGPU_CPP_
#include "snes9x.h"
#include "memmap.h"
#include "3dsgpu.h"

// Compiled shaders
//
#include "shader_citra_shbin.h"
#include "shader_3ds_shbin.h"



//int     vramCacheFrameNumber[MAX_HASH];                       


//int currentRenderTarget = 0;
bool somethingWasDrawn = false;
bool somethingWasFlushed = false;

void cacheInit()
{
    //printf ("Cache %8x\n", &vramCacheFrameNumber);
    //memset(&vramCacheFrameNumber, 0, MAX_HASH * 4);
    memset(&GPU3DS.vramCacheHashToTexturePosition, 0, MAX_HASH * 2);
    memset(&GPU3DS.vramCacheTexturePositionToHash, 0, MAX_TEXTURE_POSITIONS * 4);
    GPU3DS.newCacheTexturePosition = 1;
}

int cacheGetTexturePosition(int hash)
{
    int pos = GPU3DS.vramCacheHashToTexturePosition[hash];
    
    if (pos == 0)
    {
        pos = GPU3DS.newCacheTexturePosition;
        
        //vramCacheFrameNumber[hash] = 0;
        
        GPU3DS.vramCacheTexturePositionToHash[GPU3DS.vramCacheHashToTexturePosition[hash]] = 0;
        GPU3DS.vramCacheHashToTexturePosition[GPU3DS.vramCacheTexturePositionToHash[pos]] = 0;
        
        GPU3DS.vramCacheHashToTexturePosition[hash] = pos;
        GPU3DS.vramCacheTexturePositionToHash[pos] = hash;
        
        GPU3DS.newCacheTexturePosition++;
        if (GPU3DS.newCacheTexturePosition > MAX_TEXTURE_POSITIONS)
            GPU3DS.newCacheTexturePosition = 1;
    }
    
    return pos;
}



// Memory Usage = 2.00 MB   for texture cache
#define TEXTURE_SIZE            1024

// Memory Usage = 2.00 MB   for 2 x GPU command buffers
#define COMMAND_BUFFER_SIZE     0x100000  

// Memory Usage = 0.20 MB   for 4-point rectangle (triangle strip) vertex buffer
#define RECTANGLE_BUFFER_SIZE   0x100000

// Memory Usage = 18.00 MB   for 3-point triangle vertex buffer (Citra only)
#define CITRA_VERTEX_BUFFER_SIZE      0x1000000
#define CITRA_TILE_BUFFER_SIZE        0x200

// Memory Usage = 0.06 MB   for 3-point triangle vertex buffer (Real 3DS only)
#define REAL3DS_VERTEX_BUFFER_SIZE      0x80000

// Memory Usage = 4.00 MB   for 2-point rectangle vertex buffer (Real 3DS only)
#define REAL3DS_TILE_BUFFER_SIZE        0x400000

/*
sf2d_rendertarget *snesMainScreenTarget;
sf2d_rendertarget *snesSubScreenTarget;
sf2d_texture *snesTileCacheTexture;
*/

SGPUTexture *snesMainScreenTarget;
SGPUTexture *snesSubScreenTarget;
SGPUTexture *snesTileCacheTexture;



u32 *gpuCommandBuffer1;
u32 *gpuCommandBuffer2;
int gpuCommandBufferSize = 0;
int gpuCurrentCommandBuffer = 0;


void gpu3dsEnableDepthTest()
{
	GPU_SetDepthTestAndWriteMask(true, GPU_GEQUAL, GPU_WRITE_ALL);
}

void gpu3dsDisableDepthTest()
{
	GPU_SetDepthTestAndWriteMask(false, GPU_ALWAYS, GPU_WRITE_ALL);
}

void gpu3dsClearTextureEnv(u8 num)
{
	GPU_SetTexEnv(num,
		GPU_TEVSOURCES(GPU_PREVIOUS, 0, 0),
		GPU_TEVSOURCES(GPU_PREVIOUS, 0, 0),
		GPU_TEVOPERANDS(0,0,0),
		GPU_TEVOPERANDS(0,0,0),
		GPU_REPLACE,
		GPU_REPLACE,
		0x80808080);
}

void gpu3dsSetTextureEnvironmentReplaceColor()
{
	GPU_SetTexEnv(
		0,
		GPU_TEVSOURCES(GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR),
		GPU_TEVSOURCES(GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR),
		GPU_TEVOPERANDS(0, 0, 0),
		GPU_TEVOPERANDS(0, 0, 0),
		GPU_REPLACE, GPU_REPLACE,
		0x80808080
	);
    
	gpu3dsClearTextureEnv(1);
	//gpu3dsClearTextureEnv(2);
	//gpu3dsClearTextureEnv(3);
}

void gpu3dsSetTextureEnvironmentReplaceTexture0()
{
	GPU_SetTexEnv(
		0,
		GPU_TEVSOURCES(GPU_TEXTURE0, GPU_TEXTURE0, GPU_TEXTURE0),
		GPU_TEVSOURCES(GPU_TEXTURE0, GPU_TEXTURE0, GPU_TEXTURE0),
		GPU_TEVOPERANDS(0, 0, 0),
		GPU_TEVOPERANDS(0, 0, 0),
		GPU_REPLACE, GPU_REPLACE,
		0x80808080
	);

	gpu3dsClearTextureEnv(1);
	//gpu3dsClearTextureEnv(2);
	//gpu3dsClearTextureEnv(3);
}



static inline u32 gpu3dsMortonInterleave(u32 x, u32 y)
{
	u32 i = (x & 7) | ((y & 7) << 8); // ---- -210
	i = (i ^ (i << 2)) & 0x1313;      // ---2 --10
	i = (i ^ (i << 1)) & 0x1515;      // ---2 -1-0
	i = (i | (i >> 7)) & 0x3F;
	return i;
}

/*
void gpu3dsSetTexturePixel16(sf2d_texture *texture, int x, int y, u16 new_color)
{
	y = (texture->pow2_h - 1 - y);
	
    u32 coarse_y = y & ~7;
    u32 coarse_x = x & ~7;
    u32 offset = gpu3dsMortonInterleave(x, y) + 
        coarse_x * 8 +
        coarse_y * texture->pow2_w;
    ((u16 *)texture->data)[offset] = new_color;
}
*/

bool gpu3dsInitialize()
{
    // Initialize the 3DS screen
    //
    gfxInit	(GSP_RGBA8_OES, GSP_RGBA8_OES, false);
	GPU_Init(NULL);
	gfxSet3D(false);

    // Create the frame and depth buffers for the top screen.
    //
	GPU3DS.frameBuffer = (u32 *) vramMemAlign(400*240*8, 0x100);
	GPU3DS.frameDepthBuffer = (u32 *) vramMemAlign(400*240*8, 0x100);
    if (GPU3DS.frameBuffer == NULL || 
        GPU3DS.frameDepthBuffer == NULL)
        return false;

    // Initialize the bottom screen for console output.
    //  
    consoleInit(GFX_BOTTOM, NULL);
    
    // Create the command buffers
    //
    gpuCommandBufferSize = COMMAND_BUFFER_SIZE;
    gpuCommandBuffer1 = (u32 *)linearAlloc(COMMAND_BUFFER_SIZE);
    gpuCommandBuffer2 = (u32 *)linearAlloc(COMMAND_BUFFER_SIZE);
    if (gpuCommandBuffer1 == NULL || gpuCommandBuffer2 == NULL)
        return false;
	GPU_Reset(NULL, gpuCommandBuffer1, gpuCommandBufferSize);
    gpuCurrentCommandBuffer = 0;
    
    printf ("Buffer: %8x\n", (u32) gpuCommandBuffer1);
    //if ((u32)gpuCommandBuffer1 < 0x20000000)
        GPU3DS.isReal3DS = false;
    //else
    //    GPU3DS.isReal3DS = true;

    // Initialize the projection matrix for the top / bottom
    // screens
    //
	matrix_init_orthographic(GPU3DS.projectionTopScreen, 
        0.0f, 400.0f, 0.0f, 240.0f, 0.0f, 1.0f);
	matrix_init_orthographic(GPU3DS.projectionBottomScreen, 
        0.0f, 320.0f, 0.0f, 240.0f, 0.0f, 1.0f);

    //sf2d_init(true);
    //sf2d_set_vblank_wait(false);

	// Load up and initialize any shaders
	// 
    if (GPU3DS.isReal3DS)
    {
    	gpu3dsLoadShader(0, (u32 *)shader_3ds_shbin, shader_3ds_shbin_size, 0);
        printf ("Using shader_3ds_shbin shader\n");
    }
    else
    {
	    gpu3dsLoadShader(0, (u32 *)shader_citra_shbin, shader_citra_shbin_size, 0);
        printf ("Using shader_citra_shbin shader\n");
    }
	
    // Create all the necessary textures
    //
    snesTileCacheTexture = gpu3dsCreateTextureInLinearMemory(1024, 1024, GPU_RGBA5551);
    snesMainScreenTarget = gpu3dsCreateTextureInVRAM(256, 256, GPU_RGBA8);
    snesSubScreenTarget = gpu3dsCreateTextureInVRAM(256, 256, GPU_RGBA8);

    // Allocate all necessary buffers
    //
    if (GPU3DS.isReal3DS)
    {
        GPU3DS.vertexListBase = (SVertex *) linearAlloc(REAL3DS_VERTEX_BUFFER_SIZE);
        GPU3DS.tileListBase = (STileVertex *) linearAlloc(REAL3DS_TILE_BUFFER_SIZE);
        GPU3DS.rectangleVertexListBase = (SVertexColor *) linearAlloc(RECTANGLE_BUFFER_SIZE);
    }
    else
    { 
        GPU3DS.vertexListBase = (SVertex *) linearAlloc(CITRA_VERTEX_BUFFER_SIZE);
        GPU3DS.tileListBase = (STileVertex *) linearAlloc(CITRA_TILE_BUFFER_SIZE);
        GPU3DS.rectangleVertexListBase = (SVertexColor *) linearAlloc(RECTANGLE_BUFFER_SIZE);
    }
        
    if (GPU3DS.vertexListBase == NULL || 
        GPU3DS.tileListBase == NULL || 
        GPU3DS.rectangleVertexListBase == NULL)
        return false;
        
	GPUCMD_SetBufferOffset(0);
	gpu3dsUseShader(0);

	GPU_DepthMap(-1.0f, 0.0f);
	GPU_SetFaceCulling(GPU_CULL_NONE);
	GPU_SetStencilTest(false, GPU_ALWAYS, 0x00, 0xFF, 0x00);
	GPU_SetStencilOp(GPU_STENCIL_KEEP, GPU_STENCIL_KEEP, GPU_STENCIL_KEEP);
	GPU_SetBlendingColor(0,0,0,0);
	GPU_SetDepthTestAndWriteMask(false, GPU_GEQUAL, GPU_WRITE_ALL);
	GPUCMD_AddMaskedWrite(GPUREG_EARLYDEPTH_TEST1, 0x1, 0);
	GPUCMD_AddWrite(GPUREG_EARLYDEPTH_TEST2, 0);

	GPU_SetAlphaBlending(
		GPU_BLEND_ADD,
		GPU_BLEND_ADD,
		GPU_SRC_ALPHA, GPU_ONE_MINUS_SRC_ALPHA,
		GPU_ONE, GPU_ZERO
	);
	GPU_SetAlphaTest(true, GPU_NOTEQUAL, 0x00);

    gpu3dsSetTextureEnvironmentReplaceTexture0();
    
	GPUCMD_Finalize();
	GPUCMD_FlushAndRun();    
    gspWaitForP3D();         
    
    return true;
}



void gpu3dsCacheToTexturePosition(
    uint8 *snesTilePixels, 
	uint16 *snesPalette,
    uint16 texturePosition)
{
    int tx = texturePosition % 128;
    int ty = (texturePosition / 128) & 0x7f;
    texturePosition = (127 - ty) * 128 + tx;    // flip vertically.
    uint32 base = texturePosition * 64;
    
    uint16 *tileTexture = (uint16 *)snesTileCacheTexture->PixelData;
	
    #define GET_TILE_PIXEL(x)   (snesTilePixels[x] == 0 ? 0 : snesPalette[snesTilePixels[x]])
    tileTexture [base + 0] = GET_TILE_PIXEL(56); 
    tileTexture [base + 1] = GET_TILE_PIXEL(57); 
    tileTexture [base + 4] = GET_TILE_PIXEL(58); 
    tileTexture [base + 5] = GET_TILE_PIXEL(59); 
    tileTexture [base + 16] = GET_TILE_PIXEL(60); 
    tileTexture [base + 17] = GET_TILE_PIXEL(61); 
    tileTexture [base + 20] = GET_TILE_PIXEL(62); 
    tileTexture [base + 21] = GET_TILE_PIXEL(63); 
    
    tileTexture [base + 2] = GET_TILE_PIXEL(48); 
    tileTexture [base + 3] = GET_TILE_PIXEL(49); 
    tileTexture [base + 6] = GET_TILE_PIXEL(50); 
    tileTexture [base + 7] = GET_TILE_PIXEL(51); 
    tileTexture [base + 18] = GET_TILE_PIXEL(52); 
    tileTexture [base + 19] = GET_TILE_PIXEL(53); 
    tileTexture [base + 22] = GET_TILE_PIXEL(54); 
    tileTexture [base + 23] = GET_TILE_PIXEL(55);
     
    tileTexture [base + 8] = GET_TILE_PIXEL(40); 
    tileTexture [base + 9] = GET_TILE_PIXEL(41); 
    tileTexture [base + 12] = GET_TILE_PIXEL(42); 
    tileTexture [base + 13] = GET_TILE_PIXEL(43); 
    tileTexture [base + 24] = GET_TILE_PIXEL(44); 
    tileTexture [base + 25] = GET_TILE_PIXEL(45); 
    tileTexture [base + 28] = GET_TILE_PIXEL(46); 
    tileTexture [base + 29] = GET_TILE_PIXEL(47); 
    
    tileTexture [base + 10] = GET_TILE_PIXEL(32); 
    tileTexture [base + 11] = GET_TILE_PIXEL(33); 
    tileTexture [base + 14] = GET_TILE_PIXEL(34); 
    tileTexture [base + 15] = GET_TILE_PIXEL(35); 
    tileTexture [base + 26] = GET_TILE_PIXEL(36); 
    tileTexture [base + 27] = GET_TILE_PIXEL(37); 
    tileTexture [base + 30] = GET_TILE_PIXEL(38); 
    tileTexture [base + 31] = GET_TILE_PIXEL(39); 
    
    tileTexture [base + 32] = GET_TILE_PIXEL(24); 
    tileTexture [base + 33] = GET_TILE_PIXEL(25); 
    tileTexture [base + 36] = GET_TILE_PIXEL(26); 
    tileTexture [base + 37] = GET_TILE_PIXEL(27); 
    tileTexture [base + 48] = GET_TILE_PIXEL(28); 
    tileTexture [base + 49] = GET_TILE_PIXEL(29); 
    tileTexture [base + 52] = GET_TILE_PIXEL(30); 
    tileTexture [base + 53] = GET_TILE_PIXEL(31); 
    
    tileTexture [base + 34] = GET_TILE_PIXEL(16); 
    tileTexture [base + 35] = GET_TILE_PIXEL(17); 
    tileTexture [base + 38] = GET_TILE_PIXEL(18); 
    tileTexture [base + 39] = GET_TILE_PIXEL(19); 
    tileTexture [base + 50] = GET_TILE_PIXEL(20); 
    tileTexture [base + 51] = GET_TILE_PIXEL(21); 
    tileTexture [base + 54] = GET_TILE_PIXEL(22); 
    tileTexture [base + 55] = GET_TILE_PIXEL(23); 
    
    tileTexture [base + 40] = GET_TILE_PIXEL(8); 
    tileTexture [base + 41] = GET_TILE_PIXEL(9); 
    tileTexture [base + 44] = GET_TILE_PIXEL(10); 
    tileTexture [base + 45] = GET_TILE_PIXEL(11); 
    tileTexture [base + 56] = GET_TILE_PIXEL(12); 
    tileTexture [base + 57] = GET_TILE_PIXEL(13); 
    tileTexture [base + 60] = GET_TILE_PIXEL(14); 
    tileTexture [base + 61] = GET_TILE_PIXEL(15); 
    
    tileTexture [base + 42] = GET_TILE_PIXEL(0); 
    tileTexture [base + 43] = GET_TILE_PIXEL(1); 
    tileTexture [base + 46] = GET_TILE_PIXEL(2); 
    tileTexture [base + 47] = GET_TILE_PIXEL(3); 
    tileTexture [base + 58] = GET_TILE_PIXEL(4); 
    tileTexture [base + 59] = GET_TILE_PIXEL(5); 
    tileTexture [base + 62] = GET_TILE_PIXEL(6); 
    tileTexture [base + 63] = GET_TILE_PIXEL(7);   
    /*
    for (int i=0; i<16; i++)
        printf ("%4x ", snesPalette[i]);
    
    printf ("base: %d\n", base);
    for (int i = 0; i<64; i++)
    {
        printf ("%4x", tileTexture[base + i]);
        if (i % 8 == 7)
            printf ("\n");
    } */
}

/*
sf2d_texture *gpu3dsCreateTexture(int width, int height, sf2d_texfmt pixel_format, sf2d_place place)
{
    return sf2d_create_texture(width, height, pixel_format, place);
}
*/

int gpu3dsGetPixelSize(GPU_TEXCOLOR pixelFormat)
{
    if (pixelFormat == GPU_RGBA8)
        return 4;
    if (pixelFormat == GPU_RGB8)
        return 3;
    if (pixelFormat == GPU_RGBA5551)
        return 2;
    if (pixelFormat == GPU_RGB565)
        return 2;
    if (pixelFormat == GPU_RGBA4)
        return 2;
    return 0;
}


SGPUTexture *gpu3dsCreateTextureInLinearMemory(int width, int height, GPU_TEXCOLOR pixelFormat)
{
	int size = width * height * gpu3dsGetPixelSize(pixelFormat);
    if (size == 0)
        return NULL;

	void *data = linearMemAlign(size, 0x80);

	SGPUTexture *texture = (SGPUTexture *) malloc(sizeof(SGPUTexture));

	texture->Memory = 0;
	texture->PixelFormat = pixelFormat;
	texture->Params = GPU_TEXTURE_MAG_FILTER(GPU_NEAREST)
		| GPU_TEXTURE_MIN_FILTER(GPU_NEAREST)
		| GPU_TEXTURE_WRAP_S(GPU_CLAMP_TO_BORDER)
		| GPU_TEXTURE_WRAP_T(GPU_CLAMP_TO_BORDER);
	texture->Width = width;
	texture->Height = height;
	texture->PixelData = data;
    texture->BufferSize = size;
    texture->TextureScale[3] = 1.0f / texture->Width;
    texture->TextureScale[2] = 1.0f / texture->Height;
    texture->TextureScale[1] = 1.0f / texture->Width;
    texture->TextureScale[0] = 1.0f / texture->Height;

    memset(texture->PixelData, 0, size);

	return texture;
}

SGPUTexture *gpu3dsCreateTextureInVRAM(int width, int height, GPU_TEXCOLOR pixelFormat)
{
	int size = width * height * gpu3dsGetPixelSize(pixelFormat);
    if (size == 0)
        return NULL;

	void *data = vramMemAlign(size, 0x80);

	SGPUTexture *texture = (SGPUTexture *) malloc(sizeof(SGPUTexture));

	texture->Memory = 1;
	texture->PixelFormat = pixelFormat;
	texture->Params = GPU_TEXTURE_MAG_FILTER(GPU_NEAREST)
		| GPU_TEXTURE_MIN_FILTER(GPU_NEAREST)
		| GPU_TEXTURE_WRAP_S(GPU_CLAMP_TO_BORDER)
		| GPU_TEXTURE_WRAP_T(GPU_CLAMP_TO_BORDER);
	texture->Width = width;
	texture->Height = height;
	texture->PixelData = data;
    texture->BufferSize = size;
    texture->TextureScale[3] = 1.0f / texture->Width;
    texture->TextureScale[2] = 1.0f / texture->Height;
    texture->TextureScale[1] = 1.0f / texture->Width;
    texture->TextureScale[0] = 1.0f / texture->Height;

	matrix_init_orthographic(texture->Projection, 0.0f, width, height, 0.0f, 0.0f, 1.0f);
	matrix_rotate_z(texture->Projection, M_PI / 2.0f);

    GX_MemoryFill(
        (u32*)texture->PixelData, 0x00000000, 
        (u32*)&((u8*)texture->PixelData)[texture->BufferSize], 
        GX_FILL_TRIGGER | GX_FILL_32BIT_DEPTH,
        NULL, 0x00000000, NULL, 0);
    gspWaitForPSC0();
        printf ("clear: %x %d\n", texture->PixelData, texture->BufferSize);

	return texture;
}


// Clear the render targets by drawing black to them.
//
void gpu3dsClearAllRenderTargets()
{
    gpu3dsSetRenderTarget(1);
    gpu3dsDrawRectangle(0, 0, 256, 256, 0.1f, 0xff); 
    gpu3dsSetRenderTarget(2);
    gpu3dsDrawRectangle(0, 0, 256, 256, 0.1f, 0xff); 
    gpu3dsFlush();
    gpu3dsWaitForPreviousFlush();
}

void gpu3dsClearTexture(SGPUTexture *texture)
{
    //if (GPU3DS.targetDepthBuffer != NULL)
    /* 
    why doesn't this work in Citra??? It's the same as the 
    GX_MemoryFill above!
    {
        GX_MemoryFill(
            (u32*)texture->PixelData, 0x000000ff, 
            (u32*)&((u8*)texture->PixelData)[texture->BufferSize - 1], 
            GX_FILL_TRIGGER | GX_FILL_32BIT_DEPTH,
            NULL, 0x00000000, NULL, 0);
        gspWaitForPSC0();
        printf ("clear: %x %d\n", texture->PixelData, texture->BufferSize);
    } */ 
}


void gpu3dsStartNewFrame()
{
    //if (GPU3DS.enableDebug)
        //printf("  gpu3dsStartNewFrame\n");
    
    gpuCurrentCommandBuffer = 1 - gpuCurrentCommandBuffer;
    if (gpuCurrentCommandBuffer == 0)
    {
	    GPU_Reset(NULL, gpuCommandBuffer1, gpuCommandBufferSize);

        GPU3DS.vertexList = GPU3DS.vertexListBase;
        GPU3DS.vertexTotal = 0;
        GPU3DS.vertexCount = 0;
        
        GPU3DS.tileList = GPU3DS.tileListBase;
        GPU3DS.tileTotal = 0;
        GPU3DS.tileCount = 0;
        
        GPU3DS.rectangleVertexList = GPU3DS.rectangleVertexListBase;
    }
    else
    {
	    GPU_Reset(NULL, gpuCommandBuffer2, gpuCommandBufferSize);
        
        if (GPU3DS.isReal3DS)
        {
            GPU3DS.vertexList = (SVertex *)
                ((uint32)GPU3DS.vertexListBase + (REAL3DS_VERTEX_BUFFER_SIZE / 2));
            GPU3DS.vertexTotal = 0;
            GPU3DS.vertexCount = 0;
            
            GPU3DS.tileList = (STileVertex *)
                ((uint32)GPU3DS.tileListBase + (REAL3DS_TILE_BUFFER_SIZE / 2));
            GPU3DS.tileTotal = 0;
            GPU3DS.tileCount = 0;
        }
        else
        {
            GPU3DS.vertexList = (SVertex *)
                ((uint32)GPU3DS.vertexListBase + (CITRA_VERTEX_BUFFER_SIZE / 2));
            GPU3DS.vertexTotal = 0;
            GPU3DS.vertexCount = 0;
            
            GPU3DS.tileList = (STileVertex *)
                ((uint32)GPU3DS.tileListBase + (CITRA_TILE_BUFFER_SIZE / 2));
            GPU3DS.tileTotal = 0;
            GPU3DS.tileCount = 0;
        }

        GPU3DS.rectangleVertexList = (SVertexColor *)
            ((uint32)GPU3DS.rectangleVertexListBase + (RECTANGLE_BUFFER_SIZE / 2));
        
    }
}


//int currentShaderIndex = -1;

void gpu3dsUseShader(int shaderIndex)
{
    /*
    if (currentShaderIndex != shaderIndex)
    {
        sf2d_use_shader(shaderIndex);
        currentShaderIndex = shaderIndex;
    }
*/

    //if (GPU3DS.currentShader != shaderIndex)
    {
        printf ("Using shader = %d\n", shaderIndex);
        GPU3DS.currentShader = shaderIndex;
        shaderProgramUse(&GPU3DS.shaders[shaderIndex].shaderProgram);

        if (!GPU3DS.currentRenderTarget)
        {
	        GPU_SetFloatUniform(GPU_VERTEX_SHADER, 
                GPU3DS.shaders[shaderIndex].projectionRegister, 
                (u32 *)GPU3DS.projectionTopScreen, 4);
        }
        else
        {
	        GPU_SetFloatUniform(GPU_VERTEX_SHADER, 
                GPU3DS.shaders[shaderIndex].projectionRegister, 
                (u32 *)GPU3DS.currentRenderTarget->Projection, 4);
        }

        if (GPU3DS.currentTexture != NULL)
        {
            /*
            GPU_SetFloatUniform(GPU_VERTEX_SHADER, 
                GPU3DS.shaders[shaderIndex].textureScaleRegister, 
                (u32 *)GPU3DS.currentTexture->TextureScale, 1);*/
        }
        
    }
}


void gpu3dsLoadShader(int shaderIndex, u32 *shaderBinary, 
    int size, int geometryShaderStride)
{
	GPU3DS.shaders[shaderIndex].dvlb = DVLB_ParseFile((u32 *)shaderBinary, size);
	shaderProgramInit(&GPU3DS.shaders[shaderIndex].shaderProgram);
	shaderProgramSetVsh(&GPU3DS.shaders[shaderIndex].shaderProgram, 
        &GPU3DS.shaders[shaderIndex].dvlb->DVLE[0]);
	
	if (geometryShaderStride)
		shaderProgramSetGsh(&GPU3DS.shaders[shaderIndex].shaderProgram, 
			&GPU3DS.shaders[shaderIndex].dvlb->DVLE[1], geometryShaderStride);
		
	GPU3DS.shaders[shaderIndex].projectionRegister = 
		shaderInstanceGetUniformLocation(GPU3DS.shaders[shaderIndex].shaderProgram.vertexShader, 
        "projection");
	
	GPU3DS.shaders[shaderIndex].textureScaleRegister = 
		shaderInstanceGetUniformLocation(GPU3DS.shaders[shaderIndex].shaderProgram.vertexShader, 
        "textureScale");
}

void gpu3dsEnableAlphaBlending()
{
	GPU_SetAlphaBlending(
		GPU_BLEND_ADD,
		GPU_BLEND_ADD,
		GPU_SRC_ALPHA, GPU_ONE_MINUS_SRC_ALPHA,
		GPU_ONE, GPU_ZERO
	);
}

void gpu3dsDisableAlphaBlending()
{
	GPU_SetAlphaBlending(
		GPU_BLEND_ADD,
		GPU_BLEND_ADD,
		GPU_ONE, GPU_ZERO,
		GPU_ONE, GPU_ZERO
	);
}

void gpu3dsEnableAdditiveBlending()
{
	GPU_SetAlphaBlending(
		GPU_BLEND_ADD,
		GPU_BLEND_ADD,
		GPU_ONE, GPU_ONE,
		GPU_ONE, GPU_ZERO
	);
}

void gpu3dsEnableSubtractiveBlending()
{
	GPU_SetAlphaBlending(
		GPU_BLEND_REVERSE_SUBTRACT,
		GPU_BLEND_ADD,
		GPU_ONE, GPU_ONE,
		GPU_ONE, GPU_ZERO
	);
}

void gpu3dsEnableAdditiveDiv2Blending()
{
    GPU_SetBlendingColor(0, 0, 0, 0x80);
	GPU_SetAlphaBlending(
		GPU_BLEND_ADD,
		GPU_BLEND_ADD,
		GPU_CONSTANT_ALPHA, GPU_CONSTANT_ALPHA,
		GPU_ONE, GPU_ZERO
	);
}

void gpu3dsEnableSubtractiveDiv2Blending()
{
    GPU_SetBlendingColor(0, 0, 0, 0x80);
	GPU_SetAlphaBlending(
		GPU_BLEND_REVERSE_SUBTRACT,
		GPU_BLEND_ADD,
		GPU_CONSTANT_ALPHA, GPU_CONSTANT_ALPHA,
		GPU_ONE, GPU_ZERO
	);
}


void gpu3dsResetState()
{
	//sf2d_pool_reset();
	GPUCMD_SetBufferOffset(0);

	GPU_DepthMap(-1.0f, 0.0f);
	GPU_SetFaceCulling(GPU_CULL_NONE);
	GPU_SetStencilTest(false, GPU_ALWAYS, 0x00, 0xFF, 0x00);
	GPU_SetStencilOp(GPU_STENCIL_KEEP, GPU_STENCIL_KEEP, GPU_STENCIL_KEEP);
	GPU_SetBlendingColor(0,0,0,0);
	GPU_SetDepthTestAndWriteMask(false, GPU_GEQUAL, GPU_WRITE_ALL);
	GPUCMD_AddMaskedWrite(GPUREG_EARLYDEPTH_TEST1, 0x1, 0);
	GPUCMD_AddWrite(GPUREG_EARLYDEPTH_TEST2, 0);

	GPU_SetAlphaBlending(
		GPU_BLEND_ADD,
		GPU_BLEND_ADD,
		GPU_SRC_ALPHA, GPU_ONE_MINUS_SRC_ALPHA,
		GPU_ONE, GPU_ZERO
	);

	GPU_SetAlphaTest(true, GPU_NOTEQUAL, 0x00);

	gpu3dsClearTextureEnv(1);
	gpu3dsClearTextureEnv(2);
	gpu3dsClearTextureEnv(3);
	gpu3dsClearTextureEnv(4);
	gpu3dsClearTextureEnv(5);
   
	GPUCMD_Finalize();
	GPUCMD_FlushAndRun();    
    gspWaitForP3D();
}


void gpu3dsSetRenderTargetToTopFrameBuffer()
{
    GPU_SetFloatUniform(GPU_VERTEX_SHADER, 
        GPU3DS.shaders[GPU3DS.currentShader].projectionRegister, 
        (u32 *)GPU3DS.projectionTopScreen, 4);
    
    GPU3DS.currentRenderTarget = NULL;
    
    GPU_SetViewport(
        (u32 *)osConvertVirtToPhys(GPU3DS.frameDepthBuffer),
        (u32 *)osConvertVirtToPhys(GPU3DS.frameBuffer),
        0, 0, 240, 400);
}


void gpu3dsSetRenderTargetToTexture(SGPUTexture *texture)
{
    int bufferLen = texture->Width * texture->Height * 4; 

    if (bufferLen > GPU3DS.targetDepthBufferSize) 
    {
        if (GPU3DS.targetDepthBufferSize > 0) 
            linearFree(GPU3DS.targetDepthBuffer);

        GPU3DS.targetDepthBuffer = linearAlloc(bufferLen);
        GPU3DS.targetDepthBufferSize = bufferLen;
        memset(GPU3DS.targetDepthBuffer, 0, bufferLen);
    }
    
    // Upload saved uniform
    
    GPU_SetFloatUniform(GPU_VERTEX_SHADER, 
        GPU3DS.shaders[GPU3DS.currentShader].projectionRegister, 
        (u32 *)texture->Projection, 4);
    
    GPU3DS.currentRenderTarget = texture;
    
    GPU_SetViewport((u32 *)osConvertVirtToPhys(GPU3DS.targetDepthBuffer),
        (u32 *)osConvertVirtToPhys(texture->PixelData),
        0, 0, texture->Width, texture->Height);
}

void gpu3dsSetRenderTarget(int renderTargetIndex)
{
    //GPU_Reset(NULL, gpuCommandBuffers[0], gpuCommandBufferSize);
	//GPUCMD_SetBufferOffset(0);
    
    /*
    if (renderTarget == 0)
        sf2d_set_render_target(NULL);
    else if (renderTarget == 1)
        sf2d_set_render_target(snesMainScreenTarget);
    else if (renderTarget == 2)
        sf2d_set_render_target(snesSubScreenTarget);
        
    currentRenderTarget = renderTarget;
    */

    if (renderTargetIndex == 0)
        gpu3dsSetRenderTargetToTopFrameBuffer();
    else if (renderTargetIndex == 1)
        gpu3dsSetRenderTargetToTexture(snesMainScreenTarget);
    else if (renderTargetIndex == 2)
        gpu3dsSetRenderTargetToTexture(snesSubScreenTarget);
        
    GPU3DS.currentRenderTargetIndex = renderTargetIndex;
}



void gpu3dsClearRenderTarget()
{
    if (GPU3DS.currentRenderTargetIndex == 1)
    {
        gpu3dsClearTexture(snesMainScreenTarget);
    //}
    //else if (GPU3DS.currentRenderTargetIndex == 2)
    //{
        gpu3dsClearTexture(snesSubScreenTarget);
    }

}


extern Handle gspEvents[GSPGPU_EVENT_MAX];

bool gpu3dsCheckEvent(GSPGPU_Event id)
{
	Result res = svcWaitSynchronization(gspEvents[id], 0);
	if (!res)
	{
		svcClearEvent(gspEvents[id]);
		return true;
	}
	
	return false;
}


bool gpu3dsWaitEvent(GSPGPU_Event id, u64 timeInMilliseconds)
{
    //if (GPU3DS.enableDebug)
    //    printf("  gpu3dsWaitEvent\n");
    
	Result res = svcWaitSynchronization(gspEvents[id], timeInMilliseconds * 1000000);
	if (!res)
	{
        //if (GPU3DS.enableDebug)
        //    printf("  gpu3dsWaitEvent complete\n");
            
		svcClearEvent(gspEvents[id]);
		return true;
	}
	
    //if (GPU3DS.enableDebug)
    //    printf("  gpu3dsWaitEvent timeout\n");
        
	return false;
}



void gpu3dsFlush()
{
    u32 offset;
    GPUCMD_GetBuffer(NULL, NULL, &offset);
    
    //if (GPU3DS.enableDebug)
        printf("  cmdbuf: %x r: %x tl: %x qd: %x\n", offset*4,
            (uint32)GPU3DS.rectangleVertexList - (uint32)GPU3DS.rectangleVertexListBase, 
            (uint32)GPU3DS.tileList - (uint32)GPU3DS.tileListBase, 
            (uint32)GPU3DS.vertexList - (uint32)GPU3DS.vertexListBase);
      

    GPUCMD_Finalize();
    GPUCMD_FlushAndRun();      
     
    GPUCMD_SetBufferOffset(0);
    somethingWasFlushed = true;
    somethingWasDrawn = false;
}

void gpu3dsWaitForPreviousFlush()
{
    if (somethingWasFlushed)
    {
        gpu3dsWaitEvent(GSPGPU_EVENT_P3D, 1000);
        somethingWasFlushed = false;
    }
    
}


void gpu3dsFlushIfPossible()
{
    if (somethingWasDrawn && gpu3dsCheckEvent(GSPGPU_EVENT_P3D))
        gpu3dsFlush();
}


void gpu3dsFrameEnd()
{
    if (somethingWasDrawn)
    {
        gpu3dsWaitForPreviousFlush();
        gpu3dsFlush();
    }
}


//extern     u32 *gpu_fb_addr;


void *gpu3dsAlignTo0x80 (void *addr)
{
    if ((u32)addr & 0x7f)
        return (void *)(((u32)addr & ~0x7f) + 0x80);
    return addr;
}

void gpu3dsTransferToScreenBuffer()
{
    gpu3dsWaitForPreviousFlush();
    
	if (GPU3DS.currentRenderTargetIndex == 0) 
	{
        //if (GPU3DS.enableDebug)
        //    printf("  GX_DisplayTransfer\n");
        
        GX_DisplayTransfer(GPU3DS.frameBuffer, GX_BUFFER_DIM(240, 400),
            (u32 *)gfxGetFramebuffer(GFX_TOP, GFX_LEFT, NULL, NULL),
            GX_BUFFER_DIM(240, 400), 
            GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) | 
            GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGBA8));
        //gspWaitForPPF();
    }
}

void gpu3dsSwapScreenBuffers()
{
	gfxSwapBuffersGpu();
    //sf2d_pool_reset();        
}



//uint32 currentTexture = 0;

void gpu3dsBindTexture(SGPUTexture *texture, GPU_TEXUNIT unit)
{
    if (GPU3DS.currentTexture != texture)
    {
        GPU_SetTextureEnable(unit);

        GPU_SetTexEnv(
            0,
            GPU_TEVSOURCES(GPU_TEXTURE0, GPU_TEXTURE0, GPU_TEXTURE0),
            GPU_TEVSOURCES(GPU_TEXTURE0, GPU_TEXTURE0, GPU_TEXTURE0),
            GPU_TEVOPERANDS(0, 0, 0),
            GPU_TEVOPERANDS(0, 0, 0),
            GPU_REPLACE, GPU_REPLACE,
            0x80808080          // constant color used only during half-color math.
        );

        GPU_SetTexture(
            unit,
            (u32 *)osConvertVirtToPhys(texture->PixelData),
            texture->Width,
            texture->Height,
            texture->Params,
            texture->PixelFormat
        );

        /*GPU_SetFloatUniform(GPU_VERTEX_SHADER, 
            GPU3DS.shaders[GPU3DS.currentShader].textureScaleRegister, 
            (u32 *)texture->TextureScale, 1);
        */
        GPU3DS.currentTexture = texture;
    }
}


void gpu3dsBindTextureSnesTileCache(GPU_TEXUNIT unit)
{
    gpu3dsBindTexture(snesTileCacheTexture, unit);
}

void gpu3dsBindTextureMainScreen(GPU_TEXUNIT unit)
{
    gpu3dsBindTexture(snesMainScreenTarget, unit);
}

void gpu3dsBindTextureSubScreen(GPU_TEXUNIT unit)
{
    gpu3dsBindTexture(snesSubScreenTarget, unit);
}


void gpu3dsScissorTest(GPU_SCISSORMODE mode, uint32 x, uint32 y, uint32 w, uint32 h)
{
    GPU_SetScissorTest(mode, x, y, w, h);
}


u32 rectBufferOffsets[1] = { 0 };
u64 rectAttribPermutations[1] = { 0x210 };
u8 rectNumberOfAttribs[1] = { 2 };

void gpu3dsDrawRectangle(int x0, int y0, int x1, int y1, float depth, u32 color)
{
    if (GPU3DS.isReal3DS)
    {
        SVertexColor *vertices = GPU3DS.rectangleVertexList;
        if (!vertices) return;

        vertices[0].Position = (SVector3f){(float)x0, (float)y0, depth};
        vertices[1].Position = (SVector3f){(float)x1, (float)y1, depth};

        u32 swappedColor = ((color & 0xff) << 24) | ((color & 0xff00) << 8) | ((color & 0xff0000) >> 8) | ((color & 0xff000000) >> 24);
        vertices[0].Color = swappedColor;
        vertices[1].Color = swappedColor;

        GPU_SetAttributeBuffers(
            2, // number of attributes
            (u32*)osConvertVirtToPhys(vertices),
            GPU_ATTRIBFMT(0, 3, GPU_FLOAT) | GPU_ATTRIBFMT(1, 4, GPU_UNSIGNED_BYTE),
            0xFFFF, //0b1100
            0x10,
            1, //number of buffers
            rectBufferOffsets, // buffer offsets (placeholders)
            rectAttribPermutations, // attribute permutations for each buffer
            rectNumberOfAttribs // number of attributes for each buffer
        );

        GPU3DS.rectangleVertexList = (SVertexColor *) gpu3dsAlignTo0x80(&GPU3DS.rectangleVertexList[2]);        

        GPU_DrawArray(GPU_GEOMETRY_PRIM, 0, 2);
        somethingWasDrawn = true;

        printf ("Draw rect fast\n");
    }
    else
    {
        SVertexColor *vertices = GPU3DS.rectangleVertexList;
        if (!vertices) return;

        vertices[0].Position = (SVector3f){(float)x0, (float)y0, depth};
        vertices[1].Position = (SVector3f){(float)x1, (float)y0, depth};
        vertices[2].Position = (SVector3f){(float)x0, (float)y1, depth};
        vertices[3].Position = (SVector3f){(float)x1, (float)y0, depth};
        vertices[4].Position = (SVector3f){(float)x0, (float)y1, depth};
        vertices[5].Position = (SVector3f){(float)x1, (float)y1, depth};

        u32 swappedColor = ((color & 0xff) << 24) | ((color & 0xff00) << 8) | ((color & 0xff0000) >> 8) | ((color & 0xff000000) >> 24);
        vertices[0].Color = swappedColor;
        vertices[1].Color = swappedColor;
        vertices[2].Color = swappedColor;
        vertices[3].Color = swappedColor;
        vertices[4].Color = swappedColor;
        vertices[5].Color = swappedColor;

        GPU_SetAttributeBuffers(
            2, // number of attributes
            (u32*)osConvertVirtToPhys(vertices),
            GPU_ATTRIBFMT(0, 3, GPU_FLOAT) | GPU_ATTRIBFMT(1, 4, GPU_UNSIGNED_BYTE),
            0xFFFF, //0b1100
            0x10,
            1, //number of buffers
            rectBufferOffsets, // buffer offsets (placeholders)
            rectAttribPermutations, // attribute permutations for each buffer
            rectNumberOfAttribs // number of attributes for each buffer
        );

        GPU3DS.rectangleVertexList = (SVertexColor *) gpu3dsAlignTo0x80(&GPU3DS.rectangleVertexList[6]);        

        GPU_DrawArray(GPU_TRIANGLES, 0, 6);
        somethingWasDrawn = true;
    }
}




u32 tileBufferOffsets[1] = { 0 };
u64 tileAttribPermutations[1] = { 0x210 };
u8 tileNumberOfAttribs[1] = { 2 };

u32 vertexBufferOffsets[1] = { 0 };
u64 vertexAttribPermutations[1] = { 0x10 };
u8 vertexNumberOfAttribs[1] = { 2 };

void gpu3dsDrawVertexes()
{
    if (GPU3DS.tileCount > 0)
    {
        GPU_SetAttributeBuffers(
            2, // number of attributes
            (u32*)osConvertVirtToPhys(GPU3DS.tileList),
            GPU_ATTRIBFMT(0, 3, GPU_SHORT) | GPU_ATTRIBFMT(1, 2, GPU_SHORT),
            0xFFFF, //0b1100
            0x10,
            1, //number of buffers
            tileBufferOffsets, // buffer offsets (placeholders)
            tileAttribPermutations, // attribute permutations for each buffer
            tileNumberOfAttribs // number of attributes for each buffer
        );    
            
        GPU_DrawArray(GPU_GEOMETRY_PRIM, 0, GPU3DS.tileCount);

        //printf ("DrawVertexes tileCount = %d\n", GPU3DS.tileCount);
        
        GPU3DS.tileTotal += GPU3DS.tileCount;
        GPU3DS.tileList = (STileVertex *) gpu3dsAlignTo0x80(&GPU3DS.tileList[GPU3DS.tileCount]);        
        GPU3DS.tileCount = 0;
        somethingWasDrawn = true;
        
        //if (GPU3DS.enableDebug)
        //    printf ("  tList base:%x cur: %x\n", GPU3DS.tileListBase, (uint32)GPU3DS.tileList - (uint32)GPU3DS.tileListBase);
    }
    
    if (GPU3DS.vertexCount > 0)
    {
        
        GPU_SetAttributeBuffers(
            2, // number of attributes
            (u32*)osConvertVirtToPhys(GPU3DS.vertexList),
            GPU_ATTRIBFMT(0, 3, GPU_FLOAT) | GPU_ATTRIBFMT(1, 2, GPU_FLOAT),
            0, //0b1100
            0x10,
            1, //number of buffers
            vertexBufferOffsets, // buffer offsets (placeholders)
            vertexAttribPermutations, // attribute permutations for each buffer
            vertexNumberOfAttribs // number of attributes for each buffer
        );    
            
        GPU_DrawArray(GPU_TRIANGLES, 0, GPU3DS.vertexCount);

        //printf ("DrawVertexes vertexCount = %d\n", GPU3DS.vertexCount);

        GPU3DS.vertexTotal += GPU3DS.vertexCount;
        GPU3DS.vertexList = (SVertex *) gpu3dsAlignTo0x80(&GPU3DS.vertexList[GPU3DS.vertexCount]);        
        GPU3DS.vertexCount = 0;
        somethingWasDrawn = true;
        
        //if (GPU3DS.enableDebug)
        //    printf ("  vList base:%x cur: %x\n", GPU3DS.vertexListBase, (uint32)GPU3DS.vertexList - (uint32)GPU3DS.vertexListBase);
        
    }    
}

