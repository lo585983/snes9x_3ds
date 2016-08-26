#include <algorithm>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <vector>

#include <unistd.h>
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
#include "3dsui.h"
#include "3dsfont.h"

#include "lodepng.h"

#define S9X3DS_VERSION	        "0.3" 


typedef struct
{
    int     MaxFrameSkips = 4;              // 0 - disable, 
                                            // 1 - enable (max 1 consecutive skipped frame)
                                            // 2 - enable (max 2 consecutive skipped frames)
                                            // 3 - enable (max 3 consecutive skipped frames)
                                            // 4 - enable (max 4 consecutive skipped frames)

    int     ScreenStretch = 0;              // 0 - no stretch, 1 - stretch full, 2 - aspect fit

    int     ForceFrameRate = 0;             // 0 - Use ROM's Region, 1 - Force 50 fps, 2 - Force 60 fps

    int     ScreenX0, ScreenX1, ScreenY0, ScreenY1;

    int     Turbo[6] = {0, 0, 0, 0, 0, 0};  // Turbo buttons: 0 - No turbo, 1 - Release/Press every alt frame.
                                            // Indexes: 0 - A, 1 - B, 2 - X, 3 - Y, 4 - L, 5 - R

    long    TicksPerFrame;                  // Ticks per frame. Will change depending on PAL/NTSC

} S9xSettings3DS;


S9xSettings3DS settings3DS; 


#define TICKS_PER_SEC (268123480)
#define TICKS_PER_FRAME_NTSC (4468724)
#define TICKS_PER_FRAME_PAL (5362469)


int frameCount60 = 60;
u64 frameCountTick = 0;
int framesSkippedCount = 0;
char *romFileName = 0;
char romFileNameFullPath[_MAX_PATH];
char cwd[_MAX_PATH];


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
    ui3dsSetColor(0x3f7fff, 0);
    ui3dsDrawString(100, 140, 220, true, "Saving SRAM to SD card...");
    
    // We use this to force the sound to stop mixing.
    //
    GPU3DS.emulatorState = EMUSTATE_PAUSEMENU;
    int millisecondsToWait = 5;
    svcSleepThread ((long)(millisecondsToWait * 1000));
    
	Memory.SaveSRAM (S9xGetFilename (".srm"));

    ui3dsSetColor(0x7f7f7f, 0);
    ui3dsDrawString(100, 140, 220, true, "");

    // Then we re-start the sound mixing again.
    //
    GPU3DS.emulatorState = EMUSTATE_EMULATE;
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
    //Settings.SDD1Pack=FALSE;
    
    char filename [_MAX_PATH + 1];
    char index [_MAX_PATH + 1];
    char data [_MAX_PATH + 1];

	Settings.SDD1Pack=FALSE;
    Memory.FreeSDD1Data ();

    if (strncmp (Memory.ROMName, ("Star Ocean"), 10) == 0)
	{
		Settings.SDD1Pack=TRUE;
	}
    else if(strncmp(Memory.ROMName, ("STREET FIGHTER ALPHA2"), 21)==0)
	{
		if(Memory.ROMRegion==1)
		{
			Settings.SDD1Pack=TRUE;
		}
		else
		{
			Settings.SDD1Pack=TRUE;
		}
	}
	else
	{ 
		Settings.SDD1Pack=TRUE;
	}

    /*
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
	snprintf(s, PATH_MAX + 1, "%s/%s%s", dir, fname, ex);

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
		snprintf(s, PATH_MAX + 1, "%s/%s.%03d%s", dir, fname, i++, ex);
	while (stat(s, &buf) == 0 && i < 1000);

	return (s);
}

uint32 n3dsKeysHeld = 0;
uint32 lastKeysHeld = 0;
uint32 menuKeyDown = 0;
uint32 prevSnesJoyPad = 0;

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

    uint32 snesJoyPad = 0;

    if (s9xKeysHeld & KEY_UP) snesJoyPad |= SNES_UP_MASK;
    if (s9xKeysHeld & KEY_DOWN) snesJoyPad |= SNES_DOWN_MASK;
    if (s9xKeysHeld & KEY_LEFT) snesJoyPad |= SNES_LEFT_MASK;
    if (s9xKeysHeld & KEY_RIGHT) snesJoyPad |= SNES_RIGHT_MASK;
    if (s9xKeysHeld & KEY_L) snesJoyPad |= SNES_TL_MASK;
    if (s9xKeysHeld & KEY_R) snesJoyPad |= SNES_TR_MASK;
    if (s9xKeysHeld & KEY_SELECT) snesJoyPad |= SNES_SELECT_MASK;
    if (s9xKeysHeld & KEY_START) snesJoyPad |= SNES_START_MASK;
    if (s9xKeysHeld & KEY_A) snesJoyPad |= SNES_A_MASK;
    if (s9xKeysHeld & KEY_B) snesJoyPad |= SNES_B_MASK;
    if (s9xKeysHeld & KEY_X) snesJoyPad |= SNES_X_MASK;
    if (s9xKeysHeld & KEY_Y) snesJoyPad |= SNES_Y_MASK;

    // Handle turbo buttons.
    //
    #define HANDLE_TURBO(i, mask) if (settings3DS.Turbo[i] && (prevSnesJoyPad & mask) && (snesJoyPad & mask)) snesJoyPad &= ~mask;
    HANDLE_TURBO(0, SNES_A_MASK);
    HANDLE_TURBO(1, SNES_B_MASK);
    HANDLE_TURBO(2, SNES_X_MASK);
    HANDLE_TURBO(3, SNES_Y_MASK);
    HANDLE_TURBO(4, SNES_TL_MASK);
    HANDLE_TURBO(5, SNES_TR_MASK);    
    
    prevSnesJoyPad = snesJoyPad;

    return snesJoyPad;
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


//-------------------------------------------------------
// Clear top screen with logo.
//-------------------------------------------------------


void clearTopScreenWithLogo()
{
	unsigned char* image;
	unsigned width, height;

    int error = lodepng_decode32_file(&image, &width, &height, "./snes9x_3ds_top.png");

    if (!error && width == 400 && height == 240)
    {
        // GX_DisplayTransfer needs input buffer in linear RAM

        // lodepng outputs big endian rgba so we need to convert
        for (int i = 0; i < 2; i++)
        {
            u8* src = image; 
            uint32* fb = (uint32 *) gfxGetFramebuffer(GFX_TOP, GFX_LEFT, NULL, NULL);
            for (int y = 0; y < 240; y++)
                for (int x = 0; x < 400; x++) 
                {
                    uint32 r = *src++;
                    uint32 g = *src++;
                    uint32 b = *src++;
                    uint32 a = *src++;

                    //r >>= 3;
                    //g >>= 3;
                    //b >>= 3;
                    //uint16 c = (uint16)((r << 11) | (g << 6) | (b << 1) | 1);
                    uint32 c = ((r << 24) | (g << 16) | (b << 8) | 0xff);
                    fb[x * 240 + (239 - y)] = c;
                }
            gfxSwapBuffers();
        }
        
        free(image);
    } 
}



//-------------------------------------------
// Reads and processes Joy Pad buttons.
//-------------------------------------------
int debugFrameCounter = 0;
uint32 readJoypadButtons()
{
    hidScanInput();
    n3dsKeysHeld = hidKeysHeld();

    u32 keysDown = (~lastKeysHeld) & n3dsKeysHeld;

#ifndef RELEASE    
    // -----------------------------------------------
    // For debug only
    // -----------------------------------------------
    if (GPU3DS.enableDebug)
    {
        keysDown = keysDown & (~lastKeysHeld);
        if (keysDown || (n3dsKeysHeld & KEY_L))
        {
            //printf ("  kd:%x lkh:%x nkh:%x\n", keysDown, lastKeysHeld, n3dsKeysHeld);
            Settings.Paused = false;
        }
        else
        {
            //printf ("  kd:%x lkh:%x nkh:%x\n", keysDown, lastKeysHeld, n3dsKeysHeld);
            Settings.Paused = true;
        }
    }

    if (keysDown & (KEY_SELECT))
    {
        GPU3DS.enableDebug = !GPU3DS.enableDebug;
        printf ("Debug mode = %d\n", GPU3DS.enableDebug);
    }
    // -----------------------------------------------
#endif    

    if (keysDown & KEY_TOUCH)
    {
        // Save the SRAM if it has been modified before we going
        // into the menu.
        //
        if (CPU.SRAMModified || CPU.AutoSaveTimer)
        {
            S9xAutoSaveSRAM();
        }
             
        if (GPU3DS.emulatorState == EMUSTATE_EMULATE)
            GPU3DS.emulatorState = EMUSTATE_PAUSEMENU;
    }
    lastKeysHeld = n3dsKeysHeld;
    return keysDown;
    
}


//----------------------------------------------------------------------
// Menu options
//----------------------------------------------------------------------

SMenuItem emulatorMenu[] = { 
    { -1, "Resume", -1 }, 
    { 1000, "  Resume Game", -1 }, 
    { -1, NULL, -1 }, 
    { -1, "Savestates", -1 }, 
    { 2001, "  Save Slot #1", -1}, 
    { 2002, "  Save Slot #2", -1}, 
    { 2003, "  Save Slot #3", -1}, 
    { 2004, "  Save Slot #4", -1}, 
    { -1, NULL, -1 }, 
    { 3001, "  Load Slot #1", -1}, 
    { 3002, "  Load Slot #2", -1}, 
    { 3003, "  Load Slot #3", -1}, 
    { 3004, "  Load Slot #4", -1}, 
    { -1, NULL, -1 }, 
    { -1, "Emulation", -1 }, 
    { 4001, "  Take Screenshot", -1 }, 
    { 5001, "  Reset SNES", -1 }, 
    { 6001, "  Exit SNES9X", -1 } 
    };

SMenuItem emulatorNewMenu[] = { 
    { 6001, "  Exit SNES9X", -1 } 
    };

SMenuItem optionMenu[] = { 
    { -1, "Frameskip", -1 }, 
    { 10000, "  Disabled                    ", 0}, 
    { 10001, "  Enabled (max 1 frame)       ", 0}, 
    { 10002, "  Enabled (max 2 frames)      ", 0}, 
    { 10003, "  Enabled (max 3 frames)      ", 0}, 
    { 10004, "  Enabled (max 4 frames)      ", 1}, 
    { -1, NULL, -1 }, 
    { -1, "Screen", -1 }, 
    { 11000, "  No stretch                  ", 1}, 
    { 11001, "  Stretch to 4:3              ", 0}, 
    { 11002, "  Stretch to fullscreen       ", 0}, 
    { -1, NULL, -1 }, 
    { -1, "Turbo Buttons", -1 }, 
    { 13000, "  Button A                    ", 0}, 
    { 13001, "  Button B                    ", 0}, 
    { 13002, "  Button X                    ", 0}, 
    { 13003, "  Button Y                    ", 0}, 
    { 13004, "  Button L                    ", 0}, 
    { 13005, "  Button R                    ", 0}, 
    { -1, NULL, -1 }, 
    { -1, "Frame Rate", -1 }, 
    { 12000, "  Depending on ROM's Region   ", 1}, 
    { 12001, "  Run at 50 FPS               ", 0}, 
    { 12002, "  Run at 60 FPS               ", 0}, 
    };


int emulatorMenuCount = 0;
int optionMenuCount = 0;


//----------------------------------------------------------------------
// Update settings
//----------------------------------------------------------------------

void settingsUpdateScreen()
{
    if (settings3DS.ScreenStretch == 0)
    {
        settings3DS.ScreenX0 = 72;
        settings3DS.ScreenX1 = 72 + 256;
        settings3DS.ScreenY0 = 0;
        settings3DS.ScreenY1 = PPU.ScreenHeight;
    }
    else if (settings3DS.ScreenStretch == 1)
    {
        // Added support for 320x240 (4:3) screen ratio
        settings3DS.ScreenX0 = 40;
        settings3DS.ScreenX1 = 360;
        settings3DS.ScreenY0 = 0;
        settings3DS.ScreenY1 = 240;
    }
    else if (settings3DS.ScreenStretch == 2)
    {
        settings3DS.ScreenX0 = 0;
        settings3DS.ScreenX1 = 400;
        settings3DS.ScreenY0 = 0;
        settings3DS.ScreenY1 = 240;
    }
}

//----------------------------------------------------------------------
// Update settings
//----------------------------------------------------------------------

void settingsUpdateFrameRate()
{
    if (Settings.PAL)
        settings3DS.TicksPerFrame = TICKS_PER_FRAME_PAL;
    else
        settings3DS.TicksPerFrame = TICKS_PER_FRAME_NTSC;

    if (settings3DS.ForceFrameRate == 1)
        settings3DS.TicksPerFrame = TICKS_PER_FRAME_PAL;

    else if (settings3DS.ForceFrameRate == 2)
        settings3DS.TicksPerFrame = TICKS_PER_FRAME_NTSC;

        
}



//----------------------------------------------------------------------
// Save settings specific to game.
//----------------------------------------------------------------------
bool settingsWriteMode = 0;
void settingsReadWrite(FILE *fp, char *format, int *value, int minValue, int maxValue)
{
    //if (strlen(format) == 0)
    //    return;

    if (settingsWriteMode)
    {
        if (value != NULL)
        {
            //printf ("Writing %s %d\n", format, *value);
        	fprintf(fp, format, *value);
        }
        else
        {
            //printf ("Writing %s\n", format);
        	fprintf(fp, format);
            
        }
    }
    else
    {
        if (value != NULL)
        {
            fscanf(fp, format, value);
            if (*value < minValue)
                *value = minValue;
            if (*value > maxValue)
                *value = maxValue;
            //printf ("Scanned %d\n", *value);
        }
        else
        {
            fscanf(fp, format);
        }
    }
}

//----------------------------------------------------------------------
// Read/write all possible settings.
//----------------------------------------------------------------------
void settingsReadWriteFullList(FILE *fp)
{
    settingsReadWrite(fp, "#v1\n", NULL, 0, 0);
    settingsReadWrite(fp, "# Do not modify this file or risk losing your settings.\n", NULL, 0, 0);

    settingsReadWrite(fp, "Frameskips=%d\n", &settings3DS.MaxFrameSkips, 0, 4);
    settingsReadWrite(fp, "ScreenStretch=%d\n", &settings3DS.ScreenStretch, 0, 2);
    settingsReadWrite(fp, "Framerate=%d\n", &settings3DS.ForceFrameRate, 0, 2);
    settingsReadWrite(fp, "TurboA=%d\n", &settings3DS.Turbo[0], 0, 1);
    settingsReadWrite(fp, "TurboB=%d\n", &settings3DS.Turbo[1], 0, 1);
    settingsReadWrite(fp, "TurboX=%d\n", &settings3DS.Turbo[2], 0, 1);
    settingsReadWrite(fp, "TurboY=%d\n", &settings3DS.Turbo[3], 0, 1);
    settingsReadWrite(fp, "TurboL=%d\n", &settings3DS.Turbo[4], 0, 1);
    settingsReadWrite(fp, "TurboR=%d\n", &settings3DS.Turbo[5], 0, 1);

    // All new options should come here!
}


//----------------------------------------------------------------------
// Update the checkboxes to keep them in sync with the
// actual loaded settings.
//----------------------------------------------------------------------
void settingsUpdateMenuCheckboxes()
{
    S9xUncheckGroup(optionMenu, optionMenuCount, settings3DS.MaxFrameSkips + 10000);
    S9xCheckItemByID(optionMenu, optionMenuCount, settings3DS.MaxFrameSkips + 10000);

    S9xUncheckGroup(optionMenu, optionMenuCount, settings3DS.ScreenStretch + 11000);
    S9xCheckItemByID(optionMenu, optionMenuCount, settings3DS.ScreenStretch + 11000);

    S9xUncheckGroup(optionMenu, optionMenuCount, settings3DS.ForceFrameRate + 12000);
    S9xCheckItemByID(optionMenu, optionMenuCount, settings3DS.ForceFrameRate + 12000);

    for (int i = 0; i < 6; i++)
        S9xSetCheckItemByID(optionMenu, optionMenuCount, 13000 + i, settings3DS.Turbo[i]);

}

//----------------------------------------------------------------------
// Save settings by game.
//----------------------------------------------------------------------
bool settingsSaveByGame()
{
    FILE *fp = fopen(S9xGetFilename(".cfg"), "w+");
    //printf ("write fp = %x\n", (uint32)fp);
    if (fp != NULL)
    {
        settingsWriteMode = true;
        settingsReadWriteFullList(fp);
        fclose(fp);
        return true;
    }
    return false;
}

//----------------------------------------------------------------------
// Load settings by game.
//----------------------------------------------------------------------
bool settingsLoadByGame()
{
    FILE *fp = fopen(S9xGetFilename(".cfg"), "r");
    //printf ("fp = %x\n", (uint32)fp);
    if (fp != NULL)
    {
        settingsWriteMode = false;
        settingsReadWriteFullList(fp);

        settingsUpdateFrameRate();
        settingsUpdateScreen();
        settingsUpdateMenuCheckboxes();
    }
    else
    {
        // If we can't find the saved settings, let's
        // save the current set for this game.
        //
        settingsSaveByGame();
    }
    return false;
}




//-------------------------------------------------------
// Load the ROM and reset the CPU.
//-------------------------------------------------------

void snesLoadRom()
{
    consoleClear();
    snprintf(romFileNameFullPath, _MAX_PATH, "%s%s", cwd, romFileName);

    bool loaded = Memory.LoadROM(romFileNameFullPath);
    Memory.LoadSRAM (S9xGetFilename (".srm"));

    gpu3dsInitializeMode7Vertexes();
    gpu3dsCopyVRAMTilesIntoMode7TileVertexes(Memory.VRAM);
    cacheInit();
    gpu3dsClearAllRenderTargets();
    if (loaded)
    {
        printf ("  ROM Loaded...\n");
    }
    GPU3DS.emulatorState = EMUSTATE_EMULATE;

    consoleClear();
    settingsLoadByGame();
    settingsUpdateScreen();
    settingsUpdateFrameRate();

    debugFrameCounter = 0;
    prevSnesJoyPad = 0;
}


//----------------------------------------------------------------------
// Menus
//----------------------------------------------------------------------
SMenuItem fileMenu[1000];
char romFileNames[1000][_MAX_PATH];


int totalRomFileCount = 0;



//----------------------------------------------------------------------
// Go up to the parent directory.
//----------------------------------------------------------------------
void fileGoToParentDirectory(void)
{
    int len = strlen(cwd);

    if (len > 1)
    {
        for (int i = len - 2; i>=0; i--)
        {
            if (cwd[i] == '/')
            {

                cwd[i + 1] = 0;
                break;
            }
        }
    }
}


//----------------------------------------------------------------------
// Go up to the child directory.
//----------------------------------------------------------------------
void fileGoToChildDirectory(char *childDir)
{
    strncat(cwd, childDir, _MAX_PATH);
    strncat(cwd, "/", _MAX_PATH);
}


//----------------------------------------------------------------------
// Load all ROM file names (up to 512 ROMs)
//----------------------------------------------------------------------
void fileGetAllFiles(void)
{
    std::vector<std::string> files;
    char buffer[_MAX_PATH];
    
    struct dirent* dir;
    DIR* d = opendir(cwd);

    if (strlen(cwd) > 1)
    {
        snprintf(buffer, _MAX_PATH, "\x01 ..");
        files.push_back(buffer);
    }
    
    if (d)
    {
        while ((dir = readdir(d)) != NULL)
        {
            char *dot = strrchr(dir->d_name, '.');

            if (dir->d_name[0] == '.')
                continue;
            if (dir->d_type == DT_DIR)
            {
                snprintf(buffer, _MAX_PATH, "\x01 %s", dir->d_name);
                files.push_back(buffer);
            }
            if (dir->d_type == DT_REG)
            {
                if (!strstr(dir->d_name, ".smc") &&
                    !strstr(dir->d_name, ".fig") &&
                    !strstr(dir->d_name, ".sfc"))
                    continue;
                    
                files.push_back(dir->d_name);
            }
        }
        closedir(d);
    }

    std::sort(files.begin(), files.end());

    totalRomFileCount = 0;

    // Increase the total number of files we can display.
    for (int i = 0; i < files.size() && i < 1000; i++)
    {
        strncpy(romFileNames[i], files[i].c_str(), _MAX_PATH);
        totalRomFileCount++;
        fileMenu[i].ID = i;
        fileMenu[i].Text = romFileNames[i];
        fileMenu[i].Checked = -1;
    }
}






//----------------------------------------------------------------------
// Handle menu settings.
//----------------------------------------------------------------------
bool menuHandleSettings(int selection)
{
    if (selection / 1000 == 10)
    {
        settings3DS.MaxFrameSkips = selection % 1000;
        settingsUpdateMenuCheckboxes();
        return true;
    }
    else if (selection / 1000 == 11)
    {
        settings3DS.ScreenStretch = selection % 1000;
        settingsUpdateMenuCheckboxes();
        settingsUpdateScreen();
        return true;
    }        
    else if (selection / 1000 == 12)
    {
        settings3DS.ForceFrameRate = selection % 1000;
        settingsUpdateMenuCheckboxes();
        settingsUpdateFrameRate();
        return true;
    }        
    else if (selection / 1000 == 13)
    {
        settings3DS.Turbo[selection % 1000] = 1 - settings3DS.Turbo[selection % 1000];
        settingsUpdateMenuCheckboxes();
        return true;
    }
    return false;
}

//----------------------------------------------------------------------
// Start up menu.
//----------------------------------------------------------------------
void menuSelectFile(void)
{
    emulatorMenuCount = sizeof(emulatorNewMenu) / sizeof(SMenuItem);
    optionMenuCount = sizeof(optionMenu) / sizeof(SMenuItem);
    
    fileGetAllFiles();
    S9xClearMenuTabs();
    S9xAddTab("Emulator", emulatorNewMenu, emulatorMenuCount);
    S9xAddTab("Options", optionMenu, optionMenuCount);
    S9xAddTab("Select ROM", fileMenu, totalRomFileCount);
    S9xSetTabSubTitle(0, NULL);
    S9xSetTabSubTitle(2, cwd);
    S9xSetCurrentMenuTab(2);
    S9xSetTransferGameScreen(false);

    int selection = 0;
    do
    {
        APT_AppStatus appStatus = aptGetStatus();
        if (appStatus == APP_EXITING)
            return;
        
        selection = S9xMenuSelectItem();

        if (selection >= 0 && selection < 1000)
        {
            // Load ROM
            //
            romFileName = romFileNames[selection];
            if (romFileName[0] == 1)
            {
                if (strcmp(romFileName, "\x01 ..") == 0)
                    fileGoToParentDirectory();
                else
                    fileGoToChildDirectory(&romFileName[2]);

                fileGetAllFiles();
                S9xClearMenuTabs();
                S9xAddTab("Emulator", emulatorMenu, emulatorMenuCount);
                S9xAddTab("Options", optionMenu, optionMenuCount);
                S9xAddTab("Select ROM", fileMenu, totalRomFileCount);
                S9xSetCurrentMenuTab(2);
                S9xSetTabSubTitle(2, cwd);
                selection = -1;
            }
            else
            {
                snesLoadRom();
                return;
            }
        }
        else if (selection == 6001)
        {
            if (S9xConfirm("Exit SNES9X", "Are you sure you want to exit?", ""))
            {
                GPU3DS.emulatorState = EMUSTATE_END;
                return;
            }
        }

        // Handle all other settings.
        //
        menuHandleSettings(selection);

        selection = -1;     // Bug fix: Fixes crashing when setting options before any ROMs are loaded.
    } 
    while (selection == -1);

    snesLoadRom();
}


//----------------------------------------------------------------------
// Menu when the emulator is paused in-game.
//----------------------------------------------------------------------
 
void menuPause()
{
    emulatorMenuCount = sizeof(emulatorMenu) / sizeof(SMenuItem);
    optionMenuCount = sizeof(optionMenu) / sizeof(SMenuItem);
    bool settingsUpdated = false;
    
    S9xClearMenuTabs();
    S9xAddTab("Emulator", emulatorMenu, emulatorMenuCount);
    S9xAddTab("Options", optionMenu, optionMenuCount);
    S9xAddTab("Select ROM", fileMenu, totalRomFileCount);
    S9xSetTabSubTitle(0, NULL);
    S9xSetTabSubTitle(2, cwd);
    S9xSetTransferGameScreen(true);

    while (true)
    {
        APT_AppStatus appStatus = aptGetStatus();
        if (appStatus == APP_EXITING)
        {
            if (settingsUpdated)
                settingsSaveByGame();  
            return;
        }
        
        int selection = S9xMenuSelectItem();

        if (selection == -1 || selection == 1000)
        {
            // Cancels the menu and resumes game
            //
            GPU3DS.emulatorState = EMUSTATE_EMULATE;
            consoleClear();

            if (settingsUpdated)
                settingsSaveByGame();  
            
            return;
        }
        else if (selection < 1000)
        {
            // Load ROM
            //
            romFileName = romFileNames[selection];
            if (romFileName[0] == 1)
            {
                if (strcmp(romFileName, "\x01 ..") == 0)
                    fileGoToParentDirectory();
                else
                    fileGoToChildDirectory(&romFileName[2]);

                fileGetAllFiles();
                S9xClearMenuTabs();
                S9xAddTab("Emulator", emulatorMenu, emulatorMenuCount);
                S9xAddTab("Options", optionMenu, optionMenuCount);
                S9xAddTab("Select ROM", fileMenu, totalRomFileCount);
                S9xSetCurrentMenuTab(2);
                S9xSetTabSubTitle(2, cwd);
            }
            else
            {
                if (settingsUpdated)
                    settingsSaveByGame();  
                
                snesLoadRom();
                return;
            }
        }
        else if (selection >= 2001 && selection <= 2010)
        {
            int slot = selection - 2000;
            char s[_MAX_PATH];
            sprintf(s, "Saving into slot %d.", slot);
            S9xShowWaitingMessage("Save State", s, "This may take a while...");

            sprintf(s, ".%d.frz", slot);
            Snapshot(S9xGetFilename (s)); 

            sprintf(s, "Slot %d save complete", slot);
            S9xAlertSuccess("Save State", s, "");

            S9xSetSelectedItemIndexByID(0, 1000);
        }
        else if (selection >= 3001 && selection <= 3010)
        {
            int slot = selection - 3000;
            char s[_MAX_PATH];
            
            sprintf(s, ".%d.frz", slot);
            if (S9xLoadSnapshot(S9xGetFilename (s)))
            {     
                gpu3dsCopyVRAMTilesIntoMode7TileVertexes(Memory.VRAM);
                debugFrameCounter = 0;
                gpu3dsClearAllRenderTargets();
                GPU3DS.emulatorState = EMUSTATE_EMULATE;
                consoleClear();

                if (settingsUpdated)
                    settingsSaveByGame();  
                
                return;
            }
            else
            {
                sprintf(s, "Unable to load slot %d", slot);
                S9xAlertFailure("Load State", s, "");
            }
        }
        else if (selection == 4001)
        { 
            char path[256];
            u32 timestamp = (u32)(svcGetSystemTick() / 446872);
            snprintf(path, 256, "%ssnes9x_%08d.bmp", cwd, timestamp);
            S9xShowWaitingMessage("Take Screenshot", "Now taking a screenshot.", "This may take a while...");
            if (S9xTakeScreenshot(path)) S9xAlertSuccess("Take Screenshot", "Screenshot saved to", path);
            else S9xAlertFailure("Take Screenshot", "Unable to save screenshot.", "");
        }
        else if (selection == 5001)
        {            
            S9xReset();
            cacheInit();
            gpu3dsInitializeMode7Vertexes();
            gpu3dsCopyVRAMTilesIntoMode7TileVertexes(Memory.VRAM);
            gpu3dsClearAllRenderTargets();
            GPU3DS.emulatorState = EMUSTATE_EMULATE;
            consoleClear();

            prevSnesJoyPad = 0;

            if (settingsUpdated)
                settingsSaveByGame();  
            
            return;
        }
        else if (selection == 6001)
        {
            if (S9xConfirm("Exit SNES9X", "Are you sure you want to exit?", ""))
            {
                GPU3DS.emulatorState = EMUSTATE_END;

                if (settingsUpdated)
                    settingsSaveByGame();  
                
                return;
            }
        }
        
        // Handle all other settings.
        //
        bool handled = menuHandleSettings(selection);
        if (handled)
            settingsUpdated = true;
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
    Settings.Paused = false;
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
    Settings.AutoSaveDelay = 60;         // Bug fix to save SRAM within 60 frames (1 second instead of 30 seconds)
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


//--------------------------------------------------------
// Initialize the Snes9x engine and everything else.
//--------------------------------------------------------
void emulatorInitialize()
{
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

    /*if (romfsInit()!=0)
    {
        printf ("Unable to initialize romfs\n");
        exit (0);
    }
    */
    printf ("Initialization complete\n");
        
}


//--------------------------------------------------------
// Finalize the emulator.
//--------------------------------------------------------
void emulatorFinalize()
{
    consoleClear();
    printf("gspWaitForP3D:\n");
    gspWaitForVBlank();
    gpu3dsWaitForPreviousFlush();
    gspWaitForVBlank();

    printf("snd3dsFinalize:\n");
    snd3dsFinalize();
    printf("gpu3dsFinalize:\n");
    gpu3dsFinalize();

    printf("S9xGraphicsDeinit:\n");
    S9xGraphicsDeinit();
    printf("S9xDeinitAPU:\n");
    S9xDeinitAPU();
    printf("Memory.Deinit:\n");
    Memory.Deinit();

    //printf("romfsExit:\n");
    //romfsExit();
    printf("hidExit:\n");
	hidExit();
    printf("aptExit:\n");
	aptExit();
    printf("srvExit:\n");
	srvExit(); 
}



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



char frameCountBuffer[70];
void updateFrameCount()
{
    if (frameCountTick == 0)
        frameCountTick = svcGetSystemTick();
    
    if (frameCount60 == 0)
    {
        u64 newTick = svcGetSystemTick();
        float timeDelta = ((float)(newTick - frameCountTick))/TICKS_PER_SEC;
        int fpsmul10 = (int)((float)600 / timeDelta);
        
#if !defined(RELEASE) && !defined(DEBUG_CPU) && !defined(DEBUG_APU)
        consoleClear();
#endif

        if (framesSkippedCount)
            snprintf (frameCountBuffer, 69, "FPS: %2d.%1d (%d skipped)\n", fpsmul10 / 10, fpsmul10 % 10, framesSkippedCount);
        else
            snprintf (frameCountBuffer, 69, "FPS: %2d.%1d \n", fpsmul10 / 10, fpsmul10 % 10);

        ui3dsSetColor(0x7f7f7f, 0);
        ui3dsDrawString(2, 2, 200, false, frameCountBuffer);

        frameCount60 = 60;
        framesSkippedCount = 0;


#if !defined(RELEASE) && !defined(DEBUG_CPU) && !defined(DEBUG_APU)
        printf ("\n\n");
        for (int i=0; i<100; i++)
        {
            t3dsShowTotalTiming(i);
        } 
        t3dsResetTimings();
#else
        ui3dsDrawString(100, 100, 220, true, "Touch screen for menu");
#endif
        frameCountTick = newTick;

    }
    
    frameCount60--;    
}





//----------------------------------------------------------
// Main SNES emulation loop.
//----------------------------------------------------------
void snesEmulatorLoop()
{
	// Main loop
    //GPU3DS.enableDebug = true;

    int snesFramesSkipped = 0;
    long snesFrameTotalActualTicks = 0;
    long snesFrameTotalAccurateTicks = 0;
 
    bool firstFrame = true;
    gpu3dsResetState();
    
    frameCount60 = 60;
    frameCountTick = 0;
    framesSkippedCount = 0;

    long startFrameTick = svcGetSystemTick();

    IPPU.RenderThisFrame = true;
	while (aptMainLoop())
	{
        t3dsStartTiming(1, "aptMainLoop");

        startFrameTick = svcGetSystemTick();
        
        APT_AppStatus appStatus = aptGetStatus();
        if (appStatus == APP_EXITING)
            return;
        
        updateFrameCount();

        gpu3dsStartNewFrame();
        gpu3dsEnableAlphaBlending();

		readJoypadButtons();

		gpu3dsSetRenderTargetToMainScreenTexture();
		gpu3dsUseShader(2);             // for drawing tiles
		gpu3dsClearRenderTarget();

        S9xMainLoop();
/*
        if (IPPU.RenderThisFrame)
        {
            for (int j = 0; j < 10; j++)
                for (int i = 0; i < 65536; i++)
                    forSlowSimulation[i] = Memory.VRAM[i] + (i*j);
        }
  */      

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
            gpu3dsDisableAlphaBlending();
            gpu3dsSetTextureEnvironmentReplaceColor();
            gpu3dsDrawRectangle(0, 0, 400, 240, 0, 0x000000ff);
            gpu3dsEnableAlphaBlending();
        }

        gpu3dsUseShader(1);             // for copying to screen.
        gpu3dsDisableAlphaBlending();

        gpu3dsBindTextureMainScreen(GPU_TEXUNIT0);
        gpu3dsSetTextureEnvironmentReplaceTexture0();
        
        gpu3dsAddQuadVertexes(
            settings3DS.ScreenX0, settings3DS.ScreenY0, settings3DS.ScreenX1, settings3DS.ScreenY1, 
            0, 0, 256, PPU.ScreenHeight, 0.1f);
        gpu3dsDrawVertexes();
        t3dsEndTiming(3);     
        
        if (!firstFrame)      
        {
            // ----------------------------------------------
            // Wait for the rendering to the SNES 
            // main/sub screen for the previous frame
            // to complete
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

        // For debugging only.
        /*if (!GPU3DS.isReal3DS)
        {
            snd3dsMixSamples();            
        }*/

#ifndef RELEASE
        if (GPU3DS.isReal3DS)
#endif
        {
     
            long currentTick = svcGetSystemTick();
            long actualTicksThisFrame = currentTick - startFrameTick;

            snesFrameTotalActualTicks += actualTicksThisFrame;  // actual time spent rendering past x frames.
            snesFrameTotalAccurateTicks += settings3DS.TicksPerFrame;  // time supposed to be spent rendering past x frames.

            //printf ("%7.5f - %7.5f = %7.5f ",
            //    snesFrameTotalActualTime, snesFrameTotalCorrectTime, 
            //    snesFrameTotalActualTime - snesFrameTotalCorrectTime);

            int isSlow = 0;


            long skew = snesFrameTotalAccurateTicks - snesFrameTotalActualTicks;

            //printf ("%ld %ld sk : %ld", snesFrameTotalAccurateTicks, snesFrameTotalActualTicks, skew);
            if (skew < 0)
            {
                // We've skewed out of the actual frame rate.
                // Once we skew beyond 0.1 (10%) frames slower, skip the frame.
                // 
                if (skew < -settings3DS.TicksPerFrame/10 && snesFramesSkipped < settings3DS.MaxFrameSkips)
                {
                    //printf (" s\n");
                    // Skewed beyond threshold. So now we skip.
                    //
                    IPPU.RenderThisFrame = false;
                    snesFramesSkipped++;

                    framesSkippedCount++;   // this is used for the stats display every 60 frames.
                }
                else
                {
                    //printf (" -\n");
                    IPPU.RenderThisFrame = true;

                    if (snesFramesSkipped >= settings3DS.MaxFrameSkips)
                    {
                        snesFramesSkipped = 0;
                        snesFrameTotalActualTicks = actualTicksThisFrame;
                        snesFrameTotalAccurateTicks = settings3DS.TicksPerFrame;
                    }
                }
            }
            else
            {

                float timeDiffInMilliseconds = (float)skew * 1000000 / TICKS_PER_SEC;

                //printf (" +\n");
                svcSleepThread ((long)(timeDiffInMilliseconds * 1000));

                IPPU.RenderThisFrame = true;

                // Reset the counters.
                //
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

    emulatorInitialize();    
    clearTopScreenWithLogo();
   
    getcwd(cwd, 1023);
    menuSelectFile();

    while (true)
    {
        APT_AppStatus appStatus = aptGetStatus();
        if (appStatus == APP_EXITING)
            goto quit;
        
        switch (GPU3DS.emulatorState)
        {
            case EMUSTATE_PAUSEMENU:
                menuPause();
                break;

            case EMUSTATE_EMULATE:
                menuKeyDown = 0xffffff;
                snesEmulatorLoop();
                break;

            case EMUSTATE_END:
                goto quit;

        }

    }
    
quit:
    printf("emulatorFinalize:\n");
    emulatorFinalize();
    printf ("Exiting...\n");
	exit(0);
}
