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

int snesFrameCount = 0;


//-------------------------------------------
// Reads and processes Joy Pad buttons.
//-------------------------------------------
uint32 readJoypadButtons()
{
    hidScanInput();
    n3dsKeysHeld = hidKeysHeld();

    // Capture buttons so they can be replayed later on.
    //if (lastKeysHeld != n3dsKeysHeld)
    //{
    // printf ("keysheld = %x, frame = %d\n", n3dsKeysHeld, snesFrameCount);
    //}

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



int frameCount60 = 60;
u64 lastTick = 0;

void updateFrameCount()
{
    if (lastTick == 0)
        lastTick = svcGetSystemTick();
        
    if (frameCount60 == 0)
    {
        u64 newTick = svcGetSystemTick();
        float timeDelta = ((float)(newTick-lastTick))/TICKS_PER_SEC;
        int fpsmul10 = (int)((float)600 / timeDelta);
        
        consoleClear();
        printf ("FPS: %2d.%1d\n", fpsmul10 / 10, fpsmul10 % 10);
        frameCount60 = 60;

#ifndef RELEASE
        for (int i=0; i<50; i++)
        {
            t3dsShowTotalTiming(i);
        } 
        t3dsResetTimings();
#endif
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
    int frameCount = 0;
    //GPU3DS.enableDebug = true;
 
    bool firstFrame = true;
    gpu3dsResetState();
    
    frameCount60 = 60;
	while (aptMainLoop())
	{
        t3dsStartTiming(1, "aptMainLoop");

        if (!Settings.Paused)
        {
            frameCount ++;
            updateFrameCount();
        }

        long startFrameTick = svcGetSystemTick();
        gpu3dsStartNewFrame();
        gpu3dsEnableAlphaBlending();

		readJoypadButtons();

		gpu3dsSetRenderTargetToMainScreenTexture();
		gpu3dsUseShader(2);             // for drawing tiles
		gpu3dsClearRenderTarget();
        if (!Settings.Paused)
        {
            IPPU.RenderThisFrame = true;
            //printf("main loop\n");
            S9xMainLoop();
        }
        
        // ----------------------------------------------
        // Copy the SNES main/sub screen to the 3DS frame
        // buffer
        // (Can this be done in the V_BLANK?)
        t3dsStartTiming(3, "CopyFB");	
        gpu3dsSetRenderTargetToTopFrameBuffer();
        gpu3dsUseShader(1);             // for copying to screen.
        gpu3dsDisableAlphaBlending();

        gpu3dsBindTextureMainScreen(GPU_TEXUNIT0);
        gpu3dsSetTextureEnvironmentReplaceTexture0();
        
        gpu3dsAddQuadVertexes(76, 0, 76 + 256, 224, 0, 0, 256, 224, 0.1f);
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

        snesFrameCount++;

        if (GPU3DS.isReal3DS)
        {
            // This gives us the total time spent emulating 1 frame.
            //
            float timePerFrame = 1.0f / 60;

            long deltaFrameTick = svcGetSystemTick() - startFrameTick;
            float timeThisFrame = (float)deltaFrameTick / TICKS_PER_SEC;
            if (timeThisFrame > timePerFrame)
            {
                // Do frame skipping. (later)
            }
            else
            {
                float timeDiffInMilliseconds = (timePerFrame - timeThisFrame) * 1000000;
                svcSleepThread ((int64)(timeDiffInMilliseconds * 1000));
            }

        }

        if (GPU3DS.emulatorState != EMUSTATE_EMULATE)
            break;
	}    
}


int main()
{
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
                snesEmulatorLoop();
                break;

        }
    }

  
	gfxExit();
	return 0;
}
