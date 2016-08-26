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
#include "shaderfast_shbin.h"
#include "shaderfast2_shbin.h"
#include "shaderfast3_shbin.h"
#include "shaderfastm7_shbin.h"

#include "shaderslow_shbin.h"
#include "shaderslow2_shbin.h"
#include "shaderslow3_shbin.h"
#include "shaderslowm7_shbin.h"

//--------------------------------------------------
// Important:
// Uncomment this when compiling for a real
// 3DS for speed improvements.
//--------------------------------------------------
//#define     REAL3DS     
 
//int     vramCacheFrameNumber[MAX_HASH];                       


//int currentRenderTarget = 0;
bool somethingWasDrawn = false;
bool somethingWasFlushed = false;

/*
For reference only:

GSPGPU_FramebufferFormats { 
  GSP_RGBA8_OES =0, 
  GSP_BGR8_OES =1, 
  GSP_RGB565_OES =2, 
  GSP_RGB5_A1_OES =3, 
  GSP_RGBA4_OES =4 
}

GPU_TEXCOLOR { 
  GPU_RGBA8 = 0x0, 
  GPU_RGB8 = 0x1, 
  GPU_RGBA5551 = 0x2, 
  GPU_RGB565 = 0x3, 
  GPU_RGBA4 = 0x4, 
  GPU_LA8 = 0x5, 
  GPU_HILO8 = 0x6, 
  GPU_L8 = 0x7, 
  GPU_A8 = 0x8, 
  GPU_LA4 = 0x9, 
  GPU_L4 = 0xA, 
  GPU_ETC1 = 0xB, 
  GPU_ETC1A4 = 0xC 
}

GX_TRANSFER_FORMAT { 
  GX_TRANSFER_FMT_RGBA8 = 0, 
  GX_TRANSFER_FMT_RGB8 = 1, 
  GX_TRANSFER_FMT_RGB565 = 2, 
  GX_TRANSFER_FMT_RGB5A1 = 3, 
  GX_TRANSFER_FMT_RGBA4 = 4 
}        
*/

#define LINEARFREE_SAFE(x)  if (x) linearFree(x);


void cacheInit()
{
    //printf ("Cache %8x\n", &vramCacheFrameNumber);
    //memset(&vramCacheFrameNumber, 0, MAX_HASH * 4);
    memset(&GPU3DS.vramCacheHashToTexturePosition, 0, (MAX_HASH + 1) * 2);

    // Fixes issue in Thunder Spirits where the tileaddr + pal
    // gives the texturePos of 0, but gets overwritten by other tiles
    //
    GPU3DS.vramCacheTexturePositionToHash[0] = 0;
    for (int i = 1; i < MAX_TEXTURE_POSITIONS; i++)
        GPU3DS.vramCacheTexturePositionToHash[i] = MAX_HASH;
    GPU3DS.newCacheTexturePosition = 1;
}

/*
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
        if (GPU3DS.newCacheTexturePosition >= MAX_TEXTURE_POSITIONS)
            GPU3DS.newCacheTexturePosition = 1;
    }
    
    return pos;
}
*/

int cacheGetMode7TexturePosition(int tileNumber)
{
    int y = tileNumber >> 8;
    int x = tileNumber & 0xff;
    return (y * 1024 + x);

}


// Memory Usage = 2.00 MB   for texture cache
#define TEXTURE_SIZE                    1024

// Increased buffer size to 1MB for screens with heavy effects (multiple wavy backgrounds and line-by-line windows).
// Memory Usage = 1.00 MB   for GPU command buffer
#define COMMAND_BUFFER_SIZE             0x100000  

// Memory Usage = 0.26 MB   for 4-point rectangle (triangle strip) vertex buffer
#define RECTANGLE_BUFFER_SIZE           0x40000

// Memory Usage = 8.00 MB   for 6-point quad vertex buffer (Citra only)
#define CITRA_VERTEX_BUFFER_SIZE        0x800000

#define CITRA_TILE_BUFFER_SIZE          0x200

// Memory usage = 2.00 MB   for 6-point full texture mode 7 update buffer
#define CITRA_M7_BUFFER_SIZE            0x200000

// Memory Usage = 0.06 MB   for 6-point quad vertex buffer (Real 3DS only)
#define REAL3DS_VERTEX_BUFFER_SIZE      0x1000

// Memory Usage = 3.00 MB   for 2-point rectangle vertex buffer (Real 3DS only)
#define REAL3DS_TILE_BUFFER_SIZE        0x300000

// Memory usage = 0.78 MB   for 2-point full texture mode 7 update buffer
#define REAL3DS_M7_BUFFER_SIZE          0xC0000


/*
sf2d_rendertarget *snesMainScreenTarget;
sf2d_rendertarget *snesSubScreenTarget;
sf2d_texture *snesTileCacheTexture;
*/

SGPUTexture *snesMainScreenTarget;
SGPUTexture *snesSubScreenTarget;
SGPUTexture *snesTileCacheTexture;
SGPUTexture *snesMode7FullTexture;
SGPUTexture *snesMode7TileCacheTexture;
SGPUTexture *snesMode7Tile0Texture;


u32 *gpuCommandBuffer1;
u32 *gpuCommandBuffer2;
int gpuCommandBufferSize = 0;
int gpuCurrentCommandBuffer = 0;


void gpu3dsSetMode7UpdateFrameCountUniform();

inline void gpu3dsSetAttributeBuffers(
    u8 totalAttributes, 
    u32 *listAddress, u64 attributeFormats, u16 attributeMask, u64 attributePermutation, 
    u8 numBuffers, u32 bufferOffsets[], u64 bufferPermutations[], u8 bufferNumAttributes[])
{
    if (GPU3DS.currentAttributeBuffer != listAddress)
    {
        u32 *osAddress = (u32 *)osConvertVirtToPhys(listAddress);
        GPU_SetAttributeBuffers(
            totalAttributes, // number of attributes
            osAddress,
            attributeFormats,
            attributeMask, //0b1100
            attributePermutation, 
            numBuffers, //number of buffers
            bufferOffsets, // buffer offsets (placeholders)
            bufferPermutations, // attribute permutations for each buffer
            bufferNumAttributes // number of attributes for each buffer
        );    
        GPU3DS.currentAttributeBuffer = listAddress; 
    }
    
}

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


void *gpu3dsAlignTo0x80 (void *addr)
{
    if ((u32)addr & 0x7f)
        return (void *)(((u32)addr & ~0x7f) + 0x80);
    return addr;
}


void gpu3dsAllocVertexList(SVertexList *list, int sizeInBytes, int vertexSize, 
    u8 totalAttributes, u64 attributeFormats)
{
    list->TotalAttributes = totalAttributes;
    list->AttributeFormats = attributeFormats;
    list->VertexSize = vertexSize;
    list->SizeInBytes = sizeInBytes;
    list->ListBase = (STileVertex *) linearAlloc(sizeInBytes);
    list->List = list->ListBase;
    list->ListOriginal = list->List;
    list->Total = 0;
    list->Count = 0;
    list->Flip = 1;
}

void gpu3dsDeallocVertexList(SVertexList *list)
{
    LINEARFREE_SAFE(list->ListBase);
}

void gpu3dsSwapVertexListForNextFrame(SVertexList *list)
{
    if (list->Flip)
        list->List = (void *)((uint32)(list->ListBase) + list->SizeInBytes / 2);
    else
        list->List = list->ListBase;
    list->ListOriginal = list->List;
    list->Flip = 1 - list->Flip;
    list->Total = 0;
    list->Count = 0;
    list->FirstIndex = 0;
    list->PrevCount = 0;
    list->PrevFirstIndex = 0;
}


u32 vertexListBufferOffsets[1] = { 0 };
u64 vertexListAttribPermutations[1] = { 0x3210 };
u8 vertexListNumberOfAttribs[1] = { 2 };

inline void gpu3dsDrawVertexList(SVertexList *list, GPU_Primitive_t type, bool repeatLastDraw)
{
    if (!repeatLastDraw)
    {
        if (list->Count > 0)
        {
            gpu3dsSetAttributeBuffers(
                list->TotalAttributes,          // number of attributes
                (u32*)list->List,
                list->AttributeFormats,
                0xFFFF,                         //0b1100
                0x3210, 
                1,                              //number of buffers
                vertexListBufferOffsets,        // buffer offsets (placeholders)
                vertexListAttribPermutations,   // attribute permutations for each buffer
                vertexListNumberOfAttribs       // number of attributes for each buffer
            );    

            GPU_DrawArray(type, 0, list->Count);

            // Saves this just in case it can be re-used for windowing
            // or HDMA effects.
            //
            list->PrevCount = list->Count;
            list->PrevFirstIndex = list->FirstIndex;
            list->PrevList = list->List;

            u8 *p = (u8 *)list->List;
            list->List = (STileVertex *) gpu3dsAlignTo0x80(p + (list->Count * list->VertexSize));    

            list->FirstIndex += list->Count;
            list->Total += list->Count;
            list->Count = 0;

            somethingWasDrawn = true;
        }
    }
    else
    {
        if (list->PrevCount > 0)
        {
            GPU_DrawArray(type, list->PrevFirstIndex, list->PrevCount);

            somethingWasDrawn = true;
        }   
    }
}


inline void gpu3dsDrawMode7VertexList(SVertexList *list, GPU_Primitive_t type, int fromIndex, int tileCount)
{
    if (tileCount > 0)
    {
        gpu3dsSetAttributeBuffers(
            list->TotalAttributes,          // number of attributes
            (u32 *)list->List,
            list->AttributeFormats,
            0xFFFF,                         // 0b1100
            0x3210, 
            1, //number of buffers
            vertexListBufferOffsets,        // buffer offsets (placeholders)
            vertexListAttribPermutations,   // attribute permutations for each buffer
            vertexListNumberOfAttribs       // number of attributes for each buffer
        );   


        if (GPU3DS.isReal3DS)
            GPU_DrawArray(type, fromIndex, tileCount);
        else
            GPU_DrawArray(type, fromIndex * 6, tileCount * 6);

        somethingWasDrawn = true;
    }
}


void gpu3dsInitializeMode7Vertex(int idx, int x, int y)
{
    int x0 = 0;
    int y0 = 0;

    if (x < 64)
    {
        x0 = x * 8;
        y0 = (y * 2 + 1) * 8;
    }
    else
    {
        x0 = (x - 64) * 8;
        y0 = (y * 2) * 8;
    }
    
    int x1 = x0 + 8;
    int y1 = y0 + 8;

    if (GPU3DS.isReal3DS)
    {
        SMode7TileVertex *m7vertices = &((SMode7TileVertex *)GPU3DS.mode7TileVertexes.List) [idx];

        m7vertices[0].Position = (SVector4i){x0, y0, 0, -1};
        //m7vertices[1].Position = (SVector4i){x1, y1, 0, -1};
        
        m7vertices[0].TexCoord = (STexCoord2i){0, 0};
        //m7vertices[1].TexCoord = (STexCoord2i){8, 8};
        
    }
    else
    {
        SMode7TileVertex *m7vertices = &((SMode7TileVertex *)GPU3DS.mode7TileVertexes.List) [idx * 6];

        m7vertices[0].Position = (SVector4i){x0, y0, 0, -1};
        m7vertices[1].Position = (SVector4i){x1, y0, 0, -1};
        m7vertices[2].Position = (SVector4i){x0, y1, 0, -1};

        m7vertices[3].Position = (SVector4i){x1, y1, 0, -1};
        m7vertices[4].Position = (SVector4i){x0, y1, 0, -1};
        m7vertices[5].Position = (SVector4i){x1, y0, 0, -1};

        m7vertices[0].TexCoord = (STexCoord2i){0, 0};
        m7vertices[1].TexCoord = (STexCoord2i){8, 0};
        m7vertices[2].TexCoord = (STexCoord2i){0, 8};
        
        m7vertices[3].TexCoord = (STexCoord2i){8, 8};
        m7vertices[4].TexCoord = (STexCoord2i){0, 8};
        m7vertices[5].TexCoord = (STexCoord2i){8, 0};
        
    }
}

void gpu3dsInitializeMode7VertexForTile0(int idx, int x, int y)
{
    int x0 = x;
    int y0 = y;

    int x1 = x0 + 8;
    int y1 = y0 + 8;

    if (GPU3DS.isReal3DS)
    {
        SMode7TileVertex *m7vertices = &((SMode7TileVertex *)GPU3DS.mode7TileVertexes.List) [idx];

        m7vertices[0].Position = (SVector4i){x0, y0, 0, 0x3fff};
        //m7vertices[1].Position = (SVector4i){x1, y1, 0, 0};
        
        m7vertices[0].TexCoord = (STexCoord2i){0, 0};
        //m7vertices[1].TexCoord = (STexCoord2i){8, 8};
        
    }
    else
    {
        SMode7TileVertex *m7vertices = &((SMode7TileVertex *)GPU3DS.mode7TileVertexes.List) [idx * 6];

        m7vertices[0].Position = (SVector4i){x0, y0, 0, 0x3fff};
        m7vertices[1].Position = (SVector4i){x1, y0, 0, 0x3fff};
        m7vertices[2].Position = (SVector4i){x0, y1, 0, 0x3fff};

        m7vertices[3].Position = (SVector4i){x1, y1, 0, 0x3fff};
        m7vertices[4].Position = (SVector4i){x0, y1, 0, 0x3fff};
        m7vertices[5].Position = (SVector4i){x1, y0, 0, 0x3fff};

        m7vertices[0].TexCoord = (STexCoord2i){0, 0};
        m7vertices[1].TexCoord = (STexCoord2i){8, 0};
        m7vertices[2].TexCoord = (STexCoord2i){0, 8};
        
        m7vertices[3].TexCoord = (STexCoord2i){8, 8};
        m7vertices[4].TexCoord = (STexCoord2i){0, 8};
        m7vertices[5].TexCoord = (STexCoord2i){8, 0};
        
    }
}

void gpu3dsInitializeMode7Vertexes()
{
    GPU3DS.mode7FrameCount = 0;
    gpu3dsSetMode7UpdateFrameCountUniform();
    for (int f = 0; f < 2; f++)
    {
        int idx = 0;
        for (int section = 0; section < 4; section++)
        {
            for (int y = 0; y < 32; y++)
                for (int x = 0; x < 128; x++)
                    gpu3dsInitializeMode7Vertex(idx++, x, y); 
        }

        gpu3dsInitializeMode7VertexForTile0(16384, 0, 0);
        gpu3dsInitializeMode7VertexForTile0(16385, 0, 8);
        gpu3dsInitializeMode7VertexForTile0(16386, 8, 0);
        gpu3dsInitializeMode7VertexForTile0(16387, 8, 8);

        gpu3dsSwapVertexListForNextFrame(&GPU3DS.mode7TileVertexes);
    }
}


bool gpu3dsInitialize()
{
    // Initialize the 3DS screen
    //
    //gfxInit	(GSP_RGB5_A1_OES, GSP_RGB5_A1_OES, false);
    //GPU3DS.screenFormat = GSP_RGBA8_OES;
    GPU3DS.screenFormat = GSP_RGBA8_OES;
    gfxInit	(GPU3DS.screenFormat, GPU3DS.screenFormat, false);
	GPU_Init(NULL);
	gfxSet3D(false);

    // Create the frame and depth buffers for the top screen.
    //
    GPU3DS.frameBufferFormat = GPU_RGBA8;
	GPU3DS.frameBuffer = (u32 *) vramMemAlign(400*240*8, 0x100);
	GPU3DS.frameDepthBuffer = (u32 *) vramMemAlign(400*240*8, 0x100);
    if (GPU3DS.frameBuffer == NULL || 
        GPU3DS.frameDepthBuffer == NULL)
    {
        printf ("Unable to allocate frame/depth buffers\n");
        return false;
    }

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

#ifdef REAL3DS
    GPU3DS.isReal3DS = true;
#else
    GPU3DS.isReal3DS = false;
#endif

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
        gpu3dsLoadShader(0, (u32 *)shaderfast_shbin, shaderfast_shbin_size, 6);
    	gpu3dsLoadShader(1, (u32 *)shaderslow_shbin, shaderslow_shbin_size, 0);
    	gpu3dsLoadShader(2, (u32 *)shaderfast2_shbin, shaderfast2_shbin_size, 6);
        gpu3dsLoadShader(3, (u32 *)shaderfastm7_shbin, shaderfastm7_shbin_size, 3);
    }
    else
    {
    	gpu3dsLoadShader(0, (u32 *)shaderslow_shbin, shaderslow_shbin_size, 0);
    	gpu3dsLoadShader(1, (u32 *)shaderslow_shbin, shaderslow_shbin_size, 0);
        gpu3dsLoadShader(2, (u32 *)shaderslow2_shbin, shaderslow2_shbin_size, 0);
        gpu3dsLoadShader(3, (u32 *)shaderslowm7_shbin, shaderslowm7_shbin_size, 0);
    }
	
    // Create all the necessary textures
    //
    snesTileCacheTexture = gpu3dsCreateTextureInLinearMemory(1024, 1024, GPU_RGBA5551);
    snesMode7TileCacheTexture = gpu3dsCreateTextureInLinearMemory(128, 128, GPU_RGBA5551);

    // This requires 16x16 texture as a minimum
    snesMode7Tile0Texture = gpu3dsCreateTextureInVRAM(16, 16, GPU_RGBA5551);
    snesMode7FullTexture = gpu3dsCreateTextureInVRAM(1024, 1024, GPU_RGBA5551);

    // Main screen requires 8-bit alpha, otherwise alpha blending will not work well
    snesMainScreenTarget = gpu3dsCreateTextureInVRAM(256, 256, GPU_RGBA8);      
    snesSubScreenTarget = gpu3dsCreateTextureInVRAM(256, 256, GPU_RGBA5551);

    if (snesTileCacheTexture == NULL || snesMode7FullTexture == NULL || 
        snesMode7TileCacheTexture == NULL || snesMode7Tile0Texture == NULL ||
        snesMainScreenTarget == NULL || snesSubScreenTarget == NULL)
    {
        printf ("Unable to allocate textures\n");
        return false;
    }
    
    printf ("gpu3dsInitialize - Allocate buffers\n");
   
    if (GPU3DS.isReal3DS)
    {
        GPU3DS.rectangleVertexListBase = (SVertexColor *) linearAlloc(RECTANGLE_BUFFER_SIZE);
        gpu3dsAllocVertexList(&GPU3DS.mode7TileVertexes, sizeof(SMode7TileVertex) * 16400 * 1 * 2 + 0x200, sizeof(SMode7TileVertex), 2, SMODE7TILEVERTEX_ATTRIBFORMAT);
        gpu3dsAllocVertexList(&GPU3DS.quadVertexes, REAL3DS_VERTEX_BUFFER_SIZE, sizeof(STileVertex), 2, STILEVERTEX_ATTRIBFORMAT);
        gpu3dsAllocVertexList(&GPU3DS.tileVertexes, REAL3DS_TILE_BUFFER_SIZE, sizeof(STileVertex), 2, STILEVERTEX_ATTRIBFORMAT);
    }
    else
    {
        GPU3DS.rectangleVertexListBase = (SVertexColor *) linearAlloc(RECTANGLE_BUFFER_SIZE);
        gpu3dsAllocVertexList(&GPU3DS.mode7TileVertexes, sizeof(SMode7TileVertex) * 16400 * 6 * 2 + 0x200, sizeof(SMode7TileVertex), 2, SMODE7TILEVERTEX_ATTRIBFORMAT);
        gpu3dsAllocVertexList(&GPU3DS.quadVertexes, CITRA_VERTEX_BUFFER_SIZE, sizeof(STileVertex), 2, STILEVERTEX_ATTRIBFORMAT);
        gpu3dsAllocVertexList(&GPU3DS.tileVertexes, CITRA_TILE_BUFFER_SIZE, sizeof(STileVertex), 2, STILEVERTEX_ATTRIBFORMAT);
    }
        
    if (GPU3DS.quadVertexes.ListBase == NULL ||
        GPU3DS.tileVertexes.ListBase == NULL ||
        GPU3DS.rectangleVertexListBase == NULL ||
        GPU3DS.mode7TileVertexes.ListBase == NULL)
    {
        printf ("Unable to allocate vertex list buffers \n");   
        return false;
    }

    gpu3dsInitializeMode7Vertexes();

    printf ("gpu3dsInitialize - Set GPU statuses\n");
        
	//sf2d_pool_reset();
	GPUCMD_SetBufferOffset(0);

	gpu3dsUseShader(0);

	GPU_DepthMap(-1.0f, 0.0f);
	GPU_SetDepthTestAndWriteMask(false, GPU_GEQUAL, GPU_WRITE_ALL);
	GPUCMD_AddMaskedWrite(GPUREG_EARLYDEPTH_TEST1, 0x1, 0);
	GPUCMD_AddWrite(GPUREG_EARLYDEPTH_TEST2, 0);

	GPU_SetFaceCulling(GPU_CULL_NONE);
	GPU_SetStencilTest(false, GPU_ALWAYS, 0x00, 0xFF, 0x00);
	GPU_SetStencilOp(GPU_STENCIL_KEEP, GPU_STENCIL_KEEP, GPU_STENCIL_KEEP);

	GPU_SetBlendingColor(0,0,0,0);
	GPU_SetAlphaBlending(
		GPU_BLEND_ADD,
		GPU_BLEND_ADD,
		GPU_SRC_ALPHA, GPU_ONE_MINUS_SRC_ALPHA,
		GPU_ONE, GPU_ZERO
	);
	gpu3dsEnableAlphaTest();
    GPU_SetTextureBorderColor(GPU_TEXUNIT0, 0);

    gpu3dsSetTextureEnvironmentReplaceTexture0();
    
	GPUCMD_Finalize();
	GPUCMD_FlushAndRun();    
    gspWaitForP3D();         
    
    return true;
}


void gpu3dsFinalize()
{
    LINEARFREE_SAFE(GPU3DS.rectangleVertexListBase);
    gpu3dsDeallocVertexList(&GPU3DS.mode7TileVertexes);
    gpu3dsDeallocVertexList(&GPU3DS.quadVertexes);
    gpu3dsDeallocVertexList(&GPU3DS.tileVertexes);
    
    gpu3dsDestroyTextureFromLinearMemory(snesTileCacheTexture);
    gpu3dsDestroyTextureFromLinearMemory(snesMode7TileCacheTexture);
    
    gpu3dsDestroyTextureFromVRAM(snesMode7Tile0Texture);
    gpu3dsDestroyTextureFromVRAM(snesMode7FullTexture);
    gpu3dsDestroyTextureFromVRAM(snesMainScreenTarget);      
    gpu3dsDestroyTextureFromVRAM(snesSubScreenTarget);

    LINEARFREE_SAFE(gpuCommandBuffer1);
    LINEARFREE_SAFE(gpuCommandBuffer2);

    printf("gfxExit:\n");
	gfxExit();

}

void gpu3dsEnableAlphaTest()
{
    GPU_SetAlphaTest(true, GPU_NOTEQUAL, 0x00);
}

void gpu3dsEnableAlphaTestEqualsOne()
{
    GPU_SetAlphaTest(true, GPU_EQUAL, 0x01);
}


void gpu3dsDisableAlphaTest()
{
    GPU_SetAlphaTest(false, GPU_NOTEQUAL, 0x00);
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

}


void gpu3dsCacheToMode7TexturePosition(
    uint8 *snesTilePixels, 
	uint16 *snesPalette,
    uint16 texturePosition, 
    uint32 *paletteMask)
{
    int tx = texturePosition % 16;              // should never be >= 16
    int ty = (texturePosition / 16) & 0xf;      // should never be >= 16
    texturePosition = (15 - ty) * 16 + tx;      // flip vertically.
    uint32 base = texturePosition * 64;
    
    uint16 *tileTexture = (uint16 *)snesMode7TileCacheTexture->PixelData;
	uint32 charPaletteMask = 0; 

    #define GET_TILE_PIXEL(x)   (snesTilePixels[x * 2] == 0 ? 0 : snesPalette[snesTilePixels[x * 2]]); charPaletteMask |= (1 << (snesTilePixels[x * 2] >> 3));  
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

    *paletteMask = charPaletteMask;
}


void gpu3dsCacheToMode7Tile0TexturePosition(
    uint8 *snesTilePixels, 
	uint16 *snesPalette,
    uint16 texturePosition, 
    uint32 *paletteMask)
{
    int tx = texturePosition % 16;              // should never be >= 16
    int ty = (texturePosition / 16) & 0xf;      // should never be >= 16
    texturePosition = (15 - ty) * 16 + tx;      // flip vertically.
    uint32 base = texturePosition * 64;
    
    uint16 *tileTexture = (uint16 *)snesMode7Tile0Texture->PixelData;
	uint32 charPaletteMask = 0; 

    #define GET_TILE_PIXEL(x)   (snesTilePixels[x * 2] == 0 ? 0 : snesPalette[snesTilePixels[x * 2]]); charPaletteMask |= (1 << (snesTilePixels[x * 2] >> 3));  
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

    *paletteMask = charPaletteMask;
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
    texture->TextureScale[3] = 1.0f / texture->Width;  // x
    texture->TextureScale[2] = 1.0f / texture->Height; // y
    texture->TextureScale[1] = 0;  // z
    texture->TextureScale[0] = 0;  // w

    memset(texture->PixelData, 0, size);
    printf ("Allocated %d x %d in linear mem (%d)\n", width, height, size);

	return texture;
}

void gpu3dsDestroyTextureFromLinearMemory(SGPUTexture *texture)
{
    LINEARFREE_SAFE(texture->PixelData);
    if (texture) free(texture);
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
    texture->TextureScale[3] = 1.0f / texture->Width;  // x
    texture->TextureScale[2] = 1.0f / texture->Height; // y
    texture->TextureScale[1] = 0;  // z
    texture->TextureScale[0] = 0;  // w


    int vpWidth = width;
    int vpHeight = height;

    // 3DS does not allow rendering to a viewport whose width > 512.
    //
    if (vpWidth > 512) vpWidth = 512;
    if (vpHeight > 512) vpHeight = 512;


	matrix_init_orthographic(texture->Projection, 0.0f, vpWidth, vpHeight, 0.0f, 0.0f, 1.0f);
	matrix_rotate_z(texture->Projection, M_PI / 2.0f);

    GX_MemoryFill(
        (u32*)texture->PixelData, 0x00000000, 
        (u32*)&((u8*)texture->PixelData)[texture->BufferSize], 
        GX_FILL_TRIGGER | GX_FILL_32BIT_DEPTH,
        NULL, 0x00000000, NULL, 0);
    gspWaitForPSC0();
    printf ("clear: %x %d\n", texture->PixelData, texture->BufferSize);

    printf ("Allocated %d x %d in VRAM (%d)\n", width, height, size);

	return texture;
}

void gpu3dsDestroyTextureFromVRAM(SGPUTexture *texture)
{
    if (texture->PixelData) vramFree(texture->PixelData);
    if (texture) free(texture);
}


// Clear the render targets by drawing black to them.
//
void gpu3dsClearAllRenderTargets()
{
    gpu3dsSetRenderTargetToMainScreenTexture();
    gpu3dsDrawRectangle(0, 0, 256, 256, 0, 0xff); 
    gpu3dsSetRenderTargetToSubScreenTexture();
    gpu3dsDrawRectangle(0, 0, 256, 256, 0, 0xff); 
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
    //    printf("  gpu3dsStartNewFrame\n");
    
    gpuCurrentCommandBuffer = 1 - gpuCurrentCommandBuffer;

    gpu3dsSwapVertexListForNextFrame(&GPU3DS.quadVertexes);
    gpu3dsSwapVertexListForNextFrame(&GPU3DS.tileVertexes);

    if (gpuCurrentCommandBuffer == 0)
    {
	    GPU_Reset(NULL, gpuCommandBuffer1, gpuCommandBufferSize);

        GPU3DS.rectangleVertexList = GPU3DS.rectangleVertexListBase;
    }
    else
    {
	    GPU_Reset(NULL, gpuCommandBuffer2, gpuCommandBufferSize);
        
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

    if (GPU3DS.currentShader != shaderIndex)
    {
        GPU3DS.currentShader = shaderIndex;
        shaderProgramUse(&GPU3DS.shaders[shaderIndex].shaderProgram);

        if (!GPU3DS.currentRenderTarget)
        {
            GPU_SetFloatUniform(GPU_VERTEX_SHADER, 0, (u32 *)GPU3DS.projectionTopScreen, 4);
            GPU_SetFloatUniform(GPU_GEOMETRY_SHADER, 10, (u32 *)GPU3DS.projectionTopScreen, 4);
        }
        else
        {
            GPU_SetFloatUniform(GPU_VERTEX_SHADER, 0, (u32 *)GPU3DS.currentRenderTarget->Projection, 4);
            GPU_SetFloatUniform(GPU_GEOMETRY_SHADER, 10, (u32 *)GPU3DS.projectionTopScreen, 4);
        }

        gpu3dsSetMode7UpdateFrameCountUniform();
        
        if (GPU3DS.currentTexture != NULL)
        {
            GPU_SetFloatUniform(GPU_VERTEX_SHADER, 4, (u32 *)GPU3DS.currentTexture->TextureScale, 1);
            GPU_SetFloatUniform(GPU_GEOMETRY_SHADER, 14, (u32 *)GPU3DS.currentTexture->TextureScale, 1);
        }
        
    }
}


void gpu3dsLoadShader(int shaderIndex, u32 *shaderBinary, 
    int size, int geometryShaderStride)
{
	GPU3DS.shaders[shaderIndex].dvlb = DVLB_ParseFile((u32 *)shaderBinary, size);
    printf ("Load DVLB %x size=%d shader=%d\n", GPU3DS.shaders[shaderIndex].dvlb, size, shaderIndex);

	shaderProgramInit(&GPU3DS.shaders[shaderIndex].shaderProgram);
	shaderProgramSetVsh(&GPU3DS.shaders[shaderIndex].shaderProgram, 
        &GPU3DS.shaders[shaderIndex].dvlb->DVLE[0]);
    printf ("  Vertex shader loaded: %x\n", GPU3DS.shaders[shaderIndex].shaderProgram.vertexShader);
	 
	if (geometryShaderStride)
    {
		shaderProgramSetGsh(&GPU3DS.shaders[shaderIndex].shaderProgram, 
			&GPU3DS.shaders[shaderIndex].dvlb->DVLE[1], geometryShaderStride);
        printf ("  Geometry shader loaded: %x\n", GPU3DS.shaders[shaderIndex].shaderProgram.geometryShader);
    }

	GPU3DS.shaders[shaderIndex].projectionRegister = 
		shaderInstanceGetUniformLocation(GPU3DS.shaders[shaderIndex].shaderProgram.vertexShader, 
        "projection");
    printf ("  Uniform: projection: %d\n", GPU3DS.shaders[shaderIndex].projectionRegister);

	GPU3DS.shaders[shaderIndex].projectionRegister = 
		shaderInstanceGetUniformLocation(GPU3DS.shaders[shaderIndex].shaderProgram.geometryShader, 
        "projection1");
    printf ("  Uniform (g): projection: %d\n", GPU3DS.shaders[shaderIndex].projectionRegister);

	int textureScaleRegister = 
		shaderInstanceGetUniformLocation(GPU3DS.shaders[shaderIndex].shaderProgram.vertexShader, 
        "textureScale");
    printf ("  Uniform: textureScale: %d\n", textureScaleRegister);
	
	int textureScale1Register = 
		shaderInstanceGetUniformLocation(GPU3DS.shaders[shaderIndex].shaderProgram.geometryShader, 
        "textureScale1");
    printf ("  Uniform (g): textureScale: %d\n", textureScale1Register);
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


/*
The following array is based on 
    https://www.3dbrew.org/wiki/GPU/Internal_Registers#GPUREG_COLORBUFFER_FORMAT and
supports only the following frame buffer format types:

  GPU_RGBA8 = 0x0, 
  GPU_RGB8 = 0x1, 
  GPU_RGBA5551 = 0x2, 
  GPU_RGB565 = 0x3, 
  GPU_RGBA4 = 0x4 
*/
const uint32 GPUREG_COLORBUFFER_FORMAT_VALUES[5] = { 0x0002, 0x00010001, 0x00020000, 0x00030000, 0x00040002 };


void gpu3dsSetRenderTargetToTopFrameBuffer()
{
    GPU_SetFloatUniform(GPU_VERTEX_SHADER, 0, (u32 *)GPU3DS.projectionTopScreen, 4);
    GPU_SetFloatUniform(GPU_GEOMETRY_SHADER, 10, (u32 *)GPU3DS.projectionTopScreen, 4);

    GPU3DS.currentRenderTarget = NULL;
    
    GPU_SetViewport(
        (u32 *)osConvertVirtToPhys(GPU3DS.frameDepthBuffer),
        (u32 *)osConvertVirtToPhys(GPU3DS.frameBuffer),
        0, 0, 240, 400);

    GPUCMD_AddSingleParam(0x000F0117, GPUREG_COLORBUFFER_FORMAT_VALUES[GPU3DS.frameBufferFormat]); //color buffer format        

    GPU3DS.currentRenderTargetIndex = 0;

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
    GPU_SetFloatUniform(GPU_VERTEX_SHADER, 0, (u32 *)texture->Projection, 4);
    GPU_SetFloatUniform(GPU_GEOMETRY_SHADER, 10, (u32 *)texture->Projection, 4);
    //matrix_gpu_set_uniform(texture->Projection, GPU3DS.shaders[GPU3DS.currentShader].projectionRegister);

    GPU3DS.currentRenderTarget = texture;

    int vpWidth = texture->Width;
    int vpHeight = texture->Height;

    // 3DS does not allow rendering to a viewport whose width > 512.
    //
    if (vpWidth > 512) vpWidth = 512;
    if (vpHeight > 512) vpHeight = 512;

    GPU_SetViewport(
        (u32 *)osConvertVirtToPhys(GPU3DS.targetDepthBuffer),
        (u32 *)osConvertVirtToPhys(texture->PixelData),
        0, 0, vpWidth, vpHeight);

     GPUCMD_AddSingleParam(0x000F0117, GPUREG_COLORBUFFER_FORMAT_VALUES[texture->PixelFormat]); //color buffer format        
        
}


void gpu3dsSetRenderTargetToTextureSpecific(SGPUTexture *texture, int addressOffset, int width, int height)
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
    GPU_SetFloatUniform(GPU_VERTEX_SHADER, 0, (u32 *)texture->Projection, 4);
    GPU_SetFloatUniform(GPU_GEOMETRY_SHADER, 10, (u32 *)texture->Projection, 4);
    //matrix_gpu_set_uniform(texture->Projection, GPU3DS.shaders[GPU3DS.currentShader].projectionRegister);

    GPU3DS.currentRenderTarget = texture;
    
    GPU_SetViewport(
        (u32 *)osConvertVirtToPhys(GPU3DS.targetDepthBuffer),
        (u32 *)osConvertVirtToPhys((void *)((int)texture->PixelData + addressOffset)),
        0, 0, width, height);

     GPUCMD_AddSingleParam(0x000F0117, GPUREG_COLORBUFFER_FORMAT_VALUES[texture->PixelFormat]); //color buffer format        
}


void gpu3dsSetRenderTargetToMainScreenTexture()
{
    gpu3dsSetRenderTargetToTexture(snesMainScreenTarget);
    GPU3DS.currentRenderTargetIndex = 1;
}

void gpu3dsSetRenderTargetToSubScreenTexture()
{
    gpu3dsSetRenderTargetToTexture(snesSubScreenTarget);
    GPU3DS.currentRenderTargetIndex = 2;
}

void gpu3dsSetRenderTargetToMode7FullTexture(int pixelOffset, int width, int height)
{
    gpu3dsSetRenderTargetToTextureSpecific(snesMode7FullTexture, 
        pixelOffset * gpu3dsGetPixelSize(snesMode7FullTexture->PixelFormat), width, height);
    GPU3DS.currentRenderTargetIndex = 3;
}

void gpu3dsSetRenderTargetToMode7Tile0Texture()
{
    gpu3dsSetRenderTargetToTexture(snesMode7Tile0Texture);
    GPU3DS.currentRenderTargetIndex = 4;
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
    else if (renderTargetIndex == 4)
        gpu3dsSetRenderTargetToTexture(snesMode7Tile0Texture);
        
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
        if (GPU3DS.isReal3DS)
            gpu3dsWaitEvent(GSPGPU_EVENT_P3D, 500);
        else
            gpu3dsWaitEvent(GSPGPU_EVENT_P3D, 1);
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

/*
Translate from the following GPU_TEXCOLOR to their respective GX_TRANSFER_FMT values.
  GPU_RGBA8 = 0x0, 
  GPU_RGB8 = 0x1, 
  GPU_RGBA5551 = 0x2, 
  GPU_RGB565 = 0x3, 
  GPU_RGBA4 = 0x4 
*/
const uint32 GX_TRANSFER_FRAMEBUFFER_FORMAT_VALUES[5] = { 
    GX_TRANSFER_FMT_RGBA8, GX_TRANSFER_FMT_RGB8, GX_TRANSFER_FMT_RGB5A1, GX_TRANSFER_FMT_RGB565, GX_TRANSFER_FMT_RGBA4 };

/*
Translate from the following GSPGPU_FramebufferFormats to their respective GX_TRANSFER_FMT values:
  GSP_RGBA8_OES =0, 
  GSP_BGR8_OES =1, 
  GSP_RGB565_OES =2, 
  GSP_RGB5_A1_OES =3, 
  GSP_RGBA4_OES =4 
*/
const uint32 GX_TRANSFER_SCREEN_FORMAT_VALUES[5]= { 
    GX_TRANSFER_FMT_RGBA8, GX_TRANSFER_FMT_RGB8, GX_TRANSFER_FMT_RGB565, GX_TRANSFER_FMT_RGB5A1, GX_TRANSFER_FMT_RGBA4 };


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
            GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FRAMEBUFFER_FORMAT_VALUES[GPU3DS.frameBufferFormat]) | 
            GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_SCREEN_FORMAT_VALUES[GPU3DS.screenFormat]));
        //gspWaitForPPF();
    }
}

void gpu3dsSwapScreenBuffers()
{
	gfxSwapBuffersGpu();
    //sf2d_pool_reset();        
}



uint32 currentTexture = 0;

void gpu3dsBindTexture(SGPUTexture *texture, GPU_TEXUNIT unit)
{
    if (currentTexture != (uint32) texture)
    {
        GPU_SetTextureEnable(unit);

        GPU_SetTexEnv(
            0,
            GPU_TEVSOURCES(GPU_TEXTURE0, GPU_TEXTURE0, GPU_TEXTURE0),
            GPU_TEVSOURCES(GPU_TEXTURE0, GPU_TEXTURE0, GPU_TEXTURE0),
            GPU_TEVOPERANDS(0, 0, 0),
            GPU_TEVOPERANDS(0, 0, 0),
            GPU_REPLACE, GPU_REPLACE,
            0x80808080
        );

        GPU_SetTexture(
            unit,
            (u32 *)osConvertVirtToPhys(texture->PixelData),
            texture->Width,
            texture->Height,
            texture->Params,
            texture->PixelFormat
        );

        GPU_SetFloatUniform(GPU_VERTEX_SHADER, 4, (u32 *)texture->TextureScale, 1);
        GPU_SetFloatUniform(GPU_GEOMETRY_SHADER, 14, (u32 *)texture->TextureScale, 1);
        
        currentTexture = (uint32) texture;
        GPU3DS.currentTexture = texture;
    }
}

void gpu3dsBindTextureWithParams(SGPUTexture *texture, GPU_TEXUNIT unit, u32 param)
{
    if (currentTexture != (uint32) texture)
    {
        GPU_SetTextureEnable(unit);

        GPU_SetTexEnv(
            0,
            GPU_TEVSOURCES(GPU_TEXTURE0, GPU_TEXTURE0, GPU_TEXTURE0),
            GPU_TEVSOURCES(GPU_TEXTURE0, GPU_TEXTURE0, GPU_TEXTURE0),
            GPU_TEVOPERANDS(0, 0, 0),
            GPU_TEVOPERANDS(0, 0, 0),
            GPU_REPLACE, GPU_REPLACE,
            0x80808080
        );

        GPU_SetTexture(
            unit,
            (u32 *)osConvertVirtToPhys(texture->PixelData),
            texture->Width,
            texture->Height,
            param,
            texture->PixelFormat
        );

        GPU_SetFloatUniform(GPU_VERTEX_SHADER, 4, (u32 *)texture->TextureScale, 1);
        GPU_SetFloatUniform(GPU_GEOMETRY_SHADER, 14, (u32 *)texture->TextureScale, 1);
        
        currentTexture = (uint32) texture;
        GPU3DS.currentTexture = texture;
    }
}

void gpu3dsSetMode7UpdateFrameCountUniform()
{
    int updateFrame = GPU3DS.mode7FrameCount;
    GPU3DS.mode7UpdateFrameCount[0] = ((float)updateFrame) - 0.5f;      // set 'w' to updateFrame

    GPU_SetFloatUniform(GPU_VERTEX_SHADER, 5, (u32 *)GPU3DS.mode7UpdateFrameCount, 1);
    GPU_SetFloatUniform(GPU_GEOMETRY_SHADER, 15, (u32 *)GPU3DS.mode7UpdateFrameCount, 1);
}


void gpu3dsCopyVRAMTilesIntoMode7TileVertexes(uint8 *VRAM)
{
    for (int i = 0; i < 16384; i++)
    {
        gpu3dsSetMode7TileTexturePos(i, VRAM[i * 2]);
        gpu3dsSetMode7TileModifiedFlag(i);
    }
}

void gpu3dsIncrementMode7UpdateFrameCount()
{
    gpu3dsSwapVertexListForNextFrame(&GPU3DS.mode7TileVertexes);
    GPU3DS.mode7FrameCount ++;

    if (GPU3DS.mode7FrameCount == 0x3fff)
    {
        GPU3DS.mode7FrameCount = 1;
        for (int i = 0; i < 16384; )
        {
            gpu3dsSetMode7TileModifiedFlag(i++, 0);
            gpu3dsSetMode7TileModifiedFlag(i++, 0);
            gpu3dsSetMode7TileModifiedFlag(i++, 0);
            gpu3dsSetMode7TileModifiedFlag(i++, 0);

            gpu3dsSetMode7TileModifiedFlag(i++, 0);
            gpu3dsSetMode7TileModifiedFlag(i++, 0);
            gpu3dsSetMode7TileModifiedFlag(i++, 0);
            gpu3dsSetMode7TileModifiedFlag(i++, 0);
        }
    }

    gpu3dsSetMode7UpdateFrameCountUniform();
}


void gpu3dsBindTextureSnesMode7TileCache(GPU_TEXUNIT unit)
{
    if (currentTexture != (uint32) snesMode7TileCacheTexture)
    {
        gpu3dsBindTexture(snesMode7TileCacheTexture, unit);
        currentTexture = (uint32) snesMode7TileCacheTexture;
    }
}

void gpu3dsBindTextureSnesMode7Tile0CacheRepeat(GPU_TEXUNIT unit)
{
    if (currentTexture != (uint32) snesMode7Tile0Texture)
    {
        gpu3dsBindTextureWithParams(snesMode7Tile0Texture, unit,
            GPU_TEXTURE_MAG_FILTER(GPU_NEAREST)
            | GPU_TEXTURE_MIN_FILTER(GPU_NEAREST)
            | GPU_TEXTURE_WRAP_S(GPU_REPEAT)
            | GPU_TEXTURE_WRAP_T(GPU_REPEAT));
        currentTexture = (uint32) snesMode7Tile0Texture;
    }
}

void gpu3dsBindTextureSnesMode7Full(GPU_TEXUNIT unit)
{
    if (currentTexture != (uint32) snesMode7FullTexture)
    {
        gpu3dsBindTextureWithParams(snesMode7FullTexture, unit,
            GPU_TEXTURE_MAG_FILTER(GPU_NEAREST)
            | GPU_TEXTURE_MIN_FILTER(GPU_NEAREST)
            | GPU_TEXTURE_WRAP_S(GPU_CLAMP_TO_BORDER)
            | GPU_TEXTURE_WRAP_T(GPU_CLAMP_TO_BORDER));
        currentTexture = (uint32) snesMode7FullTexture;
    }
}

void gpu3dsBindTextureSnesMode7FullRepeat(GPU_TEXUNIT unit)
{
    if (currentTexture != (uint32) snesMode7FullTexture)
    {
        gpu3dsBindTextureWithParams(snesMode7FullTexture, unit,
            GPU_TEXTURE_MAG_FILTER(GPU_NEAREST)
            | GPU_TEXTURE_MIN_FILTER(GPU_NEAREST)
            | GPU_TEXTURE_WRAP_S(GPU_REPEAT)
            | GPU_TEXTURE_WRAP_T(GPU_REPEAT));
        currentTexture = (uint32) snesMode7FullTexture;
    }
}


void gpu3dsBindTextureSnesTileCache(GPU_TEXUNIT unit)
{
    if (currentTexture != (uint32) snesTileCacheTexture)
    {
        gpu3dsBindTexture(snesTileCacheTexture, unit);
        currentTexture = (uint32) snesTileCacheTexture;
    }
}

void gpu3dsBindTextureMainScreen(GPU_TEXUNIT unit)
{
    if (currentTexture != (uint32) snesMainScreenTarget)
    {
        gpu3dsBindTextureWithParams(snesMainScreenTarget, unit,
            GPU_TEXTURE_MAG_FILTER(GPU_LINEAR)
            | GPU_TEXTURE_MIN_FILTER(GPU_LINEAR)
            | GPU_TEXTURE_WRAP_S(GPU_CLAMP_TO_BORDER)
            | GPU_TEXTURE_WRAP_T(GPU_CLAMP_TO_BORDER));
        
        currentTexture = (uint32) snesMainScreenTarget;
    }
}

void gpu3dsBindTextureSubScreen(GPU_TEXUNIT unit)
{
    if (currentTexture != (uint32) snesSubScreenTarget)
    {
        gpu3dsBindTexture(snesSubScreenTarget, unit);
        currentTexture = (uint32) snesSubScreenTarget;
    }
}


void gpu3dsScissorTest(GPU_SCISSORMODE mode, uint32 x, uint32 y, uint32 w, uint32 h)
{
    GPU_SetScissorTest(mode, x, y, w, h);
}


u32 rectBufferOffsets[1] = { 0 };
u64 rectAttribPermutations[1] = { 0x210 };
u8 rectNumberOfAttribs[1] = { 2 };

void gpu3dsDrawRectangle(int x0, int y0, int x1, int y1, int depth, u32 color)
{
    if (GPU3DS.isReal3DS)
    {
        SVertexColor *vertices = GPU3DS.rectangleVertexList;
        if (!vertices) return;

        vertices[0].Position = (SVector4i){x0, y0, depth, 1};
        vertices[1].Position = (SVector4i){x1, y1, depth, 1};

        u32 swappedColor = ((color & 0xff) << 24) | ((color & 0xff00) << 8) | ((color & 0xff0000) >> 8) | ((color & 0xff000000) >> 24);
        vertices[0].Color = swappedColor;
        vertices[1].Color = swappedColor;

        gpu3dsSetAttributeBuffers(
            2, // number of attributes
            (u32*)vertices,
            GPU_ATTRIBFMT(0, 4, GPU_SHORT) | GPU_ATTRIBFMT(1, 4, GPU_UNSIGNED_BYTE),
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
        
    }
    else
    {
        SVertexColor *vertices = GPU3DS.rectangleVertexList;
        if (!vertices) return;

        vertices[0].Position = (SVector4i){x0, y0, depth, 1};
        vertices[1].Position = (SVector4i){x1, y0, depth, 1};
        vertices[2].Position = (SVector4i){x0, y1, depth, 1};
        vertices[3].Position = (SVector4i){x1, y1, depth, 1};

        u32 swappedColor = ((color & 0xff) << 24) | ((color & 0xff00) << 8) | ((color & 0xff0000) >> 8) | ((color & 0xff000000) >> 24);
        vertices[0].Color = swappedColor;
        vertices[1].Color = swappedColor;
        vertices[2].Color = swappedColor;
        vertices[3].Color = swappedColor;

        gpu3dsSetAttributeBuffers(
            2, // number of attributes
            (u32*)vertices,
            GPU_ATTRIBFMT(0, 4, GPU_SHORT) | GPU_ATTRIBFMT(1, 4, GPU_UNSIGNED_BYTE),
            0xFFFF, //0b1100
            0x10,
            1, //number of buffers
            rectBufferOffsets, // buffer offsets (placeholders)
            rectAttribPermutations, // attribute permutations for each buffer
            rectNumberOfAttribs // number of attributes for each buffer
        );

        GPU3DS.rectangleVertexList = (SVertexColor *) gpu3dsAlignTo0x80(&GPU3DS.rectangleVertexList[4]);        

        GPU_DrawArray(GPU_TRIANGLE_STRIP, 0, 4);
        somethingWasDrawn = true;
    }
}




void gpu3dsDrawVertexes(bool repeatLastDraw)
{
    gpu3dsDrawVertexList(&GPU3DS.quadVertexes, GPU_TRIANGLES, repeatLastDraw);
    gpu3dsDrawVertexList(&GPU3DS.tileVertexes, GPU_GEOMETRY_PRIM, repeatLastDraw);
}


void gpu3dsDrawMode7Vertexes(int fromIndex, int tileCount)
{
    if (GPU3DS.isReal3DS)
        gpu3dsDrawMode7VertexList(&GPU3DS.mode7TileVertexes, GPU_GEOMETRY_PRIM, fromIndex, tileCount);
    else
        gpu3dsDrawMode7VertexList(&GPU3DS.mode7TileVertexes, GPU_TRIANGLES, fromIndex, tileCount);
    
}
