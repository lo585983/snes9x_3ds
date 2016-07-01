
#ifndef _3DSGPU_H_
#define _3DSGPU_H_

#include <3ds.h>
#include "sf2d_private.h"

//#define DRAWTILES
#define COMPOSE_HASH(vramAddr, pal)   ((vramAddr) << 4) + ((pal) & 0xf)

#define SUBSCREEN_Y         256

struct SVector3i
{
    int16 x, y, z;
};

struct STexCoord2i
{
    int16 u, v;
};

struct SVector3f
{
    float x, y, z;
};

struct STexCoord2f
{
    float u, v;
};

struct STileVertex {
    struct SVector3i    Position;  
	struct STexCoord2i  TexCoord;  
};

struct SVertex {
    struct SVector3f    Position;  
	struct STexCoord2f  TexCoord;
};

struct SVertexColor {
    struct SVector3f    Position;  
	u32                 Color;
};

#define MAX_TEXTURE_POSITIONS		16384
#define MAX_HASH					(65536 * 16)


typedef struct
{
	int             Memory;                     // 0 = linear memory, 1 = VRAM
	GPU_TEXCOLOR    PixelFormat;  
	u32             Params;                
	int             Width;                 
	int             Height;                
	void            *PixelData;
    int             BufferSize;
	float           Projection[4*4];      /**< Orthographic projection matrix for this target */
    float           TextureScale[4];
} SGPUTexture;

typedef struct {
	DVLB_s 				*dvlb;
	shaderProgram_s 	shaderProgram;
	u32					projectionRegister;
} SGPUShader;


typedef struct 
{
    u32             *frameBuffer;
    u32             *frameDepthBuffer;

    float           projectionTopScreen[16];
    float           projectionBottomScreen[16];

    int             vertexTotal = 0;
    int             vertexCount = 0;
    SVertex         *vertexList;
    SVertex         *vertexListBase;

    int             tileTotal = 0;
    int             tileCount = 0;
    STileVertex     *tileList;
    STileVertex     *tileListBase;

    int             rectangleCount = 0;
    SVertexColor    *rectangleVertexList;
    SVertexColor    *rectangleVertexListBase;

    SGPUTexture     *currentTexture;
    SGPUTexture     *currentRenderTarget;
    int             currentRenderTargetIndex;
    int             targetDepthBufferSize = 0;
    void            *targetDepthBuffer;

    SGPUShader      shaders[10];
    int             currentShader = -1;

    // Memory Usage = 2.00 MB (for hashing of the texture position)
    uint16  vramCacheHashToTexturePosition[MAX_HASH];               

    // Memory Usage = 0.06 MB
    int     vramCacheTexturePositionToHash[MAX_TEXTURE_POSITIONS];
  
    int     newCacheTexturePosition = 1;    

    bool    isReal3DS = false;
    bool    enableDebug = false;
    int     emulatorState = 0;
    
} SGPU3DS;


#ifndef _3DSGPU_CPP_
extern SGPU3DS GPU3DS;
#else
SGPU3DS GPU3DS;


#endif

void cacheInit();

int cacheGetTexturePosition(int hash);

inline int cacheGetTexturePositionFast(int hash)
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

void gpu3dsCacheToTexturePosition(
    uint8 *snesTilePixels, 
	uint16 *snesPalette,
    uint16 texturePosition);

	
bool gpu3dsInitialize();

SGPUTexture *gpu3dsCreateTextureInLinearMemory(int width, int height, GPU_TEXCOLOR pixelFormat);

SGPUTexture *gpu3dsCreateTextureInVRAM(int width, int height, GPU_TEXCOLOR pixelFormat);

//sf2d_texture *gpu3dsCreateTexture(int width, int height, sf2d_texfmt pixel_format, sf2d_place place);

void gpu3dsStartNewFrame();

void gpu3dsResetState();

void gpu3dsLoadShader(int shaderIndex, u32 *shaderBinary, 
    int size, int geometryShaderStride);

void gpu3dsUseShader(int shaderIndex);

void gpu3dsSetRenderTarget(int renderTarget);

void gpu3dsFlush();

void gpu3dsWaitForPreviousFlush();

void gpu3dsFrameEnd();

void gpu3dsClearRenderTarget();

void gpu3dsClearAllRenderTargets();

void gpu3dsTransferToScreenBuffer();

void gpu3dsSwapScreenBuffers();

void gpu3dsEnableDepthTest();

void gpu3dsDisableDepthTest();

void gpu3dsClearTextureEnv(u8 num);

void gpu3dsSetTextureEnvironmentReplaceColor();

void gpu3dsSetTextureEnvironmentReplaceTexture0();

void gpu3dsBindTexture(SGPUTexture *texture, GPU_TEXUNIT unit);

void gpu3dsBindTextureSnesTileCache(GPU_TEXUNIT unit);

void gpu3dsBindTextureSubScreen(GPU_TEXUNIT unit);

void gpu3dsBindTextureMainScreen(GPU_TEXUNIT unit);

void gpu3dsScissorTest(GPU_SCISSORMODE mode, uint32 x, uint32 y, uint32 w, uint32 h);

void gpu3dsEnableAlphaBlending();

void gpu3dsEnableAdditiveBlending();

void gpu3dsEnableSubtractiveBlending();

void gpu3dsEnableAdditiveDiv2Blending();

void gpu3dsEnableSubtractiveDiv2Blending();

void gpu3dsDisableAlphaBlending();

inline void __attribute__((always_inline)) gpu3dsAddQuadVertexes(
    float x0, float y0, float x1, float y1,
    float tx0, float ty0, float tx1, float ty1,
    float depth)
{
    SVertex *vertices = &GPU3DS.vertexList[GPU3DS.vertexCount];

	vertices[0].Position = (SVector3f){x0, y0, depth};
	vertices[1].Position = (SVector3f){x1, y0, depth};
	vertices[2].Position = (SVector3f){x0, y1, depth};

	vertices[3].Position = (SVector3f){x1, y1, depth};
	vertices[4].Position = (SVector3f){x0, y1, depth};
	vertices[5].Position = (SVector3f){x1, y0, depth};

	vertices[0].TexCoord = (STexCoord2f){tx0, ty0};
	vertices[1].TexCoord = (STexCoord2f){tx1, ty0};
	vertices[2].TexCoord = (STexCoord2f){tx0, ty1};
    
	vertices[3].TexCoord = (STexCoord2f){tx1, ty1};
	vertices[4].TexCoord = (STexCoord2f){tx0, ty1};
	vertices[5].TexCoord = (STexCoord2f){tx1, ty0};
    
    GPU3DS.vertexCount += 6;
}


inline void __attribute__((always_inline)) gpu3dsAddTileVertexes(
    int16 x0, int16 y0, int16 x1, int16 y1, 
    int16 tx0, int16 ty0, int16 tx1, int16 ty1, 
    int16 depth)
{
#ifndef DRAWTILES
    if (GPU3DS.isReal3DS)
    {
#endif
        STileVertex *vertices = &GPU3DS.tileList[GPU3DS.tileCount];

        vertices[0].Position = (SVector3i){x0, y0, depth};
        vertices[0].TexCoord = (STexCoord2i){tx0, ty0};
        
        vertices[1].Position = (SVector3i){x1, y1, depth};
        vertices[1].TexCoord = (STexCoord2i){tx1, ty1};
        
        GPU3DS.tileCount += 2;

#ifndef DRAWTILES        
    }
    else
    {        
        // This is used for testing in Citra, since Citra doesn't implement
        // the geometry shader required in the tile renderer
        //
        SVertex *vertices = &GPU3DS.vertexList[GPU3DS.vertexCount];
        
        /*tx0 = tx0 / 1024;
        tx1 = tx1 / 1024;
        ty0 = ty0 / 1024;
        ty1 = ty1 / 1024;*/

        vertices[0].Position = (SVector3f){x0, y0, depth};
        vertices[1].Position = (SVector3f){x1, y0, depth};
        vertices[2].Position = (SVector3f){x0, y1, depth};

        vertices[3].Position = (SVector3f){x1, y1, depth};
        vertices[4].Position = (SVector3f){x0, y1, depth};
        vertices[5].Position = (SVector3f){x1, y0, depth};

        vertices[0].TexCoord = (STexCoord2f){tx0, ty0};
        vertices[1].TexCoord = (STexCoord2f){tx1, ty0};
        vertices[2].TexCoord = (STexCoord2f){tx0, ty1};
        
        vertices[3].TexCoord = (STexCoord2f){tx1, ty1};
        vertices[4].TexCoord = (STexCoord2f){tx0, ty1};
        vertices[5].TexCoord = (STexCoord2f){tx1, ty0};
        
        GPU3DS.vertexCount += 6;
    }
#endif

}


void gpu3dsDrawVertexes();

void gpu3dsDrawRectangle(int x0, int y0, int x1, int y1, float depth, u32 color);

#endif

