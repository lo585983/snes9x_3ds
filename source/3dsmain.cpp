#include <algorithm>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <vector>

#include <string.h>
#include <stdio.h>
#include <3ds.h>

#include <dirent.h>
#include "snes9x.h"
#include "memmap.h"
#include "apu.h"
#include "gfx.h"
#include "snapshot.h"
#include "cheats.h"
#include "movie.h"
#include "display.h"
#include "soundux.h"

#include "3dsgpu.h"
#include "3dsopt.h"
#include "3dssound.h"
#include "3dsmenu.h"


#define CONSOLE_WIDTH           40
#define CONSOLE_HEIGHT          (28 - 2)
#define S9X3DS_VERSION	        "0.1" 


typedef struct
{
    int     EnableFrameSkipping = 1;        // 0 - disable, 1 - enable
    int     EnableScreenStretch = 0;        // 0 - no stretch, 1 - aspect fit, 2 - scale to full
} S9x3DSSettings;




void _splitpath (const char *path, char *drive, char *dir, char *fname, char *ext)
{
	*drive = 0;

	const char	*slash = strrchr(path, SLASH_CHAR),
				*dot   = strrchr(path, '.');

	if (dot && slash && dot < slash)
		dot = NULL;

	if (!slash)
	{
		*dir = 0;

		strcpy(fname, path);

		if (dot)
		{
			fname[dot - path] = 0;
			strcpy(ext, dot + 1);
		}
		else
			*ext = 0;
	}
	else
	{
		strcpy(dir, path);
		dir[slash - path] = 0;

		strcpy(fname, slash + 1);

		if (dot)
		{
			fname[dot - slash - 1] = 0;
			strcpy(ext, dot + 1);
		}
		else
			*ext = 0;
	}
}

void _makepath (char *path, const char *, const char *dir, const char *fname, const char *ext)
{
	if (dir && *dir)
	{
		strcpy(path, dir);
		strcat(path, SLASH_STR);
	}
	else
		*path = 0;

	strcat(path, fname);

	if (ext && *ext)
	{
		strcat(path, ".");
		strcat(path, ext);
	}
}

void S9xMessage (int type, int number, const char *message)
{
	printf("%s\n", message);
}

bool8 S9xInitUpdate (void)
{
	return (TRUE);
}

bool8 S9xDeinitUpdate (int width, int height, bool8 sixteen_bit)
{
	return (TRUE);
}


void S9xAutoSaveSRAM (void)
{
	Memory.SaveSRAM (S9xGetFilename (".srm"));
}

void S9xGenerateSound ()
{
}


void S9xExit (void)
{

}

void S9xSetPalette (void)
{
	return;
}


bool8 S9xOpenSoundDevice(int mode, bool8 stereo, int buffer_size)
{
	return (TRUE);
}

void S9xLoadSDD1Data ()
{
    /*
    char filename [_MAX_PATH + 1];
    char index [_MAX_PATH + 1];
    char data [_MAX_PATH + 1];

	Settings.SDD1Pack=FALSE;
    Memory.FreeSDD1Data ();

    if (strncmp (Memory.ROMName, TEXT("Star Ocean"), 10) == 0)
	{
		if(strlen(GUI.StarOceanPack)!=0)
			strcpy(filename, GUI.StarOceanPack);
		else Settings.SDD1Pack=TRUE;
	}
    else if(strncmp(Memory.ROMName, TEXT("STREET FIGHTER ALPHA2"), 21)==0)
	{
		if(Memory.ROMRegion==1)
		{
			if(strlen(GUI.SFA2NTSCPack)!=0)
				strcpy(filename, GUI.SFA2NTSCPack);
			else Settings.SDD1Pack=TRUE;
		}
		else
		{
			if(strlen(GUI.SFA2PALPack)!=0)
				strcpy(filename, GUI.SFA2PALPack);
			else Settings.SDD1Pack=TRUE;
		}
	}
	else
	{
		if(strlen(GUI.SFZ2Pack)!=0)
			strcpy(filename, GUI.SFZ2Pack);
		else Settings.SDD1Pack=TRUE;
	}

	if(Settings.SDD1Pack==TRUE)
		return;

    strcpy (index, filename);
    strcat (index, "\\SDD1GFX.IDX");
    strcpy (data, filename);
    strcat (data, "\\SDD1GFX.DAT");

    FILE *fs = fopen (index, "rb");
    int len = 0;
    
    if (fs)
    {
        // Index is stored as a sequence of entries, each entry being
        // 12 bytes consisting of:
        // 4 byte key: (24bit address & 0xfffff * 16) | translated block
        // 4 byte ROM offset
        // 4 byte length
        fseek (fs, 0, SEEK_END);
        len = ftell (fs);
        rewind (fs);
        Memory.SDD1Index = (uint8 *) malloc (len);
        fread (Memory.SDD1Index, 1, len, fs);
        fclose (fs);
        Memory.SDD1Entries = len / 12;
        
        if (!(fs = fopen (data, "rb")))
        {
            free ((char *) Memory.SDD1Index);
            Memory.SDD1Index = NULL;
            Memory.SDD1Entries = 0;
        }
        else
        {
            fseek (fs, 0, SEEK_END);
            len = ftell (fs);
            rewind (fs);
            Memory.SDD1Data = (uint8 *) malloc (len);
            fread (Memory.SDD1Data, 1, len, fs);
            fclose (fs);
            
            qsort (Memory.SDD1Index, Memory.SDD1Entries, 12,
                   S9xCompareSDD1IndexEntries);
        }
    }*/
}

const char * S9xGetFilename (const char *ex)
{
	static char	s[PATH_MAX + 1];
	char		drive[_MAX_DRIVE + 1], dir[_MAX_DIR + 1], fname[_MAX_FNAME + 1], ext[_MAX_EXT + 1];

	_splitpath(Memory.ROMFilename, drive, dir, fname, ext);
	snprintf(s, PATH_MAX + 1, "%s%s", fname, ex);

	return (s);
}

const char * S9xGetFilenameInc (const char *ex)
{
	static char	s[PATH_MAX + 1];
	char		drive[_MAX_DRIVE + 1], dir[_MAX_DIR + 1], fname[_MAX_FNAME + 1], ext[_MAX_EXT + 1];

	unsigned int	i = 0;
	const char		*d;
	struct stat		buf;

	_splitpath(Memory.ROMFilename, drive, dir, fname, ext);

	do
		snprintf(s, PATH_MAX + 1, "%s.%03d%s", fname, i++, ex);
	while (stat(s, &buf) == 0 && i < 1000);

	return (s);
}

uint32 n3dsKeysHeld = 0;
uint32 lastKeysHeld = 0;
uint32 menuKeyDown = 0;

uint32 S9xReadJoypad (int which1_0_to_4)
{
    if (which1_0_to_4 != 0)
        return 0;   
    
    uint32 s9xKeysHeld = n3dsKeysHeld;

    if (menuKeyDown)
    {
        // If the key remains pressed after coming back
        // from the menu, we are going to mask it
        // until the user releases it.
        //
        if (s9xKeysHeld & menuKeyDown)
        {
            s9xKeysHeld = s9xKeysHeld & ~menuKeyDown;
        }
        else
            menuKeyDown = 0;
    }

    uint32 joyPad = 0;

    if (s9xKeysHeld & KEY_UP) joyPad |= SNES_UP_MASK;
    if (s9xKeysHeld & KEY_DOWN) joyPad |= SNES_DOWN_MASK;
    if (s9xKeysHeld & KEY_LEFT) joyPad |= SNES_LEFT_MASK;
    if (s9xKeysHeld & KEY_RIGHT) joyPad |= SNES_RIGHT_MASK;
    if (s9xKeysHeld & KEY_L) joyPad |= SNES_TL_MASK;
    if (s9xKeysHeld & KEY_R) joyPad |= SNES_TR_MASK;
    if (s9xKeysHeld & KEY_SELECT) joyPad |= SNES_SELECT_MASK;
    if (s9xKeysHeld & KEY_START) joyPad |= SNES_START_MASK;
    if (s9xKeysHeld & KEY_A) joyPad |= SNES_A_MASK;
    if (s9xKeysHeld & KEY_B) joyPad |= SNES_B_MASK;
    if (s9xKeysHeld & KEY_X) joyPad |= SNES_X_MASK;
    if (s9xKeysHeld & KEY_Y) joyPad |= SNES_Y_MASK;
    
    return joyPad;
}

bool8 S9xReadMousePosition (int which1_0_to_1, int &x, int &y, uint32 &buttons)
{
    
}

bool8 S9xReadSuperScopePosition (int &x, int &y, uint32 &buttons)
{
    
}

bool JustifierOffscreen()
{
	return 0;
}

void JustifierButtons(uint32& justifiers)
{
	
}

char * osd_GetPackDir(void)
{
    
    return NULL;
}

const char *S9xBasename (const char *f)
{
    const char *p;
    if ((p = strrchr (f, '/')) != NULL || (p = strrchr (f, '\\')) != NULL)
	return (p + 1);

    if (p = strrchr (f, SLASH_CHAR))
	return (p + 1);

    return (f);
}

bool8 S9xOpenSnapshotFile (const char *filename, bool8 read_only, STREAM *file)
{
    
	char	s[PATH_MAX + 1];
	char	drive[_MAX_DRIVE + 1], dir[_MAX_DIR + 1], fname[_MAX_FNAME + 1], ext[_MAX_EXT + 1];

    snprintf(s, PATH_MAX + 1, "%s", filename);

	if ((*file = OPEN_STREAM(s, read_only ? "rb" : "wb")))
		return (TRUE);

	return (FALSE);
}

void S9xCloseSnapshotFile (STREAM file)
{
	CLOSE_STREAM(file);
}

void S9xParseArg (char **argv, int &index, int argc)
{
    
}

void S9xExtraUsage ()
{

}

void S9xGraphicsMode ()
{
    
}
void S9xTextMode ()
{
    
}
void S9xSyncSpeed (void)
{
}


//-------------------------------------------
// Reads and processes Joy Pad buttons.
//-------------------------------------------
uint32 readJoypadButtons()
{
    hidScanInput();
    n3dsKeysHeld = hidKeysHeld();

    u32 keysDown = (~lastKeysHeld) & n3dsKeysHeld;
    lastKeysHeld = n3dsKeysHeld;

    if (GPU3DS.enableDebug)
    {
        if (keysDown || (n3dsKeysHeld & KEY_L))
            Settings.Paused = false;
        else
            Settings.Paused = true;
    }
    /*if (keysDown & (KEY_SELECT))
    {
        GPU3DS.enableDebug = !GPU3DS.enableDebug;
        printf ("Debug mode = %d\n", GPU3DS.enableDebug);
    }*/
    if (keysDown & KEY_TOUCH)
    {
        if (GPU3DS.emulatorState == EMUSTATE_EMULATE)
            GPU3DS.emulatorState = EMUSTATE_PAUSEMENU;
    }
    return keysDown;
    
}




//-------------------------------------------------------
// Load the ROM and reset the CPU.
//-------------------------------------------------------
char *romFileName;

// This gives us the total time spent emulating 1 frame.
//
float timePerFrame = 1.0f / 50;
int screenHeight = 224;

int frameCount60 = 60;
u64 lastTick = 0;


void snesLoadRom()
{
    printf ("load ROM\n");
    bool loaded = Memory.LoadROM(romFileName);
    Memory.LoadSRAM (S9xGetFilename (".srm"));
    
    gpu3dsClearAllRenderTargets();
    if (loaded)
    {
        printf ("  ROM Loaded...\n");
    }
    GPU3DS.emulatorState = EMUSTATE_EMULATE;
    consoleClear();
    screenHeight = PPU.ScreenHeight;

    frameCount60 = 60;
}


//----------------------------------------------------------------------
// Menus
//----------------------------------------------------------------------
SMenuItem fileMenu[512];
char romFileNames[512][256];
int totalRomFileCount = 0;


//----------------------------------------------------------------------
// Load all ROM file names (up to 512 ROMs)
//----------------------------------------------------------------------
void fileGetAllFiles(void)
{
    std::vector<std::string> files;
    
    struct dirent* dir;
    DIR* d = opendir(".");
    if (d)
    {
        while ((dir = readdir(d)) != NULL)
        {
            if (dir->d_name[0] == '.')
                continue;
                
            char *dot = strrchr(dir->d_name, '.');
            if (strstr(dir->d_name, ".srm"))
                continue;
            if (!strstr(dir->d_name, ".smc") &&
                !strstr(dir->d_name, ".fig") &&
                !strstr(dir->d_name, ".sfc"))
                continue;
                
            files.push_back(dir->d_name);
        }
        closedir(d);
    }

    std::sort(files.begin(), files.end());

    totalRomFileCount = 0;
    for (int i = 0; i < files.size() && i < 512; i++)
    {
        strncpy(romFileNames[i], files[i].c_str(), 255);
        totalRomFileCount++;
        fileMenu[i].ID = i;
        fileMenu[i].Text = romFileNames[i];
    }
}


//----------------------------------------------------------------------
// Select the ROM file to load.
//----------------------------------------------------------------------
void menuSelectFile(void)
{
    fileGetAllFiles();
    S9xClearMenuTabs();
    S9xAddTab("Select ROM", fileMenu, totalRomFileCount);

    int selection = 0;
    do
    {
        selection = S9xMenuSelectItem();
    } while (selection == -1);

    romFileName = romFileNames[selection];
    snesLoadRom();
}


SMenuItem emulatorMenu[] = { 
    { 1000, "Resume Game" }, 
    { -1, NULL }, 
    { 2001, "Save Slot #1"}, 
    { 2002, "Save Slot #2"}, 
    { 2003, "Save Slot #3"}, 
    { 2004, "Save Slot #4"}, 
    { -1, NULL }, 
    { 3001, "Load Slot #1"}, 
    { 3002, "Load Slot #2"}, 
    { 3003, "Load Slot #3"}, 
    { 3004, "Load Slot #4"}, 
    { -1, NULL }, 
    { 4001, "Reset SNES"} 
    };



void menuPause()
{
    S9xClearMenuTabs();
    S9xAddTab("Emulator", emulatorMenu, 13);
    S9xAddTab("Select ROM", fileMenu, totalRomFileCount);

    while (true)
    {
        int selection = S9xMenuSelectItem();

        if (selection == -1 || selection == 1000)
        {
            // Cancels the menu and resumes game
            //
            GPU3DS.emulatorState = EMUSTATE_EMULATE;
            consoleClear();
            frameCount60 = 60;
            return;
        }
        else if (selection < 1000)
        {
            // Load ROM
            //
            romFileName = romFileNames[selection];
            snesLoadRom();
            return;
        }
        else if (selection >= 2001 && selection <= 2010)
        {
            int slot = selection - 2000;
            char s[256];
            sprintf(s, "Saving into slot %d...", slot);
            S9xShowMessage("Save State", s, "");

            sprintf(s, ".%d.frz", slot);
            Snapshot(S9xGetFilename (s)); 

            sprintf(s, "Slot %d save complete", slot);
            S9xShowAlert("Save State", s, "");
        }
        else if (selection >= 3001 && selection <= 3010)
        {
            int slot = selection - 3000;
            char s[256];
            
            sprintf(s, ".%d.frz", slot);
            if (S9xLoadSnapshot(S9xGetFilename (s)))
            {     
                
                gpu3dsClearAllRenderTargets();
                GPU3DS.emulatorState = EMUSTATE_EMULATE;
                consoleClear();
                return;
            }
            else
            {
                sprintf(s, "Unable to load slot %d", slot);
                S9xShowAlert("Load State", s, "");
            }
        }
        else if (selection == 4001)
        {
            S9xReset();
            gpu3dsClearAllRenderTargets();
            GPU3DS.emulatorState = EMUSTATE_EMULATE;
            consoleClear();
            return;

        }
    }
   
}


#define P1_ButtonA 1
#define P1_ButtonB 2
#define P1_ButtonX 3
#define P1_ButtonY 4
#define P1_ButtonL 5
#define P1_ButtonR 6
#define P1_Up 7
#define P1_Down 8
#define P1_Left 9
#define P1_Right 10
#define P1_Select 11
#define P1_Start 1


//-------------------------------------------------------
// Initialize the SNES 9x engine.
//-------------------------------------------------------
bool snesInitialize()
{
   
    memset(&Settings, 0, sizeof(Settings));
    Settings.BGLayering = TRUE;
    Settings.SoundBufferSize = 0;
    Settings.CyclesPercentage = 100;
    Settings.APUEnabled = Settings.NextAPUEnabled = TRUE;
    Settings.H_Max = SNES_CYCLES_PER_SCANLINE;
    Settings.SkipFrames = 0;
    Settings.ShutdownMaster = TRUE;
    Settings.FrameTimePAL = 20000;
    Settings.FrameTimeNTSC = 16667;
    Settings.FrameTime = Settings.FrameTimeNTSC;
    Settings.DisableSampleCaching = FALSE;
    Settings.DisableMasterVolume = FALSE;
    Settings.Mouse = FALSE;
    Settings.SuperScope = FALSE;
    Settings.MultiPlayer5 = FALSE;
    Settings.ControllerOption = SNES_JOYPAD;
    Settings.SupportHiRes = FALSE;
    Settings.NetPlay = FALSE;
    Settings.ServerName [0] = 0;
    Settings.ThreadSound = FALSE;
    Settings.AutoSaveDelay = 30;
#ifdef _NETPLAY_SUPPORT
    Settings.Port = NP_DEFAULT_PORT;
#endif
    Settings.ApplyCheats = TRUE;
    Settings.TurboMode = FALSE;
    Settings.TurboSkipFrames = 15;

    Settings.Transparency = FALSE;
    Settings.SixteenBit = TRUE;
    Settings.HBlankStart = (256 * Settings.H_Max) / SNES_HCOUNTER_MAX;

    // Sound related settings.
    Settings.DisableSoundEcho = TRUE;
    Settings.SixteenBitSound = TRUE;
    Settings.SoundPlaybackRate = SAMPLE_RATE;
    Settings.Stereo = TRUE;
    Settings.SoundBufferSize = 0;
    Settings.APUEnabled = Settings.NextAPUEnabled = TRUE;
    Settings.InterpolatedSound = FALSE;
    Settings.AltSampleDecode = 1;

    if(!Memory.Init())
    {
        printf ("Unable to initialize memory.\n");
        return false;
    }
     
    if(!S9xInitAPU())
    {
        printf ("Unable to initialize APU.\n");
        return false;
    }
     
    if(!S9xGraphicsInit())
    {
        printf ("Unable to initialize graphics.\n");
        return false;
    }
    

    if(!S9xInitSound (
        7, Settings.Stereo,
        Settings.SoundBufferSize))
    {
        printf ("Unable to initialize sound.\n");
        return false;
    }
    so.playback_rate = Settings.SoundPlaybackRate;
    so.stereo = Settings.Stereo;
    so.sixteen_bit = Settings.SixteenBitSound;
    so.buffer_size = 32768;
    so.encoded = FALSE;
        
    
    return true;
}



#define TICKS_PER_SEC (268123480)
#define TICKS_PER_FRAME (4468724)

bool firstFrame = true;


// Get the morton interleave offset of the pixel
// within the 8x8 tile.
//
static inline u32 G3D_MortonInterleave(u32 x, u32 y)
{
	u32 i = (x & 7) | ((y & 7) << 8); // ---- -210
	i = (i ^ (i << 2)) & 0x1313;      // ---2 --10
	i = (i ^ (i << 1)) & 0x1515;      // ---2 -1-0
	i = (i | (i >> 7)) & 0x3F;
	return i;
}

/*
void G3D_SetTexturePixel16(sf2d_texture *texture, int x, int y, u16 new_color)
{
	y = (texture->pow2_h - 1 - y);
	
    u32 coarse_y = y & ~7;
    u32 coarse_x = x & ~7;
    u32 offset = G3D_MortonInterleave(x, y) + 
        coarse_x * 8 +
        coarse_y * texture->pow2_w;
    ((u16 *)texture->data)[offset] = new_color;
}
*/

void G3D_SetTexturePixel16(SGPUTexture *texture, int x, int y, u16 new_color)
{
	y = (texture->Height - 1 - y);
	
    u32 coarse_y = y & ~7;
    u32 coarse_x = x & ~7;
    u32 offset = G3D_MortonInterleave(x, y) + 
        coarse_x * 8 +
        coarse_y * texture->Width;
    if (offset < 1024 * 1024)
        ((u16 *)texture->PixelData)[offset] = new_color;
}


int framesSkippedCount = 0;

void updateFrameCount()
{
    if (lastTick == 0)
        lastTick = svcGetSystemTick();
        
    if (frameCount60 == 0)
    {
        u64 newTick = svcGetSystemTick();
        float timeDelta = ((float)(newTick-lastTick))/TICKS_PER_SEC;
        int fpsmul10 = (int)((float)600 / timeDelta);
        
        //consoleClear();
        printf ("FPS: %2d.%1d\n", fpsmul10 / 10, fpsmul10 % 10);

        if (framesSkippedCount)
            printf ("(%d skipped)\n", framesSkippedCount);

        frameCount60 = 60;
        framesSkippedCount = 0;

/*
#ifndef RELEASE
        for (int i=0; i<50; i++)
        {
            t3dsShowTotalTiming(i);
        } 
        t3dsResetTimings();
#endif*/
        lastTick = newTick;

    }
    
    frameCount60--;    
}



void testGPU()
{
    bool firstFrame = true;

    if (!gpu3dsInitialize())
    {
        printf ("Unabled to initialized GPU\n");
        exit(0);
    }
    
    u32 *gpuCommandBuffer;
    u32 gpuCommandBufferSize;
    u32 gpuCommandBufferOffset;
    GPUCMD_GetBuffer(&gpuCommandBuffer, &gpuCommandBufferSize, &gpuCommandBufferOffset);
    printf ("Buffer: %d %d\n", gpuCommandBufferSize, gpuCommandBufferOffset);
    
    SGPUTexture *tex1 = gpu3dsCreateTextureInLinearMemory(1024, 1024, GPU_RGBA5551);
    
    for (int y=0; y<16; y++)
    {
        for (int x=0; x<8; x++)
        {
             uint16 c1 = 0x1f - (x + y) & 0x1f;
             uint8 alpha = (x + y) < 5 ? 0 : 1;
             uint32 c = c1 << 11 | c1 << 5 | c1 << 1 | alpha;
             G3D_SetTexturePixel16(tex1, x, y, c);
        }
    }
    
    float fc = 0;
    float rad = 0;
    
    gpu3dsResetState();
    
 	while (aptMainLoop())
	{
        updateFrameCount();
        gpu3dsStartNewFrame();
        
        //----------------------------------------------------
        // Draw the game screen.
        //----------------------------------------------------
        t3dsStartTiming(1, "Start Frame");
        gpu3dsDisableAlphaBlending();
        gpu3dsDisableDepthTest();
        gpu3dsSetRenderTargetToMainScreenTexture();
        gpu3dsUseShader(0);
        gpu3dsSetTextureEnvironmentReplaceColor();
        gpu3dsDrawRectangle(0, 0, 256, 240, 0, 0x000000ff);
        gpu3dsSetTextureEnvironmentReplaceTexture0();
        gpu3dsBindTexture(tex1, GPU_TEXUNIT0);
        //gpu3dsClearRenderTarget();
        t3dsEndTiming(1);
        
        
        t3dsStartTiming(2, "Draw Tiles");
        gpu3dsDisableDepthTest();
        
        for (int i=0; i<4; i++)
        {   
            for (int y=0; y<28; y++)
            {
                for (int x=0; x<32; x++)
                {
                    if (x % 2 == 0)
                        gpu3dsAddTileVertexes( 
                            x * 8 + fc * i, y * 8  + fc * i, 
                            x * 8 + 8 + fc * i, y * 8 + 8 + fc * i, 
                            0, 0, 8.0f, 8.0f,
                            i % 2
                            );
                }
            }
            gpu3dsDrawVertexes();
        }
        t3dsEndTiming(2);
        
        // Draw some test rectangles with alpha blending
        gpu3dsSetTextureEnvironmentReplaceColor();
        gpu3dsEnableDepthTest();
        gpu3dsEnableAlphaBlending();
        gpu3dsDrawRectangle(16, 1, 96, 96, 0, 0xff0000af);  // red rectangle
        gpu3dsDrawRectangle(96, 1, 192, 96, 1, 0x0000ffaf);  // blue rectangle

        t3dsStartTiming(3, "End Frame");
        t3dsEndTiming(3);

        //----------------------------------------------------
        // Draw the texture to the frame buffer. And
        // swap the screens to show.
        //----------------------------------------------------
        t3dsStartTiming(5, "Texture to frame");
        gpu3dsDisableDepthTest();
        
        gpu3dsSetRenderTargetToTopFrameBuffer();
        gpu3dsUseShader(1);            
        gpu3dsDisableAlphaBlending();
        gpu3dsBindTextureMainScreen(GPU_TEXUNIT0);
        gpu3dsSetTextureEnvironmentReplaceTexture0();
        gpu3dsAddQuadVertexes(0, 0, 256, 224, 0, 0, 256, 224, 0.1f);
        gpu3dsDrawVertexes();
        t3dsEndTiming(5);

        if (!firstFrame)
        {
            t3dsStartTiming(6, "Transfer");
            gpu3dsTransferToScreenBuffer();
            t3dsEndTiming(6);
            
            t3dsStartTiming(7, "Swap Buffers");
            gpu3dsSwapScreenBuffers();
            t3dsEndTiming(7);
        }
        else
            firstFrame = false;        

        gpu3dsFlush();


        fc = (fc + 0.1);
        if (fc > 60)
            fc = 0;
        rad += 0.2f;
    }    
}


void testNewShader()
{
    bool firstFrame = true;

    if (!gpu3dsInitialize())
    {
        printf ("Unabled to initialized GPU\n");
        exit(0);
    }
    
    u32 *gpuCommandBuffer;
    u32 gpuCommandBufferSize;
    u32 gpuCommandBufferOffset;
    GPUCMD_GetBuffer(&gpuCommandBuffer, &gpuCommandBufferSize, &gpuCommandBufferOffset);
    printf ("Buffer: %d %d\n", gpuCommandBufferSize, gpuCommandBufferOffset);
    
    SGPUTexture *tex1 = gpu3dsCreateTextureInLinearMemory(1024, 1024, GPU_RGBA5551);
    
    for (int i=0; i<128; i++)
    {
        for (int y=0; y<8; y++)
        {
            for (int x=0; x<8; x++)
            {
                uint16 c1 = (x + y) & 0x1f;
                uint8 alpha = (x + y) < 5 ? 0 : 1;

                uint32 c = 0;
                if (alpha) 
                    c = c1 << 11 | c1 << 5 | c1 << 1 | 1;
                G3D_SetTexturePixel16(tex1, x + i*8, y, c);
            }
        }
    }
    
    // Put some text on the texture so that we know we are printing
    // out the correct image
    //
    const int fontBitmap[10][16] =
    { 
        { 
            1,1,1,0,
            1,0,1,0,
            1,0,1,0,
            1,1,1,0 },
        { 
            0,0,1,0,
            0,0,1,0,
            0,0,1,0,
            0,0,1,0 }, 
        { 
            1,1,1,0,
            0,0,1,0,
            1,1,0,0,
            1,1,1,0 }, 
        { 
            1,1,1,0,
            0,0,1,0,
            0,1,1,0,
            1,1,1,0 }, 
        { 
            1,0,1,0,
            1,0,1,0,
            1,1,1,0,
            0,0,1,0 },
        { 
            1,1,1,0,
            1,0,0,0,
            0,0,1,0,
            1,1,1,0 },
        { 
            1,0,0,0,
            1,1,1,0,
            1,0,1,0,
            1,1,1,0 },
        { 
            1,1,1,0,
            0,0,1,0,
            0,0,1,0,
            0,0,1,0 },
        { 
            1,1,1,0,
            0,1,0,0,
            1,0,1,0,
            1,1,1,0 },
        { 
            1,1,1,0,
            1,0,1,0,
            1,1,1,0,
            0,0,1,0 }
    };
    for (int c=0; c<10; c++)
        for (int y=0; y<4; y++)
        {
            for (int x=0; x<4; x++)
            {
                if (fontBitmap[c][y*4+x])
                    G3D_SetTexturePixel16(tex1, x + 4 + c * 8, y + 4, 0xffffffff);
                if (fontBitmap[c][y*4+x])
                    G3D_SetTexturePixel16(tex1, x + 4 + c * 8 + 114 * 8, y + 4, 0xffffffff);
            }
        }
    
    
    float fc = 8;
    float rad = 0;
    
    gpu3dsResetState();
    
 	while (aptMainLoop())
	{
        //updateFrameCount();
        gpu3dsStartNewFrame();
        
        //----------------------------------------------------
        // Draw the game screen.
        //----------------------------------------------------
        
        t3dsStartTiming(1, "Start Frame");
        gpu3dsDisableAlphaBlending();
        gpu3dsDisableDepthTest();
        gpu3dsSetRenderTargetToMainScreenTexture();
        gpu3dsUseShader(2);
        gpu3dsSetTextureEnvironmentReplaceColor();
        gpu3dsDrawRectangle(0, 0, 256, 240, 0, 0x000000ff);
        gpu3dsSetTextureEnvironmentReplaceTexture0();
        gpu3dsBindTexture(tex1, GPU_TEXUNIT0);
        //gpu3dsClearRenderTarget();
        t3dsEndTiming(1);
         
        
        t3dsStartTiming(2, "Draw Tiles");
        gpu3dsDisableDepthTest();
        gpu3dsUseShader(2);
        
        for (int i=0; i<4; i++)
        {   
            int depth = (i % 2) * 16384;
            //int depth = 0;
            for (int y=0; y<28; y++)
            {
                for (int x=0; x<32; x++)
                {
                    if (x % 2 == 0)
                    {
                        gpu3dsAddTileVertexes( 
                            x * 8 + fc * i, y * 8  + fc * i + depth, 
                            x * 8 + 8 + fc * i, y * 8 + 8 + fc * i + depth, 
                            0, 0, 8, 8,
                            ((x & 6) << 13) + (i + 114)
                            //0
                            );
                        //y = x = i = 100;
                    }
                }
                //y = i = 100;
            }
            
        }
        gpu3dsDrawVertexes();
        //printf ("a");
        t3dsEndTiming(2);
        
 
        // Draw some test rectangles with alpha blending
        gpu3dsUseShader(2);
        gpu3dsSetTextureEnvironmentReplaceColor();
        gpu3dsEnableDepthTest();
        gpu3dsEnableAlphaBlending();
        gpu3dsDrawRectangle(16, 1, 96, 96, 1, 0xff0000af);  // red rectangle
        gpu3dsDrawRectangle(96, 1, 192, 96, 1, 0x0000ffaf);  // blue rectangle

        t3dsStartTiming(3, "End Frame");
        t3dsEndTiming(3);

        //----------------------------------------------------
        // Draw the texture to the frame buffer. And
        // swap the screens to show.
        //----------------------------------------------------
        t3dsStartTiming(5, "Texture to frame");
        gpu3dsDisableDepthTest();
        
        gpu3dsSetRenderTargetToTopFrameBuffer();
        gpu3dsUseShader(1);            
        gpu3dsDisableAlphaBlending();
        gpu3dsBindTextureMainScreen(GPU_TEXUNIT0);
        gpu3dsSetTextureEnvironmentReplaceTexture0();
        gpu3dsAddQuadVertexes(0, 0, 256, 224, 0, 0, 256, 224, 0.1f);
        gpu3dsDrawVertexes();
        t3dsEndTiming(5);

        if (!firstFrame)
        {
            t3dsStartTiming(6, "Transfer");
            gpu3dsTransferToScreenBuffer();
            t3dsEndTiming(6);
            
            t3dsStartTiming(7, "Swap Buffers");
            gpu3dsSwapScreenBuffers();
            t3dsEndTiming(7);
        }
        else
            firstFrame = false;        

        gpu3dsFlush();


        rad += 0.2f;
    }    
}


    static uint16 testBuffer[65536];
    static uint8 testVram[65536];
    static uint16 palette[256];

void testMode7Texture()
{
    bool firstFrame = true;

    if (!gpu3dsInitialize())
    {
        printf ("Unabled to initialized GPU\n");
        exit(0);
    }
    
    u32 *gpuCommandBuffer;
    u32 gpuCommandBufferSize;
    u32 gpuCommandBufferOffset;
    GPUCMD_GetBuffer(&gpuCommandBuffer, &gpuCommandBufferSize, &gpuCommandBufferOffset);
    printf ("Buffer: %d %d\n", gpuCommandBufferSize, gpuCommandBufferOffset);
    
    SGPUTexture *tex1 = gpu3dsCreateTextureInLinearMemory(1024, 1024, GPU_RGBA5551);
    
    for (int j=0; j<128; j++)
    for (int i=0; i<128; i++)
    {
        for (int y=0; y<8; y++)
        {
            for (int x=0; x<8; x++)
            {
                uint16 c1 = (x + y) & 0x1f;
                uint8 alpha = (x + y) < 5 ? 0 : 1;

                uint32 c = 0;
                if (alpha) 
                    c = c1 << 11 | c1 << 5 | c1 << 1 | 1;
                G3D_SetTexturePixel16(tex1, x + i*8, y + j*8, c);
            }
        }
    }

    for (int i = 0; i < 65536; i++)
    {
        int c = rand() % 256;
        testVram[i] = c;
    }
    for (int i = 0; i < 256; i++)
    {
        int c = (rand() % 65536) | 1;
        palette[i] = c;
    }
    
    // Put some text on the texture so that we know we are printing
    // out the correct image
    //
    const int fontBitmap[10][16] =
    { 
        { 
            1,1,1,0,
            1,0,1,0,
            1,0,1,0,
            1,1,1,0 },
        { 
            0,0,1,0,
            0,0,1,0,
            0,0,1,0,
            0,0,1,0 }, 
        { 
            1,1,1,0,
            0,0,1,0,
            1,1,0,0,
            1,1,1,0 }, 
        { 
            1,1,1,0,
            0,0,1,0,
            0,1,1,0,
            1,1,1,0 }, 
        { 
            1,0,1,0,
            1,0,1,0,
            1,1,1,0,
            0,0,1,0 },
        { 
            1,1,1,0,
            1,0,0,0,
            0,0,1,0,
            1,1,1,0 },
        { 
            1,0,0,0,
            1,1,1,0,
            1,0,1,0,
            1,1,1,0 },
        { 
            1,1,1,0,
            0,0,1,0,
            0,0,1,0,
            0,0,1,0 },
        { 
            1,1,1,0,
            0,1,0,0,
            1,0,1,0,
            1,1,1,0 },
        { 
            1,1,1,0,
            1,0,1,0,
            1,1,1,0,
            0,0,1,0 }
    };
    for (int c=0; c<10; c++)
        for (int y=0; y<4; y++)
        {
            for (int x=0; x<4; x++)
            {
                if (fontBitmap[c][y*4+x])
                    G3D_SetTexturePixel16(tex1, x + 4 + c * 8, y + 4, 0xffffffff);
                if (fontBitmap[c][y*4+x])
                    G3D_SetTexturePixel16(tex1, x + 4 + c * 8 + 114 * 8, y + 4, 0xffffffff);
            }
        }
    
    
    float fc = 8;
    float rad = 0;
    
    gpu3dsResetState();
    
 	while (aptMainLoop())
	{
        updateFrameCount();
        gpu3dsStartNewFrame();
        
        //----------------------------------------------------
        // Draw the game screen.
        //----------------------------------------------------
        
        t3dsStartTiming(1, "Start Frame");
        gpu3dsDisableAlphaBlending();
        gpu3dsDisableDepthTest();
        gpu3dsSetRenderTargetToMainScreenTexture();
        gpu3dsUseShader(2);
        gpu3dsSetTextureEnvironmentReplaceColor();
        gpu3dsDrawRectangle(0, 0, 256, 240, 0, 0x000000ff);
        gpu3dsSetTextureEnvironmentReplaceTexture0();
        gpu3dsBindTexture(tex1, GPU_TEXUNIT0);
        //gpu3dsClearRenderTarget();
        t3dsEndTiming(1);
         
        static int ct = 0;
        t3dsStartTiming(2, "Draw Tiles");
        gpu3dsDisableDepthTest();

        gpu3dsUseShader(2);
        
        static int tc = 0;
        int count = 0;
        gpu3dsSetTextureEnvironmentReplaceTexture0();
        gpu3dsBindTexture(tex1, GPU_TEXUNIT0);
        for (int ofs = 0; ofs < 4; ofs ++)
        {
            gpu3dsSetRenderTargetToMode7FullTexture((3 - ofs) * 0x100000, 512, 512);
            for (int y = 0; y < 32; y++)
            {
                int x = 0;
                for (x = 0; x < 128; x++)
                {
                    int tx = 0;
                    int ty = 0;

                    if (x < 64)
                    {
                        tx = x * 8;
                        ty = (y * 2 + 1) * 8;
                    }
                    else
                    {
                        tx = (x - 64) * 8;
                        ty = (y * 2) * 8;
                    
                    }
                    int c = (x % 10);
                    gpu3dsAddTileVertexes(
                        tx, ty, tx+8, ty+8, 
                        0, 0, 8, 8, 0);

                }
            }
            gpu3dsDrawVertexes();
            //printf ("a");
            ct = (ct + 1) % 128;
        }
        tc++;
        t3dsEndTiming(2); 
        /*
        static int sx = 0;
        static int sy = 0;
        sx ++;
        sy ++;
        int dx = 87;
        int dy = 283;
        for (int y = 0; y < 256; y ++)
        {
            int ax = sx;
            int ay = sy;
            int adx = 92;
            int ady = 52;
            for (int x = 0; x < 256; )
            {
                {
                    ax = ax & 0x3ff;
                    ay = ay & 0x3ff;
                    int tileAddr = (((ay & ~7)<<4) + (ax >> 3)) << 1;
                    int tileNo = testVram[tileAddr];
                    int charAddr = (((tileNo<<6) + ((ay&7)<<3) + (ax&7))<<1) + 1;
                    int pixel = palette[testVram[charAddr]];
                    
                    //G3D_SetTexturePixel16(tex1, x + i*8, y, c);
                    ((uint16 *)(tex1->PixelData))[y * 1024 + x] = pixel;
                    ax += adx;
                    ay += ady;
                    x++;
                }
                                
            }
            sx += dx;
            sy += dy;
        }*/



        gpu3dsBindTextureSnesMode7Full(GPU_TEXUNIT0);
        gpu3dsSetRenderTargetToMainScreenTexture();
        gpu3dsAddTileVertexes(0, 0, 256, 240, 0, 0, 1024, 1024, 0);
        gpu3dsDrawVertexes();
 
        // Draw some test rectangles with alpha blending
        gpu3dsUseShader(2);
        gpu3dsSetTextureEnvironmentReplaceColor();
        gpu3dsEnableDepthTest();
        gpu3dsEnableAlphaBlending();
        gpu3dsDrawRectangle(16, 1, 96, 96, 1, 0xff0000af);  // red rectangle
        gpu3dsDrawRectangle(96, 1, 192, 96, 1, 0x0000ffaf);  // blue rectangle

        t3dsStartTiming(3, "End Frame");
        t3dsEndTiming(3);

        //----------------------------------------------------
        // Draw the texture to the frame buffer. And
        // swap the screens to show.
        //----------------------------------------------------
        t3dsStartTiming(5, "Texture to frame");
        gpu3dsDisableDepthTest();
        
        gpu3dsSetRenderTargetToTopFrameBuffer();
        gpu3dsUseShader(1);            
        gpu3dsDisableAlphaBlending();
        gpu3dsBindTextureMainScreen(GPU_TEXUNIT0);
        gpu3dsSetTextureEnvironmentReplaceTexture0();
        gpu3dsAddQuadVertexes(0, 0, 256, 240, 0, 0, 256, 240, 0.1f);
        gpu3dsDrawVertexes();
        t3dsEndTiming(5);

        if (!firstFrame)
        {
            t3dsStartTiming(6, "Transfer");
            gpu3dsTransferToScreenBuffer();
            t3dsEndTiming(6);
            
            t3dsStartTiming(7, "Swap Buffers");
            gpu3dsSwapScreenBuffers();
            t3dsEndTiming(7);
        }
        else
            firstFrame = false;        

        gpu3dsFlush();


        rad += 0.2f;
    }    
}

void testMode7Construct()
{

static uint8 chronoTriggerVRAM[32768] = {
87,21,71,20,87,19,71,19,87,19,71,19,87,20,71,21,87,21,71,20,87,19,71,19,87,19,71,19,87,20,71,21,87,21,71,20,87,19,71,19,87,19,71,19,87,20,71,21,87,21,71,20,87,19,71,19,87,19,71,19,87,20,71,21,87,21,71,20,87,19,71,19,87,19,71,19,87,20,71,21,87,21,71,20,87,19,71,19,87,19,94,19,93,20,71,21,87,21,71,20,87,19,71,19,87,19,71,19,87,20,71,21,48,21,71,20,87,19,71,19,87,19,71,19,87,20,71,21,87,44,71,44,87,44,71,57,87,57,71,57,87,44,71,57,87,44,71,44,87,39,71,57,87,65,71,44,87,52,71,65,87,44,71,57,87,57,71,69,87,68,71,52,87,34,71,63,87,44,71,57,87,67,71,68,87,68,71,35,87,34,71,70,87,57,71,57,87,57,71,65,87,67,71,61,87,70,71,70,87,57,71,57,87,57,71,65,87,67,94,69,93,70,71,72,87,44,71,57,87,44,71,44,87,65,71,68,87,70,71,72,48,44,71,44,87,44,71,44,87,44,71,65,87,69,71,70,71,57,87,57,71,57,87,57,71,44,87,44,71,44,48,44,71,52,64,52,65,57,66,44,67,44,87,44,71,44,87,44,71,63,87,52,71,52,87,65,71,65,87,57,71,44,87,57,48,70,87,72,71,63,87,69,5,68,6,67,71,57,19,57,20,72,30,75,31,72,87,70,71,73,87,71,71,67,87,57,71,75,94,75,75,72,76,72,76,78,110,75,109,71,76,65,75,75,76,72,93,63,87,72,71,79,87,78,7,71,8,65,71,76,87,70,71,34,87,72,71,78,87,75,71,67,87,65,71,28,87,25,71,26,87,32,71,29,87,25,71,34,48,70,71,30,64,29,65,29,66,30,67,31,87,34,71,70,87,72,71,26,87,28,71,27,87,25,71,34,87,70,71,73,87,60,48,29,87,26,71,24,87,34,5,70,6,72,71,73,19,54,20,25,30,26,31,34,87,70,71,72,87,73,71,50,87,50,71,24,94,34,75,70,76,72,76,73,110,62,109,49,76,55,75,34,76,70,93,72,87,73,71,55,87,66,7,42,8,42,71,34,87,72,71,72,87,66,71,66,87,66,71,41,87,41,93,70,71,72,87,73,71,66,87,66,71,66,87,39,71,41,87,29,80,70,81,72,82,72,83,73,71,66,87,59,71,47,87,29,71,29,87,34,71,70,87,72,71,73,87,59,71,47,87,28,68,27,69,26,70,34,87,70,71,72,87,55,44,55,45,27,46,27,47,24,71,24,87,70,71,72,87,52,71,49,94,24,110,25,91,25,92,27,92,34,126,72,125,73,92,73,91,30,92,29,109,29,93,30,87,31,71,70,23,70,24,72,87,28,71,25,94,26,75,32,76,29,75,25,93,34,94,70,93,44,71,44,87,44,71,44,87,44,71,57,87,57,71,57,87,44,80,57,81,57,82,65,83,44,71,44,87,57,71,65,87,57,71,57,87,67,71,68,87,65,71,44,87,57,71,67,87,57,68,67,69,68,70,65,87,57,71,57,87,61,44,71,45,65,46,67,47,68,71,65,87,52,71,61,87,63,71,70,94,44,110,65,91,65,92,57,92,52,126,69,125,70,92,76,91,44,92,57,109,57,93,52,87,63,71,70,23,72,24,75,87,57,71,57,94,57,75,57,76,52,75,69,93,70,94,72,109,65,75,67,76,65,76,57,76,57,93,44,71,65,87,44,71,68,96,76,97,71,98,67,99,57,87,65,71,68,87,65,71,74,1,78,2,75,87,68,71,65,87,67,71,68,87,65,71,75,84,78,85,75,86,76,71,67,87,65,71,65,87,57,71,76,87,70,71,70,87,76,71,76,94,68,75,57,76,57,110,72,126,34,71,70,87,78,71,79,87,76,71,65,87,57,71,75,87,72,125,72,109,78,76,78,93,75,71,67,87,44,71,75,94,75,110,72,91,72,92,76,91,68,109,57,110,44,109,52,75,68,76,68,76,52,76,44,93,44,71,57,87,52,71,43,96,43,97,43,98,43,99,43,87,35,71,35,87,43,71,35,1,35,2,35,87,35,71,35,87,43,71,68,87,73,71,35,84,35,85,35,86,35,71,68,87,73,71,73,87,73,71,35,87,35,71,35,87,71,71,74,94,73,75,73,76,73,110,35,126,35,71,43,87,74,71,74,87,73,71,73,87,73,71,35,87,35,125,69,109,74,76,73,93,73,71,73,87,73,71,43,94,43,110,73,91,71,92,67,91,68,109,68,110,67,125,68,91,71,92,69,92,69,92,68,109,68,93,69,71,57,87,69,112,71,113,73,114,73,115,68,71,68,87,69,71,65,87,74,41,74,47,74,71,73,87,73,71,68,87,68,71,65,87,73,71,74,87,74,71,74,87,71,71,68,87,69,71,57,87,69,94,71,76,73,76,74,76,73,110,73,91,73,92,68,126,67,71,65,87,67,71,73,87,73,71,73,87,73,5,73,6,67,71,57,87,52,125,52,92,68,109,67,75,73,76,73,75,44,110,57,126,68,71,61,87,68,71,57,125,71,126,73,125,44,91,65,92,68,92,76,92,75,109,72,93,76,71,76,87,44,112,44,113,68,114,75,115,78,71,78,87,70,71,70,87,44,41,44,47,70,71,75,87,78,71,78,87,63,71,21,87,57,71,57,87,69,71,72,87,75,71,75,87,52,71,21,87,57,94,67,76,68,76,72,76,72,110,63,91,61,92,63,126,57,71,65,87,68,71,76,87,72,71,63,87,69,5,71,6,57,71,65,87,68,125,71,92,72,109,76,75,73,76,68,75,57,110,65,126,69,71,71,87,76,71,76,125,71,126,44,71,70,87,69,71,63,87,76,71,78,125,79,109,75,93,76,71,63,87,61,71,57,87,43,71,70,87,75,71,78,87,78,71,21,87,52,71,35,87,35,71,43,87,52,71,63,87,75,71,21,87,63,71,34,87,21,94,23,76,23,75,34,76,63,76,34,110,34,92,34,92,63,92,23,126,21,71,21,87,21,71,72,87,75,71,75,87,78,71,72,20,23,20,23,21,21,22,75,87,78,71,75,87,75,87,72,125,70,91,34,92,34,91,70,126,75,71,76,87,76,71,71,87,69,71,61,87,70,71,78,87,78,71,79,87,79,71,78,125,78,109,34,93,20,71,78,87,78,71,78,87,79,71,78,87,76,71,21,87,19,71,78,87,78,71,78,87,79,71,79,87,72,71,21,87,21,71,72,87,72,71,76,87,78,94,78,76,63,75,21,76,23,76,23,110,75,92,75,92,78,92,75,126,63,71,21,87,34,71,23,87,75,71,72,87,72,71,63,20,34,20,23,21,34,22,70,87,76,71,21,87,21,87,21,125,21,91,63,92,76,91,75,126,70,71,21,87,20,71,21,87,21,71,23,87,34,87,23,71,75,87,78,71,78,87,78,71,79,125,78,109,75,93,34,71,79,87,79,71,78,87,78,71,78,87,79,71,75,87,72,71,78,87,78,71,78,87,78,71,78,87,78,71,78,87,78,71,79,87,78,94,78,110,78,92,78,91,75,92,34,92,78,126,79,87,78,71,78,87,78,71,75,87,70,17,23,18,78,19,78,20,78,21,78,35,78,35,70,36,34,37,63,38,78,71,79,87,78,71,75,87,76,71,63,87,52,71,52,87,34,71,78,87,75,71,70,87,43,71,52,87,52,71,43,87,75,71,75,87,75,71,75,87,75,71,72,125,63,109,63,93,75,71,78,87,78,71,78,87,76,71,63,87,35,71,57,87,75,71,72,87,75,71,78,87,75,71,72,87,35,71,44,87,23,71,23,87,34,94,70,110,72,92,75,91,39,92,39,92,63,126,23,87,34,71,70,87,34,71,70,87,39,17,35,18,72,19,21,20,63,21,75,35,70,35,52,36,43,37,35,38,34,71,21,87,63,71,78,87,76,71,63,87,57,71,57,87,63,71,34,87,72,71,75,87,70,71,61,87,65,71,65,87,69,71,72,87,70,71,69,87,73,71,68,87,57,125,57,109,68,93,76,87,72,71,70,87,73,71,67,94,57,93,65,87,70,94,75,93,75,71,72,87,71,71,67,87,65,71,68,87,70,71,72,94,75,110,63,126,57,71,65,87,68,71,73,87,34,71,23,87,52,71,35,87,44,71,67,87,71,33,68,34,63,35,70,36,71,34,44,51,44,51,65,52,65,53,65,54,70,55,76,87,73,71,44,87,44,71,57,87,44,71,57,87,70,48,72,87,68,71,52,87,57,71,57,87,39,71,43,87,34,71,24,87,25,71,29,87,32,71,70,87,72,125,34,109,72,93,34,87,25,71,31,87,34,71,72,94,29,93,70,87,37,94,72,93,33,71,33,87,32,71,70,87,70,71,34,87,40,71,58,94,63,110,34,126,32,71,29,87,31,71,32,87,60,71,45,87,72,71,70,87,32,71,32,87,30,33,33,34,52,35,52,36,42,34,72,51,37,51,34,52,27,53,31,54,48,55,48,87,35,71,66,87,72,71,72,87,30,71,33,87,42,48,42,87,38,71,66,87,66,71,66,87,72,71,72,71,42,87,42,87,38,71,66,20,66,21,72,22,72,87,72,125,43,109,43,75,62,76,66,75,72,76,23,110,25,109,29,76,42,110,39,109,72,76,70,76,33,76,30,75,31,76,33,75,43,76,46,110,70,126,34,71,31,87,31,71,33,87,31,5,55,6,72,71,34,1,30,2,32,71,29,87,33,5,33,10,58,11,72,12,34,50,29,10,31,11,28,12,29,13,31,71,72,87,70,1,25,2,31,71,30,5,29,6,29,87,30,71,70,87,34,68,25,69,29,70,32,87,26,71,25,87,28,71,57,87,57,87,44,71,44,20,44,21,44,22,65,87,66,125,57,109,57,75,57,76,44,75,44,76,57,110,57,109,57,76,57,110,57,109,44,76,44,76,44,76,44,75,57,76,57,75,65,76,57,110,57,126,44,71,57,87,57,71,44,87,44,5,57,6,57,71,57,1,57,2,57,71,57,87,57,5,57,10,57,11,57,12,57,50,65,10,67,11,68,12,65,13,65,71,57,87,57,1,67,2,68,71,71,5,68,6,69,87,69,71,65,87,52,68,68,69,71,70,75,87,72,71,70,87,70,87,76,87,75,20,72,30,72,35,76,36,68,38,65,71,71,87,70,125,75,91,78,92,76,91,68,92,57,126,65,125,71,92,66,126,76,125,75,92,71,92,65,92,57,91,44,92,44,91,57,92,67,126,67,71,65,87,57,71,57,87,57,71,57,87,57,47,57,87,57,17,44,18,44,19,57,20,44,21,57,26,52,27,52,28,65,29,65,26,57,27,44,28,44,29,65,55,52,71,34,87,70,46,68,87,67,71,57,46,57,71,61,87,34,71,21,84,63,85,70,86,70,71,70,87,63,71,63,87,76,87,70,20,57,30,44,35,57,36,57,38,57,71,57,87,76,125,72,91,68,92,65,91,44,92,57,126,57,125,57,92,76,126,75,125,70,92,68,92,65,92,44,91,57,92,57,91,70,92,76,126,34,71,70,87,67,71,65,87,44,71,44,87,63,47,72,87,34,17,72,18,70,19,61,20,65,21,44,26,69,27,72,28,70,29,72,26,34,27,34,28,52,29,43,55,69,71,72,87,70,46,63,87,23,71,34,46,34,71,52,87,63,71,72,84,72,85,75,86,72,71,70,87,34,71,70,71,44,87,57,46,57,40,65,50,57,36,44,52,44,55,44,1,57,2,44,71,57,87,65,5,65,6,65,71,65,87,57,71,57,68,57,69,67,70,71,71,70,87,70,55,71,1,68,2,65,71,65,87,69,5,73,6,72,71,72,71,75,87,75,71,69,87,71,71,70,33,72,34,75,35,76,36,34,37,63,34,63,35,70,38,72,35,72,36,78,37,76,54,34,54,34,55,69,7,73,8,72,87,72,71,76,87,72,71,63,87,34,71,72,87,72,71,70,87,70,71,72,87,63,71,63,87,34,71,44,87,57,46,57,40,65,50,69,36,69,52,70,55,72,1,65,2,67,71,67,87,67,5,67,6,61,71,63,87,70,71,71,68,76,69,76,70,70,71,65,87,65,55,69,1,69,2,75,71,72,87,72,5,70,6,67,71,65,71,65,87,65,71,72,87,76,71,72,33,72,34,76,35,68,36,67,37,65,34,63,35,76,38,75,35,75,36,75,37,70,54,76,54,71,55,63,7,70,8,72,87,72,71,63,87,21,71,63,87,75,71,63,87,63,71,70,87,63,71,23,87,19,71,21,87,63,87,75,71,70,87,63,17,69,11,67,54,57,46,44,71,39,17,73,18,69,19,69,20,68,21,65,22,65,87,44,71,39,87,68,84,65,85,65,86,65,87,65,71,67,71,44,17,39,18,65,19,57,20,65,21,65,22,67,87,68,87,57,71,39,87,44,71,65,87,67,49,65,50,65,35,65,11,57,33,44,50,67,51,67,52,68,51,67,52,65,53,57,14,65,71,65,87,75,23,75,24,71,71,61,87,57,71,65,64,67,65,67,66,75,67,78,87,75,71,70,87,57,71,44,87,44,71,44,87,43,71,52,87,73,17,68,11,65,54,67,46,68,71,57,17,68,18,69,19,73,20,71,21,67,22,65,87,67,71,52,87,69,84,73,85,73,86,74,87,71,71,66,71,52,17,57,18,68,19,69,20,73,21,73,22,73,87,73,87,67,71,65,87,52,71,68,87,73,49,73,50,73,35,73,11,73,33,73,50,52,51,69,52,73,51,73,52,73,53,73,14,73,71,74,87,52,23,69,24,73,71,73,87,67,71,67,64,73,65,73,66,68,67,68,87,71,71,68,87,65,71,57,87,57,71,52,71,35,87,52,71,68,33,73,27,69,31,52,71,69,32,73,33,43,34,69,35,73,36,69,37,52,38,61,39,73,87,73,71,52,87,69,71,69,7,52,8,57,70,71,32,74,33,71,34,57,35,35,36,35,37,52,38,68,39,74,71,73,87,71,71,71,87,71,71,71,9,73,11,74,50,74,50,73,51,67,10,76,11,76,12,76,35,74,12,74,13,73,30,71,31,65,71,73,87,73,71,73,87,73,71,73,87,73,80,68,81,57,82,68,83,68,71,69,87,71,71,71,87,69,71,67,87,57,71,57,87,65,71,71,33,76,27,72,31,72,71,73,32,35,33,44,34,44,35,71,36,75,37,72,38,70,39,73,87,44,71,39,87,35,71,70,7,75,8,75,70,70,32,69,33,57,34,44,35,39,36,70,37,75,38,72,39,70,71,72,87,75,71,44,87,57,71,70,9,75,11,75,50,75,50,78,51,79,10,44,11,44,12,68,35,76,12,78,13,78,30,79,31,78,71,44,87,44,71,71,87,75,71,78,87,79,80,79,81,75,82,57,83,71,71,78,87,78,71,72,87,72,71,72,87,72,71,70,1,75,2,75,49,72,43,69,87,70,87,63,71,72,49,76,50,75,51,75,52,75,53,72,54,75,55,34,71,21,87,75,71,78,87,78,23,78,24,75,86,72,71,21,49,19,50,78,51,79,52,78,53,78,54,76,55,34,87,20,71,21,87,79,71,78,87,75,25,76,34,72,35,23,50,20,37,23,26,78,27,78,28,75,51,75,28,72,29,34,46,23,47,72,5,75,6,75,87,75,71,75,87,76,71,63,96,72,97,75,98,72,99,75,87,72,71,76,1,70,2,63,87,75,71,75,71,75,1,34,2,20,49,20,43,20,87,20,87,20,71,20,49,34,50,21,51,20,52,20,53,20,54,20,55,20,71,21,87,34,71,23,87,20,23,20,24,20,86,20,71,20,49,21,50,76,51,63,52,19,53,19,54,20,55,20,87,19,71,23,87,75,71,70,87,19,25,19,34,19,35,23,50,34,37,70,26,78,27,72,28,23,51,23,28,63,29,75,46,75,47,70,5,75,6,75,87,72,71,72,87,79,71,78,96,75,97,34,98,75,99,72,87,78,71,78,1,78,2,78,87,75,71,63,40,21,41,75,47,75,9,70,47,52,71,61,87,61,87,43,9,63,10,78,11,76,12,72,13,63,14,63,71,70,71,70,1,76,2,75,87,72,71,75,87,70,71,63,87,70,9,69,10,76,11,70,12,66,13,75,14,75,71,63,87,63,71,63,87,70,71,63,40,52,41,71,50,75,51,72,52,70,53,70,51,34,52,70,53,76,43,76,44,75,45,75,87,76,71,72,17,34,47,72,87,75,71,72,87,72,71,70,112,72,113,72,114,63,115,75,87,75,40,72,41,72,47,75,87,75,71,75,40,63,41,70,47,75,9,70,47,43,71,68,87,67,87,61,9,61,10,44,11,39,12,52,13,61,14,69,71,70,71,70,1,65,2,67,87,52,71,52,87,63,71,63,87,63,9,72,10,43,11,66,12,70,13,75,14,76,71,63,87,63,71,72,87,70,71,69,40,73,41,78,50,75,51,63,52,63,53,70,51,76,52,72,53,73,43,73,44,76,45,72,87,34,71,63,17,75,47,75,87,78,71,76,87,70,71,70,112,63,113,63,114,75,115,75,87,78,40,75,41,72,47,72,87,72,71,72,71,63,87,63,71,35,87,34,71,63,87,69,71,70,71,70,25,34,26,21,27,21,28,34,29,70,30,70,31,69,40,69,41,70,47,34,71,52,87,70,71,68,87,52,71,52,25,67,26,76,27,63,28,61,29,68,30,70,31,70,71,71,87,69,71,71,87,61,71,69,9,70,10,72,53,72,12,75,13,70,35,69,12,69,13,70,14,72,71,72,87,76,1,75,2,72,71,70,87,72,5,72,6,72,71,72,87,72,71,72,87,70,71,76,87,75,71,76,87,72,46,72,87,72,71,72,87,72,71,70,87,76,71,68,87,67,71,57,87,57,71,57,71,43,25,68,26,68,27,67,28,68,29,70,30,68,31,65,40,57,41,67,47,71,71,70,87,76,71,76,87,70,71,67,25,69,26,63,27,72,28,72,29,70,30,68,31,67,71,65,87,73,71,23,87,63,71,70,9,69,10,44,53,44,12,65,13,69,35,34,12,52,13,69,14,71,71,68,87,65,1,44,2,66,71,63,87,34,5,63,6,70,71,71,87,68,71,65,87,61,71,72,87,72,71,76,87,76,46,71,87,68,71,65,87,61,87,44,68,57,69,57,70,68,87,68,71,71,87,70,40,70,41,44,42,57,43,67,44,65,45,65,46,67,47,67,64,68,65,44,66,65,67,68,5,65,6,44,71,57,40,65,41,61,42,57,43,44,44,65,45,57,46,44,47,57,87,57,71,61,87,57,71,57,87,57,25,57,26,57,51,44,28,65,29,61,51,57,28,44,29,44,30,65,31,57,71,44,17,57,18,61,19,57,20,44,21,44,22,68,87,65,71,57,19,67,20,71,30,57,31,57,87,67,71,68,87,65,71,65,87,68,71,68,87,61,68,52,69,76,70,72,87,72,71,70,87,63,40,72,41,52,42,61,43,75,44,72,45,63,46,63,47,70,64,78,65,21,66,21,67,72,5,70,6,34,71,34,40,72,41,78,42,23,43,23,44,70,45,70,46,63,47,70,87,70,71,72,87,34,71,34,87,70,25,70,26,70,51,72,28,34,29,34,51,34,28,34,29,63,30,63,31,70,71,76,17,75,18,75,19,76,20,70,21,70,22,63,87,34,71,34,19,72,20,78,30,75,31,70,87,34,71,70,87,70,71,70,87,63,71,73,71,70,84,63,85,70,86,70,71,70,87,70,71,72,19,70,20,78,30,75,31,70,87,34,71,34,87,63,71,72,80,72,81,79,82,79,83,79,87,76,71,70,87,63,19,70,20,76,30,78,31,78,87,79,71,75,87,75,71,76,1,72,2,75,71,72,87,76,40,79,41,78,42,75,43,78,44,75,45,75,43,75,44,72,45,75,46,75,47,78,32,79,33,75,34,70,35,79,36,78,37,70,38,34,39,72,87,75,44,78,45,76,46,79,47,78,71,75,87,72,71,63,87,34,71,34,87,70,71,70,84,70,85,34,86,75,71,78,87,76,71,70,19,72,20,72,30,63,31,63,87,78,71,78,87,72,71,72,80,75,81,75,82,63,83,75,87,78,71,78,87,78,19,78,20,78,30,72,31,70,87,78,71,78,87,78,71,78,1,78,2,78,71,63,87,72,40,78,41,79,42,79,43,78,44,78,45,75,43,34,44,72,45,78,46,78,47,79,32,79,33,79,34,79,35,72,36,70,37,70,38,76,39,78,87,79,44,79,45,78,46,75,47,75,71,72,87,75,71,78,87,79,71,78,87,78,87,70,71,63,87,23,71,63,87,70,71,21,87,63,44,72,45,72,46,70,47,72,71,78,87,72,71,21,87,72,96,79,97,75,98,75,99,78,71,76,48,34,71,34,44,75,45,79,46,78,47,78,71,76,87,34,71,21,87,63,41,78,47,78,87,75,71,78,87,72,71,21,87,21,59,72,60,78,87,79,59,79,60,78,87,72,71,23,87,63,71,75,49,78,50,78,51,78,52,78,53,75,54,63,55,75,71,75,87,79,71,78,87,78,71,72,87,72,71,75,87,78,71,78,87,78,71,75,87,75,71,78,87,76,71,70,87,61,71,52,87,52,44,69,45,78,46,79,47,73,71,70,87,72,71,75,87,72,96,72,97,78,98,78,99,76,71,72,48,72,71,72,44,72,45,75,46,78,47,72,71,70,87,70,71,72,87,76,41,75,47,76,87,78,71,70,87,61,71,69,87,70,59,78,60,78,87,72,59,75,60,63,87,44,71,44,87,70,71,78,49,75,50,63,51,71,52,69,53,44,54,39,55,76,71,79,87,72,71,63,87,44,71,57,87,57,71,57,87,75,71,78,87,34,71,34,71,69,87,76,71,78,87,75,71,67,87,44,71,44,87,44,71,34,87,63,71,78,87,75,48,57,87,44,71,44,112,44,113,34,114,63,115,75,87,72,71,68,87,44,71,44,87,57,71,34,87,72,71,72,87,70,71,71,87,57,71,44,87,57,71,34,87,76,71,72,87,63,71,57,87,44,71,44,87,44,71,72,87,78,71,78,87,76,71,66,87,44,9,57,10,44,11,75,12,78,13,75,14,75,71,76,87,71,71,71,87,44,71,78,87,78,71,70,87,63,71,72,87,75,71,75,87,71,71,44,87,44,71,57,87,57,71,57,87,57,71,43,87,52,71,57,87,52,71,57,87,57,48,57,87,57,71,44,112,52,113,57,114,67,115,67,87,57,71,57,87,57,71,44,87,52,71,57,87,57,71,57,87,57,71,57,87,57,71,57,87,52,71,57,87,44,71,44,87,57,71,57,87,57,71,57,87,61,71,44,87,44,71,57,87,57,71,57,87,57,9,44,10,52,11,44,12,57,13,57,14,67,71,52,87,57,71,57,87,52,71,65,87,57,71,67,87,68,71,65,87,57,71,44,87,52,87,65,71,57,87,44,71,57,100,67,101,68,100,76,101,76,102,57,100,57,101,44,102,65,87,68,71,75,87,78,71,78,87,44,71,57,87,44,71,65,87,68,71,75,87,78,71,78,87,57,71,57,87,57,71,44,87,57,71,75,87,75,71,75,87,65,71,57,87,57,71,65,87,67,71,76,87,75,71,75,87,65,71,44,87,44,71,67,87,57,71,71,25,76,26,76,27,65,28,65,29,67,30,67,31,35,71,68,87,76,71,72,87,65,71,44,87,57,71,57,87,67,71,69,87,72,71,63,87,75,71,78,87,78,71,72,100,63,101,34,100,72,101,75,102,79,100,76,101,34,102,23,87,23,71,34,87,70,71,71,87,72,71,34,87,34,71,70,87,63,71,34,87,69,71,69,87,70,71,63,87,63,71,63,87,70,71,63,87,69,71,68,87,70,71,34,87,23,71,21,87,70,71,63,87,69,71,68,87,72,71,63,87,34,71,34,87,70,71,70,25,69,26,68,27,70,28,69,29,70,30,63,31,70,71,69,87,73,71,67,87,43,71,57,87,70,71,69,87,71,71,67,87,68,71,67,100,71,101,71,102,69,100,71,105,69,106,73,104,78,105,75,106,68,104,67,104,65,105,65,108,67,100,71,101,74,102,72,107,68,108,57,100,57,101,57,102,57,107,67,108,68,87,76,71,57,87,57,71,57,87,65,71,44,87,57,71,65,87,68,100,57,101,57,107,67,108,67,71,65,87,44,71,44,87,65,71,57,87,65,71,68,87,69,71,67,40,65,41,44,42,65,43,57,44,57,45,65,46,65,47,65,71,44,71,44,87,57,71,57,87,44,71,44,71,57,87,57,71,57,87,44,71,44,100,70,101,34,102,75,100,78,105,78,106,78,104,75,105,63,106,34,104,21,104,72,105,78,108,78,100,78,101,72,102,63,107,75,108,72,100,75,101,78,102,78,107,78,108,71,87,69,71,75,87,79,71,76,87,75,71,75,87,75,71,68,87,65,100,73,101,78,107,75,108,72,71,73,87,71,71,65,87,57,71,68,87,75,71,78,87,72,71,76,40,71,41,65,42,44,43,67,44,71,45,75,46,72,47,71,71,68,71,65,87,57,71,44,87,39,71,44,71,68,87,65,71,65,87,65,71,67,88,61,89,71,90,72,88,72,89,72,90,76,88,72,89,76,90,61,88,69,88,70,89,70,90,69,88,68,89,72,90,78,88,65,89,65,90,66,88,57,89,65,88,66,89,72,108,79,87,57,71,65,100,65,101,65,102,67,107,65,108,70,100,75,90,65,88,67,89,67,90,65,108,65,100,67,101,76,102,79,107,44,108,65,87,67,71,67,87,71,71,76,87,78,71,78,59,65,60,68,87,76,71,72,87,76,71,70,87,72,71,72,87,71,100,75,101,78,102,75,100,70,101,34,102,61,100,68,88,72,89,72,90,72,88,72,89,72,90,75,88,72,89,76,90,75,88,72,88,72,89,75,90,75,88,75,89,75,90,75,88,79,89,76,90,72,88,76,89,76,88,72,89,75,108,76,87,78,71,75,100,75,101,76,102,70,107,69,108,71,100,68,90,78,88,72,89,72,90,70,108,69,100,65,101,65,102,65,107,75,108,76,87,73,71,68,87,65,71,57,87,67,71,67,59,76,60,68,87,68,71,65,87,44,71,65,87,68,71,69,87,67,100,65,101,65,102,44,100,65,101,67,102,68,100,68,104,68,105,68,3,70,15,70,105,72,106,75,104,72,105,72,106,67,104,52,104,63,105,70,106,72,104,76,105,70,106,69,104,65,105,52,106,52,104,61,105,69,104,63,105,61,106,67,108,65,100,44,106,52,104,67,105,68,106,69,104,65,105,65,106,44,104,44,105,44,106,57,104,65,105,65,106,65,104,44,105,65,106,57,108,44,100,44,101,65,102,68,100,67,107,65,108,65,100,57,101,57,102,57,71,65,87,67,71,65,87,65,100,57,105,39,106,44,104,57,105,57,106,67,104,57,105,44,104,72,105,70,3,67,15,65,105,65,106,65,104,57,105,52,106,71,104,67,104,57,105,57,106,65,104,65,105,57,106,61,104,65,105,57,106,57,104,67,105,68,104,67,105,65,106,61,108,57,100,57,106,65,104,69,105,68,106,65,104,44,105,57,106,44,104,65,105,65,106,67,104,65,105,44,106,57,104,57,105,65,106,67,108,65,100,65,101,44,102,57,100,57,107,61,108,67,100,68,101,65,102,57,71,57,87,57,71,57,87,61,100,65,105,65,106,44,104,57,105,44,106,44,104,57,105,57,88,44,89,35,4,35,16,35,89,35,90,57,88,57,89,57,90,44,88,35,88,43,89,69,90,73,88,73,89,67,90,67,88,35,89,35,90,68,88,73,89,73,88,71,89,73,90,68,88,35,89,43,90,73,88,73,89,68,90,65,88,67,89,73,90,35,88,69,89,73,90,67,88,57,89,39,90,69,88,73,89,39,90,69,88,71,89,67,88,39,89,52,90,73,88,73,89,52,90,68,88,73,89,73,108,73,100,73,101,73,100,69,88,69,89,61,90,68,88,73,89,73,90,68,88,68,89,69,88,75,89,70,4,70,16,72,89,72,90,72,88,70,89,72,90,70,88,63,88,70,89,72,90,72,88,70,89,63,90,63,88,70,89,70,90,72,88,72,89,70,88,63,89,63,90,34,88,69,89,70,90,72,88,70,89,70,90,70,88,70,89,63,90,69,88,70,89,72,90,72,88,70,89,70,90,72,88,76,89,52,90,61,88,63,89,70,88,70,89,72,90,72,88,72,89,35,90,35,88,52,89,63,108,70,100,72,101,72,100,70,88,57,89,57,90,69,88,69,89,70,90,72,88,76,89,76,104,78,105,78,106,78,104,78,105,75,106,70,104,23,105,21,106,75,104,78,104,72,105,34,106,34,104,34,105,34,119,34,120,75,121,79,122,78,104,72,105,34,104,21,105,21,106,23,104,76,105,72,106,75,104,78,105,75,106,72,104,34,105,34,106,75,104,78,105,75,106,75,104,75,105,78,106,78,104,78,105,63,106,70,104,72,105,75,104,72,105,76,106,75,104,75,105,63,106,72,104,72,105,34,106,34,104,75,105,78,106,78,104,69,105,71,106,72,104,76,105,72,105,70,3,70,15,75,104,34,105,70,106,72,104,78,105,78,106,79,104,78,105,78,106,63,104,34,104,34,105,70,106,72,104,78,105,76,119,34,120,63,121,72,122,70,104,34,105,63,104,78,105,75,106,34,104,23,105,34,106,70,104,72,105,75,106,79,104,78,105,75,106,72,104,34,105,21,106,23,104,78,105,79,106,79,104,75,105,78,106,78,104,75,105,72,104,78,105,79,106,78,104,75,105,75,106,78,104,78,105,78,106,78,104,79,105,75,106,76,104,78,105,78,106,79,104,79,105,79,105,78,3,72,15,21,88,78,89,34,90,21,88,76,89,78,90,78,88,78,89,72,90,72,88,23,88,21,89,76,90,78,88,78,89,79,103,76,95,76,111,23,127,21,88,78,89,78,88,78,89,78,90,76,88,76,89,34,90,70,88,79,89,76,90,70,88,76,89,76,90,34,88,34,89,78,90,78,88,72,89,72,90,72,88,75,89,63,90,72,88,78,89,78,88,78,89,75,90,72,88,72,89,76,90,79,88,78,89,78,90,78,88,75,89,63,90,61,88,21,89,72,90,78,88,79,89,78,89,75,4,72,16,70,88,57,89,66,90,61,88,76,89,78,90,76,88,34,89,70,90,75,88,72,88,34,89,75,90,78,88,72,89,34,103,75,95,75,111,72,127,72,88,75,89,75,88,34,89,34,90,78,88,76,89,72,90,75,88,76,89,63,90,34,88,63,89,78,90,72,88,72,89,78,90,72,88,34,89,23,90,72,88,78,89,70,90,75,88,78,89,72,88,21,89,21,90,76,88,79,89,52,90,71,88,75,89,75,90,70,88,72,89,78,90,78,88,70,89,76,90,75,88,79,89,78,89,78,4,75,16,72,104,78,105,78,106,70,104,63,105,63,106,72,104,78,105,78,106,78,104,78,104,72,105,72,106,70,104,70,105,76,106,78,104,79,105,72,106,63,104,70,105,69,104,69,105,70,106,72,104,72,105,21,106,21,104,63,105,69,106,57,104,69,105,70,106,63,104,23,105,21,106,63,104,73,105,35,106,57,104,44,105,70,106,63,104,34,105,63,104,73,105,57,106,57,104,44,105,75,106,72,104,70,105,63,106,69,104,67,105,57,106,44,104,72,105,70,106,70,104,69,105,73,106,67,104,57,105,65,104,76,105,71,106,68,104,67,105,57,106,44,104,57,105,57,106,78,104,78,104,73,105,67,106,57,104,57,105,57,106,61,104,76,105,75,106,76,104,67,105,57,104,57,105,57,106,61,104,72,105,72,106,73,104,67,105,57,106,65,104,57,105,61,106,68,104,70,105,71,106,67,104,65,105,67,106,65,104,61,105,65,106,65,104,67,105,67,104,67,105,65,106,44,104,61,105,65,106,65,104,57,105,57,106,65,104,44,105,39,106,61,104,67,105,67,106,57,104,44,105,57,106,57,104,39,105,69,104,26,105,24,106,26,88,26,89,26,90,26,88,26,89,25,90,26,88,24,88,25,89,26,90,25,88,26,89,26,90,25,88,24,89,24,90,25,88,25,89,24,88,25,89,24,90,24,88,24,89,24,90,24,88,24,89,24,90,24,88,23,89,24,104,29,104,29,105,31,106,25,88,31,89,28,90,25,88,26,89,29,90,28,88,29,88,25,89,31,90,28,88,25,89,24,89,29,90,25,88,29,89,25,90,27,88,26,88,28,89,27,90,27,88,25,89,29,90,29,88,28,89,28,90,26,88,28,104,24,105,29,106,27,88,29,89,29,90,31,88,31,89,27,90,25,88,28,88,26,89,29,90,29,88,32,89,31,90,27,88,26,89,25,90,28,88,27,89,29,88,29,89,29,90,27,88,24,89,25,90,26,88,28,89,29,90,31,88,29,89,25,104,28,104,29,105,29,106,31,88,24,89,25,90,24,88,24,89,28,90,29,88,28,88,31,89,24,90,24,88,23,89,24,89,24,90,24,88,23,89,25,90,25,88,23,88,24,89,24,90,24,88,23,89,23,90,25,88,25,89,23,90,23,88,24,104,31,105,31,106,29,56,31,57,27,57,31,57,28,57,29,57,29,57,31,57,29,57,31,57,25,58,29,105,25,106,27,104,28,105,29,106,29,104,31,105,25,104,29,105,25,106,25,104,28,105,29,106,28,104,31,105,25,106,29,104,25,105,25,56,26,56,25,57,28,57,31,57,25,57,31,57,29,57,29,57,24,57,25,57,28,57,31,57,25,57,29,58,28,105,29,105,27,106,28,104,26,105,27,106,25,104,29,104,25,56,29,57,28,57,26,57,28,57,28,57,29,57,29,57,25,57,27,104,44,105,44,106,57,56,68,57,65,57,65,57,71,57,75,57,57,57,67,57,69,57,68,57,65,58,67,105,75,106,78,104,57,105,57,106,67,104,65,105,44,104,67,105,76,106,75,104,57,105,57,106,57,104,44,105,57,106,67,104,76,105,76,56,57,56,57,57,44,57,57,57,44,57,65,57,71,57,71,57,57,57,57,57,44,57,65,57,57,57,44,58,65,105,65,105,57,106,57,104,44,105,67,106,65,104,65,104,67,56,67,57,57,57,68,57,68,57,73,57,68,57,67,57,57,57,57,104,78,105,79,106,78,72,72,73,69,73,63,73,57,73,57,73,75,73,72,73,72,73,70,73,68,74,69,89,52,90,57,88,76,89,70,90,71,88,68,89,65,88,65,105,67,106,67,104,71,105,68,106,65,104,65,105,65,106,68,104,68,105,67,105,67,72,65,73,65,73,67,73,65,73,67,73,65,73,65,73,44,73,65,73,67,73,57,73,44,73,44,74,44,89,65,89,65,90,65,88,57,89,44,90,44,88,44,88,57,72,67,73,65,73,68,73,67,73,68,73,68,73,68,73,68,73,67,104,24,105,24,106,24,72,25,73,21,73,24,73,19,73,19,73,24,73,24,73,24,73,23,73,21,74,24,89,20,90,20,88,24,89,27,90,28,88,24,89,21,88,24,105,21,106,21,104,24,105,28,106,28,104,26,105,25,106,24,104,21,105,21,105,25,72,28,73,28,73,28,73,25,73,24,73,25,73,25,73,27,73,29,73,28,73,28,73,24,73,24,74,24,89,25,89,25,90,29,88,31,89,27,90,25,88,25,88,24,72,24,73,25,73,25,73,29,73,29,73,27,73,26,73,25,73,25,104,19,105,19,106,19,61,19,62,19,62,19,62,19,62,19,62,20,62,19,62,19,62,19,62,19,63,19,105,19,106,20,104,20,105,20,106,20,104,20,105,20,104,20,89,20,90,20,88,20,89,20,90,20,88,20,89,20,90,20,88,20,89,20,89,21,61,21,62,21,62,21,62,21,62,21,62,21,62,21,62,21,62,21,62,21,62,21,62,21,62,21,63,21,105,21,105,23,106,23,104,23,105,23,106,23,104,23,104,23,61,23,62,25,62,23,62,25,62,23,62,23,62,25,62,23,62,25,104,19,105,19,106,20,61,21,62,24,62,24,62,24,62,24,62,20,62,20,62,21,62,21,62,23,63,24,105,24,106,24,104,21,105,21,106,21,104,21,105,24,104,28,89,27,90,24,88,21,89,21,90,25,88,25,89,26,90,28,88,28,89,24,89,25,61,25,62,24,62,25,62,28,62,28,62,28,62,25,62,25,62,24,62,24,62,24,62,28,62,28,63,29,105,27,105,24,106,24,104,25,105,25,106,27,104,31,104,29,61,25,62,25,62,25,62,26,62,27,62,29,62,29,62,25,62,25,104,57,105,44,106,35,77,52,78,69,78,73,78,69,78,69,78,57,78,52,78,43,78,68,78,73,79,73,89,73,90,73,88,57,89,68,90,73,88,73,89,73,88,73,105,73,106,73,104,57,105,67,106,73,104,73,105,73,106,73,104,71,105,76,77,67,77,69,78,73,78,73,78,73,78,74,78,74,78,77,78,68,78,69,78,69,78,69,78,73,78,73,79,73,89,74,89,68,90,69,88,68,89,68,90,73,88,69,88,73,77,77,78,69,78,73,78,57,78,68,78,73,78,71,78,76,78,77,104,73,105,73,106,76,77,76,78,76,78,77,78,77,78,77,78,73,78,73,78,73,78,73,78,74,79,77,89,77,90,77,88,74,89,73,90,74,88,76,89,74,88,74,105,77,106,77,104,77,105,77,106,74,104,77,105,74,106,76,104,77,105,77,77,77,77,77,78,74,78,77,78,74,78,74,78,77,78,77,78,77,78,77,78,74,78,77,78,77,78,74,79,77,89,77,89,77,90,77,88,73,89,74,90,76,88,73,88,73,77,73,78,74,78,74,78,73,78,73,78,73,78,57,78,35,78,35,104,77,105,77,106,77,104,77,105,77,106,77,104,76,105,74,106,77,104,77,104,77,105,77,106,77,104,77,105,77,90,77,88,77,89,77,105,77,104,77,105,77,104,77,89,77,90,77,88,77,89,77,90,77,88,77,89,77,90,77,88,77,89,77,89,77,104,77,105,77,106,77,104,77,105,77,106,77,104,77,105,77,106,77,104,77,104,77,105,77,106,77,89,77,90,77,88,73,89,74,88,76,105,77,106,77,104,77,106,77,104,77,105,35,106,35,104,69,104,73,105,76,106,77,104,77,105,77,104,74,105,73,106,73,104,73,105,68,106,65,104,57,105,61,106,77,104,77,104,74,105,73,106,71,104,67,105,57,90,57,88,77,89,77,105,77,104,77,105,76,104,73,89,67,90,52,88,77,89,77,90,77,88,77,89,77,90,76,88,71,89,68,89,77,104,77,105,77,106,77,104,77,105,77,106,73,104,71,105,77,106,77,104,77,104,77,105,77,106,77,89,74,90,71,88,77,89,77,88,77,105,77,106,77,104,77,106,77,104,73,105,77,106,77,104,77,104,77,105,77,106,77,104,77,105,73,88,57,89,65,90,65,88,57,89,44,90,44,88,35,89,35,90,57,88,57,88,44,89,44,90,44,88,35,89,35,106,35,104,57,105,57,90,44,88,35,89,35,88,35,89,35,90,35,88,52,89,57,90,35,88,35,89,35,90,35,88,35,89,43,90,52,88,43,89,35,90,35,88,35,89,43,90,35,88,35,89,43,90,35,88,35,89,35,88,43,89,43,104,21,3,19,15,43,105,43,104,43,89,43,90,52,88,35,89,19,90,19,89,43,90,52,88,35,89,35,90,35,88,19,88,17,89,17,88,35,89,35,90,57,88,65,89,65,90,57,88,57,89,57,90,39,88,57,88,68,89,68,90,71,88,69,89,67,106,68,104,44,105,68,90,71,88,71,89,73,88,73,89,68,90,69,88,57,89,35,90,35,88,57,89,68,90,73,88,73,89,73,90,21,88,19,89,19,90,21,88,35,89,69,90,73,88,73,89,17,90,17,88,17,89,17,88,19,89,35,104,69,3,73,15,17,105,17,104,19,89,17,90,17,88,21,89,43,90,71,89,17,90,17,88,19,89,19,90,17,88,19,88,35,89,74,104,68,105,52,106,57,104,57,105,57,106,57,104,52,105,57,106,69,104,67,104,57,105,44,106,57,104,57,105,44,90,44,88,73,89,69,106,67,104,57,105,57,104,57,105,44,106,44,104,73,105,73,106,68,104,65,105,57,106,57,104,57,105,44,106,73,104,73,105,69,106,67,104,57,105,57,106,44,104,44,105,74,106,73,104,73,105,68,104,57,105,57,3,44,87,57,71,76,15,74,88,73,105,68,106,52,104,57,105,57,106,57,105,77,119,74,120,73,121,69,122,65,104,57,104,44,105,52,104,44,105,44,106,44,104,44,105,44,106,44,104,44,105,44,106,44,104,44,104,44,105,44,106,44,104,44,105,44,90,57,88,44,89,44,106,44,104,44,105,52,104,52,105,52,106,57,104,44,105,44,106,44,104,57,105,57,106,61,104,69,105,68,106,52,104,57,105,57,106,57,104,52,105,52,106,57,104,65,105,52,106,52,104,57,105,57,104,44,105,52,3,57,87,57,71,52,15,52,88,52,105,44,106,35,104,44,105,57,106,57,105,52,119,52,120,44,121,35,122,39,104,44,104,57,105,57,88,25,89,25,90,29,88,29,89,27,90,26,88,25,89,25,90,25,88,29,88,31,89,27,90,25,88,25,89,24,106,24,104,27,105,29,106,28,104,28,105,24,104,24,89,24,90,25,88,25,89,28,90,28,88,28,89,25,90,24,88,25,89,25,90,24,88,28,89,28,90,26,88,25,89,25,90,23,88,23,89,24,90,27,88,28,89,24,88,23,89,21,4,21,71,21,87,24,16,24,104,24,105,23,106,24,88,21,89,20,90,20,89,24,103,24,95,24,111,25,127,24,88,20,88,19,89,19,88,23,89,25,90,23,88,25,89,25,90,23,88,25,89,23,90,23,88,25,88,25,89,23,90,23,88,25,89,25,106,23,104,23,105,25,106,25,104,25,105,25,104,25,89,25,90,23,88,25,89,25,90,25,88,25,89,25,90,25,88,25,89,25,90,21,88,21,89,21,90,21,88,21,89,21,90,21,88,21,89,21,90,20,88,20,89,20,88,20,89,20,4,20,71,21,87,20,16,20,104,20,105,20,106,20,88,20,89,20,90,20,89,19,103,19,95,19,111,19,127,19,88,19,88,19,89,19,105,25,3,25,15,26,105,27,105,29,106,29,104,25,105,25,106,24,104,24,104,25,105,25,106,27,104,31,105,29,106,25,104,25,89,24,90,24,88,24,89,28,88,28,105,29,106,27,104,25,105,25,106,24,104,25,105,28,106,28,104,28,105,25,106,23,104,23,105,25,106,25,104,26,105,28,106,28,104,24,105,21,106,21,104,23,105,21,104,24,105,28,89,27,111,24,127,20,89,20,88,21,88,23,106,24,104,24,105,24,106,24,105,19,106,21,104,21,105,21,106,24,104,24,104,24,105,24,105,35,3,35,15,52,105,68,105,68,106,57,104,52,105,52,106,35,104,35,104,43,105,67,106,68,104,52,105,43,106,44,104,35,89,43,90,44,88,65,89,68,88,52,105,35,106,35,104,68,105,52,106,57,104,65,105,69,106,35,104,35,105,35,106,69,104,43,105,68,106,71,104,71,105,57,106,35,104,35,105,73,106,52,104,71,105,76,104,76,105,74,89,73,111,73,127,73,89,69,88,74,88,77,106,77,104,77,105,77,106,77,105,74,106,73,104,74,105,77,106,77,104,77,104,77,105,77,89,57,4,57,16,57,89,52,89,44,90,43,88,44,89,35,90,44,88,44,88,44,89,57,90,44,88,44,89,35,90,39,88,39,105,39,106,39,104,43,105,35,104,35,89,35,90,35,88,35,89,35,90,35,88,43,89,35,90,35,88,35,105,35,106,35,104,35,105,35,90,35,88,52,89,68,90,52,88,52,89,73,90,67,88,35,89,35,88,71,89,74,90,74,88,73,89,77,90,74,88,73,89,71,90,74,88,77,89,77,90,77,88,76,89,76,90,76,88,76,89,76,90,77,88,77,89,77,89,27,4,25,16,29,89,29,89,28,90,28,88,26,89,28,90,29,88,25,88,29,89,25,90,27,88,26,89,28,90,27,88,29,105,28,106,29,104,25,105,31,104,28,89,25,90,24,88,29,89,29,90,31,88,25,89,31,90,28,88,25,105,26,106,25,104,24,105,24,90,26,88,25,89,26,90,25,88,25,89,26,90,24,88,25,89,26,88,25,89,26,90,26,88,25,89,26,90,25,88,26,89,26,90,25,88,26,89,26,90,26,88,26,89,25,90,26,88,26,89,26,90,28,88,26,89,25,104,25,105,25,106,23,104,25,105,25,106,23,104,25,105,25,106,25,104,24,104,24,105,26,106,27,124,29,116,25,117,27,116,26,105,24,106,25,104,26,105,28,104,31,105,27,106,29,104,26,105,25,106,26,104,26,105,29,106,31,104,29,89,30,90,24,88,28,89,26,106,29,104,31,105,31,106,31,104,29,105,26,106,28,104,28,105,28,104,29,105,29,106,31,104,28,105,26,106,28,104,29,105,28,124,29,116,29,117,29,118,28,116,26,105,28,106,31,104,29,105,29,106,31,104,29,105,29,104,28,105,26,106,28,104,28,105,29,106,29,104,25,105,27,106,27,104,28,104,26,105,27,106,25,124,29,116,25,117,29,116,24,105,25,106,28,104,31,105,25,104,29,105,28,106,29,104,26,105,25,106,28,104,31,105,25,106,31,104,29,89,29,90,29,88,33,89,30,106,29,104,27,105,29,106,25,104,27,105,29,106,33,104,30,105,31,104,28,105,31,106,27,104,29,105,29,106,33,104,30,105,31,124,29,116,31,117,29,118,30,116,31,105,33,106,33,104,32,105,31,106,31,104,29,105,30,88,68,89,68,90,65,88,67,89,73,90,73,88,73,89,73,90,52,88,68,88,68,89,68,124,73,87,69,71,69,87,68,71,68,116,52,117,68,118,68,123,68,117,52,118,52,116,35,88,68,89,43,90,57,88,52,89,43,90,52,88,68,89,52,90,43,88,35,89,67,90,68,88,35,89,35,90,73,88,69,124,52,116,57,117,52,118,52,116,57,117,44,116,74,88,61,89,69,90,43,88,35,124,43,87,69,71,61,87,76,71,35,87,52,116,43,117,35,118,43,116,69,90,67,88,76,89,35,88,73,89,69,90,35,88,35,89,21,90,21,88,19,89,19,90,52,88,35,88,35,89,21,124,19,87,19,71,17,87,17,71,35,116,21,117,19,118,17,123,17,117,17,118,17,116,17,88,35,89,21,90,17,88,17,89,17,90,19,88,17,89,17,90,35,88,19,89,17,90,17,88,17,89,19,90,17,88,17,124,21,116,19,117,17,118,17,116,17,117,17,116,17,88,17,89,19,90,19,88,17,124,17,87,17,71,17,87,17,71,17,87,21,116,19,117,17,118,17,116,17,90,17,88,17,89,17,117,19,118,19,123,21,124,35,116,35,117,67,118,73,116,77,117,17,118,17,123,19,124,19,71,21,87,35,87,67,1,74,2,17,71,17,87,17,5,17,6,19,71,19,71,35,87,73,116,17,117,19,118,19,116,17,105,17,106,17,104,21,105,43,106,17,104,19,105,19,124,17,116,17,117,17,118,19,124,21,87,17,71,17,87,19,5,19,6,17,71,17,87,17,116,19,117,17,118,17,124,17,87,17,71,17,87,17,71,17,87,19,71,17,71,19,87,17,71,17,87,19,116,17,117,17,118,19,117,77,118,77,123,73,124,74,116,77,117,77,118,77,116,73,117,77,118,77,123,74,124,77,71,77,87,77,87,77,1,73,2,77,71,77,87,77,5,77,6,77,71,77,71,77,87,73,116,71,117,77,118,77,116,77,105,77,106,77,104,77,105,73,106,35,104,73,105,77,124,77,116,77,117,77,118,77,124,73,87,35,71,52,87,77,5,77,6,77,71,77,87,77,116,73,117,21,118,35,124,76,87,77,71,77,87,77,71,77,87,73,71,21,71,35,87,76,71,77,87,77,116,74,117,77,118,73,71,43,87,52,71,35,87,35,71,35,87,19,71,17,87,17,71,43,71,43,87,43,71,43,87,52,71,35,71,19,17,19,18,43,19,35,20,35,21,35,22,43,87,43,87,21,71,19,87,52,71,43,87,35,71,35,116,35,117,43,118,35,123,35,124,52,123,57,124,35,87,35,71,35,87,35,71,35,87,43,71,57,20,57,20,44,21,35,22,35,87,35,71,35,87,35,87,67,71,57,87,57,71,44,87,43,71,35,87,35,71,35,87,68,71,57,87,57,71,52,71,52,87,52,71,43,87,43,71,17,87,17,71,19,87,19,71,17,87,19,71,35,87,74,71,17,71,17,87,19,71,17,87,17,71,21,71,43,17,71,18,17,19,17,20,17,21,17,22,19,87,35,87,69,71,73,87,21,71,19,87,19,71,21,116,35,117,69,118,73,123,73,124,57,123,35,124,35,87,57,71,68,87,73,71,73,87,73,71,44,20,68,20,71,21,71,22,73,87,73,71,68,87,73,87,35,71,52,87,68,71,69,87,73,71,69,87,68,71,69,87,43,71,52,87,66,71,68,71,68,87,68,71,68,87,68,71,77,87,74,71,73,87,68,71,57,87,44,71,57,87,57,71,76,87,74,71,73,87,68,71,57,87,52,32,57,33,52,34,74,35,73,36,73,37,68,38,61,39,67,71,67,87,52,71,73,87,73,71,69,87,68,71,57,87,57,87,57,71,57,87,73,71,73,87,69,17,65,18,57,19,44,20,44,21,44,35,73,35,69,36,67,37,57,38,44,71,44,87,57,71,44,87,73,71,67,87,57,71,57,87,44,71,57,87,67,71,67,71,68,87,67,71,68,87,68,71,67,87,67,71,71,87,73,71,52,87,44,71,57,87,44,71,44,87,57,71,57,87,43,71,52,87,52,71,57,87,57,71,57,87,57,32,44,33,43,34,52,35,67,36,67,37,57,38,57,39,57,71,44,87,44,71,57,87,57,71,57,87,57,71,57,87,57,87,57,71,57,87,57,71,44,87,44,17,57,18,57,19,57,20,57,21,57,35,65,35,44,36,57,37,57,38,57,71,57,87,44,71,57,87,65,71,57,87,57,71,67,87,52,71,57,87,57,71,52,71,65,87,57,71,57,87,67,71,65,87,57,71,44,87,57,87,25,71,24,87,24,71,26,87,25,71,26,87,25,71,25,87,26,71,24,87,25,71,26,87,25,71,26,71,26,49,25,50,26,51,25,52,26,53,26,54,25,55,26,87,26,71,26,87,26,71,25,87,26,71,26,87,26,71,28,87,26,71,25,87,26,71,25,87,25,33,28,34,26,35,28,36,28,34,25,51,28,51,25,52,26,53,29,54,28,55,28,87,28,71,25,87,28,71,26,87,28,71,29,87,27,48,28,87,27,71,26,87,29,71,26,87,28,71,29,87,27,71,28,87,28,71,25,87,24,71,28,87,26,71,29,87,31,71,31,87,31,71,29,87,26,71,28,87,28,71,28,87,29,71,29,71,31,49,28,50,26,51,28,52,29,53,28,54,29,55,29,87,29,71,28,87,26,71,28,87,31,71,29,87,29,71,31,87,29,71,29,87,26,71,29,87,31,33,29,34,31,35,33,36,31,34,31,51,26,51,31,52,31,53,31,54,29,55,33,87,31,71,29,87,26,71,29,87,32,71,29,87,29,48,33,87,31,71,29,87,28,71,29,87,32,71,29,87,29,71,33,87,31,71,29,71,29,87,33,71,30,87,29,71,27,87,29,71,25,68,27,69,29,70,33,71,30,87,31,71,28,87,31,87,27,9,29,10,29,11,33,12,30,13,31,14,29,71,31,71,29,87,30,71,31,87,33,71,33,87,32,71,31,87,31,71,29,1,30,2,31,71,33,87,33,5,32,10,31,11,31,12,29,50,33,10,33,11,33,12,33,13,32,71,31,87,31,1,31,2,33,71,33,5,33,6,33,87,32,71,31,87,31,68,31,69,33,70,33,87,33,71,33,87,32,71,31,87,31,71,31,87,33,71,76,87,73,71,76,87,77,71,77,87,77,71,77,68,77,69,76,70,71,71,74,87,77,71,77,87,77,87,77,9,77,10,74,11,73,12,74,13,77,14,77,71,77,71,76,87,76,71,74,87,76,71,77,87,77,71,77,87,77,71,74,1,74,2,74,71,74,87,77,5,77,10,77,11,77,12,76,50,74,10,74,11,76,12,76,13,74,71,74,87,74,1,76,2,76,71,73,5,73,6,74,87,74,71,73,87,74,68,74,69,76,70,69,87,69,71,70,87,69,71,61,87,69,71,70,87,70,87,74,71,74,64,74,65,77,66,77,67,77,87,77,84,77,85,76,86,76,71,76,87,77,71,77,87,77,71,77,25,77,26,77,27,76,28,77,29,77,30,77,31,77,87,77,71,77,87,77,71,77,87,77,71,77,87,77,71,77,87,77,17,77,18,77,19,77,20,77,21,77,26,77,27,77,28,77,29,77,26,76,27,76,28,77,29,77,55,77,71,77,87,77,46,77,87,76,71,76,46,74,71,76,87,73,71,74,84,74,85,74,86,70,71,70,87,70,71,70,87,61,71,61,87,69,71,61,87,43,71,43,64,43,65,57,66,65,67,65,87,57,84,57,85,43,86,35,71,35,87,43,71,44,87,44,71,44,25,44,26,35,27,35,28,35,29,35,30,44,31,44,87,44,71,57,87,35,71,35,87,35,71,35,87,57,71,57,87,44,17,57,18,35,19,52,20,52,21,43,26,52,27,52,28,52,29,67,26,69,27,73,28,73,29,43,55,35,71,43,87,43,46,57,87,73,71,73,46,68,71,35,87,35,71,35,84,35,85,43,86,73,71,69,87,57,71,35,87,35,71,35,87,35,71,35,71,44,87,44,80,57,81,57,82,67,83,52,71,43,87,52,71,44,71,44,19,44,20,57,30,52,31,43,40,35,41,43,42,44,43,44,44,44,45,57,46,52,47,43,71,43,87,52,7,57,8,44,71,44,87,44,71,44,87,35,71,39,33,52,34,68,35,57,36,43,37,35,34,35,35,35,38,35,35,69,36,43,37,35,54,35,54,35,55,35,7,35,8,35,87,73,71,35,87,35,71,35,87,35,71,35,87,43,71,69,48,76,71,35,87,35,71,35,87,67,71,73,87,74,71,77,87,76,71,61,87,50,80,52,81,52,82,65,83,49,71,49,87,70,71,41,71,41,19,48,20,58,30,72,31,72,40,70,41,70,42,39,43,35,44,46,45,72,46,63,47,70,71,34,87,31,7,35,8,41,71,48,87,72,71,70,87,34,71,28,33,30,34,39,35,72,36,72,37,70,34,31,35,25,38,25,35,28,36,72,37,70,54,34,54,29,55,32,7,27,8,27,87,30,71,34,87,28,71,32,87,28,71,31,87,25,71,25,48,29,71,28,87,28,71,32,87,27,71,31,87,25,71,25,87,32,87,52,71,43,96,35,97,43,98,69,99,67,87,76,71,35,87,61,87,61,44,57,45,43,46,52,47,44,87,76,71,52,87,52,71,52,87,52,71,52,87,43,71,44,87,74,71,61,23,43,24,43,87,68,71,67,87,35,71,35,87,73,49,69,50,69,35,68,11,44,33,57,50,57,51,57,52,68,51,43,52,67,53,68,14,68,71,67,87,69,23,52,24,52,71,35,87,57,71,67,64,69,65,68,66,73,67,69,87,71,71,68,87,67,71,69,87,57,71,52,87,73,71,73,87,76,71,76,87,21,71,19,96,17,97,17,98,17,99,17,87,17,71,17,87,19,87,17,44,17,45,17,46,17,47,17,87,17,71,17,87,21,71,19,87,17,71,17,87,17,71,17,87,17,71,17,23,21,24,19,87,17,71,17,87,17,71,17,87,17,49,17,50,35,35,21,11,17,33,17,50,17,51,17,52,17,51,17,52,35,53,21,14,19,71,17,87,17,23,17,24,17,71,17,87,52,71,35,64,35,65,21,66,19,67,19,87,17,71,17,87,74,71,74,87,69,71,35,87,35,71,35,87,21,71,21,71,17,87,19,112,17,113,17,114,19,115,17,71,17,87,19,71,17,87,17,71,19,87,17,71,17,87,17,71,17,87,19,71,17,87,17,71,19,87,19,71,17,87,17,71,17,87,19,71,17,87,19,71,19,87,17,71,17,87,17,71,19,9,21,11,17,50,17,50,17,51,17,10,17,11,17,12,35,35,69,12,17,13,17,30,17,31,17,71,19,87,19,71,35,87,73,71,17,87,17,80,19,81,19,82,21,83,35,71,68,87,74,71,21,87,21,71,35,87,35,71,57,87,71,71,76,87,77,71,21,87,35,112,76,113,77,114,77,115,74,71,76,87,73,71,21,87,35,71,76,87,77,71,77,87,77,71,74,87,73,71,35,87,66,71,77,87,77,71,77,87,77,71,77,87,73,71,43,87,73,71,77,87,77,71,77,87,77,71,77,9,73,11,76,50,77,50,77,51,77,10,77,11,77,12,77,35,73,12,77,13,77,30,77,31,77,71,77,87,77,71,77,87,73,71,77,87,77,80,77,81,74,82,77,83,77,71,77,87,73,71,77,87,77,71,77,87,77,71,77,87,77,71,74,87,73,87,58,71,47,87,58,71,40,87,37,71,37,87,37,71,36,87,50,71,50,87,37,71,37,87,34,71,34,87,37,71,34,87,50,71,50,87,37,71,34,87,32,71,32,87,34,71,34,87,50,71,37,87,34,71,32,87,31,71,31,87,29,25,32,34,50,35,34,50,34,37,31,26,31,27,31,28,31,51,32,28,34,29,28,46,29,47,31,5,33,6,31,87,29,71,33,87,34,71,28,96,31,97,31,98,33,99,31,87,29,71,31,1,27,2,29,87,31,71,29,87,29,71,29,87,29,71,31,87,34,71,37,87,50,71,61,87,64,71,52,87,56,71,64,87,34,71,34,87,37,71,61,87,66,71,64,87,64,71,40,87,34,71,34,87,37,71,50,87,58,71,40,87,50,71,50,87,32,71,32,87,37,71,37,87,50,71,36,87,37,25,37,34,31,35,33,50,34,37,37,26,37,27,36,28,32,51,34,28,32,29,32,46,32,47,34,5,34,6,34,87,33,71,33,87,31,71,31,96,32,97,31,98,32,99,31,87,33,71,33,1,31,2,33,87,33,71,29,87,31,71,29,87,30,71,33,71,42,87,60,71,46,87,38,71,38,87,40,71,43,87,49,71,48,87,54,71,49,87,38,71,36,87,36,71,36,87,50,71,38,87,40,71,40,87,40,71,37,87,37,71,37,87,37,71,36,87,36,71,36,87,36,71,36,87,37,40,37,41,37,50,28,51,34,52,37,53,37,51,34,52,37,53,37,43,37,44,28,45,32,87,32,71,32,17,34,47,32,87,34,71,34,87,28,71,28,112,28,113,28,114,32,115,31,87,32,40,34,41,28,47,28,71,26,87,25,71,28,87,28,71,31,87,32,71,70,87,72,71,55,87,49,71,57,87,54,71,62,87,62,71,34,87,70,71,73,87,38,71,38,87,60,71,46,87,62,71,25,87,70,71,72,87,73,71,41,87,60,71,60,87,64,71,24,87,34,71,70,87,72,71,72,87,73,40,62,41,48,50,25,51,24,52,34,53,70,51,70,52,72,53,73,43,54,44,26,45,25,87,27,71,29,17,34,47,70,87,72,71,72,87,28,71,25,112,25,113,27,114,26,115,34,87,34,40,70,41,28,47,25,71,26,87,26,71,24,87,26,71,29,87,27,87,28,71,26,87,26,71,29,87,28,71,28,87,26,71,25,87,26,87,25,7,25,8,28,71,26,71,28,87,28,71,25,87,26,71,25,87,26,71,26,87,26,71,28,87,28,71,25,87,26,71,24,87,26,71,26,87,26,71,26,71,26,9,26,10,26,53,24,12,26,13,26,35,26,12,26,13,26,14,25,71,26,87,24,1,25,2,26,71,25,87,26,5,26,6,25,71,24,87,24,71,25,87,25,71,24,87,25,71,24,87,24,46,24,87,24,87,24,71,24,87,24,71,24,87,23,71,24,87,27,71,29,87,32,71,29,87,29,71,33,87,31,71,31,87,25,87,29,7,31,8,31,71,29,71,33,87,31,71,31,87,27,71,29,87,31,71,27,87,31,71,31,87,29,71,29,87,26,71,29,87,29,71,27,87,31,71,29,71,29,9,27,10,24,53,29,12,27,13,29,35,29,12,31,13,31,14,27,71,25,87,28,1,26,2,29,71,29,87,32,5,31,6,27,71,26,87,25,71,28,87,27,71,29,87,29,71,29,87,27,46,24,87,25,87,26,71,28,87,29,71,31,87,29,71,25,71,32,87,33,71,33,87,33,71,33,87,33,71,30,87,33,71,32,71,33,23,33,24,33,87,33,87,31,71,29,87,31,71,31,87,33,71,31,87,33,71,30,87,31,71,29,87,29,71,31,87,33,71,30,87,33,71,29,87,31,87,29,25,29,26,31,51,31,28,29,29,31,51,27,28,31,29,28,30,29,31,29,71,31,17,29,18,31,19,25,20,29,21,25,22,27,87,28,71,29,19,29,20,31,30,25,31,29,87,25,71,25,87,28,71,29,71,28,87,31,71,25,87,29,71,25,87,25,71,64,87,56,71,52,87,64,71,61,87,50,71,37,87,34,71,40,71,64,23,64,24,66,87,61,87,37,71,34,87,34,71,50,87,50,71,40,87,58,71,50,87,37,71,34,87,34,71,37,87,37,71,36,87,50,71,37,87,37,87,32,25,32,26,34,51,32,28,36,29,37,51,37,28,34,29,33,30,31,31,33,71,33,17,34,18,34,19,34,20,32,21,32,22,32,87,33,71,33,19,31,20,32,30,31,31,32,87,31,71,31,87,33,71,30,71,29,87,31,71,29,87,33,71,33,87,31,87,36,71,37,87,37,71,37,87,40,71,58,87,47,71,58,87,34,71,37,87,34,71,34,87,37,71,37,87,50,71,50,87,34,71,34,87,32,71,32,87,34,71,37,87,50,71,50,87,32,71,29,87,31,71,31,87,32,71,34,40,37,41,50,42,32,43,31,44,31,45,31,43,31,44,34,45,34,46,50,47,33,32,29,33,31,34,33,35,31,36,29,37,28,38,34,39,31,87,29,44,31,45,33,46,31,47,31,71,28,87,34,71,31,87,29,87,29,71,29,87,29,71,31,87,29,71,27,87,69,71,43,87,65,71,71,87,73,71,35,87,35,71,35,87,52,71,52,87,73,71,73,87,73,71,52,87,35,71,35,87,68,71,73,87,73,71,73,87,73,71,69,87,61,71,35,87,73,71,73,87,73,71,73,87,69,71,73,40,73,41,69,42,74,43,74,44,73,45,69,43,52,44,71,45,74,46,73,47,76,32,74,33,68,34,52,35,43,36,73,37,76,38,76,39,74,87,73,44,67,45,57,46,43,47,73,71,74,87,74,71,73,87,73,87,71,71,68,87,67,71,71,87,73,71,74,71,35,87,43,71,71,87,74,48,77,87,77,71,77,87,76,71,43,87,73,71,76,87,77,71,77,87,77,71,77,87,76,71,66,87,74,87,77,87,77,71,77,71,77,87,77,87,76,71,67,87,76,71,76,87,77,71,77,87,77,87,77,71,76,87,73,59,77,60,77,87,77,59,77,60,77,87,77,71,76,87,77,71,77,49,77,50,77,51,77,52,77,53,77,54,76,55,77,71,77,87,77,71,77,87,77,71,77,87,77,71,76,87,77,71,77,71,77,87,77,71,77,87,77,71,77,87,74,71,72,87,70,71,72,87,72,48,47,87,55,71,72,87,72,71,34,87,34,71,70,87,70,71,72,87,72,71,70,87,70,71,31,87,28,87,29,87,34,71,70,71,70,87,34,87,34,71,30,87,34,71,34,87,30,71,34,87,34,87,28,71,26,87,28,59,28,60,70,87,34,59,29,60,25,87,24,71,24,87,30,71,70,49,34,50,28,51,30,52,30,53,25,54,24,55,29,71,34,87,24,71,28,87,28,71,24,87,24,71,24,87,32,71,26,71,27,87,32,71,33,87,25,71,25,87,24,87,69,71,73,87,52,71,68,87,73,71,71,87,76,71,77,87,68,71,69,87,68,71,68,87,73,71,69,87,74,71,77,87,68,71,69,71,73,7,73,8,73,87,74,71,76,71,77,87,61,71,69,87,73,71,73,87,73,71,74,71,74,87,77,71,52,87,68,71,73,87,73,71,73,87,73,71,71,87,76,71,52,87,52,9,69,10,73,11,73,12,73,13,73,14,74,71,52,87,43,71,43,87,68,71,73,87,73,71,73,87,73,71,52,87,43,87,35,71,52,87,68,71,71,87,69,71,68,87,74,71,74,87,73,71,73,87,71,71,65,87,35,71,35,87,77,71,77,87,73,71,74,87,74,71,73,87,73,71,73,87,77,71,77,71,73,7,74,8,74,87,74,71,77,71,77,87,77,71,77,87,74,71,76,87,74,71,74,71,77,87,77,71,77,87,77,71,74,87,76,71,74,87,77,71,77,87,77,71,74,87,74,9,73,10,73,11,76,12,77,13,77,14,77,71,73,87,73,71,73,87,73,71,74,87,77,71,77,87,77,71,73,87,73,87,76,71,76,87,76,71,77,87,77,71,76,71,35,87,35,1,68,2,73,71,76,87,77,5,77,6,77,71,73,87,74,71,76,87,77,71,77,87,77,71,77,87,77,71,77,87,77,87,77,23,77,24,77,71,77,87,77,64,77,65,77,66,77,67,77,87,77,71,77,87,77,87,77,71,77,87,77,71,77,87,77,71,77,87,77,71,77,87,77,71,77,87,77,71,77,25,77,26,77,27,77,28,77,29,77,30,77,31,77,71,77,87,77,71,77,87,77,71,77,87,77,71,77,87,77,71,77,71,77,87,77,71,77,87,77,71,76,87,74,71,77,87,77,1,77,2,77,71,77,87,77,5,74,6,73,71,77,87,77,71,77,87,77,71,77,87,77,71,73,87,73,71,77,87,77,87,77,23,77,24,77,71,77,87,68,64,68,65,77,66,77,67,77,87,77,71,77,87,77,87,67,71,52,87,77,71,77,87,77,71,77,87,77,71,76,87,67,71,68,87,77,71,77,25,74,26,74,27,73,28,71,29,57,30,52,31,77,71,77,87,74,71,73,87,73,71,67,87,57,71,57,87,73,71,73,71,71,87,73,71,68,87,65,71,57,87,52,87,34,71,34,17,32,18,32,19,33,20,31,21,31,22,31,87,34,71,34,87,34,7,32,8,31,71,31,87,31,71,31,87,37,71,36,71,34,87,34,71,31,87,31,71,31,80,32,81,37,82,36,83,34,71,34,87,34,71,34,71,32,87,32,71,37,87,36,71,34,87,37,71,37,87,34,71,34,87,34,71,50,40,36,41,36,42,37,43,37,44,36,45,37,46,37,47,40,87,40,71,36,87,36,71,50,71,36,87,37,71,50,87,43,71,43,71,50,87,50,71,50,87,50,87,37,71,50,87,31,71,33,17,33,18,31,19,32,20,31,21,33,22,33,87,31,71,32,87,32,7,32,8,32,71,32,87,32,71,32,87,32,71,34,71,34,87,34,71,32,87,32,71,34,80,34,81,34,82,34,83,34,71,34,87,36,71,37,71,36,87,37,71,34,87,37,71,37,87,37,71,37,87,50,71,50,87,50,71,36,40,37,41,50,42,50,43,45,44,60,45,49,46,49,47,40,87,40,71,40,87,40,71,42,71,42,87,49,71,49,87,40,71,47,71,53,87,54,71,48,87,54,87,47,71,55,71,28,32,25,33,25,34,27,35,29,36,31,37,28,38,31,39,28,87,28,71,28,23,28,24,28,87,32,87,32,71,34,87,32,71,32,87,28,71,26,87,28,71,34,87,34,96,34,97,34,98,37,99,34,87,28,71,34,87,37,87,37,87,36,48,37,87,37,71,34,87,34,71,36,87,37,71,37,87,36,71,36,87,36,71,37,87,34,71,40,87,50,71,50,87,50,71,61,87,37,71,37,87,37,71,40,87,59,71,49,87,52,71,36,87,36,87,36,71,50,87,40,71,54,71,55,87,49,71,28,32,25,33,26,34,28,35,26,36,26,37,28,38,28,39,28,87,25,71,27,23,25,24,26,87,26,87,29,71,29,87,26,71,25,87,29,71,29,87,25,71,25,87,34,96,34,97,25,98,24,99,27,87,27,71,34,87,70,87,70,87,70,48,24,87,24,71,34,87,70,71,72,87,72,71,72,87,72,71,25,87,70,71,72,87,72,71,73,87,50,71,57,87,38,71,34,87,70,71,73,87,56,71,47,87,49,71,49,87,60,71,72,87,72,87,73,71,64,87,48,71,60,71,45,87,44,87,34,71,34,49,31,50,30,51,29,52,26,53,24,54,24,55,34,71,70,87,34,71,31,87,29,71,26,71,25,19,26,20,70,30,72,71,70,87,34,71,29,87,28,71,28,112,24,113,72,114,73,115,72,71,70,87,34,71,28,71,26,71,26,87,73,71,39,87,73,71,70,87,34,71,34,87,70,71,70,87,48,71,42,87,42,71,72,87,70,71,70,87,72,71,72,87,42,71,47,87,47,71,72,87,72,71,72,87,42,71,42,87,54,71,61,71,68,87,67,71,65,87,51,87,54,71,54,87,24,71,24,49,26,50,25,51,27,52,25,53,25,54,25,55,26,71,25,87,26,71,26,87,26,71,26,71,28,19,26,20,24,30,28,71,28,87,26,71,27,87,27,71,23,112,63,113,26,114,26,115,28,71,26,87,27,71,25,71,34,71,34,87,70,71,70,87,34,71,23,87,28,71,28,87,34,71,70,87,72,71,72,87,70,71,70,87,23,71,34,87,70,71,72,87,42,71,42,87,72,71,70,87,34,71,70,87,72,71,58,87,54,71,54,71,51,87,72,71,70,87,72,87,67,71,43,71,28,87,32,9,31,10,31,11,27,12,29,13,25,14,28,71,28,87,29,71,27,87,29,71,29,87,30,87,25,44,28,45,28,46,32,87,31,71,29,87,29,71,31,87,25,87,26,71,34,87,34,71,33,87,33,71,31,87,33,87,24,87,25,71,70,87,70,71,34,87,32,71,30,87,33,71,24,87,24,71,72,87,72,71,70,87,34,71,34,87,34,71,34,87,25,71,58,87,58,7,72,8,70,71,70,71,70,87,70,71,34,87,49,71,60,71,58,87,72,71,72,87,72,71,70,87,34,71,33,87,33,9,31,10,32,11,31,12,33,13,33,14,31,71,32,87,32,71,32,87,32,71,32,87,32,87,32,44,31,45,34,46,34,87,32,71,32,87,34,71,34,87,34,87,32,71,37,87,36,71,37,87,36,71,34,87,34,87,34,87,34,71,50,87,50,71,50,87,37,71,37,87,37,71,37,87,34,71,49,87,49,71,60,87,45,71,50,87,50,71,37,87,36,71,49,87,49,7,42,8,42,71,40,71,40,87,40,71,40,87,55,71,47,71,54,87,48,71,54,87,53,71,47,87,40,87,31,71,31,25,31,26,33,27,32,28,32,29,34,30,34,31,31,71,31,87,31,71,31,87,32,71,34,87,34,71,34,87,32,71,31,71,31,87,31,71,34,87,34,71,36,71,37,87,32,71,32,87,34,71,34,87,34,71,34,87,36,71,37,87,34,71,34,87,34,71,37,87,37,71,34,87,36,71,37,87,37,71,37,87,36,71,37,87,37,71,36,87,36,71,50,87,50,71,37,23,36,24,50,87,36,87,36,71,40,87,40,71,50,87,37,87,50,71,50,87,50,71,50,87,43,71,43,87,74,71,76,25,76,26,74,27,74,28,73,29,73,30,74,31,74,71,76,87,76,71,76,87,77,71,76,87,74,71,74,87,73,71,73,71,74,87,74,71,77,87,74,71,74,71,74,87,69,71,73,87,73,71,74,87,77,71,74,87,74,71,74,87,61,71,68,87,73,71,71,87,76,71,76,87,73,71,73,87,52,71,69,87,73,71,71,87,76,71,74,87,67,71,67,87,52,71,69,23,73,24,71,87,74,87,73,71,67,87,67,71,61,87,69,87,69,71,69,87,70,71,69,87,61,71,61,71,77,40,77,41,77,42,77,43,77,44,77,45,76,46,74,47,77,87,77,71,77,87,77,71,77,87,76,71,76,87,73,71,77,87,77,71,77,87,77,71,76,87,74,71,73,87,73,71,77,87,77,71,77,87,76,71,76,87,73,71,71,87,69,71,77,87,76,71,76,87,74,71,73,87,71,71,67,87,61,71,74,87,73,71,71,87,73,71,73,87,68,71,57,87,52,71,71,87,73,71,68,87,68,71,68,71,67,87,57,71,52,87,69,71,61,71,52,87,52,71,52,87,52,71,43,87,52,71,64,40,56,41,62,42,54,43,49,44,55,45,72,46,72,47,53,87,53,71,54,87,54,71,55,87,72,71,72,87,24,71,72,87,52,71,48,87,59,71,72,87,70,71,24,87,25,71,70,87,70,71,72,87,72,71,70,87,23,71,24,87,24,71,23,87,34,71,70,87,70,71,34,87,28,71,24,87,25,71,30,87,30,71,34,87,34,71,28,87,28,71,25,87,26,71,26,87,28,71,29,87,32,71,26,71,26,87,25,71,28,87,27,71,31,71,33,87,30,71,29,87,31,71,25,87,28,87,0,71,0,87,0,71,0,87,0,71,0,87,0,71,0,87,0,71,0,87,0,71,0,87,0,71,0,87,0,71,0,87,151,71,22,87,156,71,28,87,156,71,28,87,63,71,127,87,0,71,0,87,0,71,0,87,0,71,0,87,63,71,127,87,236,71,110,87,60,71,62,87,60,71,62,87,255,71,253,87,0,71,0,87,0,71,0,87,0,94,0,93,252,71,254,87,46,71,66,87,143,71,62,87,192,71,14,87,239,71,191,48,0,71,0,87,13,71,32,87,0,71,14,87,109,71,191,87,126,71,145,87,166,71,127,87,95,71,255,87,255,71,237,87,32,71,0,87,2,71,51,87,31,71,255,87,255,71,254,87,112,71,239,87,159,71,255,87,254,71,188,87,99,71,143,87,0,71,103,87,159,71,254,87,240,71,224,87,128,71,0,87,170,71,248,87,127,71,199,87,59,71,31,87,47,71,193,87,160,71,248,87,255,71,63,87,15,94,3,93,1,71,0,87,112,71,134,87,203,71,255,87,255,71,229,87,79,71,248,48,0,71,2,87,139,71,255,87,255,71,255,87,240,71,0,71,10,87,108,71,254,87,255,71,103,87,255,71,131,48,3,71,0,64,76,65,246,66,255,67,255,87,7,71,0,87,0,71,112,87,142,71,91,87,255,71,255,87,245,71,222,87,199,48,64,87,8,71,25,87,255,5,255,6,255,71,225,19,1,20,112,30,134,31,76,87,229,71,239,87,212,71,248,87,253,71,0,94,0,75,64,76,101,76,226,110,208,109,240,76,253,75,121,76,161,93,67,87,195,71,123,87,147,7,103,8,31,71,0,87,0,71,0,87,0,71,0,87,0,71,0,87,0,71,0,87,0,71,0,87,0,71,0,87,0,71,10,48,19,71,0,64,0,65,0,66,0,67,0,87,0,71,10,87,18,71,4,87,0,71,0,87,16,71,48,87,40,71,56,87,176,48,7,87,4,71,8,87,12,5,5,6,0,71,1,19,3,20,0,30,184,31,68,87,71,71,131,87,3,71,3,87,3,71,128,94,192,75,124,76,126,76,239,110,239,109,239,76,239,75,0,76,2,93,40,87,252,71,252,87,143,7,195,8,251,71,0,87,2,71,3,87,7,71,1,87,128,71,192,87,224,93,0,71,0,87,96,71,96,87,96,71,24,87,60,71,30,87,0,80,0,81,224,82,224,83,224,71,0,87,0,71,0,87,0,71,0,87,0,71,16,87,16,71,16,87,24,71,8,87,0,68,0,69,0,70,48,87,48,71,112,87,120,44,120,45,10,46,87,47,163,71,255,87,251,71,63,87,223,71,127,94,2,110,23,91,3,92,71,92,11,126,31,125,223,92,127,91,255,92,254,109,247,93,238,87,112,71,65,23,131,24,7,87,127,71,255,94,248,75,240,76,128,75,128,93,128,94,128,93,255,71,192,87,33,71,63,87,116,71,243,87,255,71,127,87,128,80,0,81,0,82,0,83,0,71,1,87,3,71,1,87,16,71,40,87,243,71,127,87,192,71,192,87,128,71,0,87,0,68,0,69,3,70,15,87,255,71,255,87,255,44,255,45,60,46,15,47,49,71,241,87,56,71,20,87,3,71,0,94,0,110,0,91,0,92,224,92,240,126,244,125,255,92,255,91,225,92,129,109,223,93,123,87,131,71,112,23,192,24,0,87,0,71,0,94,0,75,0,76,3,75,127,93,255,94,255,109,126,75,193,76,113,76,2,76,236,93,30,71,7,87,0,71,0,96,0,97,0,98,224,99,224,87,222,71,255,87,255,71,3,1,225,2,240,87,68,71,3,68,2,69,137,70,227,71,0,84,0,85,0,86,0,71,0,87,0,71,128,87,224,71,188,87,190,71,255,87,55,71,221,94,134,75,3,76,211,110,252,126,122,71,127,87,63,71,3,87,1,71,1,87,0,71,254,87,53,125,196,109,224,76,249,93,246,71,251,87,126,71,192,94,36,110,192,91,192,92,240,91,246,109,251,110,254,109,18,75,7,76,71,76,67,76,66,93,108,71,82,87,216,71,18,96,63,97,63,98,22,99,0,87,0,71,0,87,128,71,96,1,192,2,129,87,1,71,2,68,5,69,3,70,7,71,7,84,7,85,6,86,6,71,4,87,0,71,0,87,0,71,33,87,65,71,184,87,108,71,214,94,195,75,131,76,3,110,207,126,143,71,3,87,1,71,0,87,0,71,0,87,0,71,122,87,146,125,191,109,111,76,95,93,63,71,183,87,151,71,112,94,144,110,148,91,182,92,166,91,199,109,79,110,111,125,247,91,207,92,222,92,191,92,255,109,127,93,254,71,126,87,240,112,224,113,192,114,128,115,128,71,0,87,0,71,0,87,127,41,144,47,167,71,103,87,91,84,63,85,95,86,63,87,3,71,0,87,3,71,39,87,11,71,63,87,95,71,63,87,251,94,247,76,220,76,249,76,180,110,96,91,211,92,211,126,255,71,248,87,224,71,224,87,192,71,128,87,128,5,0,6,156,71,184,87,63,125,111,92,76,109,127,75,254,76,142,75,0,110,0,126,0,71,0,87,3,71,3,125,7,126,15,125,56,91,224,92,96,92,140,92,8,109,14,93,0,71,0,87,63,112,255,113,127,114,51,115,247,71,241,87,255,71,255,87,112,41,134,47,65,71,2,87,108,84,73,85,122,86,40,87,0,71,0,87,9,71,0,87,67,71,107,87,7,71,133,87,10,94,65,76,172,76,192,76,105,110,252,91,137,92,130,126,3,71,7,87,5,71,127,87,95,71,127,87,127,5,255,6,112,71,134,87,78,125,2,92,36,109,163,75,146,76,128,75,0,110,1,126,2,71,35,87,10,71,220,125,247,126,255,71,10,87,64,71,162,87,27,71,119,125,114,109,110,93,207,71,0,87,0,71,0,87,160,71,192,87,196,71,248,87,254,71,55,87,0,71,0,87,4,71,40,87,120,71,240,48,0,71,192,87,248,71,248,87,248,94,215,76,135,75,15,76,255,76,217,110,8,92,6,92,22,92,14,126,66,71,39,87,59,71,0,87,0,71,0,87,0,71,0,20,128,20,192,21,192,22,191,87,247,71,127,87,111,87,47,125,159,91,26,92,131,91,254,126,31,71,7,87,7,71,3,87,3,71,1,87,0,71,48,87,198,71,204,87,161,71,249,125,252,109,250,93,92,71,0,87,196,71,192,87,160,71,248,87,248,71,250,87,252,71,128,87,128,71,240,87,64,71,0,87,0,71,23,48,62,71,127,87,127,71,15,87,191,94,255,76,255,75,232,76,192,76,47,110,47,92,30,92,109,92,126,126,60,71,31,87,11,71,208,87,208,71,224,87,144,71,128,20,192,20,224,21,244,22,112,87,208,71,160,87,224,87,192,125,216,91,236,92,215,91,15,126,47,71,95,87,31,71,63,87,39,71,19,87,40,87,112,71,134,87,76,71,1,87,47,71,47,125,119,109,175,93,0,71,0,87,4,71,0,87,35,71,11,87,23,71,15,87,242,71,238,87,254,71,236,87,251,71,249,87,217,71,179,87,254,71,120,87,240,94,240,110,240,92,240,91,224,92,192,92,135,126,15,87,255,71,127,87,222,71,189,87,113,17,49,18,0,19,0,20,0,21,0,35,1,35,3,36,13,37,3,38,4,71,96,87,192,71,4,87,15,71,59,87,33,71,32,87,15,71,31,87,63,71,251,87,240,71,196,87,222,71,223,87,47,71,83,87,131,71,62,87,208,71,1,125,210,109,0,93,7,71,19,87,0,71,2,87,16,71,0,87,0,71,0,87,122,71,144,87,161,71,98,87,93,71,76,87,109,71,142,87,31,71,39,87,55,94,23,110,111,92,127,91,255,92,191,92,12,126,137,87,192,71,168,87,189,71,139,87,9,17,206,18,255,19,255,20,255,21,255,35,255,35,255,36,255,37,255,38,240,71,132,87,12,71,190,87,64,71,1,87,210,71,0,87,192,71,194,87,240,71,255,87,252,71,243,87,255,71,255,87,10,71,65,87,32,71,64,87,91,71,66,87,105,125,206,109,1,93,3,87,187,71,255,87,223,71,255,94,255,93,255,87,216,94,100,93,143,71,175,87,26,71,25,87,18,71,158,87,0,71,0,94,96,110,64,126,225,71,224,87,96,71,96,87,93,71,99,87,24,71,14,87,0,71,130,87,225,33,192,34,192,35,224,36,248,34,254,51,254,51,127,52,31,53,63,54,129,55,226,87,108,71,23,87,2,71,7,87,71,71,159,87,0,48,0,87,0,71,0,87,0,71,0,87,128,71,224,87,238,71,63,87,63,71,55,87,23,71,27,87,159,125,230,109,60,93,31,87,15,71,15,87,15,71,15,94,7,93,3,87,93,94,63,93,222,71,29,87,96,71,112,87,120,71,44,87,160,71,192,94,33,110,226,126,159,71,143,87,135,71,211,87,87,71,2,87,15,71,39,87,70,71,250,87,63,33,62,34,168,35,252,36,240,34,216,51,184,51,4,52,192,53,192,54,107,55,115,87,169,71,184,87,112,71,248,87,98,71,96,87,20,48,12,87,22,71,7,87,15,71,7,87,29,71,31,71,30,87,91,87,191,71,191,20,222,21,122,22,116,87,123,125,31,109,27,75,62,76,62,75,92,76,124,110,120,109,120,76,99,110,199,109,7,76,111,76,26,76,12,75,157,76,155,75,128,76,0,110,0,126,0,71,0,87,0,71,0,87,0,5,98,6,216,71,176,1,240,2,228,71,232,87,144,5,49,10,15,11,7,12,15,50,7,10,15,11,31,12,127,13,126,71,10,87,64,1,162,2,191,71,227,5,48,6,110,87,27,71,255,87,127,68,95,69,119,70,59,87,31,71,11,87,79,71,124,87,144,87,134,71,67,20,95,21,2,22,110,87,155,125,3,109,3,75,35,76,3,75,7,76,95,110,31,109,95,76,16,110,24,109,15,76,1,76,16,76,224,75,64,76,128,75,239,76,231,110,240,126,254,71,239,87,31,71,191,87,127,5,8,6,128,71,16,1,160,2,96,71,96,87,64,5,192,10,247,11,127,12,239,50,95,10,159,11,159,12,191,13,63,71,16,87,16,1,0,2,0,71,0,5,0,6,1,87,34,71,239,87,239,68,255,69,255,70,255,87,255,71,254,87,221,87,96,87,0,20,64,30,224,35,208,36,127,38,2,71,0,87,159,125,255,91,191,92,31,91,47,92,128,126,253,125,255,92,46,126,88,125,163,92,62,92,196,92,1,91,150,92,4,91,255,92,255,126,255,71,255,87,255,71,255,87,255,71,255,87,126,47,209,87,164,17,64,18,217,19,64,20,105,21,206,26,255,27,255,28,255,29,255,26,255,27,255,28,255,29,255,55,95,71,49,87,56,46,15,87,3,71,197,46,15,71,0,87,224,71,240,84,248,85,248,86,252,71,62,87,255,71,240,87,243,87,249,20,24,30,216,35,124,36,156,38,124,71,238,87,0,125,0,91,0,92,0,91,0,92,0,126,0,125,192,92,8,126,130,125,23,92,163,92,103,92,99,91,71,92,7,91,247,92,125,126,232,71,92,87,152,71,156,87,184,71,248,87,16,47,16,87,12,17,31,18,95,19,127,20,231,21,151,26,239,27,239,28,243,29,224,26,160,27,128,28,0,29,0,55,32,71,0,87,96,46,110,87,240,71,255,46,231,71,249,87,223,71,255,84,159,85,145,86,15,71,0,87,0,71,0,71,250,87,236,46,244,40,126,50,255,36,127,52,125,55,251,1,240,2,120,71,120,87,124,5,188,6,60,71,30,87,190,71,62,68,188,69,115,70,36,71,48,87,48,55,24,1,158,2,0,71,1,87,0,5,0,6,0,71,0,71,0,87,0,71,3,87,33,71,80,33,48,34,112,35,92,36,110,37,242,34,220,35,30,38,63,35,63,36,127,37,103,54,135,54,3,55,126,7,145,8,180,87,64,71,25,87,64,71,105,87,206,71,15,87,63,71,15,87,31,71,95,87,63,71,127,87,255,71,30,87,65,46,164,40,176,50,209,36,48,52,111,55,27,1,31,2,15,71,31,87,15,5,39,6,3,71,1,87,0,71,0,68,3,69,0,70,0,71,1,87,64,55,32,1,16,2,255,71,252,87,255,5,255,6,254,71,191,71,223,87,239,71,128,87,192,71,0,33,64,34,64,35,128,36,128,37,160,34,127,35,63,38,255,35,191,36,191,37,127,54,127,54,95,55,52,7,48,8,20,87,28,71,15,87,34,71,96,87,96,71,203,87,207,71,235,87,227,71,240,87,221,71,159,87,159,87,0,71,0,87,0,17,0,11,224,54,64,46,1,71,1,17,255,18,255,19,255,20,255,21,31,22,191,87,254,71,254,87,80,84,132,85,78,86,21,87,45,71,36,71,104,17,169,18,255,19,255,20,255,21,255,22,255,87,255,87,255,71,255,87,10,71,64,87,166,49,155,50,247,35,54,11,110,33,27,50,255,51,255,52,255,51,255,52,255,53,255,14,255,71,255,87,24,23,22,24,15,71,29,87,9,71,0,64,8,65,14,66,240,67,248,87,248,71,254,87,248,71,240,87,240,71,240,87,38,71,23,87,3,17,195,11,123,54,147,46,103,71,31,17,0,18,0,19,0,20,0,21,0,22,0,87,0,71,0,87,160,84,160,85,232,86,224,87,240,71,240,71,192,17,240,18,95,19,95,20,23,21,31,22,15,87,15,87,63,71,15,87,126,71,240,87,113,49,63,50,30,35,32,11,32,33,0,50,1,51,15,52,142,51,192,52,225,53,223,14,223,71,255,87,190,23,47,24,3,71,208,87,224,71,0,64,0,65,0,66,64,67,208,87,252,71,47,87,31,71,255,87,255,71,255,71,27,87,126,71,254,33,252,27,244,31,124,71,126,32,253,33,28,34,60,35,124,36,120,37,120,38,120,39,124,87,255,71,79,87,3,71,69,7,1,8,148,70,134,32,0,33,166,34,0,35,0,36,0,37,0,38,0,39,0,71,0,87,0,71,228,87,254,71,190,9,191,11,121,50,195,50,192,51,41,10,3,11,1,12,1,35,1,12,1,13,3,30,0,31,0,71,0,87,0,71,0,87,128,71,224,87,64,80,68,81,46,82,255,83,255,71,255,87,255,71,255,87,255,71,127,87,71,71,119,87,133,71,91,33,1,27,13,31,36,71,120,32,169,33,7,34,5,35,19,36,1,37,4,38,0,39,0,87,0,71,48,87,0,71,32,7,0,8,0,70,0,32,128,33,224,34,207,35,255,36,223,37,255,38,255,39,255,71,127,87,31,71,192,87,192,71,240,9,48,11,0,50,0,50,16,51,24,10,63,11,63,12,15,35,207,12,255,13,255,30,239,31,231,71,32,87,32,71,48,87,124,71,97,87,63,80,30,81,0,82,223,83,223,71,207,87,131,71,158,87,192,71,225,87,255,71,0,1,0,2,0,49,0,43,128,87,24,87,108,71,71,49,255,50,255,51,255,52,255,53,127,54,231,55,147,71,184,87,15,71,67,87,166,23,186,24,244,86,50,71,110,49,27,50,7,51,3,52,36,53,2,54,0,55,0,87,0,71,0,87,193,71,67,87,2,25,0,34,1,35,57,50,57,37,123,26,63,27,191,28,255,51,255,28,254,29,254,46,254,47,248,5,175,6,249,87,184,71,157,87,143,71,11,96,31,97,63,98,208,99,192,87,128,71,128,1,0,2,0,87,0,71,0,71,126,1,124,2,181,49,225,43,240,87,192,87,208,71,162,49,0,50,0,51,0,52,0,53,0,54,0,55,0,71,0,87,254,71,53,87,196,23,224,24,249,86,246,71,251,49,126,50,192,51,36,52,192,53,192,54,240,55,246,87,251,71,254,87,39,71,39,87,7,25,111,34,127,35,185,50,247,37,175,26,216,27,216,28,248,51,144,28,128,29,64,46,0,47,0,5,240,6,112,87,64,71,248,87,64,71,216,96,236,97,199,98,15,99,15,87,63,71,7,1,63,2,39,87,19,71,56,40,63,41,255,47,230,9,95,47,127,71,95,87,111,87,203,9,63,10,255,11,103,12,31,13,47,14,31,71,15,71,3,1,229,2,198,87,229,71,70,87,200,71,216,87,236,9,96,10,0,11,0,12,128,13,128,14,128,71,128,87,128,71,128,87,142,71,78,40,199,41,39,50,3,51,225,52,129,53,125,51,0,52,0,53,0,43,0,44,0,45,0,87,0,71,0,17,90,47,250,87,254,71,252,87,186,71,230,112,142,113,207,114,7,115,7,87,7,40,7,41,13,47,1,87,1,71,1,40,0,41,4,47,80,9,96,47,56,71,128,87,248,87,222,9,255,10,251,11,175,12,159,13,199,14,255,71,255,71,255,1,96,2,127,87,56,71,0,87,0,71,0,87,64,9,0,10,159,11,128,12,199,13,255,14,255,71,255,87,191,71,255,87,0,71,0,40,0,41,0,50,96,51,112,52,120,53,44,51,255,52,255,53,255,43,255,44,159,45,143,87,135,71,211,17,80,47,0,87,0,71,32,87,66,71,227,112,57,113,60,114,175,115,255,87,255,40,223,41,189,47,28,87,198,71,195,71,3,87,3,71,1,87,24,71,16,87,224,71,226,71,224,25,252,26,252,27,254,28,231,29,239,30,31,31,29,40,31,41,208,47,224,71,97,87,1,71,1,87,3,71,3,25,63,26,47,27,31,28,159,29,255,30,255,31,255,71,255,87,255,71,157,87,140,71,252,9,191,10,159,53,95,12,15,13,199,35,252,12,244,13,192,14,160,71,128,87,192,1,192,2,128,71,243,87,244,5,249,6,249,71,242,87,208,71,198,87,224,71,0,87,0,71,0,87,0,46,0,87,0,71,0,87,0,71,1,87,3,71,6,87,37,71,21,87,223,71,7,71,15,25,0,26,1,27,1,28,3,29,3,30,3,31,3,40,3,41,46,47,66,71,131,87,254,71,192,87,1,71,210,25,192,26,0,27,0,28,0,29,192,30,0,31,0,71,0,87,192,71,159,87,250,71,252,9,224,10,66,53,227,12,57,13,60,35,0,12,5,13,3,14,31,71,189,87,28,1,198,2,195,71,3,87,3,5,1,6,24,71,16,87,224,71,226,87,192,71,252,87,252,71,254,87,231,46,239,87,31,71,29,87,63,87,127,68,159,69,167,70,77,87,86,71,65,87,105,40,206,41,7,42,15,43,7,44,13,45,6,46,1,47,0,64,0,65,249,66,190,67,223,5,252,6,255,71,255,40,255,41,15,42,128,43,192,44,240,45,255,46,127,47,255,87,63,71,15,87,129,71,1,87,205,25,192,26,208,51,96,28,191,29,207,51,0,28,0,29,0,30,128,31,128,71,128,17,192,18,252,19,254,20,190,21,252,22,176,87,194,71,31,19,64,20,66,30,0,31,0,87,0,71,0,87,0,71,0,87,0,71,0,87,99,68,139,69,151,70,36,87,12,71,41,87,157,40,249,41,99,42,1,43,3,44,0,45,0,46,0,47,0,64,0,65,4,66,129,67,133,5,91,6,232,71,225,40,233,41,245,42,251,43,254,44,250,45,60,46,15,47,7,87,3,71,3,87,0,71,68,87,159,25,31,26,26,51,181,28,232,29,217,51,255,28,191,29,127,30,255,31,254,71,252,17,224,18,193,19,0,20,28,21,130,22,144,87,158,71,40,19,114,20,190,30,255,31,227,87,253,71,255,87,255,71,31,87,77,71,129,71,0,84,14,85,125,86,124,71,252,87,248,71,136,19,74,20,255,30,254,31,252,87,252,71,252,87,248,71,128,80,64,81,253,82,252,83,129,87,194,71,113,87,249,19,91,20,71,30,253,31,252,87,1,71,0,87,0,71,0,1,0,2,0,71,227,87,3,40,31,41,143,42,15,43,255,44,254,45,255,43,128,44,0,45,0,46,128,47,0,32,0,33,0,34,0,35,128,36,192,37,131,38,151,39,211,87,14,44,185,45,255,46,0,47,0,71,0,87,1,71,1,87,3,71,15,87,63,71,11,84,119,85,158,86,254,71,249,87,192,71,233,19,78,20,7,30,15,31,254,87,252,71,248,87,192,71,192,80,0,81,223,82,240,83,238,87,187,71,247,87,50,19,110,20,27,30,219,31,240,87,204,71,128,87,16,71,0,1,0,2,0,71,31,87,30,40,30,41,13,42,30,43,121,44,225,45,25,43,224,44,224,45,224,46,240,47,224,32,128,33,0,34,0,35,112,36,208,37,224,38,96,39,126,87,63,44,119,45,24,46,15,47,47,71,31,87,31,71,1,87,0,71,0,87,0,87,213,71,52,87,102,71,13,87,159,71,252,87,247,44,188,45,0,46,0,47,0,71,0,87,4,71,224,87,224,96,248,97,90,98,52,99,141,71,155,48,98,71,191,44,167,45,173,46,0,47,0,71,0,87,0,71,0,87,1,41,7,47,175,87,253,71,255,87,191,71,223,87,47,59,43,60,117,87,175,59,255,60,255,87,191,71,223,87,15,71,43,49,5,50,7,51,224,52,232,53,246,54,112,55,188,71,254,87,159,71,251,87,0,71,128,87,128,71,128,87,192,71,192,87,240,71,255,87,30,71,7,87,41,71,29,87,8,71,99,87,198,44,249,45,0,46,0,47,0,71,0,87,0,71,0,87,0,96,0,97,212,98,166,99,199,71,151,48,63,71,9,44,78,45,35,46,3,47,1,71,0,87,0,71,0,87,0,41,0,47,0,87,204,71,148,87,14,71,143,87,175,59,150,60,85,87,152,59,192,60,128,87,0,71,0,87,0,71,0,49,0,50,0,51,126,52,118,53,230,54,73,55,201,71,115,87,30,71,52,87,0,71,0,87,0,71,0,87,0,71,0,87,0,71,0,71,76,87,167,71,163,87,147,71,255,87,157,71,40,87,176,71,0,87,0,71,0,87,0,48,0,87,0,71,0,112,0,113,253,114,217,115,192,87,136,71,149,87,14,71,69,87,113,71,0,87,0,71,0,87,0,71,0,87,0,71,0,87,0,71,103,87,134,71,14,87,15,71,79,87,251,71,231,87,223,68,0,69,1,70,1,87,3,71,3,87,7,9,63,10,127,11,255,12,248,13,226,14,251,71,223,87,210,71,238,87,27,71,255,87,248,71,224,87,240,71,204,87,192,71,224,87,0,71,112,87,221,71,244,87,255,71,190,87,122,71,52,87,123,71,192,87,225,71,248,87,250,48,60,87,124,71,56,112,120,113,48,114,134,115,76,87,1,71,45,87,36,71,120,87,169,71,0,87,0,71,0,87,0,71,0,87,0,71,0,87,0,71,120,87,254,71,119,87,62,71,30,87,46,71,45,87,6,68,0,69,0,70,136,87,192,71,224,87,208,9,208,10,248,11,120,12,175,13,163,14,112,71,248,87,96,71,96,87,96,71,0,87,16,71,28,87,15,71,7,87,31,71,31,87,31,87,254,71,239,87,47,71,31,87,150,71,172,87,131,71,143,87,60,71,63,87,31,71,6,87,0,71,0,87,0,71,0,87,125,71,222,87,250,71,254,87,108,71,228,87,128,71,3,87,46,71,158,87,252,71,252,87,240,71,0,87,0,71,0,87,2,71,36,87,116,71,254,87,199,71,251,87,189,71,139,84,0,85,0,86,0,71,64,87,220,71,252,25,62,26,14,27,126,28,95,29,191,30,87,31,95,71,87,87,107,71,207,87,127,71,95,87,63,71,23,87,7,71,23,87,65,71,7,87,225,71,116,87,112,71,176,87,191,71,223,87,163,71,250,87,128,71,128,87,192,71,192,87,192,71,224,87,248,71,254,87,128,71,3,87,19,71,32,87,28,71,164,87,226,71,240,87,0,71,0,87,0,71,0,87,0,71,0,87,0,71,128,87,248,71,115,87,22,71,209,87,16,71,96,87,192,71,31,84,0,85,0,86,0,71,0,87,0,71,0,25,0,26,0,27,41,28,82,29,8,30,128,31,56,71,44,87,36,71,137,87,0,71,0,87,0,71,0,87,0,71,0,87,0,71,0,71,162,87,29,71,168,87,32,71,41,87,83,71,67,87,111,71,0,87,0,71,0,87,0,71,0,87,0,71,0,87,0,71,193,87,17,71,131,87,3,71,239,87,191,71,217,87,127,71,0,87,0,71,0,87,0,71,0,87,1,71,63,87,255,71,159,87,190,71,243,87,190,71,184,87,105,71,242,87,192,87,127,71,126,48,112,87,126,71,240,40,232,41,240,42,192,43,187,44,94,45,124,46,63,47,46,87,61,71,127,87,189,71,120,87,60,71,60,87,30,71,30,87,29,71,62,87,253,71,65,87,1,71,8,87,52,71,29,87,254,71,127,87,255,71,254,87,254,71,247,87,203,71,250,87,57,71,255,87,127,71,194,87,200,71,192,87,192,71,12,87,14,71,255,87,255,71,127,87,247,71,255,87,255,71,255,87,255,71,255,87,255,71,46,87,66,71,131,87,62,71,192,87,1,71,210,87,0,87,0,71,0,48,0,87,0,71,0,40,0,41,0,42,0,43,126,44,145,45,164,46,64,47,89,87,64,71,105,87,206,71,0,87,0,71,0,87,0,71,0,87,0,71,0,87,0,87,193,71,193,87,109,71,240,87,204,71,254,87,127,71,155,87,128,68,128,69,128,70,192,87,240,71,248,87,124,71,156,87,254,87,190,1,252,2,176,71,194,87,31,5,64,6,66,71,0,71,0,87,0,71,0,87,0,68,0,69,0,70,0,87,224,71,252,87,129,71,194,87,113,71,249,87,91,71,71,87,224,71,252,71,1,71,0,87,0,71,0,87,0,71,0,87,247,71,17,87,24,71,184,87,124,71,248,87,224,71,255,87,128,71,0,87,0,71,128,87,0,71,0,87,0,71,0,87,191,71,127,87,79,71,5,87,61,71,44,87,120,71,169,87,191,68,127,69,7,70,5,87,16,71,8,87,0,71,0,87,127,87,223,1,245,2,254,71,175,87,63,5,251,6,31,71,192,71,224,87,254,71,255,87,175,68,55,69,155,70,5,87,31,71,247,87,238,71,31,87,223,71,188,87,250,71,105,87,0,71,15,71,31,71,255,87,255,71,252,87,242,71,64,87,224,71,248,87,127,71,207,87,252,71,253,87,127,71,31,87,0,71,128,87,128,71,240,87,255,71,255,87,63,71,7,71,190,87,61,71,231,87,223,71,63,87,244,71,233,87,205,71,1,84,3,85,31,86,63,71,253,87,244,71,225,87,192,71,223,71,240,17,238,18,187,19,247,20,50,21,110,22,27,87,219,71,240,71,204,87,128,71,16,84,0,85,0,86,0,71,191,87,127,71,79,87,5,71,61,87,44,71,120,87,169,71,191,87,127,71,7,87,5,71,16,87,8,71,0,87,0,71,126,87,221,71,247,87,255,71,175,94,59,93,255,94,30,75,193,76,231,93,255,87,255,71,175,87,50,71,152,87,1,71,127,87,255,71,127,87,191,71,103,87,49,71,192,87,112,71,255,84,127,85,255,86,127,71,191,87,238,71,127,87,207,71,255,71,255,17,255,18,254,19,252,20,133,21,54,22,2,87,255,71,255,71,255,87,255,71,255,84,127,85,203,86,255,71,112,87,134,71,76,87,1,71,45,87,36,71,120,87,169,71,0,87,0,71,0,87,0,71,0,87,0,71,0,87,0,71,10,87,64,71,162,87,187,71,247,94,50,93,110,94,27,75,0,76,0,93,0,87,0,71,0,87,0,71,0,87,0,87,255,71,63,87,63,71,95,87,47,71,47,87,47,48,31,87,128,87,128,71,192,48,192,87,224,71,224,87,224,71,224,87,0,32,0,33,4,34,1,35,204,36,181,37,51,38,167,39,255,71,255,87,251,71,255,87,60,71,112,87,228,71,64,87,0,1,16,2,56,71,231,87,227,5,138,6,197,71,205,87,255,71,255,87,239,71,159,87,28,71,4,87,2,71,48,87,0,94,0,93,11,94,31,76,245,110,232,109,242,110,183,91,255,92,255,109,255,93,245,87,232,71,48,87,0,71,64,87,7,71,15,87,127,71,113,87,252,71,228,87,122,48,26,87,248,87,240,71,128,48,142,87,131,71,123,87,13,71,13,87,211,32,131,33,51,34,3,35,71,36,191,37,255,38,255,39,12,71,60,87,78,71,126,87,62,71,56,87,0,71,0,87,191,1,47,2,3,71,253,87,115,5,57,6,16,71,0,87,252,71,214,87,254,71,3,87,141,71,199,87,239,71,255,87,255,94,63,93,127,94,255,76,253,110,232,109,251,110,253,91,0,92,192,109,192,93,64,87,0,71,144,87,6,71,131,71,25,87,150,71,144,87,208,68,15,69,40,70,208,87,32,71,230,87,105,71,111,87,47,71,255,87,223,71,56,87,224,71,208,71,160,49,32,50,96,51,192,52,0,53,0,54,0,55,172,71,88,71,216,48,176,71,240,87,224,71,0,87,0,71,254,17,220,18,249,19,120,20,152,21,187,22,251,87,51,71,0,87,32,94,0,75,3,93,51,87,32,71,0,87,224,94,126,110,224,109,192,110,208,92,172,126,184,125,56,126,0,71,31,87,63,125,127,109,239,93,211,87,199,71,199,87,255,71,161,87,66,71,145,87,41,68,142,69,28,70,163,87,143,71,0,87,1,71,0,87,0,71,0,87,0,71,0,87,0,71,250,71,252,49,127,50,199,51,59,52,31,53,47,54,193,55,248,71,244,71,255,48,63,71,15,87,3,71,1,87,0,71,193,17,17,18,131,19,2,20,205,21,15,22,191,87,39,71,0,87,0,94,0,75,1,93,3,87,3,71,7,87,31,94,207,110,110,109,255,110,253,92,237,126,250,125,252,126,255,71,48,87,16,125,1,109,0,93,1,87,1,71,0,87,0,87,0,71,10,87,14,71,28,84,63,85,65,86,3,71,30,87,255,71,245,87,241,71,227,71,193,87,191,87,254,71,248,87,99,87,227,9,223,10,223,11,239,12,143,13,255,14,143,71,206,71,206,87,128,71,128,87,144,71,56,87,48,71,0,32,200,33,255,34,255,35,239,36,211,37,239,38,223,39,235,87,57,94,48,92,0,91,0,109,0,76,0,93,0,94,0,110,125,126,253,125,253,126,157,87,239,71,127,87,255,71,191,87,128,71,0,87,0,125,0,109,0,76,0,93,0,71,0,87,232,71,148,87,227,71,249,84,173,85,236,86,250,71,250,87,39,71,115,87,0,71,0,71,88,87,24,87,0,71,0,87,63,87,155,9,66,10,98,11,248,12,144,13,96,14,8,71,192,71,100,87,189,71,157,87,7,71,111,87,31,71,7,32,255,33,159,34,79,35,79,36,191,37,247,38,255,39,255,87,0,94,0,92,48,91,56,109,24,76,8,93,0,94,0,110,119,126,247,125,90,126,222,87,87,71,203,87,112,71,56,87,192,71,64,87,176,125,56,109,172,76,55,93,143,71,199,76,255,93,199,94,10,93,77,71,254,87,229,71,99,87,3,71,0,87,0,71,56,87,56,87,33,71,90,87,156,71,252,87,254,71,40,25,231,26,240,27,24,28,199,29,225,30,16,31,1,71,208,71,0,87,0,71,224,87,56,71,30,87,239,71,118,49,204,50,154,51,58,52,230,53,158,54,60,55,0,71,193,87,131,71,5,87,5,125,25,92,97,109,195,110,255,126,0,87,13,71,10,87,27,71,57,87,57,71,41,87,99,71,255,87,243,71,247,87,231,125,231,92,247,109,247,76,255,76,24,93,228,94,244,93,243,71,240,87,248,71,248,87,252,71,231,87,251,71,251,87,252,87,255,71,255,87,255,71,255,87,46,71,115,25,23,26,167,27,121,28,255,29,255,30,64,31,255,71,255,71,255,87,127,71,255,87,127,71,63,87,191,71,111,49,63,50,47,51,126,52,255,53,254,54,190,55,223,71,31,87,30,71,30,87,62,125,61,92,60,109,120,110,125,126,253,87,248,71,250,87,246,71,228,87,140,71,48,87,0,71,254,87,255,71,253,87,249,125,251,92,243,109,207,76,255,92,61,109,51,110,28,109,157,76,159,93,62,94,27,93,73,87,224,71,224,87,227,71,115,87,115,19,224,20,224,30,176,31,0,40,0,41,0,42,200,43,54,44,1,45,0,46,0,47,0,71,0,87,0,7,0,8,200,71,254,87,255,71,255,87,255,9,191,10,175,11,247,12,246,13,253,14,250,71,255,87,0,71,0,87,0,71,0,87,0,71,0,125,0,126,0,87,127,71,127,87,171,71,231,87,223,71,191,87,247,71,255,87,0,71,0,87,0,71,0,87,0,71,0,125,0,92,0,92,232,109,216,110,24,109,24,76,12,93,12,94,12,93,4,87,240,71,244,87,244,71,244,87,250,19,250,20,250,30,250,31,140,40,136,41,88,42,40,43,184,44,44,45,76,46,144,47,3,71,63,87,55,7,7,8,7,71,227,87,195,71,15,87,255,9,255,10,253,11,251,12,228,13,201,14,215,71,215,87,0,71,0,87,0,71,0,87,3,71,6,125,8,126,8,87,255,71,255,87,255,71,199,87,3,71,163,87,131,71,7,87,0,71,0,87,0,71,56,87,252,71,92,125,126,92,254,71,0,125,0,126,0,125,12,92,1,109,0,110,6,109,5,75,255,76,255,75,255,93,243,71,254,44,255,45,249,46,251,47,0,87,0,71,15,87,126,71,33,87,239,71,239,87,223,71,255,71,255,71,243,23,191,24,255,87,255,71,255,87,255,71,0,25,0,26,232,27,124,28,247,29,250,30,254,31,255,71,255,87,255,71,215,87,227,71,248,87,253,71,255,87,255,71,0,87,0,71,0,87,0,71,0,87,248,64,24,65,128,66,255,67,255,71,255,87,255,71,255,87,7,71,231,87,255,71,23,125,47,126,47,125,31,92,95,109,31,110,31,109,31,75,255,76,255,75,255,93,255,71,191,44,255,45,239,46,239,47,255,87,255,71,255,87,255,71,255,87,255,71,255,87,255,71,255,71,255,71,255,23,255,24,255,87,255,71,255,87,255,71,255,25,255,26,255,27,255,28,246,29,240,30,240,31,224,71,255,87,255,71,255,87,255,71,249,87,239,71,207,87,223,71,248,87,240,71,172,87,220,71,248,87,64,64,0,65,0,66,255,67,255,71,243,87,227,71,7,87,191,71,255,87,255,87,109,71,47,87,13,71,29,87,26,125,31,126,63,125,127,91,144,92,216,91,248,92,240,93,224,71,240,87,224,71,224,87,0,71,0,87,0,71,128,87,240,71,176,87,144,71,144,87,0,71,0,87,0,71,128,87,128,71,192,87,224,71,224,40,125,41,223,42,253,43,251,44,255,45,239,46,239,47,255,87,0,71,0,87,0,71,0,87,0,71,0,87,0,71,0,87,206,71,246,87,254,71,250,87,247,71,255,80,142,81,187,82,0,83,0,87,0,71,0,87,0,71,0,87,0,71,0,87,132,71,0,87,132,71,4,87,24,125,32,126,48,125,240,91,122,92,254,91,122,92,250,93,244,71,252,87,236,71,140,87,22,71,150,87,150,71,250,87,248,71,164,87,180,71,242,87,9,71,9,87,9,71,5,87,7,71,67,87,67,71,1,40,214,41,194,42,200,43,229,44,255,45,255,46,255,47,255,87,9,71,13,87,7,71,3,87,0,71,0,87,0,71,0,87,31,71,159,87,31,71,63,87,255,71,255,80,255,81,255,82,248,83,112,87,240,71,240,87,192,71,0,87,0,71,0,71,15,87,55,48,14,87,25,71,27,87,123,71,123,87,35,71,247,87,207,71,255,87,239,71,239,87,143,71,143,87,223,71,159,87,255,71,191,71,255,71,255,71,255,71,255,87,255,48,255,87,255,71,255,94,255,75,255,76,255,75,255,76,255,93,255,87,255,71,255,87,255,71,255,87,255,71,255,87,255,71,255,87,255,71,255,87,255,71,255,87,255,71,255,87,255,71,208,87,224,71,224,87,240,71,240,87,248,96,248,97,248,98,255,99,255,71,255,87,255,71,255,87,255,71,255,87,255,71,31,87,31,48,31,87,15,71,15,87,15,71,2,87,0,71,239,87,239,71,231,87,243,71,243,87,241,71,253,87,255,71,255,87,255,71,254,71,255,71,253,71,255,71,116,87,16,48,255,87,255,71,255,94,254,75,254,76,252,75,251,76,239,93,204,87,114,71,200,87,16,71,0,87,0,71,0,87,0,71,255,87,255,71,127,87,255,71,255,87,255,71,255,87,255,71,183,87,63,71,94,87,50,71,119,87,35,96,13,97,128,98,72,99,192,71,161,87,205,71,136,87,220,71,243,87,255,87,0,71,8,87,8,71,6,87,40,71,24,87,0,68,0,69,0,70,8,87,4,71,4,87,108,71,56,87,16,7,0,8,0,71,0,87,2,71,2,87,4,71,14,87,8,71,40,87,0,71,0,94,0,110,0,91,0,92,0,91,16,92,16,109,0,76,0,93,0,94,1,93,3,71,0,87,0,71,0,87,0,71,0,87,0,71,0,87,0,71,0,87,0,71,0,94,36,93,144,87,53,71,243,87,101,71,138,112,36,113,8,114,0,115,0,87,0,68,0,69,128,70,0,87,0,71,0,87,112,71,176,87,96,71,91,87,27,71,18,87,0,68,1,69,3,70,1,87,1,71,0,87,0,71,0,87,0,7,0,8,133,71,163,87,179,71,191,87,5,71,37,87,65,71,18,87,208,71,216,94,216,110,248,91,254,92,223,91,63,92,63,109,241,76,225,93,111,94,158,93,158,71,190,87,206,71,214,87,128,71,128,87,128,71,192,87,192,71,192,87,240,71,250,94,180,93,60,87,56,71,241,87,105,71,9,112,44,113,190,114,0,115,0,87,0,68,0,69,0,70,0,87,0,71,128,71,209,87,124,64,156,65,159,66,108,67,160,71,64,84,48,85,6,86,3,71,11,87,8,71,27,87,31,71,15,23,79,24,96,87,192,71,224,87,252,71,239,87,210,71,201,94,1,93,255,94,127,110,127,126,51,71,32,87,0,71,0,87,0,125,0,92,0,109,0,110,0,109,64,93,80,71,184,87,252,71,192,87,192,71,224,87,224,71,48,87,48,71,24,94,12,110,160,109,128,93,136,87,196,71,225,87,241,71,249,87,200,71,252,87,252,71,254,84,254,85,255,86,255,71,255,87,255,71,0,87,0,64,0,65,0,66,0,67,128,71,128,84,128,85,0,86,0,71,0,87,0,71,0,87,128,71,128,23,128,24,0,87,0,71,0,87,0,71,1,87,2,71,2,94,6,93,0,94,0,110,0,126,0,71,0,87,0,71,0,87,0,125,0,92,65,109,243,110,126,109,84,93,236,71,141,87,157,71,0,87,0,71,16,87,14,71,12,87,12,71,56,94,56,110,0,109,128,93,192,87,96,71,160,87,48,71,48,87,48,71,0,87,0,71,0,84,32,85,96,86,224,71,240,87,240,87,0,71,0,80,0,81,3,82,3,83,5,87,24,71,48,87,1,71,0,87,0,71,0,71,0,1,0,2,0,71,0,87,192,71,198,87,134,71,180,94,179,93,131,94,178,110,173,109,0,110,0,126,0,71,0,87,0,71,0,87,0,71,2,87,1,71,17,125,3,126,1,125,135,109,15,76,175,93,36,87,0,71,0,87,0,94,0,93,0,94,0,76,0,110,0,126,8,125,0,109,128,93,133,87,130,71,235,87,145,71,3,87,0,71,0,87,0,71,0,87,0,71,0,48,0,71,0,87,7,71,45,80,11,81,41,82,6,83,102,87,78,71,73,87,0,71,0,87,0,71,0,71,0,1,0,2,0,71,0,87,56,71,0,87,8,71,56,94,32,93,32,94,104,110,248,109,31,110,63,126,63,71,95,87,95,71,127,87,127,71,63,87,114,71,51,125,48,126,30,125,19,109,18,76,24,93,9,87,254,71,254,87,255,94,255,93,255,94,254,76,254,110,254,126,171,125,219,109,179,93,97,87,211,71,149,87,87,71,229,87,196,71,134,87,129,71,1,87,1,71,1,48,1,71,1,8,168,87,224,96,192,97,224,98,224,99,193,71,225,87,226,71,94,87,88,71,120,87,248,87,252,41,248,47,240,87,224,71,64,87,196,71,0,94,2,110,6,109,1,110,53,126,80,125,0,126,0,71,0,87,0,71,0,87,0,71,0,87,0,71,208,87,110,71,99,87,179,71,241,125,33,92,99,109,179,75,14,76,1,76,1,110,1,109,3,110,3,92,0,126,24,71,0,87,0,125,224,109,128,93,64,94,0,93,0,87,128,71,0,87,0,71,240,87,248,71,252,87,252,71,254,7,255,8,0,87,0,96,0,97,0,98,0,99,0,71,1,87,33,71,0,87,0,71,0,87,0,87,0,41,0,47,6,87,2,71,4,87,12,71,12,94,7,110,102,109,182,110,126,126,254,125,0,126,0,71,0,87,0,71,0,87,0,71,0,87,0,71,216,87,216,71,65,87,98,71,102,125,12,92,124,109,240,75,57,76,49,76,176,110,128,109,0,110,0,92,0,126,0,71,112,87,248,125,184,109,72,93,60,94,28,93,28,87,60,71,240,87,120,71,120,87,56,71,4,87,12,71,14,7,14,24,255,71,255,112,255,113,248,114,241,115,246,87,112,71,91,87,0,71,0,87,0,71,0,87,0,71,0,87,0,71,0,94,191,93,252,94,221,110,42,126,7,125,224,126,249,71,250,87,0,71,0,87,0,71,16,87,64,71,0,19,0,20,0,30,249,31,0,87,8,71,97,87,88,71,97,87,239,125,255,91,0,92,0,92,0,126,0,125,0,126,2,87,0,71,0,87,41,71,3,87,6,125,194,109,206,110,205,109,131,76,131,93,0,71,0,87,0,71,32,87,0,71,0,87,0,23,0,24,88,71,159,112,116,113,56,114,1,115,7,87,126,71,94,87,0,71,0,87,0,71,0,87,0,71,0,87,1,71,33,94,232,93,92,94,174,110,83,126,65,125,64,126,228,71,216,87,63,71,127,87,31,71,47,87,63,71,63,19,63,20,62,30,11,31,7,87,2,71,180,87,15,71,59,87,114,125,66,91,254,92,255,92,255,126,242,125,224,126,128,87,1,71,31,87,77,71,250,87,225,125,81,109,103,110,95,109,72,76,8,93,129,71,1,87,0,71,0,87,0,71,4,87,4,23,0,71,241,87,241,71,238,87,236,71,224,87,226,71,231,87,189,71,224,94,224,75,241,76,115,76,127,76,127,75,126,76,126,110,226,109,130,110,6,126,24,71,82,87,250,71,241,87,243,71,0,87,0,71,129,87,0,71,0,87,0,44,0,45,0,46,139,47,128,71,5,87,3,71,135,87,143,71,142,87,111,71,28,87,15,71,143,87,133,71,3,87,3,71,7,87,7,71,64,87,65,71,192,87,192,125,128,126,0,125,0,92,0,109,255,76,255,93,255,87,255,71,255,87,255,71,255,87,255,71,127,87,191,71,255,87,247,71,59,87,6,71,1,87,1,71,0,94,128,75,240,76,252,76,255,76,255,75,254,76,254,110,0,109,0,110,0,126,192,71,128,87,160,71,192,87,32,71,0,87,128,71,128,87,192,71,192,87,224,44,240,45,248,46,169,47,79,71,95,87,43,71,60,87,217,71,222,87,166,71,0,87,0,71,0,87,0,71,0,87,4,71,7,87,7,71,255,87,131,71,2,87,39,125,39,126,12,125,152,92,137,109,0,76,0,93,8,87,48,71,224,87,203,71,31,87,62,87,204,71,252,87,120,71,208,87,241,71,249,87,211,71,1,94,0,110,0,91,0,92,0,92,0,92,0,91,8,92,0,126,247,125,37,126,123,71,102,87,70,71,204,87,125,71,157,87,0,71,0,87,0,87,0,87,0,71,0,87,130,71,0,87,189,71,19,87,39,71,111,87,66,94,188,75,24,76,8,76,0,76,0,76,0,76,0,75,4,76,0,93,0,71,0,87,19,71,65,87,129,71,8,87,40,71,52,87,240,71,237,125,32,92,0,109,0,93,0,87,0,71,0,87,0,71,3,87,166,71,163,87,221,71,12,87,62,71,231,87,238,71,248,94,25,110,29,91,63,92,127,92,255,92,255,91,241,92,224,126,81,125,50,126,37,71,161,87,242,71,129,87,144,71,104,87,188,71,216,87,248,87,240,87,224,71,192,87,67,71,3,87,66,71,164,87,193,71,105,87,99,94,0,75,230,76,99,76,63,76,31,76,62,76,30,75,158,76,254,93,124,71,252,87,0,71,2,87,0,71,0,87,88,71,88,87,156,71,132,125,0,92,0,109,0,93,0,87,0,71,0,87,0,71,0,71,185,87,185,71,255,87,18,71,8,87,9,71,224,94,224,110,126,126,126,71,126,87,115,71,3,87,3,71,7,87,7,71,147,87,154,71,2,87,70,71,52,87,247,71,111,87,30,71,64,87,68,7,196,8,129,71,141,87,15,47,158,71,254,87,7,71,3,87,37,71,7,94,0,110,0,91,0,92,64,92,7,126,7,125,131,92,129,91,3,92,1,109,1,93,0,87,128,71,128,23,128,24,128,87,130,71,1,87,0,71,8,87,255,71,255,125,255,109,255,93,255,87,255,71,255,87,255,71,0,87,0,71,0,87,0,71,0,87,13,71,134,94,230,110,255,126,255,71,255,87,255,71,255,87,255,71,253,87,190,71,15,87,15,71,18,87,11,71,11,87,134,71,3,87,71,71,224,87,240,7,252,8,252,71,252,87,254,47,254,71,255,87,172,71,51,87,168,71,110,94,76,110,159,91,85,92,82,92,3,126,4,125,0,92,0,91,0,92,0,109,0,93,0,87,31,71,83,23,111,24,111,87,127,71,255,87,124,71,252,87,124,71,124,125,120,109,112,93,120,87,112,71,0,87,0,87,194,71,248,94,221,76,35,76,95,75,205,76,224,110,199,126,0,71,0,87,0,71,28,87,7,71,0,87,0,71,0,87,140,71,26,87,53,71,51,87,59,71,143,87,46,71,123,87,0,71,0,23,0,24,0,87,196,71,116,71,94,87,28,71,4,94,2,75,16,76,142,110,148,126,64,71,20,87,40,71,3,87,13,71,15,87,63,71,127,87,223,125,31,109,127,76,123,93,83,71,135,87,143,71,159,94,190,93,248,87,240,71,135,87,143,87,63,125,63,109,127,76,255,93,255,71,254,87,242,71,196,94,202,76,153,76,159,75,59,76,51,110,113,126,192,71,192,87,128,71,130,87,0,71,0,87,0,71,1,87,173,71,137,87,199,71,127,87,18,71,64,87,71,71,143,87,2,71,6,23,7,24,7,87,47,71,63,71,63,87,63,71,192,94,194,75,194,76,129,110,131,126,34,71,34,87,64,71,252,87,252,71,252,87,254,71,254,87,255,125,255,109,255,76,128,93,4,71,1,87,0,71,64,94,64,93,208,87,112,71,0,87,0,87,0,125,0,109,0,76,0,93,0,71,136,75,100,76,132,110,0,92,80,92,196,91,97,92,50,126,34,71,15,87,15,71,15,87,31,71,31,87,143,87,143,1,135,2,188,71,125,87,125,5,61,6,125,71,30,71,143,87,15,71,254,87,254,71,254,87,254,71,254,87,255,94,254,76,254,76,129,110,1,91,195,92,195,126,158,71,148,87,127,71,255,87,0,71,0,87,0,5,0,6,24,71,0,87,0,125,0,92,4,109,130,75,67,76,96,75,152,110,192,109,192,76,192,93,255,71,255,71,127,87,31,125,15,92,135,109,1,76,0,75,99,76,55,110,27,92,159,92,110,91,50,92,29,126,8,71,239,87,247,71,249,87,252,71,252,87,254,87,246,1,249,2,191,71,255,87,195,5,57,6,71,71,7,71,143,87,191,71,255,87,159,71,207,87,207,71,62,87,30,94,126,76,120,76,190,110,54,91,152,92,52,126,113,71,114,87,118,71,242,87,0,71,0,87,64,5,64,6,192,71,192,87,128,125,4,92,124,109,119,75,199,76,199,75,223,110,159,109,159,76,222,93,7,71,15,71,63,87,63,125,55,92,119,109,119,76,55,91,153,92,124,126,53,71,167,87,192,71,249,87,234,71,248,87,0,71,0,1,0,2,0,87,0,71,0,71,0,17,0,18,126,19,248,20,102,21,109,22,223,87,142,87,127,71,127,87,24,71,1,87,3,71,3,87,31,94,127,109,0,92,0,92,120,126,224,71,64,87,223,71,226,87,240,71,128,87,251,71,255,20,255,20,255,21,224,22,192,87,0,71,0,87,0,87,112,125,18,91,102,92,152,91,102,126,211,125,226,92,39,109,252,93,224,87,128,71,0,87,0,71,0,125,1,92,0,91,117,92,98,126,194,71,193,87,201,71,68,87,68,71,163,87,0,71,0,1,0,2,0,87,0,71,0,71,0,17,0,18,79,19,207,20,132,21,211,22,124,87,60,87,56,71,29,87,191,71,63,87,111,71,39,87,19,94,15,109,7,92,3,92,65,126,224,71,176,87,224,71,144,87,32,71,96,87,196,71,255,20,255,20,127,21,127,22,127,87,255,71,255,87,255,87,48,125,128,91,64,92,112,91,60,126,56,125,62,92,254,109,201,93,251,87,255,71,255,87,255,71,255,125,255,92,255,71,42,87,33,71,227,87,178,71,58,87,32,71,32,87,6,71,7,40,135,41,143,47,207,71,199,87,207,32,223,33,217,34,7,35,11,36,131,37,215,38,255,39,31,71,255,87,207,71,255,87,255,71,255,87,255,71,255,87,255,87,255,71,255,87,30,71,157,87,207,17,192,18,197,19,136,20,16,21,19,35,128,35,192,36,240,37,252,38,248,71,240,87,224,71,224,87,192,71,224,87,224,71,252,87,238,71,127,87,62,71,30,125,0,109,0,76,0,93,0,71,16,87,4,71,37,87,39,71,4,87,32,71,64,87,101,71,12,87,3,71,7,87,215,71,252,40,95,41,47,47,30,71,46,87,28,32,120,33,224,34,0,35,0,36,0,37,0,38,0,39,5,71,1,87,7,71,0,87,0,71,0,87,0,71,0,87,0,87,0,71,0,87,242,71,151,87,17,17,49,18,38,19,35,20,51,21,177,35,4,35,0,36,2,37,2,38,1,71,0,87,0,71,0,87,222,71,110,87,62,71,23,87,63,71,119,87,247,71,212,125,51,109,19,76,1,93,1,71,1,87,129,71,128,87,131,87,192,71,193,87,198,71,250,87,223,71,171,87,151,71,191,87,255,87,254,46,248,87,225,87,231,71,207,71,159,49,191,50,128,51,112,52,40,53,204,54,236,55,240,87,248,71,252,87,0,71,4,87,7,71,195,87,227,71,243,87,251,71,251,87,47,71,111,87,111,33,39,34,55,35,24,36,24,34,37,51,191,51,191,52,191,53,255,54,255,55,255,87,255,71,255,87,252,71,245,87,225,71,229,87,239,48,223,87,79,71,31,87,255,125,255,125,255,93,255,87,255,71,255,87,255,71,255,87,39,71,187,87,179,71,61,87,29,71,143,87,228,71,226,87,128,87,0,46,0,87,0,87,0,71,0,71,0,49,0,50,14,51,247,52,63,53,159,54,248,55,250,87,254,71,127,87,1,71,0,87,192,71,96,87,0,71,0,87,0,71,0,87,221,71,46,87,148,33,227,34,254,35,255,36,31,34,239,51,255,51,255,52,127,53,28,54,0,55,0,87,0,71,0,87,255,71,255,87,31,71,159,87,56,48,2,87,231,71,255,87,255,125,255,125,255,93,127,87,127,71,61,87,24,71,0,71,149,87,178,71,148,87,19,71,38,87,71,71,139,68,7,69,195,70,199,71,228,87,224,71,192,87,128,87,0,9,0,10,24,11,39,12,66,13,63,14,124,71,15,71,71,87,231,71,231,87,192,71,0,87,0,71,0,87,0,71,0,1,0,2,51,71,163,87,99,5,3,10,64,11,96,12,146,50,222,10,192,11,64,12,0,13,1,71,3,87,7,1,7,2,7,71,255,5,127,6,119,87,103,71,205,87,154,68,54,69,28,70,7,87,71,71,143,87,158,71,62,87,124,71,248,87,224,71,2,87,0,71,240,87,230,71,56,87,222,71,255,68,255,69,113,70,65,71,7,87,24,71,64,87,0,87,0,9,0,10,1,11,0,12,16,13,156,14,0,71,96,71,160,87,228,71,224,87,224,71,192,87,0,71,1,87,15,71,95,1,25,2,156,71,24,87,62,5,63,10,120,11,241,12,49,50,48,10,27,11,31,12,63,13,255,71,255,87,126,1,254,2,136,71,151,5,239,6,255,87,223,71,226,87,59,68,127,69,31,70,110,87,254,71,252,87,252,71,156,87,196,71,128,87,0,87,0,71,17,64,9,65,253,66,211,67,135,87,103,84,255,85,16,86,32,71,98,87,3,71,47,87,127,71,30,25,0,26,185,27,180,28,226,29,192,30,208,31,168,87,122,71,31,87,0,71,0,87,0,71,65,87,135,71,23,87,32,17,0,18,3,19,135,20,127,21,23,26,239,27,243,28,24,29,0,26,4,27,8,28,0,29,128,55,0,71,0,87,0,46,0,87,231,71,129,46,76,71,178,87,113,71,244,84,226,85,6,86,31,71,126,87,255,71,247,87,184,71,56,87,28,71,0,87,252,71,220,64,84,65,70,66,68,67,17,87,16,84,63,85,184,86,63,71,190,87,159,71,143,87,102,71,47,25,0,26,42,27,15,28,31,29,14,30,14,31,182,87,201,71,255,87,4,71,128,87,160,71,240,87,145,71,9,87,16,17,0,18,130,19,135,20,228,21,166,26,118,27,7,28,2,29,128,26,24,27,24,28,16,29,16,55,128,71,248,87,253,46,113,87,33,71,1,46,4,71,113,87,206,71,3,84,7,85,131,86,0,71,0,87,0,71,0,87,0,71,128,87,144,71,40,71,208,87,248,80,228,81,18,82,17,83,136,71,163,87,224,71,15,71,7,19,3,20,1,30,2,31,3,40,0,41,19,42,255,43,127,44,63,45,63,46,187,47,88,71,38,87,144,7,0,8,128,71,192,87,224,71,112,87,188,71,218,33,111,34,238,35,255,36,255,37,255,34,231,35,241,38,250,35,143,36,0,37,0,54,0,54,0,55,0,7,0,8,0,87,0,71,6,87,216,71,184,87,230,71,227,87,99,71,172,48,236,71,0,87,0,71,0,87,0,71,0,87,0,71,0,87,0,71,249,87,243,80,224,81,224,82,104,83,0,71,107,87,253,71,0,71,4,19,6,20,3,30,15,31,127,40,119,41,35,42,255,43,247,44,255,45,14,46,183,47,183,71,231,87,231,7,255,8,255,71,255,87,255,71,15,87,15,71,15,33,15,34,59,35,232,36,248,37,244,34,200,35,225,38,199,35,191,36,4,37,0,54,1,54,3,55,4,7,0,8,0,87,0,71,153,87,131,71,8,87,31,71,99,87,227,71,176,48,254,71,127,87,255,71,183,87,96,71,0,87,0,71,0,87,0,87,0,71,0,96,0,97,0,98,0,99,1,87,3,71,2,87,0,87,0,44,0,45,0,46,0,47,1,87,3,71,7,87,0,71,0,87,0,71,15,87,99,71,179,87,87,71,244,23,0,24,0,87,0,71,15,87,127,71,255,87,255,49,255,50,0,35,0,11,0,33,192,50,190,51,138,52,130,51,202,52,0,53,0,14,0,71,224,87,255,23,253,24,248,71,240,87,0,71,0,64,0,65,0,66,0,67,128,87,192,71,0,87,0,71,0,87,0,68,0,69,0,70,64,87,160,71,208,87,11,71,116,96,233,97,194,98,131,99,133,87,36,71,116,87,28,87,153,44,242,45,196,46,68,47,10,87,61,71,48,87,205,71,190,87,49,71,161,87,215,71,39,87,198,71,62,23,255,24,127,87,31,71,30,87,62,71,126,87,126,49,216,50,255,35,111,11,255,33,207,50,126,51,94,52,187,51,255,52,193,53,255,14,243,71,63,87,63,23,119,24,115,71,111,87,159,71,31,64,11,65,5,66,135,67,198,87,118,71,72,87,254,71,254,87,255,68,254,69,254,70,255,87,255,71,255,71,255,87,255,112,255,113,251,114,243,115,211,71,75,87,65,71,0,87,0,71,0,87,0,71,0,87,7,71,7,87,15,71,253,87,207,71,231,87,2,71,227,87,223,71,153,87,168,71,0,87,0,71,0,87,0,71,0,87,128,71,30,9,15,11,191,50,255,50,238,51,157,10,193,11,0,12,3,35,87,12,0,13,0,30,0,31,0,71,0,87,0,71,0,87,0,71,248,87,4,80,40,81,9,82,116,83,248,71,184,87,0,71,0,87,4,71,15,84,14,85,24,86,0,71,0,87,0,71,191,87,139,112,234,113,210,114,184,115,32,71,78,87,30,71,65,87,99,71,11,87,3,71,3,87,139,71,7,87,19,71,127,87,15,71,63,87,127,71,215,87,15,71,79,87,79,71,128,87,240,71,240,87,248,71,232,87,240,71,240,9,240,11,176,50,128,50,128,51,192,10,32,11,16,12,124,35,178,12,255,13,255,30,227,31,97,71,64,87,64,71,0,87,0,71,225,87,193,80,143,81,49,82,135,83,15,71,183,87,223,71,24,87,166,71,192,84,64,85,96,86,64,71,64,87,0,87,0,87,14,71,6,87,46,87,28,71,28,87,86,71,80,87,15,71,27,87,11,71,51,87,35,71,3,87,7,71,13,87,96,71,34,87,10,71,69,87,7,87,48,1,19,2,143,71,255,87,255,5,255,6,255,71,255,71,255,87,255,25,255,34,200,35,12,50,20,37,88,26,100,27,239,28,216,51,253,28,241,29,241,46,226,47,229,5,219,6,240,87,231,71,224,87,40,71,108,96,116,97,114,98,161,99,225,87,98,71,194,1,216,2,220,87,220,87,158,71,31,48,63,87,191,71,126,87,28,87,233,71,144,87,225,87,241,71,255,87,127,71,59,87,0,71,0,87,64,71,0,87,0,71,0,87,0,71,4,87,255,71,130,87,53,71,131,87,1,87,129,1,251,2,239,71,28,87,61,5,59,6,63,71,255,71,114,87,0,25,0,34,54,35,114,50,168,37,75,26,64,27,128,28,31,51,220,28,129,29,177,46,107,47,53,5,63,6,191,87,0,71,32,87,33,71,145,96,157,97,33,98,115,99,15,87,199,71,254,1,254,2,252,87,236,87,238,71,236,48,240,87,56,71,0,71,17,71,201,1,239,2,233,71,151,87,207,71,130,87,227,71,15,87,7,71,3,87,0,71,2,87,4,71,0,87,0,71,218,87,171,71,255,87,167,71,239,71,127,17,255,18,255,19,15,20,142,21,158,22,254,87,222,87,116,40,96,41,0,50,153,51,184,52,252,53,220,51,122,52,184,53,128,43,224,44,64,45,0,87,0,71,35,17,0,47,0,87,29,71,127,87,249,71,68,112,220,113,50,114,140,115,252,87,119,40,17,41,2,47,3,71,71,87,206,71,125,87,31,71,237,87,239,71,14,71,66,1,136,2,152,71,204,87,205,71,254,87,66,71,55,87,255,71,255,87,191,71,255,87,255,71,255,87,255,71,239,87,255,71,127,87,97,71,167,71,63,17,63,18,95,19,240,20,240,21,248,22,254,87,248,87,240,40,200,41,128,50,0,51,0,52,0,53,1,51,31,52,255,53,124,43,211,44,0,45,0,87,0,71,1,17,31,47,255,87,124,71,243,87,2,71,15,112,119,113,255,114,249,115,199,87,255,40,255,41,2,47,15,71,127,87,255,71,249,87,199,71,255,87,255,87,255,40,247,41,252,47,208,87,240,71,185,87,128,71,225,87,0,87,8,7,16,8,51,71,2,71,0,87,29,71,127,87,193,71,68,87,220,71,50,87,12,32,126,33,247,34,151,35,2,36,3,37,71,38,206,39,127,71,31,71,239,9,239,10,255,53,127,12,255,13,254,35,193,12,131,13,3,14,219,71,0,87,0,1,0,2,0,71,182,87,224,5,237,6,254,71,127,87,243,71,143,87,51,71,230,87,187,71,251,87,223,46,0,87,4,87,0,71,0,87,0,71,4,87,128,71,32,87,243,40,255,41,255,47,255,87,92,71,226,87,111,71,141,87,131,87,255,7,255,8,255,71,0,71,0,87,0,71,0,87,159,71,252,87,251,71,255,87,92,32,226,33,111,34,141,35,13,36,226,37,228,38,248,39,0,71,0,71,0,9,0,10,20,53,182,12,171,13,255,35,92,12,226,13,111,14,141,71,80,87,127,1,127,2,255,71,0,87,0,5,0,6,0,71,241,87,253,71,255,87,255,71,92,87,226,71,111,87,141,46,49,87,252,87,252,71,248,87,0,71,0,87,0,71,0,71,143,87,255,71,255,87,255,71,92,87,226,71,111,87,141,71,131,71,255,23,255,24,255,87,0,87,0,71,0,87,0,71,223,87,255,71,255,87,255,71,92,71,226,49,111,50,141,51,31,52,255,53,255,54,255,55,0,87,0,87,0,25,0,26,92,51,226,28,111,29,141,51,29,28,233,29,233,30,251,31,0,71,0,17,0,18,0,19,224,20,240,21,240,22,240,87,239,71,238,19,239,20,255,30,255,31,111,87,238,71,239,87,15,71,14,71,14,87,14,71,14,87,14,71,14,87,14,71,255,87,127,71,126,87,254,71,252,87,253,71,127,87,123,71,248,71,240,23,248,24,112,87,112,87,120,71,112,87,120,71,195,87,205,71,193,87,255,71,0,71,0,49,0,50,0,51,211,52,211,53,235,54,255,55,0,87,0,87,0,25,0,26,63,51,255,28,127,29,158,51,160,28,0,29,0,30,0,31,63,71,255,17,127,18,126,19,96,20,0,21,0,22,0,87,255,71,252,19,0,20,0,30,0,31,0,87,0,71,0,87,255,71,224,71,0,87,0,71,0,87,0,71,0,87,0,87,251,71,239,87,244,71,228,87,139,71,255,87,253,71,250,87,5,71,0,87,1,71,3,87,68,71,0,87,0,71,0,87,191,71,243,87,105,71,145,87,67,87,199,9,191,10,255,11,127,12,255,13,247,14,227,71,2,71,0,40,0,41,0,42,248,43,92,44,138,45,177,43,76,44,59,45,39,46,143,47,255,32,254,33,252,34,190,35,115,36,226,37,192,38,64,39,39,87,99,44,63,45,143,46,10,47,251,71,251,87,251,71,222,87,24,87,0,71,0,87,204,71,8,87,0,71,0,87,239,71,238,87,234,71,198,87,170,71,26,87,206,71,238,87,15,71,14,87,6,71,4,87,110,71,62,87,30,71,14,87,250,71,208,87,0,71,6,87,48,87,3,9,159,10,187,11,244,12,224,13,0,14,7,71,63,71,255,40,252,41,240,42,38,43,1,44,32,45,48,43,64,44,0,45,0,46,224,47,58,32,37,33,34,34,16,35,96,36,64,37,64,38,224,39,0,87,0,44,0,45,0,46,0,47,0,71,24,87,20,71,0,87,0,87,0,71,0,87,0,71,0,87,24,71,20,71,0,87,40,71,86,87,40,48,88,87,49,71,42,87,12,71,0,87,56,71,126,87,62,71,77,87,67,71,119,87,30,71,1,71,0,87,0,71,40,87,118,71,79,25,56,26,0,27,9,28,1,29,33,30,55,31,95,87,79,87,56,71,0,87,64,59,8,60,0,87,96,59,12,60,192,87,249,71,28,87,32,71,4,49,0,50,224,51,252,52,255,53,255,54,30,55,47,71,159,87,189,71,251,87,247,71,175,87,182,71,20,87,15,71,31,71,45,87,25,71,243,87,103,71,127,87,80,71,255,87,251,71,189,87,223,48,239,87,247,71,253,87,241,71,248,87,240,71,188,87,158,71,201,87,228,71,252,87,49,71,111,71,110,87,238,71,238,87,174,71,174,25,238,26,254,27,15,28,14,29,14,30,14,31,14,87,14,87,14,71,14,87,255,59,255,60,255,87,255,59,253,60,221,87,255,71,251,87,176,71,240,49,192,50,112,51,48,52,192,53,224,54,240,55,0,71,0,87,0,71,0,87,195,71,47,87,223,71,222,87,0,71,0,71,0,87,0,71,255,87,255,71,255,87,248,87,0,71,0,87,0,71,0,87,0,71,0,87,34,71,18,87,0,71,0,87,0,71,0,87,1,71,1,87,1,71,0,87,0,19,64,20,0,30,48,31,48,40,96,41,96,42,232,43,128,44,128,45,224,46,192,47,64,71,16,71,48,87,16,71,3,87,7,71,15,87,15,71,43,87,31,71,25,87,29,71,7,87,15,9,15,10,31,11,63,12,63,13,127,14,127,71,2,87,46,71,45,87,79,71,79,87,63,71,63,71,111,87,51,71,63,71,63,87,127,71,127,87,127,87,127,71,255,87,52,71,112,87,234,71,162,87,149,71,101,87,197,71,206,87,255,71,255,87,255,71,255,87,255,71,255,87,255,71,255,87,236,19,188,20,252,30,116,31,68,40,134,41,182,42,183,43,252,44,252,45,252,46,252,47,252,71,254,71,254,87,255,71,0,87,0,71,0,87,0,71,0,87,40,71,24,87,72,71,0,87,0,9,0,10,0,11,16,12,56,13,56,14,120,71,0,87,0,71,0,87,128,71,128,87,192,71,224,71,248,87,0,71,0,71,0,87,128,71,128,87,192,87,224,71,248,71,0,87,0,71,1,87,1,71,2,87,6,71,1,87,23,48,0,87,0,71,3,87,7,71,7,87,7,71,15,87,31,71,22,44,6,45,19,46,143,47,226,87,243,71,167,87,245,71,31,87,31,71,31,87,159,71,255,87,255,87,255,71,254,87,147,71,154,87,220,71,244,1,104,2,74,87,234,71,254,87,255,71,255,25,255,26,255,27,255,28,255,29,255,30,255,31,52,71,42,87,146,71,134,87,80,87,73,1,111,2,173,71,255,87,255,5,255,6,255,71,255,71,255,71,255,87,255,71,239,87,238,71,222,87,76,71,235,87,211,71,87,87,127,48,255,87,255,71,255,87,255,71,255,87,255,71,255,87,251,71,59,44,123,45,127,46,221,47,254,87,255,71,63,87,119,71,255,87,255,71,255,87,255,71,255,87,255,87,239,71,239,87,0,71,0,87,224,71,128,1,146,2,131,87,211,71,193,87,0,71,0,25,160,26,224,27,242,28,247,29,231,30,255,31,215,71,254,87,248,71,248,87,176,87,198,1,8,2,0,71,40,87,0,5,0,6,0,71,0,71,0,71,0,87,0,87,19,87,69,71,21,87,215,71,253,87,196,71,223,48,25,71,0,87,16,87,8,71,8,87,2,71,59,87,32,71,0,87,112,71,80,87,148,68,82,69,225,70,227,87,12,71,240,87,8,87,40,71,40,87,44,68,30,69,28,70,240,87,0,71,0,87,0,71,1,40,0,41,1,47,1,71,8,87,0,71,0,40,0,41,1,42,1,43,3,44,7,45,15,46,15,47,45,87,46,71,47,87,223,71,223,71,255,17,191,18,183,19,255,20,255,21,253,22,253,87,255,71,255,1,255,2,255,87,190,87,108,71,44,87,30,71,252,87,126,71,252,48,60,71,255,87,255,87,255,71,255,87,255,71,255,87,255,71,255,87,251,71,247,87,255,68,246,69,240,70,104,87,118,71,171,87,255,87,255,71,255,87,255,68,255,69,255,70,255,87,255,71,124,87,252,71,52,40,54,41,18,47,211,71,183,87,61,71,124,40,252,41,252,42,254,43,254,44,255,45,255,46,255,47,128,87,128,71,128,87,192,71,64,71,192,17,160,18,160,19,128,20,0,21,0,22,192,87,192,71,192,1,192,2,192,71,18,71,29,64,61,65,159,66,124,67,252,87,219,71,243,87,31,71,63,71,123,87,235,71,235,87,179,87,115,71,99,87,174,71,174,71,254,84,116,85,124,86,188,71,170,1,229,2,255,71,255,1,255,2,255,84,255,85,255,86,255,87,255,48,253,87,121,71,253,87,189,71,63,87,15,71,206,87,230,71,255,87,255,71,255,87,255,71,255,87,255,71,255,87,251,71,169,87,181,71,235,87,199,71,220,32,126,33,74,34,196,35,255,36,255,37,255,38,255,39,255,40,255,41,255,47,255,71,253,71,155,64,153,65,161,66,202,67,59,87,210,71,145,87,199,71,255,71,255,87,251,71,253,87,252,87,237,71,236,87,63,71,127,71,107,84,43,85,134,86,210,71,154,1,170,2,247,71,231,1,255,2,253,84,253,85,237,86,231,87,247,48,229,87,127,71,49,87,36,71,60,87,60,71,57,87,25,71,255,87,255,71,255,87,255,71,255,87,255,71,254,87,254,71,192,87,128,71,192,87,112,71,80,32,224,33,224,34,0,35,0,36,0,37,0,38,128,39,128,40,0,41,0,47,0,87,3,87,2,80,4,81,4,82,26,83,28,71,29,87,49,71,0,87,0,87,0,71,0,87,1,71,3,71,2,19,14,20,122,30,66,87,227,87,103,71,245,48,244,40,245,41,152,47,47,40,47,41,28,47,164,87,66,71,3,48,2,71,70,87,199,71,207,87,69,71,65,87,137,71,131,87,47,71,111,87,127,71,127,87,191,71,172,87,52,71,144,87,52,71,16,87,150,71,30,87,223,71,159,87,223,71,217,49,49,50,81,51,255,52,239,53,186,54,78,55,68,87,70,46,206,87,142,87,84,87,20,80,160,81,0,82,0,83,4,71,2,87,3,71,127,87,127,87,110,71,135,87,141,71,141,71,220,19,76,20,121,30,105,87,104,87,121,71,126,48,28,40,17,41,186,47,192,40,92,41,95,47,14,87,9,71,175,48,190,71,55,87,101,71,213,87,149,71,146,87,202,71,94,87,63,71,26,87,226,71,227,87,242,71,113,87,49,71,137,87,204,71,233,87,59,71,173,87,193,71,136,87,148,71,55,49,166,50,100,51,231,52,211,53,191,54,255,55,251,87,253,46,255,87,255,71,64,71,208,96,216,97,224,98,96,99,32,87,10,71,67,1,160,2,224,71,224,87,216,71,248,87,252,87,244,44,188,45,80,46,172,47,53,87,25,71,57,87,31,71,46,87,5,71,43,87,1,71,2,87,4,71,0,87,0,71,0,87,0,71,250,87,219,71,239,87,235,71,199,87,205,71,236,87,190,71,1,87,32,71,16,87,16,71,48,87,34,71,3,87,1,71,76,87,73,7,66,8,6,71,68,87,4,9,12,10,199,11,179,12,182,13,188,14,248,71,184,87,248,71,240,87,48,71,45,71,44,96,45,97,16,98,83,99,89,87,237,71,225,1,229,2,1,71,0,87,13,71,12,87,4,87,0,44,8,45,91,46,110,47,140,87,192,71,88,87,133,71,15,87,4,71,0,87,145,71,115,87,63,71,167,87,114,71,240,87,250,71,12,87,28,71,28,87,25,71,16,87,16,71,24,87,136,71,243,87,227,71,227,87,230,71,239,87,239,71,231,87,119,71,153,87,208,7,196,8,198,71,131,87,135,9,199,10,195,11,102,12,47,13,59,14,57,71,124,87,120,71,56,87,60,87,187,71,116,112,72,113,196,114,174,115,127,87,154,40,200,41,4,47,3,87,35,71,35,87,65,71,128,87,101,71,55,87,50,71,230,87,196,71,140,87,46,71,72,87,18,71,210,87,204,71,24,87,56,71,96,87,192,71,134,87,133,71,1,87,95,71,122,87,88,71,252,87,216,71,204,87,128,71,132,87,0,71,5,87,7,71,3,87,39,71,51,87,127,71,123,87,97,71,97,23,50,24,36,87,4,71,10,25,7,26,69,27,190,28,158,29,204,30,218,31,255,71,252,87,254,71,190,87,9,71,0,112,5,113,199,114,197,115,253,87,217,40,123,41,70,47,107,87,10,71,24,87,8,71,0,87,32,71,0,87,242,71,229,87,228,71,232,87,232,71,204,87,198,71,196,87,63,71,59,87,27,71,159,87,159,71,179,87,57,71,59,87,27,71,130,87,137,71,132,87,68,71,128,87,0,71,145,87,228,71,124,87,118,71,123,87,187,71,127,87,255,71,110,87,198,71,143,23,142,24,214,87,198,71,150,25,223,26,223,27,247,28,245,29,209,30,193,31,65,71,1,87,0,71,0,71,160,87,160,71,226,87,115,71,224,87,240,71,115,87,51,46,223,87,223,71,29,87,12,71,31,87,15,71,140,87,204,71,125,87,119,71,119,87,59,71,31,87,151,71,254,87,148,71,128,87,136,71,136,87,196,71,224,87,104,71,0,87,0,71,63,87,64,71,1,87,3,71,7,87,3,71,0,87,0,71,0,87,0,71,0,87,0,71,0,87,0,71,0,87,0,71,207,87,30,71,187,87,126,71,174,40,35,41,3,42,0,43,48,44,97,45,68,46,129,47,1,87,0,71,0,87,0,71,65,87,117,71,53,87,251,71,221,87,207,71,15,87,33,46,168,87,136,71,136,87,4,71,2,87,0,71,0,87,0,71,4,87,9,71,3,87,41,71,227,87,19,71,32,87,130,71,248,87,240,71,240,87,192,71,0,87,0,71,0,87,0,71,128,87,130,71,11,87,61,71,96,87,48,71,224,87,192,71,127,87,125,71,244,87,192,71,128,87,192,71,0,87,0,71,226,87,230,71,238,87,252,71,56,40,8,41,0,42,0,43,28,44,24,45,16,46,0,47,0,87,0,71,0,87,0

 };

    uint8 ctPalette[512] = {
0x00,0x00,0x4e,0x75,0x21,0x2a,0x18,0xc4,0x31,0x8e,0x4e,0x75,0x21,0x2a,0x18,0xc4,0x21,0x08,0x46,0x33,0x31,0xae,0x2d,0x69,0x31,0x8e,0x21,0x08,0x21,0x08,0x21,0x08,0x00,0x00,0x7b,0xff,0x6f,0x7d,0x5f,0x1a,0x52,0xb7,0x46,0x53,0x7f,0xff,0x39,0xf1,0x39,0xf0,0x39,0xef,0x35,0xef,0x39,0xcf,0x35,0xcf,0x35,0xce,0x35,0xce,0x35,0xce,0x35,0xce,0x35,0xcd,0x2d,0x8e,0x21,0x2c,0x29,0x2a,0x29,0x29,0x21,0x0b,0x25,0x0b,0x18,0xeb,0x18,0xeb,0x1d,0x2b,0x21,0x2b,0x21,0x0a,0x1d,0x0a,0x1d,0x0a,0x1d,0x0a,0x1d,0x0a,0x1d,0x0a,0x1d,0x09,0x1c,0xe8,0x21,0x0a,0x1d,0x09,0x18,0xea,0x1c,0xea,0x1c,0xe9,0x1d,0x09,0x1c,0xeb,0x1c,0xcb,0x14,0xc9,0x1d,0x0b,0x14,0xc9,0x25,0x29,0x18,0xc9,0x1c,0xe9,0x18,0xc9,0x1c,0xea,0x18,0xc8,0x1c,0xc8,0x1d,0x08,0x10,0xc6,0x14,0xc5,0x14,0xc7,0x14,0xa4,0x10,0x84,0x14,0xa6,0x10,0x84,0x0c,0x64,0x08,0x42,0x38,0x00,0x7b,0xdf,0x77,0x9d,0x6f,0x5b,0x67,0x39,0x62,0xf8,0x5a,0xd6,0x56,0x94,0x52,0x73,0x4e,0x51,0x46,0x0f,0x41,0xcd,0x39,0xac,0x35,0x6a,0x2d,0x48,0x29,0x06,0x38,0x00,0x77,0xdf,0x6b,0x3a,0x5e,0xd6,0x4e,0x33,0x3d,0xf0,0x31,0xad,0x2d,0x4c,0x25,0x09,0x18,0xc8,0x18,0xc5,0x18,0x86,0x08,0x64,0x0c,0x62,0x00,0x41,0x04,0x21,0x38,0x00,0x4a,0x77,0x36,0x38,0x46,0x33,0x35,0xf4,0x46,0x10,0x39,0xcf,0x2d,0x8e,0x29,0x29,0x18,0xc7,0x08,0xa6,0x0c,0x83,0x08,0x63,0x0c,0x22,0x00,0x22,0x04,0x21,0x31,0x8c,0x4b,0x5e,0x22,0x57,0x04,0xca,0x10,0xa6,0x20,0xa2,0x7f,0xff,0x7f,0x50,0x7e,0x29,0x51,0x26,0x10,0x84,0x4e,0x73,0x31,0x8c,0x1c,0xe7,0x00,0x00,0x01,0x18,0x19,0x48,0x0c,0x85,0x7f,0xff,0x6f,0x3f,0x02,0xdf,0x01,0x1c,0x6e,0xa0,0x41,0x65,0x7d,0x3c,0x34,0x10,0x29,0xb5,0x1c,0xc5,0x10,0xa6,0x2e,0x15,0x0c,0xee,0x20,0x2a,0x6e,0x50,0x04,0x24,0x04,0x88,0x08,0x42,0x05,0x11,0x0d,0xf8,0x12,0x9e,0x2f,0x7f,0x21,0x91,0x11,0x14,0x11,0x77,0x3d,0xd3,0x5e,0x76,0x6b,0x3c,0x3a,0x34,0x5a,0xd6,0x00,0x00,0x10,0x84,0x01,0xd5,0x02,0x5a,0x2d,0x6b,0x35,0xad,0x3d,0xef,0x4e,0x73,0x00,0x97,0x18,0xc7,0x1c,0xe7,0x1c,0xe7,0x11,0x30,0x15,0x94,0x1e,0x18,0x32,0xff,0x31,0x8c,0x4b,0x5e,0x22,0x57,0x04,0xca,0x10,0xa6,0x20,0xa2,0x7f,0xff,0x7f,0x50,0x7e,0x29,0x51,0x26,0x10,0x84,0x4e,0x73,0x31,0x8c,0x1c,0xe7,0x00,0x00,0x01,0x18,0x19,0x48,0x0c,0x85,0x7f,0xff,0x6f,0x3f,0x02,0xdf,0x01,0x1c,0x6e,0xa0,0x41,0x65,0x7d,0x3c,0x34,0x10,0x29,0xb5,0x1c,0xc5,0x10,0xa6,0x2e,0x15,0x0c,0xee,0x20,0x2a,0x6e,0x50,0x0c,0x66,0x60,0xc6,0x6d,0x4f,0x21,0x0e,0x21,0x71,0x22,0x17,0x23,0x1f,0x08,0xd2,0x04,0xfb,0x18,0xa9,0x14,0xcb,0x19,0x0d,0x21,0x90,0x32,0x75,0x73,0x9c,0x04,0x00,0x18,0xa5,0x00,0x00,0x49,0x43,0x55,0xa6,0x5e,0x2a,0x6a,0xb3,0x14,0xa5,0x72,0x92,0x7b,0x7b,0x2d,0x8f,0x25,0xd7,0x2e,0x7b,0x2f,0x9f,0x05,0x5c,0x25,0x08
    };

    static uint16 gfxPalette[256];

    #define BUILD_PIXEL_RGB5551(R,G,B) (((int) (R) << 11) | ((int) (G) << 6) | (int) ((B) << 1) | 1)
    for (int i = 0; i < 256; i++)
    {
        int spal = ctPalette[i * 2] * 256 + ctPalette[i * 2 + 1];
        int b = (spal >> 10) & 0x1f;
        int g = (spal >> 5) & 0x1f;
        int r = (spal) & 0x1f;
        gfxPalette[i] = BUILD_PIXEL_RGB5551(r, g, b);
    }

    bool firstFrame = true;

    if (!gpu3dsInitialize())
    {
        printf ("Unabled to initialized GPU\n");
        exit(0);
    }


    gpu3dsResetState();
    
 	while (aptMainLoop())
	{
        updateFrameCount();
        gpu3dsStartNewFrame();
        
        //----------------------------------------------------
        // Draw the game screen.
        //----------------------------------------------------
        
        t3dsStartTiming(1, "Start Frame");

        gpu3dsDisableAlphaBlending();
        gpu3dsDisableDepthTest();
        gpu3dsSetRenderTargetToMainScreenTexture();
        gpu3dsUseShader(2);
        gpu3dsSetTextureEnvironmentReplaceColor();
        gpu3dsDrawRectangle(0, 0, 256, 240, 0, 0x000000ff);
        gpu3dsSetTextureEnvironmentReplaceTexture0();
        //gpu3dsClearRenderTarget();
        t3dsEndTiming(1);


	uint8 *charMap = &chronoTriggerVRAM[1];
	uint8 *tileMap = &chronoTriggerVRAM[0];
    uint32 dummy = 0;

	
	// If any of the palette colours have changed, then we must refresh all tiles!
	//
	//if (IPPU.Mode7PaletteDirtyFlag)
	{ 
		for (int y = 0; y < 16; y++)
		{
			int y_mul_16 = y * 16;
			int y_mul_128 = y * 128;
			for (int x = 0; x < 16; x++)
			{
				gpu3dsCacheToMode7TexturePosition(
					&charMap[(y_mul_16 + x) * 128], gfxPalette, y_mul_128 + x, &dummy);
				
			}
		}
	}

	gpu3dsDisableDepthTest();
	gpu3dsSetTextureEnvironmentReplaceTexture0();
	gpu3dsBindTextureSnesMode7TileCache(GPU_TEXUNIT0);

	for (int section = 0; section < 4; section++)
	{
		gpu3dsSetRenderTargetToMode7FullTexture((3 - section) * 0x100000, 512, 512);

		for (int y = 0; y < 32; y++)
		{
			int y_mul_16 = (section * 32 + y) * 16;
			int y_mul_128 = (section * 32 + y) * 128;
			for (int x = 0; x < 128; x++)
			{
				int tileNumber = tileMap[(y_mul_128 + x) * 2];
				int texturePos = ((tileNumber & 0xf0) << 3) + (tileNumber & 0x0f); 

				// If the dirty flag is 2, this means the bitmap has
				// changed so we re-cache the updated bitmap into the texture.
				//
				// If the dirty flag is 1, this means the bitmap has
				// changed, but the new texture has already been cached,
				// and all we need to do is write the change to the large
				// mode 7 texture.
				//
				//if (IPPU.Mode7CharDirtyFlag[tileNumber] == 2)
				{
					//printf ("M7 bitmap %d (%d) changed\n", tileNumber, texturePos);
					
					//gpu3dsCacheToMode7TexturePosition(
					//	&charMap[tileNumber * 128], GFX.ScreenColors, texturePos);
				}

				// Update the large mode 7 texture when the bitmap
				// has changed or if the tile number has changed.
				//
				//if (IPPU.Mode7TileMapDirtyFlag[y_mul_128 + x] ||
				//	IPPU.Mode7CharDirtyFlag[tileNumber])
				{
					//printf ("M7 tile dirty/bitmap updated @ %d, %d\n", x, (section * 32) + y);
					
					int tx = 0;
					int ty = 0;

                    if (x < 64)
                    {
                        tx = x * 8;
                        ty = (y * 2 + 1) * 8;
                    }
                    else
                    {
                        tx = (x - 64) * 8;
                        ty = (y * 2) * 8;
                    }

                    gpu3dsAddTileVertexes(
                        tx, ty, tx+8, ty+8, 
                        0, 0, 0+8, 0+8, texturePos);
				}
			}
		}
        gpu3dsDrawVertexes();
	}
	

    gpu3dsSetRenderTargetToMainScreenTexture();
	
	gpu3dsBindTextureSnesMode7Full(GPU_TEXUNIT0);
	//gpu3dsAddTileVertexes(0, 0, 240, 240, 0, 768, 256, 1024, 0);
    for (int y = 0; y < 100; y++)
    {
	    gpu3dsAddMode7QuadVertexes(0, y, 240, y+1, 0, y, 1024, y, 0);
    }
	gpu3dsDrawVertexes();

	//gpu3dsBindTextureSnesMode7TileCache(GPU_TEXUNIT0);
	//gpu3dsAddTileVertexes(120, 0, 240, 120, 0, 0, 128, 128, 0);
	gpu3dsDrawVertexes();



        t3dsStartTiming(3, "End Frame");
        t3dsEndTiming(3);

        //----------------------------------------------------
        // Draw the texture to the frame buffer. And
        // swap the screens to show.
        //----------------------------------------------------
        t3dsStartTiming(5, "Texture to frame");
        gpu3dsDisableDepthTest();
        
        gpu3dsSetRenderTargetToTopFrameBuffer();
        gpu3dsUseShader(1);            
        gpu3dsDisableAlphaBlending();
        gpu3dsBindTextureMainScreen(GPU_TEXUNIT0);
        gpu3dsSetTextureEnvironmentReplaceTexture0();
        gpu3dsAddQuadVertexes(0, 0, 256, 240, 0, 0, 256, 240, 0.1f);
        gpu3dsDrawVertexes();
        t3dsEndTiming(5);

        if (!firstFrame)
        {
            t3dsStartTiming(6, "Transfer");
            gpu3dsTransferToScreenBuffer();
            t3dsEndTiming(6);
            
            t3dsStartTiming(7, "Swap Buffers");
            gpu3dsSwapScreenBuffers();
            t3dsEndTiming(7);
        }
        else
            firstFrame = false;        

        gpu3dsFlush();


    }    
}


void testCSND()
{
    if (!gpu3dsInitialize())
    {
        printf ("Unabled to initialized GPU\n");
        exit(0);
    }
    
    if (!snd3dsInitialize())
    {
        printf ("Unable to initialize CSND\n");
        exit (0);
    } 

    gpu3dsResetState();
    
	while (aptMainLoop())
	{

    }    
}


void testBRRDecode()
{
    if (!gpu3dsInitialize())
    {
        printf ("Unabled to initialized GPU\n");
        exit(0);
    }
    
    /*
    if (!snd3dsInitialize())
    {
        printf ("Unable to initialize CSND\n");
        exit (0);
    }
*/
    gpu3dsResetState();

    if (!snesInitialize())
    {
        printf ("Unable to initialize SNES9x\n");
        exit(0);
    }


    uint8 *data = &IAPU.RAM [0x00];

    for (int i = 0; i < 0x4000; i++)
    {
        int c = rand() & 0xff;
        data[i] = (unsigned char) c;
    }

    Channel ch, ch2;

    ch.loop = FALSE;
    ch.needs_decode = TRUE;
    ch.last_block = FALSE;
    ch.block_pointer = 0;
    ch.previous[0] = 0;
    ch.previous[1] = 0;

    ch2.loop = FALSE;
    ch2.needs_decode = TRUE;
    ch2.last_block = FALSE;
    ch2.block_pointer = 0;
    ch2.previous[0] = 0;
    ch2.previous[1] = 0;

    for (int i = 0; i < 0x4000; i += 9)
    {
        printf ("Decoding block at %x, h = %02x\n", i, data[i]);

        DecodeBlock(&ch);
        DecodeBlockFast(&ch2);
/*
        bool error = false;
        for (int j = 0; j < 16; j++)
            if (ch.block[j] != ch2.block[j]) 
                error = true;
        
        if (error)
        {
            
            printf ("i = %x\n", i);
            for (int j = 0; j < 16; j++)
                printf ("%04x ", (unsigned short)ch.block[j]);
            printf ("\n");
            for (int j = 0; j < 16; j++)
                printf ("%04x ", (unsigned short)ch2.block[j]);
            printf ("\n");
            break; 
        }   */

        //if (i == 0x2d) break;

        //break;
    }

    printf ("Test completed\n");

	while (aptMainLoop())
	{

    }    
    
}


//----------------------------------------------------------
// Main SNES emulation loop.
//----------------------------------------------------------
void snesEmulatorLoop()
{
	// Main loop
    //GPU3DS.enableDebug = true;

    int snesFrameCount = 0;
    int snesFramesSkipped = 0;
    long snesFrameTotalActualTicks = 0;
    long snesFrameTotalAccurateTicks = 0;
 
    bool firstFrame = true;
    gpu3dsResetState();
    
    frameCount60 = 60;
    long startFrameTick = svcGetSystemTick();

    IPPU.RenderThisFrame = true;
	while (aptMainLoop())
	{
        startFrameTick = svcGetSystemTick();
        
        t3dsStartTiming(1, "aptMainLoop");

        updateFrameCount();

        gpu3dsStartNewFrame();
        gpu3dsEnableAlphaBlending();

		readJoypadButtons();

		gpu3dsSetRenderTargetToMainScreenTexture();
		gpu3dsUseShader(2);             // for drawing tiles
		gpu3dsClearRenderTarget();

        S9xMainLoop();
        
        // ----------------------------------------------
        // Copy the SNES main/sub screen to the 3DS frame
        // buffer
        // (Can this be done in the V_BLANK?)
        t3dsStartTiming(3, "CopyFB");	
        gpu3dsSetRenderTargetToTopFrameBuffer();

        if (firstFrame)
        {
            // Clear the entire frame buffer to black, including the borders
            //
            gpu3dsSetTextureEnvironmentReplaceColor();
            gpu3dsDrawRectangle(0, 0, 400, 240, 0, 0x000000ff);
        }

        gpu3dsUseShader(1);             // for copying to screen.
        gpu3dsDisableAlphaBlending();

        gpu3dsBindTextureMainScreen(GPU_TEXUNIT0);
        gpu3dsSetTextureEnvironmentReplaceTexture0();
        
        gpu3dsAddQuadVertexes(72, 0, 72 + 256, screenHeight, 0, 0, 256, screenHeight, 0.1f);
        gpu3dsDrawVertexes();
        t3dsEndTiming(3);     
        
        if (!firstFrame)      
        {
            // ----------------------------------------------
            // Wait for the rendering to the SNES
            // main/sub screen to complete
            // 
            t3dsStartTiming(5, "Transfer");
            gpu3dsTransferToScreenBuffer();   
            gpu3dsSwapScreenBuffers();     
            t3dsEndTiming(5);            
        } 
        else
        {
            firstFrame = false;
        }
                
        // ----------------------------------------------
        // Flush all draw commands of the current frame
        // to the GPU.	
        t3dsStartTiming(4, "Flush");
        gpu3dsFlush();
        t3dsEndTiming(4);
                
        t3dsEndTiming(1);


        //if (GPU3DS.isReal3DS)
        {
            snesFrameCount++;
    
            long currentTick = svcGetSystemTick();
            long actualTicksThisFrame = currentTick - startFrameTick;

            if (snesFramesSkipped >= 2)
            {
                snesFramesSkipped = 0;
                snesFrameTotalActualTicks = 0;
                snesFrameTotalAccurateTicks = 0;
            }

            snesFrameTotalActualTicks += actualTicksThisFrame;  // actual time spent rendering past x frames.
            snesFrameTotalAccurateTicks += TICKS_PER_FRAME;  // time supposed to be spent rendering past x frames.

            //printf ("%7.5f - %7.5f = %7.5f ",
            //    snesFrameTotalActualTime, snesFrameTotalCorrectTime, 
            //    snesFrameTotalActualTime - snesFrameTotalCorrectTime);
            snesFrameCount ++;

            int isSlow = 0;


            long skew = snesFrameTotalAccurateTicks - snesFrameTotalActualTicks;

            //printf ("skew : %ld\n", skew);
            if (skew < 0)
            {
                // We've skewed out of the actual frame rate.
                // And we will look at subsequent frames to see if we skew beyond 0.5 fps.
                // 
                if (skew < -TICKS_PER_FRAME / 2)
                {
                    printf ("s");
                    // Skewed beyond 1 fps. So now we skip.
                    //
                    IPPU.RenderThisFrame = false;
                    snesFramesSkipped++;

                    framesSkippedCount++;   // this is used for the stats display every 60 frames.
                }
                else
                {
                    //printf ("noact\n");
                    IPPU.RenderThisFrame = true;
                }
            }
            else
            {

                float timeDiffInMilliseconds = (float)skew * 1000000 / TICKS_PER_SEC;

                printf ("w");
                svcSleepThread ((long)(timeDiffInMilliseconds * 1000));

                IPPU.RenderThisFrame = true;

                // Reset the counters.
                //
                snesFrameCount = 0;
                snesFrameTotalActualTicks = 0;
                snesFrameTotalAccurateTicks = 0;
                snesFramesSkipped = 0;
            }

        }

        if (GPU3DS.emulatorState != EMUSTATE_EMULATE)
            break;
	}    
}


int main()
{
    //testMode7Construct();
    //testMode7Texture();
    //testNewShader();
    //testGPU();
    //testCSND();
    //testBRRDecode();
    
    if (!gpu3dsInitialize())
    {
        printf ("Unable to initialized GPU\n");
        exit(0);
    }

    cacheInit();
    if (!snesInitialize())
    {
        printf ("Unable to initialize SNES9x\n");
        exit(0);
    }

    if (!snd3dsInitialize())
    {
        printf ("Unable to initialize CSND\n");
        exit (0);
    }
    printf ("Initialization complete\n");
    
    menuSelectFile();

    while (true)
    {
        switch (GPU3DS.emulatorState)
        {
            case EMUSTATE_PAUSEMENU:
                menuPause();
                break;

            case EMUSTATE_EMULATE:
                menuKeyDown = 0xffffff;
                frameCount60 = 60;
                snesEmulatorLoop();
                break;

        }
    }

  
	gfxExit();
	return 0;
}
