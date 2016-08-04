
#ifndef _3DSGPU_H_
#define _3DSGPU_H_

#include <3ds.h>
#include "sf2d_private.h"
#include "3dssnes9x.h"

#define COMPOSE_HASH(vramAddr, pal)   ((vramAddr) << 4) + ((pal) & 0xf)

#define SUBSCREEN_Y         256

struct SVector3i
{
    int16 x, y, z;
};

struct SVector4i
{
    int16 x, y, z, w;
};


struct SVector3f
{
    float x, y, z;
};

struct STexCoord2i
{
    int16 u, v;
};



struct STileVertex {
    struct SVector3i    Position;  
	struct STexCoord2i  TexCoord;  
};


struct SVertexColor {
    struct SVector4i    Position;  
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
    int             SizeInBytes = 0;
    int             Total = 0;
    int             Count = 0;
    STileVertex     *ListOriginal;
    STileVertex     *List;
    STileVertex     *ListBase;
    int             Flip = 0;
} STileVertexList; 

typedef struct 
{
    GSPGPU_FramebufferFormats   screenFormat;
    GPU_TEXCOLOR                frameBufferFormat;
    
    u32             *frameBuffer;
    u32             *frameDepthBuffer;

    float           projectionTopScreen[16];
    float           projectionBottomScreen[16];

    STileVertexList quadVertexes;
    STileVertexList tileVertexes;
    STileVertexList mode7Vertexes;

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
    uint16  vramCacheHashToTexturePosition[MAX_HASH + 1];               

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

#define EMUSTATE_EMULATE        1
#define EMUSTATE_PAUSEMENU      2


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
        if (GPU3DS.newCacheTexturePosition >= MAX_TEXTURE_POSITIONS)
            GPU3DS.newCacheTexturePosition = 1;
    }
    
    return pos;
}

inline int cacheGetMode7TexturePositionFast(int tileNumber);

void gpu3dsCacheToTexturePosition(uint8 *snesTilePixels, uint16 *snesPalette, uint16 texturePosition);
void gpu3dsCacheToMode7TexturePosition(uint8 *snesTilePixels, uint16 *snesPalette, uint16 texturePosition, uint32 *paletteMask);

	
bool gpu3dsInitialize();
void gpu3dsFinalize();

SGPUTexture *gpu3dsCreateTextureInLinearMemory(int width, int height, GPU_TEXCOLOR pixelFormat);
SGPUTexture *gpu3dsCreateTextureInVRAM(int width, int height, GPU_TEXCOLOR pixelFormat);

void gpu3dsStartNewFrame();

void gpu3dsUseNextSectionOfMode7VertexList();

void gpu3dsResetState(); 
void gpu3dsLoadShader(int shaderIndex, u32 *shaderBinary, int size, int geometryShaderStride);
void gpu3dsUseShader(int shaderIndex);

void gpu3dsSetRenderTarget(int renderTarget);
void gpu3dsSetRenderTargetToTopFrameBuffer();
void gpu3dsSetRenderTargetToMainScreenTexture();
void gpu3dsSetRenderTargetToSubScreenTexture();
void gpu3dsSetRenderTargetToMode7FullTexture(int pixelOffset, int width, int height);

void gpu3dsFlush();
void gpu3dsWaitForPreviousFlush();
void gpu3dsFrameEnd();

void gpu3dsClearRenderTarget();
void gpu3dsClearAllRenderTargets();
void gpu3dsTransferToScreenBuffer();
void gpu3dsSwapScreenBuffers();

void gpu3dsEnableAlphaTest();
void gpu3dsDisableAlphaTest();

void gpu3dsEnableDepthTest();
void gpu3dsDisableDepthTest();

void gpu3dsClearTextureEnv(u8 num);
void gpu3dsSetTextureEnvironmentReplaceColor();
void gpu3dsSetTextureEnvironmentReplaceTexture0();

void gpu3dsBindTexture(SGPUTexture *texture, GPU_TEXUNIT unit);
void gpu3dsBindTextureSnesTileCache(GPU_TEXUNIT unit);
void gpu3dsBindTextureSnesMode7TileCache(GPU_TEXUNIT unit);
void gpu3dsBindTextureSnesMode7FullRepeat(GPU_TEXUNIT unit);
void gpu3dsBindTextureSnesMode7Full(GPU_TEXUNIT unit);
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
    int x0, int y0, int x1, int y1,
    int tx0, int ty0, int tx1, int ty1,
    int data)
{
    //STileVertex *vertices = &GPU3DS.vertexList[GPU3DS.vertexCount];
    STileVertex *vertices = &GPU3DS.quadVertexes.List[GPU3DS.quadVertexes.Count];

	vertices[0].Position = (SVector3i){x0, y0, data};
	vertices[1].Position = (SVector3i){x1, y0, data};
	vertices[2].Position = (SVector3i){x0, y1, data};

	vertices[3].Position = (SVector3i){x1, y1, data};
	vertices[4].Position = (SVector3i){x0, y1, data};
	vertices[5].Position = (SVector3i){x1, y0, data};

	vertices[0].TexCoord = (STexCoord2i){tx0, ty0};
	vertices[1].TexCoord = (STexCoord2i){tx1, ty0};
	vertices[2].TexCoord = (STexCoord2i){tx0, ty1};
    
	vertices[3].TexCoord = (STexCoord2i){tx1, ty1};
	vertices[4].TexCoord = (STexCoord2i){tx0, ty1};
	vertices[5].TexCoord = (STexCoord2i){tx1, ty0};
    
    //GPU3DS.vertexCount += 6;
    GPU3DS.quadVertexes.Count += 6;
}


inline void __attribute__((always_inline)) gpu3dsAddTileVertexes(
    int x0, int y0, int x1, int y1, 
    int tx0, int ty0, int tx1, int ty1, 
    int data)
{
#ifndef RELEASE
    if (GPU3DS.isReal3DS)
    {
#endif
        //STileVertex *vertices = &GPU3DS.tileList[GPU3DS.tileCount];
        STileVertex *vertices = &GPU3DS.tileVertexes.List[GPU3DS.tileVertexes.Count];

        vertices[0].Position = (SVector3i){x0, y0, data};
        vertices[0].TexCoord = (STexCoord2i){tx0, ty0};
        
        vertices[1].Position = (SVector3i){x1, y1, data};
        vertices[1].TexCoord = (STexCoord2i){tx1, ty1};
        
        //GPU3DS.tileCount += 2;
        GPU3DS.tileVertexes.Count += 2;

#ifndef RELEASE        
    }
    else
    {        
        // This is used for testing in Citra, since Citra doesn't implement
        // the geometry shader required in the tile renderer
        //
        //STileVertex *vertices = &GPU3DS.vertexList[GPU3DS.vertexCount];
        STileVertex *vertices = &GPU3DS.quadVertexes.List[GPU3DS.quadVertexes.Count];
        
        vertices[0].Position = (SVector3i){x0, y0, data};
        vertices[1].Position = (SVector3i){x1, y0, data};
        vertices[2].Position = (SVector3i){x0, y1, data};

        vertices[3].Position = (SVector3i){x1, y1, data};
        vertices[4].Position = (SVector3i){x0, y1, data};
        vertices[5].Position = (SVector3i){x1, y0, data};

        vertices[0].TexCoord = (STexCoord2i){tx0, ty0};
        vertices[1].TexCoord = (STexCoord2i){tx1, ty0};
        vertices[2].TexCoord = (STexCoord2i){tx0, ty1};
        
        vertices[3].TexCoord = (STexCoord2i){tx1, ty1};
        vertices[4].TexCoord = (STexCoord2i){tx0, ty1};
        vertices[5].TexCoord = (STexCoord2i){tx1, ty0};
        
        //GPU3DS.vertexCount += 6;
        GPU3DS.quadVertexes.Count += 6;
    }
#endif

}


inline void __attribute__((always_inline)) gpu3dsAddMode7TileUpdateVertexes(
    int x0, int y0, int x1, int y1, 
    int data)
{
#ifndef RELEASE
    if (GPU3DS.isReal3DS)
    {
#endif
        STileVertex *vertices = &GPU3DS.mode7Vertexes.List[GPU3DS.mode7Vertexes.Count];

        vertices[0].Position = (SVector3i){x0, y0, data};
        vertices[1].Position = (SVector3i){x1, y1, data};
        
        /*
        vertices[0].TexCoord = (STexCoord2i){0, 0};
        vertices[1].TexCoord = (STexCoord2i){8, 8};
        */

        //GPU3DS.tileCount += 2;
        GPU3DS.mode7Vertexes.Count += 2;

#ifndef RELEASE        
    }
    else
    {        
        // This is used for testing in Citra, since Citra doesn't implement
        // the geometry shader required in the tile renderer
        //
        STileVertex *vertices = &GPU3DS.mode7Vertexes.List[GPU3DS.mode7Vertexes.Count];
        
        vertices[0].Position = (SVector3i){x0, y0, data};
        vertices[1].Position = (SVector3i){x1, y0, data};
        vertices[2].Position = (SVector3i){x0, y1, data};

        vertices[3].Position = (SVector3i){x1, y1, data};
        vertices[4].Position = (SVector3i){x0, y1, data};
        vertices[5].Position = (SVector3i){x1, y0, data};

        /*
        vertices[0].TexCoord = (STexCoord2i){0, 0};
        vertices[1].TexCoord = (STexCoord2i){8, 0};
        vertices[2].TexCoord = (STexCoord2i){0, 8};
        
        vertices[3].TexCoord = (STexCoord2i){8, 8};
        vertices[4].TexCoord = (STexCoord2i){0, 8};
        vertices[5].TexCoord = (STexCoord2i){8, 0};
        */
        //GPU3DS.vertexCount += 6;
        GPU3DS.mode7Vertexes.Count += 6;
    }
#endif
}

inline void __attribute__((always_inline)) gpu3dsAddMode7ScanlineVertexes(
    int x0, int y0, int x1, int y1, 
    int tx0, int ty0, int tx1, int ty1,
    int data)
{
    if (GPU3DS.isReal3DS)
    {
        STileVertex *vertices = &GPU3DS.tileVertexes.List[GPU3DS.tileVertexes.Count];

        vertices[0].Position = (SVector3i){x0, y0, data};
        vertices[0].TexCoord = (STexCoord2i){tx0, ty0};

        // yes we will use a special value for the geometry shader to detect detect mode 7
        vertices[1].Position = (SVector3i){x1, -16384, data};      
        vertices[1].TexCoord = (STexCoord2i){tx1, ty1};
        
        //GPU3DS.tileCount += 2;
        GPU3DS.tileVertexes.Count += 2;

    }
    else
    {
        //STileVertex *vertices = &GPU3DS.vertexList[GPU3DS.vertexCount];
        STileVertex *vertices = &GPU3DS.quadVertexes.List[GPU3DS.quadVertexes.Count];

        vertices[0].Position = (SVector3i){x0, y0, data};
        vertices[0].TexCoord = (STexCoord2i){tx0, ty0};

        vertices[1].Position = (SVector3i){x1, y0, data};
        vertices[1].TexCoord = (STexCoord2i){tx1, ty1};

        vertices[2].Position = (SVector3i){x0, y1, data};
        vertices[2].TexCoord = (STexCoord2i){tx0, ty0};
        

        vertices[3].Position = (SVector3i){x1, y0, data};
        vertices[3].TexCoord = (STexCoord2i){tx1, ty1};

        vertices[4].Position = (SVector3i){x1, y1, data};
        vertices[4].TexCoord = (STexCoord2i){tx1, ty1};

        vertices[5].Position = (SVector3i){x0, y1, data};
        vertices[5].TexCoord = (STexCoord2i){tx0, ty0};
        
        //GPU3DS.vertexCount += 6;
        GPU3DS.quadVertexes.Count += 6;
    }
}

void gpu3dsDrawVertexes();

void gpu3dsDrawRectangle(int x0, int y0, int x1, int y1, int depth, u32 color);

#endif

