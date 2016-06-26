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

#include "3dsgpu.h"
#include "3dsopt.h"


#define CONSOLE_WIDTH 40
#define CONSOLE_HEIGHT (28 - 2)
#define S9X3DS_VERSION	     "0.1" 


uint8 *screenBuffer; 

uint8 subscreenBuffer[2*512*512*2]; 
uint8 zBuffer[2*512*478*2]; 
uint8 zsubscreenBuffer[2*512*478*2]; 
uint16 *screenBuffer16Bit = (uint16 *)screenBuffer;
uint32 *screenBuffer32Bit = (uint32 *)screenBuffer;

std::vector<std::string> files;
unsigned int current_index = 0;


#define EMUSTATE_SELECTROM      0
#define EMUSTATE_EMULATE        1
#define EMUSTATE_PAUSEMENU      2


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
	snprintf(s, PATH_MAX + 1, "%s%s%s", SLASH_STR, fname, ex);

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
		snprintf(s, PATH_MAX + 1, "%s%s.%03d%s", SLASH_STR, fname, i++, ex);
	while (stat(s, &buf) == 0 && i < 1000);

	return (s);
}

uint32 n3dsKeysHeld = 0;
uint32 lastKeysHeld = 0;


uint32 S9xReadJoypad (int which1_0_to_4)
{
    
    if (which1_0_to_4 >= 2)
        return 0;   
        
    uint32 joyPad = 0;
    if (n3dsKeysHeld & KEY_UP) joyPad |= SNES_UP_MASK;
    if (n3dsKeysHeld & KEY_DOWN) joyPad |= SNES_DOWN_MASK;
    if (n3dsKeysHeld & KEY_LEFT) joyPad |= SNES_LEFT_MASK;
    if (n3dsKeysHeld & KEY_RIGHT) joyPad |= SNES_RIGHT_MASK;
    if (n3dsKeysHeld & KEY_L) joyPad |= SNES_TL_MASK;
    if (n3dsKeysHeld & KEY_R) joyPad |= SNES_TR_MASK;
    if (n3dsKeysHeld & KEY_SELECT) joyPad |= SNES_SELECT_MASK;
    if (n3dsKeysHeld & KEY_START) joyPad |= SNES_START_MASK;
    if (n3dsKeysHeld & KEY_A) joyPad |= SNES_A_MASK;
    if (n3dsKeysHeld & KEY_B) joyPad |= SNES_B_MASK;
    if (n3dsKeysHeld & KEY_X) joyPad |= SNES_X_MASK;
    if (n3dsKeysHeld & KEY_Y) joyPad |= SNES_Y_MASK;
    
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

/*
    // Replay buttons 
    //
    if ((snesFrameCount >= 161 && snesFrameCount <= 166) ||
        (snesFrameCount >= 261 && snesFrameCount <= 267))
    {
        n3dsKeysHeld = 8;
    }
*/

    // Capture buttons so they can be replayed later on.
    if (lastKeysHeld != n3dsKeysHeld)
    {
        //printf ("keysheld = %x, frame = %d\n", n3dsKeysHeld, snesFrameCount);
    }

    u32 keysDown = (~lastKeysHeld) & n3dsKeysHeld;
    lastKeysHeld = n3dsKeysHeld;

    if (GPU3DS.enableDebug)
    {
        if (keysDown || (n3dsKeysHeld & KEY_L))
            Settings.Paused = false;
        else
            Settings.Paused = true;
    }
    if (keysDown & (KEY_SELECT))
    {
        GPU3DS.enableDebug = !GPU3DS.enableDebug;
        printf ("Debug mode = %d\n", GPU3DS.enableDebug);
    }
    if (keysDown & KEY_TOUCH)
    {
        if (GPU3DS.emulatorState == EMUSTATE_EMULATE)
            GPU3DS.emulatorState = EMUSTATE_PAUSEMENU;
    }
    return keysDown;
    
}


void fileShowList(void)
{
    consoleClear();
    unsigned int start, end;
    
    char shortFileName[CONSOLE_WIDTH - 1];

    start = current_index + CONSOLE_HEIGHT >= files.size() ?
            (files.size() < CONSOLE_HEIGHT ? 0 : files.size() - CONSOLE_HEIGHT) :
            current_index;
            
    end = std::min(start + CONSOLE_HEIGHT, files.size() - 1);
    printf ("SNES9x v%s\n", S9X3DS_VERSION);
    printf ("-------------------------\n");
    for (unsigned int i = start; i <= end; i++)
    {
        printf(i == current_index ? ">" : " ");
        
        memset(shortFileName, 0, CONSOLE_WIDTH - 1);
        strncpy(shortFileName, files[i].c_str(), CONSOLE_WIDTH - 2);
        printf("%s\n", shortFileName);
    }
}

char romFileName[200];

void fileSelectLoop(void)
{
    if (files.empty())
    {
        printf("Place your files in the same directory as your emulator\n\n");
        romFileName[0] = 0;
    }

    fileShowList();

    bool quitting = false;
    while (aptMainLoop())
    {
        hidScanInput();
        u32 kDown = readJoypadButtons();
        if (kDown & KEY_START || kDown & KEY_A)
        {
            quitting = kDown & KEY_START;
            break;
        }
        if (kDown & KEY_UP)
        {
            current_index = (current_index == 0) ? 0 : current_index - 1;
            fileShowList();
        }
        if (kDown & KEY_DOWN)
        {
            current_index = std::min(current_index + 1, files.size() - 1);
            fileShowList();
        }

        gfxFlushBuffers();
        gfxSwapBuffers();

        gspWaitForVBlank();
    }

    if (quitting)
    {
        romFileName[0] = 0;
    }

    std::string ret = files[current_index];
    strncpy (romFileName, ret.c_str(), 199);
}


void fileGetAllFiles(void)
{
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
}


const char * menuItems[] = {
    "Resume Game",
    "Save State (Slot 1)",
    "Save State (Slot 2)",
    "Save State (Slot 3)",
    "Save State (Slot 4)",
    "Load State (Slot 1)",
    "Load State (Slot 2)",
    "Load State (Slot 3)",
    "Load State (Slot 4)",
    "Reset SNES",
    "Select ROM"
};

char menuMessage[200];

void menuSetMessage(char *message)
{
    strncpy(menuMessage, message, 199);
}

void menuShowItems(int selectedMenuItem)
{
    consoleClear();

    printf("SNES9x v%s (Paused)\n", S9X3DS_VERSION);
    printf ("-------------------------\n");
    for (int i = 0; i < 11; i++)
    {
        if (selectedMenuItem == i)
            printf ("> ");
        else
            printf ("  ");

        printf ("%s\n", menuItems[i]);
    }
    printf ("\n\n");
    printf ("%s\n\n", menuMessage);
    printf ("A - Select     B - Cancel");
}

void menuLoop()
{
    int selectedMenuItem = 0;

    menuMessage[0] = 0;

    menuShowItems(selectedMenuItem);
    while (aptMainLoop())
    {
        hidScanInput();
        u32 kDown = readJoypadButtons();

        if (kDown & KEY_B)
        {
            // Cancels the menu and resumes game
            //
            GPU3DS.emulatorState = EMUSTATE_EMULATE;
            consoleClear();
            return;
        }
        if (kDown & KEY_START || kDown & KEY_A)
        {
            if (selectedMenuItem == 0)
            {
                // Resume game
                //
                GPU3DS.emulatorState = EMUSTATE_EMULATE;
                consoleClear();
                return;
            }
            else if (selectedMenuItem == 1)
            {
                Snapshot(S9xGetFilename (".s1")); 
                menuSetMessage("Saved state 1");             
            }
            else if (selectedMenuItem == 2)
            {
                Snapshot(S9xGetFilename (".s2"));              
                menuSetMessage("Saved state 2");             
            }
            else if (selectedMenuItem == 3)
            {
                Snapshot(S9xGetFilename (".s3"));              
                menuSetMessage("Saved state 3");             
            }
            else if (selectedMenuItem == 4)
            {
                Snapshot(S9xGetFilename (".s4"));              
                menuSetMessage("Saved state 4");             
            }
            else if (selectedMenuItem == 5)
            {
                if (S9xLoadSnapshot(S9xGetFilename (".s1")))
                {     
                    gpu3dsClearAllRenderTargets();
                    GPU3DS.emulatorState = EMUSTATE_EMULATE;
                    consoleClear();
                    return;
                }
                else
                    menuSetMessage("Unable to load savestate.");
            }
            else if (selectedMenuItem == 6)
            {
                if (S9xLoadSnapshot(S9xGetFilename (".s2")))
                {     
                    S9xLoadSnapshot(S9xGetFilename (".s2"));              
                    gpu3dsClearAllRenderTargets();
                    GPU3DS.emulatorState = EMUSTATE_EMULATE;
                    consoleClear();
                    return;
                }
                else
                    menuSetMessage("Unable to load savestate.");
            }
            else if (selectedMenuItem == 7)
            { 
                if (S9xLoadSnapshot(S9xGetFilename (".s3")))
                {     
                    S9xLoadSnapshot(S9xGetFilename (".s3"));              
                    gpu3dsClearAllRenderTargets();
                    GPU3DS.emulatorState = EMUSTATE_EMULATE;
                    consoleClear();
                    return;
                }
                else
                    menuSetMessage("Unable to load savestate.");
            }
            else if (selectedMenuItem == 8)
            {
                if (S9xLoadSnapshot(S9xGetFilename (".s4")))
                {     
                    S9xLoadSnapshot(S9xGetFilename (".s4"));              
                    gpu3dsClearAllRenderTargets();
                    GPU3DS.emulatorState = EMUSTATE_EMULATE;
                    consoleClear();
                    return;
                }
                else
                    menuSetMessage("Unable to load savestate.");
            }
            else if (selectedMenuItem == 9)
            {
                // Reset SNES
                //
                S9xReset();
                gpu3dsClearAllRenderTargets();
                GPU3DS.emulatorState = EMUSTATE_EMULATE;
                consoleClear();
                return;
            }
            else if (selectedMenuItem == 10)
            {
                // Select new ROM
                //
                GPU3DS.emulatorState = EMUSTATE_SELECTROM;
                consoleClear();
                return;
            }
            menuShowItems(selectedMenuItem);
        }
        if (kDown & KEY_UP)
        {
            selectedMenuItem--;
            if (selectedMenuItem < 0)
                selectedMenuItem = 10;
            menuShowItems(selectedMenuItem);
        }
        if (kDown & KEY_DOWN)
        {
            selectedMenuItem++;
            if (selectedMenuItem > 10)
                selectedMenuItem = 0;
            menuShowItems(selectedMenuItem);
        }

        gfxFlushBuffers();
        gfxSwapBuffers();

        gspWaitForVBlank();
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
    Settings.SoundPlaybackRate = 4;
    Settings.Stereo = FALSE;
    Settings.BGLayering = TRUE;
    Settings.SoundBufferSize = 0;
    Settings.CyclesPercentage = 100;
    Settings.DisableSoundEcho = FALSE;
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

    
    screenBuffer = (uint8 *) linearAlloc(256*2*256);
    printf ("Allocated screenBuffer: %x\n", (u32)screenBuffer);
    
    screenBuffer16Bit = (uint16 *)screenBuffer;
    screenBuffer32Bit = (uint32 *)screenBuffer;
    GFX.Pitch = 512;
    GFX.Screen = screenBuffer;
    GFX.SubScreen = subscreenBuffer;
    GFX.ZBuffer = zBuffer;
    GFX.SubZBuffer = zsubscreenBuffer;


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
     
    if(!S9xInitSound(7, true, 4400))
    {
        printf ("Unable to initialize sound.\n");
        return false;
    }
    return true;
}


//-------------------------------------------------------
// Load the ROM and reset the CPU.
//-------------------------------------------------------
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
    ((u16 *)texture->PixelData)[offset] = new_color;
}

/*
void G3D_SetTexturePixel32(sf2d_texture *texture, int x, int y, u32 new_color)
{
	y = (texture->pow2_h - 1 - y);
	
    u32 coarse_y = y & ~7;
    u32 coarse_x = x & ~7;
    u32 offset = G3D_MortonInterleave(x, y) + 
        coarse_x * 8 +
        coarse_y * texture->pow2_w;
    ((u32 *)texture->data)[offset] = new_color;
}
*/


int frameCount60 = 60;
u64 lastTick = 0;

void updateFrameCount()
{
    if (lastTick == 0)
        u64 lastTick = svcGetSystemTick();
        
    if (frameCount60 == 0)
    {
        u64 newTick = svcGetSystemTick();
        float timeDelta = ((float)(newTick-lastTick))/TICKS_PER_SEC;
        int fpsmul10 = (int)((float)600 / timeDelta);
        
        printf ("FPS: %2d.%1d\n", fpsmul10 / 10, fpsmul10 % 10);
        for (int i=0; i<50; i++)
        {
            t3dsShowTotalTiming(i);
        } 
        frameCount60 = 60;
        lastTick = newTick;
        t3dsResetTimings();
    }
    
    frameCount60--;    
}


/*
void testSF2D2()
{
    if (!gpu3dsInitialize())
    {
        printf ("Unabled to initialized GPU\n");
        exit(0);
    }
       
    sf2d_texture *tex1 = gpu3dsCreateTexture(256, 256, TEXFMT_RGB5A1, SF2D_PLACE_RAM);
    for (int y=0; y<256; y++)
        for (int x=0; x<256; x++)
        {
             uint16 c1 = 0x1f - (x + y) & 0x1f;
             uint8 alpha = (x + y) < 5 ? 0 : 1;
             uint32 c = c1 << 11 | c1 << 5 | alpha;
             G3D_SetTexturePixel16(tex1, x, y, c);
        }
    printf ("Texture allocated.\n");
    
    int fc = 0;
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
        gpu3dsSetRenderTarget(1);
        gpu3dsUseShader(1);
        gpu3dsClearRenderTarget();
        gpu3dsSetTextureEnvironmentReplaceTexture0();
        gpu3dsBindTexture(tex1, GPU_TEXUNIT0);
        t3dsEndTiming(1);
        
        t3dsStartTiming(2, "Draw Tiles");
        for (int i=0; i<4; i++)
        {   
            // Scissor test:
            // y1 (from bottom), x1 (from right), y2 (from bottom), x2 (from left)
            if (i % 2 == 0)
                gpu3dsScissorTest(GPU_SCISSOR_NORMAL, 0, 0, 256, 240);
            else
                gpu3dsScissorTest(GPU_SCISSOR_NORMAL, 30, 30, 200, 230);
                
            for (int y=0; y<28; y++)
            {
                for (int x=0; x<32; x++)
                {
                    gpu3dsAddQuadVertexes( 
                        x * 8 + fc * i, y * 8  + fc * i, 
                        x * 8 + 8 + fc * i, y * 8 + 8 + fc * i, 
                        0, 0, 8.0f / 256, 8.0f / 256,
                        (3-i) * 0.1f
                        );
                }
            }
            gpu3dsDrawVertexes();
        }
        t3dsEndTiming(2);
        
        gpu3dsUseShader(0);            
        gpu3dsSetTextureEnvironmentReplaceColor();
        gpu3dsScissorTest(GPU_SCISSOR_NORMAL, 0, 0, 256, 240);
        gpu3dsDrawRectangle(10, 20, 40, 80, 0.1f, 0x00ff00cf);  // ABGR format
        t3dsStartTiming(3, "End Frame");
        gpu3dsFrameEnd();
        gpu3dsTransferToScreenBuffer();
        t3dsEndTiming(3);

        //----------------------------------------------------
        // Draw the texture to the frame buffer. And
        // swap the screens to show.
        //----------------------------------------------------
        t3dsStartTiming(5, "Texture to frame");
        gpu3dsSetRenderTarget(0);
        {
            gpu3dsUseShader(0);            
            gpu3dsBindTextureMainScreen(GPU_TEXUNIT0);
            gpu3dsSetTextureEnvironmentReplaceTexture0();
            gpu3dsAddQuadVertexes(0, 0, 256, 224, 0, 0, 1.0f, 224.0f / 256, 0.1f);
            gpu3dsDrawVertexes();
        }
        gpu3dsFrameEnd();
        gpu3dsTransferToScreenBuffer();
        t3dsEndTiming(5);
        
        t3dsStartTiming(6, "Swap Buffers");
        gpu3dsSwapScreenBuffers();
        t3dsEndTiming(6);
        
        fc = (fc + 1) % 60;
        rad += 0.2f;
    }   
}




void testSF2D3()
{

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
    
    sf2d_texture *tex1 = gpu3dsCreateTexture(1024, 1024, TEXFMT_RGB5A1, SF2D_PLACE_RAM);
    for (int y=0; y<1024; y++)
        for (int x=0; x<1024; x++)
        {
             uint16 c1 = 0x1f - (x + y) & 0x1f;
             uint8 alpha = (x + y) < 5 ? 0 : 1;
             uint32 c = c1 << 11 | c1 << 5 | alpha;
             G3D_SetTexturePixel16(tex1, x, y, c);
        }
    printf ("Texture allocated.\n");
    
    int fc = 0;
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
        //GPUCMD_SetBuffer(gpuCommandBuffer, gpuCommandBufferSize, 0);
        gpu3dsSetRenderTarget(1);
        {
            gpu3dsUseShader(1);
            gpu3dsClearRenderTarget();
            gpu3dsSetTextureEnvironmentReplaceTexture0();
            gpu3dsBindTexture(tex1, GPU_TEXUNIT0);
            gpu3dsScissorTest(GPU_SCISSOR_NORMAL, 0, 0, 256, 240);
            
            t3dsEndTiming(1);
            
            t3dsStartTiming(2, "Draw Tiles");
            for (int i=0; i<4; i++)
            {   
                for (int y=0; y<28; y++)
                {
                    for (int x=0; x<32; x++)
                    {
                        gpu3dsAddTileVertexes( 
                            x * 8 + fc * i, y * 8  + fc * i, 
                            x * 8 + 8 + fc * i, y * 8 + 8 + fc * i, 
                            0, 0, 8.0f, 8.0f,
                            (3-i) * 0.1f
                            );
                    }
                }
                gpu3dsDrawVertexes();
            }
            
            t3dsEndTiming(2);
        }
        
        gpu3dsUseShader(0);            
        gpu3dsSetTextureEnvironmentReplaceColor();
        gpu3dsScissorTest(GPU_SCISSOR_NORMAL, 0, 0, 256, 240);
        gpu3dsDrawRectangle(10, 20, 40, 80, 0.1f, 0x00ff00cf);  // ABGR format
        t3dsStartTiming(3, "End Frame");
        t3dsEndTiming(3);

        //----------------------------------------------------
        // Draw the texture to the frame buffer. And
        // swap the screens to show.
        //----------------------------------------------------
        t3dsStartTiming(5, "Texture to frame");
        gpu3dsSetRenderTarget(0);
        gpu3dsUseShader(0);            
        gpu3dsBindTextureMainScreen(GPU_TEXUNIT0);
        gpu3dsSetTextureEnvironmentReplaceTexture0();
        gpu3dsAddQuadVertexes(0, 0, 256, 224, 0, 0, 1.0f, 224.0f / 256, 0.1f);
        gpu3dsDrawVertexes();

        gpu3dsFlush();
        t3dsEndTiming(5);

        t3dsStartTiming(6, "Transfer");
        gpu3dsTransferToScreenBuffer();
        t3dsEndTiming(6);
        
        t3dsStartTiming(7, "Swap Buffers");
        gpu3dsSwapScreenBuffers();
        t3dsEndTiming(7);
        
        fc = (fc + 1) % 60;
        rad += 0.2f;
    }    
}
*/


void testSF2D3()
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
    printf ("CMD Buffer: %d %d\n", gpuCommandBufferSize, gpuCommandBufferOffset);
    
    SGPUTexture *tex1 = gpu3dsCreateTextureInLinearMemory(1024, 1024, GPU_RGBA5551);
    printf ("1");
    
    for (int y=0; y<1024; y++)
        for (int x=0; x<1024; x++)
        {
             uint16 c1 = 0x1f - (x + y) & 0x1f;
             uint8 alpha = (x + y) < 5 ? 0 : 1;
             uint32 c = c1 << 11 | c1 << 5 | c1 << 1 | alpha;
             G3D_SetTexturePixel16(tex1, x, y, c);
        }
    printf ("Texture allocated.\n");
    printf ("2");
    
    int fc = 0;
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
        printf ("a");
        gpu3dsSetRenderTarget(1);
        printf ("b");
        gpu3dsUseShader(1);
        gpu3dsSetTextureEnvironmentReplaceTexture0();
        gpu3dsBindTexture(tex1, GPU_TEXUNIT0);
        //gpu3dsClearRenderTarget();
        printf ("c");
        t3dsEndTiming(1);
        
        printf ("d");
        
        t3dsStartTiming(2, "Draw Tiles");
        for (int i=0; i<4; i++)
        {   
            for (int y=0; y<28; y++)
            {
                for (int x=0; x<32; x++)
                {
                    gpu3dsAddTileVertexes( 
                        x * 8 + fc * i, y * 8  + fc * i, 
                        x * 8 + 8 + fc * i, y * 8 + 8 + fc * i, 
                        0, 0, 8.0f, 8.0f,
                        i * 0.1f
                        );
                }
            }
            gpu3dsDrawVertexes();
        }
        
        t3dsEndTiming(2);
        
        
        gpu3dsUseShader(0);            
        gpu3dsSetTextureEnvironmentReplaceColor();
        gpu3dsDrawRectangle(10, 20, 40, 80, 0.1f, 0x00ff00cf);  // ABGR format
        t3dsStartTiming(3, "End Frame");
        t3dsEndTiming(3);

        //----------------------------------------------------
        // Draw the texture to the frame buffer. And
        // swap the screens to show.
        //----------------------------------------------------
        t3dsStartTiming(5, "Texture to frame");
        gpu3dsSetRenderTarget(0);
        gpu3dsUseShader(0);            
        gpu3dsBindTextureMainScreen(GPU_TEXUNIT0);
        gpu3dsSetTextureEnvironmentReplaceTexture0();
        gpu3dsAddQuadVertexes(0, 0, 256, 224, 0, 0, 256.0f / 256, 224.0f / 256, 0.1f);
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


        fc = (fc + 1) % 60;
        rad += 0.2f;
    }    
}


void testSF2D3a()
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
    

    for (int y=0; y<1024; y++)
        for (int x=0; x<1024; x++)
        {
             uint16 c1 = 0x1f - (x + y) & 0x1f;
             uint8 alpha = (x + y) < 5 ? 0 : 1;
             uint32 c = c1 << 11 | c1 << 5 | c1 << 1 | alpha;
             G3D_SetTexturePixel16(tex1, x, y, c);
        }
    printf ("Texture allocated.\n");
    
    int fc = 0;
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
        gpu3dsSetRenderTarget(1);
        gpu3dsUseShader(1);
        gpu3dsSetTextureEnvironmentReplaceTexture0();
        gpu3dsBindTexture(tex1, GPU_TEXUNIT0);
        //gpu3dsClearRenderTarget();
        t3dsEndTiming(1);
        
        
        t3dsStartTiming(2, "Draw Tiles");
        for (int i=0; i<4; i++)
        {   
            for (int y=0; y<28; y++)
            {
                for (int x=0; x<32; x++)
                {
                    gpu3dsAddTileVertexes( 
                        x * 8 + fc * i, y * 8  + fc * i, 
                        x * 8 + 8 + fc * i, y * 8 + 8 + fc * i, 
                        0, 0, 8.0f, 8.0f,
                        i * 0.1f
                        );
                }
            }
            gpu3dsDrawVertexes();
        }
        
        t3dsEndTiming(2);
        
        
        gpu3dsUseShader(0);            
        gpu3dsSetTextureEnvironmentReplaceColor();
        gpu3dsDrawRectangle(10, 20, 40, 80, 0.1f, 0x00ff00cf);  // ABGR format
        t3dsStartTiming(3, "End Frame");
        t3dsEndTiming(3);

        //----------------------------------------------------
        // Draw the texture to the frame buffer. And
        // swap the screens to show.
        //----------------------------------------------------
        t3dsStartTiming(5, "Texture to frame");
        gpu3dsSetRenderTarget(0);
        gpu3dsUseShader(0);            
        gpu3dsBindTextureMainScreen(GPU_TEXUNIT0);
        gpu3dsSetTextureEnvironmentReplaceTexture0();
        gpu3dsAddQuadVertexes(0, 0, 256, 224, 0, 0, 256.0f / 256, 224.0f / 256, 0.1f);
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


        fc = (fc + 1) % 60;
        rad += 0.2f;
    }    
}


void testSF2D4()
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
    

    for (int y=0; y<1024; y++)
        for (int x=0; x<1024; x++)
        {
             uint16 c1 = 0x1f - (x + y) & 0x1f;
             uint8 alpha = (x + y) < 5 ? 0 : 1;
             uint32 c = c1 << 11 | c1 << 5 | c1 << 1 | alpha;
             G3D_SetTexturePixel16(tex1, x, y, c);
        }
    printf ("Texture allocated.\n");
    
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
        gpu3dsEnableAlphaBlending();
        gpu3dsSetRenderTarget(1);
        gpu3dsUseShader(0);
        gpu3dsDrawRectangle(0, 0, 256, 240, 0, 0x000000ff);
        gpu3dsUseShader(1);
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
                    gpu3dsAddTileVertexes( 
                        x * 8 + fc * i, y * 8  + fc * i, 
                        x * 8 + 8 + fc * i, y * 8 + 8 + fc * i, 
                        0, 0, 8.0f, 8.0f,
                        i < 2 ? 0 : 1
                        );
                }
            }
            gpu3dsDrawVertexes();
        }
        
        t3dsEndTiming(2);
        
        gpu3dsUseShader(0);            
        gpu3dsSetTextureEnvironmentReplaceColor();

        gpu3dsEnableDepthTest();
        gpu3dsEnableAdditiveDiv2Blending();
        
        gpu3dsDrawRectangle(0, 0, 100, 100, 1, 0x00ff00ff);  // ABGR format
        t3dsStartTiming(3, "End Frame");
        t3dsEndTiming(3);

        //----------------------------------------------------
        // Draw the texture to the frame buffer. And
        // swap the screens to show.
        //----------------------------------------------------
        t3dsStartTiming(5, "Texture to frame");
        gpu3dsDisableDepthTest();
        
        gpu3dsSetRenderTarget(0);
        gpu3dsUseShader(0);            
        gpu3dsDisableAlphaBlending();
        gpu3dsBindTextureMainScreen(GPU_TEXUNIT0);
        gpu3dsSetTextureEnvironmentReplaceTexture0();
        gpu3dsAddQuadVertexes(0, 0, 256, 224, 0, 0, 256.0f / 256, 224.0f / 256, 0.1f);
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

/*
void testSF2D4()
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
    
    sf2d_texture *tex1 = gpu3dsCreateTexture(1024, 1024, TEXFMT_RGB5A1, SF2D_PLACE_RAM);
    for (int y=0; y<1024; y++)
        for (int x=0; x<1024; x++)
        {
             uint16 c1 = 0x1f - (x + y) & 0x1f;
             uint8 alpha = (x + y) < 5 ? 0 : 1;
             uint32 c = c1 << 11 | c1 << 5 | alpha;
             G3D_SetTexturePixel16(tex1, x, y, c);
        }
    printf ("Texture allocated.\n");
    
    int fc = 0;
    float rad = 0;
    
    gpu3dsResetState();
    
 	while (aptMainLoop())
	{
        updateFrameCount();
        gpu3dsStartNewFrame();
        
		readJoypadButtons();

        
        //----------------------------------------------------
        // Draw the game screen.
        //----------------------------------------------------
        
        t3dsStartTiming(1, "Start Frame");
        gpu3dsSetRenderTarget(1);
        gpu3dsBindTexture(tex1, GPU_TEXUNIT0);
        gpu3dsClearRenderTarget();
        t3dsEndTiming(1);
        
        
        t3dsStartTiming(2, "Draw Tiles");
        int ystep = 4;
        for (int i=0; i<4; i++)
        {   
            for (int y=0; y<228; y += ystep)
            {
                gpu3dsUseShader(1);
                gpu3dsSetTextureEnvironmentReplaceTexture0();
                for (int x=0; x<32; x++)
                {
                    gpu3dsAddTileVertexes( 
                        x * 8 + fc * i, y + fc * i, 
                        x * 8 + 8 + fc * i, y + ystep + fc * i, 
                        0, y % 8, 8.0f, y % 8 + ystep,
                        i * 0.1f
                        );
                }
                gpu3dsDrawVertexes();


                gpu3dsUseShader(0);            
                gpu3dsSetTextureEnvironmentReplaceColor();
                gpu3dsDrawRectangle(0, y, 256, y + 1, 0.1f, ((fc + y / ystep) % 16) << 4);  // ABGR format
        
            }
        }
        
        t3dsEndTiming(2);
        
        
        gpu3dsUseShader(0);            
        gpu3dsSetTextureEnvironmentReplaceColor();
        gpu3dsDrawRectangle(10, 20, 40, 80, 0.1f, 0x00ff00cf);  // ABGR format
        t3dsStartTiming(3, "End Frame");
        t3dsEndTiming(3);

        //----------------------------------------------------
        // Draw the texture to the frame buffer. And
        // swap the screens to show.
        //----------------------------------------------------
        t3dsStartTiming(5, "Texture to frame");
        gpu3dsSetRenderTarget(0);
        gpu3dsUseShader(0);            
        gpu3dsBindTextureMainScreen(GPU_TEXUNIT0);
        gpu3dsSetTextureEnvironmentReplaceTexture0();
        gpu3dsAddQuadVertexes(0, 0, 256, 224, 0, 0, 1.0f, 224.0f / 256, 0.1f);
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


        fc = (fc + 1) % 60;
        rad += 0.2f;
    }    
}
*/

/*
INLINE uint8 __attribute__((always_inline)) S9xGetByteFast2 (uint32 Address)
{
    int block;
    uint8 *GetAddress = Memory.Map [block = (Address >> MEMMAP_SHIFT) & MEMMAP_MASK];
    //printf ("%0x %0x\n", Address, (uint32)GetAddress);

	CPU.Cycles += Memory.MemorySpeed [block];
	
    if (GetAddress >= (uint8 *) CMemory::MAP_LAST)
    {
	    return (*(GetAddress + (Address & 0xffff)));
    }
	else 
		return S9xGetByteFromRegister(GetAddress, Address);
}



void testReadWriteByte()
{
    if (!gpu3dsInitialize())
    {
        printf ("Unabled to initialized GPU\n");
        exit(0);
    }
    cacheInit();
    
    fileGetAllFiles();
    fileSelectLoop();

    if (strlen(romFileName) == 0)
        return;
        
    printf ("Long size = %d\n", sizeof(long));
    printf("Loading ROM '%s'...\n", romFileName);
    snesLoadRom();
    printf ("Loaded\n");
	// Main loop
    int frameCount60 = 60;
 
    bool firstFrame = true;
    int previousFrameCmdBuffer = 0;
    int currentFrameCmdBuffer = 1;
    gpu3dsResetState();
	while (aptMainLoop())
	{
        updateFrameCount();
        
        t3dsStartTiming(1, "S9xGetByte");
        for (int i = 0; i < 100000; i++)
        {
            uint8 b = S9xGetByteFast2(0x800000 + i & 0x1fff);
            //printf ("%02x", b);
        }
        t3dsEndTiming(1);
    }    
}


void testMemoryAlloc()
{
    if (!gpu3dsInitialize())
    {
        printf ("Unabled to initialized GPU\n");
        exit(0);
    }
    cacheInit();
    
    uint32 *heap;
    uint32 *linearMem;
    
    for (int i = 0; i < 16; i++)
    {
        void *mem = malloc(0x400000);
        printf ("malloc %d MB: %x\n", (i+1) * 4, mem);
        if (mem == 0)
            break;
        heap = (uint32 *)mem;
    }
    
	while (aptMainLoop())
    {
        uint32 keysPressed = readJoypadButtons();
        if (keysPressed & KEY_A)
            break;
    }    
    
    for (int i = 0; i < 32; i++)
    {
        void *mem = linearAlloc(0x100000);
        printf ("linearAlloc %d MB: %x\n", (i+1) * 4, mem);
        if (mem == 0)
            break;
        linearMem = (uint32 *)mem;
    }
    
	while (aptMainLoop())
    {
        uint32 keysPressed = readJoypadButtons();
        if (keysPressed & KEY_A)
            break;
    }    
    
    #define HW(n)  heap[n+r+i] = r+i;
    
    printf ("Writing to heap\n");
    t3dsStartTiming(1, "Heap memory");
    for (int x = 0; x < 100; x++)
    {
        int r = rand() % 100;
        for (int i = 0; i < 20000; i++)
        {
            HW(0); HW(1); HW(2); HW(3); HW(4);
            HW(5); HW(6); HW(7); HW(8); HW(9);
            HW(10); HW(11); HW(12); HW(13); HW(14);
            HW(15); HW(16); HW(17); HW(18); HW(19);
        }
    }   
    t3dsEndTiming(1);
    
    printf ("Writing to linear memory\n");
    t3dsStartTiming(2, "Linear memory");
    heap = linearMem;
    for (int x = 0; x < 100; x++)
    {
        int r = rand() % 100;
        for (int i = 0; i < 20000; i++)
        {
            HW(0); HW(1); HW(2); HW(3); HW(4);
            HW(5); HW(6); HW(7); HW(8); HW(9);
            HW(10); HW(11); HW(12); HW(13); HW(14);
            HW(15); HW(16); HW(17); HW(18); HW(19);
        }
    }   
    t3dsEndTiming(2);
    
    for (int i=0; i<50; i++)
    {
        t3dsShowTotalTiming(i);
    }     
    while (true)
    {
        
    }

}
*/

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
    
	while (aptMainLoop())
	{
        if (!Settings.Paused)
        {
            frameCount ++;
            updateFrameCount();
        }
        gpu3dsStartNewFrame();
        gpu3dsEnableAlphaBlending();
        
        t3dsStartTiming(1, "aptMainLoop");

		readJoypadButtons();

		//printf ("Frame Begin\n");
		gpu3dsSetRenderTarget(1);
		gpu3dsUseShader(0);
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
        gpu3dsSetRenderTarget(0);
        gpu3dsUseShader(0);
        gpu3dsDisableAlphaBlending();

        //if (frameCount % 2 == 0) 
            gpu3dsBindTextureMainScreen(GPU_TEXUNIT0);
        //else
        //    gpu3dsBindTextureSubScreen(GPU_TEXUNIT0);
        gpu3dsSetTextureEnvironmentReplaceTexture0();
         
        gpu3dsAddQuadVertexes(70, 0, 70 + 256, 224, 0, 0, 256.0f / 256, 224.0f / 256, 0.1f);
        //gpu3dsAddQuadVertexes(0, 0, 256, 224, 0, 0, 256.0f / 256, (224.0f + 256.0f) / 512, 0.1f);
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

        if (GPU3DS.emulatorState != EMUSTATE_EMULATE)
            break;
	}    
}


int main()
{
    //testReadWriteByte();
    //testSF2D4();
    //testMemoryAlloc();
    
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
    fileGetAllFiles();

    while (true)
    {
        switch (GPU3DS.emulatorState)
        {
            case EMUSTATE_SELECTROM:
                fileSelectLoop();
                if (strlen(romFileName) == 0)
                    break;

                snesLoadRom();
                
                GPU3DS.emulatorState = EMUSTATE_EMULATE;
                break;

            case EMUSTATE_PAUSEMENU:
                menuLoop();
                break;

            case EMUSTATE_EMULATE:
            
                snesEmulatorLoop();
                break;

        }
    }

        
    //printf ("GFX.PPL: %d\n",  GFX.PPL);
        
    /*
    void *mem = malloc(0x100000);
    printf ("malloc 1 MB: %x\n", mem);
    if (!((uint32)mem <= (uint32)&OCPU && (uint32)&OCPU <= ((uint32)mem) + 0x100000))
    {
        printf ("OCPU %x is not aligned. Please rebuild.\n", (uint32)&OCPU);
        while (true) ;
    }
    */
        
    // ----------------------------------
    // v1.43
    // ----------------------------------
    /*
    printf ("Long size = %d\n", sizeof(long));
    printf("Loading ROM '%s'...\n", romFileName);
    snesLoadRom();
    printf ("Loaded\n");
	// Main loop
    int frameCount = 0;
    //GPU3DS.enableDebug = true;
 
    bool firstFrame = true;
    int previousFrameCmdBuffer = 0;
    int currentFrameCmdBuffer = 1;
    gpu3dsResetState();
	while (aptMainLoop())
	{
        if (!Settings.Paused)
        {
            frameCount ++;
            updateFrameCount();
        }
        gpu3dsStartNewFrame();
        gpu3dsEnableAlphaBlending();
        
        t3dsStartTiming(1, "aptMainLoop");

		readJoypadButtons();

		//printf ("Frame Begin\n");
		gpu3dsSetRenderTarget(1);
		gpu3dsUseShader(0);
		//gpu3dsClearRenderTarget();
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
        gpu3dsSetRenderTarget(0);
        gpu3dsUseShader(0);
        gpu3dsDisableAlphaBlending();

        if (frameCount % 2 == 0)
            gpu3dsBindTextureMainScreen(GPU_TEXUNIT0);
        else
            gpu3dsBindTextureSubScreen(GPU_TEXUNIT0);
        gpu3dsSetTextureEnvironmentReplaceTexture0();
         
        gpu3dsAddQuadVertexes(0, 0, 256, 224, 0, 0, 256.0f / 256, 224.0f / 256, 0.1f);
        //gpu3dsAddQuadVertexes(0, 0, 256, 224, 0, 0, 256.0f / 256, (224.0f + 256.0f) / 512, 0.1f);
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
	}
    */
    //sf2d_free_texture(tex1);
    //sf2d_fini();
	gfxExit();
	return 0;
}
