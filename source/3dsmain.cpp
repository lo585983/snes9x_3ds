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
    int     MaxFrameSkips = 1;              // 0 - disable, 
                                            // 1 - enable (max 1 consecutive skipped frame)
                                            // 2 - enable (max 2 consecutive skipped frames)
                                            // 3 - enable (max 3 consecutive skipped frames)

    int     ScreenStretch = 0;              // 0 - no stretch, 1 - stretch full, 2 - aspect fit

    int     ScreenX0, ScreenX1, ScreenY0, ScreenY1;

    long  TicksPerFrame;                  // Ticks per frame. Will change depending on PAL/NTSC
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
uint32 readJoypadButtons()
{
    hidScanInput();
    n3dsKeysHeld = hidKeysHeld();

    u32 keysDown = (~lastKeysHeld) & n3dsKeysHeld;

    /*
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
    */

    if (keysDown & KEY_TOUCH)
    {
        if (GPU3DS.emulatorState == EMUSTATE_EMULATE)
            GPU3DS.emulatorState = EMUSTATE_PAUSEMENU;
    }
    lastKeysHeld = n3dsKeysHeld;
    return keysDown;
    
}



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
        settings3DS.ScreenX0 = 0;
        settings3DS.ScreenX1 = 400;
        settings3DS.ScreenY0 = 0;
        settings3DS.ScreenY1 = 240;
    }
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
    
    cacheInit();
    gpu3dsClearAllRenderTargets();
    if (loaded)
    {
        printf ("  ROM Loaded...\n");
    }
    GPU3DS.emulatorState = EMUSTATE_EMULATE;

    if (Settings.PAL)
        settings3DS.TicksPerFrame = TICKS_PER_FRAME_PAL;
    else
        settings3DS.TicksPerFrame = TICKS_PER_FRAME_NTSC;

    consoleClear();
    settingsUpdateScreen();

}


//----------------------------------------------------------------------
// Menus
//----------------------------------------------------------------------
SMenuItem fileMenu[512];
char romFileNames[512][_MAX_PATH];


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
    for (int i = 0; i < files.size() && i < 512; i++)
    {
        strncpy(romFileNames[i], files[i].c_str(), _MAX_PATH);
        totalRomFileCount++;
        fileMenu[i].ID = i;
        fileMenu[i].Text = romFileNames[i];
        fileMenu[i].Checked = -1;
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
    S9xSetTabSubTitle(0, cwd);

    int selection = 0;
    do
    {
        selection = S9xMenuSelectItem();

        romFileName = romFileNames[selection];
        
        //romFileName = romFileNames[selection];
        if (romFileName[0] == 1)
        {
            if (strcmp(romFileName, "\x01 ..") == 0)
                fileGoToParentDirectory();
            else
                fileGoToChildDirectory(&romFileName[2]);

            
            fileGetAllFiles();
            S9xClearMenuTabs();
            S9xAddTab("Select ROM", fileMenu, totalRomFileCount);
            S9xSetTabSubTitle(0, cwd);
            selection = -1;
        }
    } while (selection == -1);

    snesLoadRom();
}


SMenuItem emulatorMenu[] = { 
    { 1000, "Resume Game", -1 }, 
    { -1, NULL, -1 }, 
    { 2001, "Save Slot #1", -1}, 
    { 2002, "Save Slot #2", -1}, 
    { 2003, "Save Slot #3", -1}, 
    { 2004, "Save Slot #4", -1}, 
    { -1, NULL, -1 }, 
    { 3001, "Load Slot #1", -1}, 
    { 3002, "Load Slot #2", -1}, 
    { 3003, "Load Slot #3", -1}, 
    { 3004, "Load Slot #4", -1}, 
    { -1, NULL, -1 }, 
    { 4001, "Reset SNES", -1} 
    };

SMenuItem optionMenu[] = { 
    { -1, "Frameskip", -1 }, 
    { 10000, "  Disabled                    ", 0}, 
    { 10001, "  Enabled (max 1 frame)       ", 1}, 
    { 10002, "  Enabled (max 2 frames)      ", 0}, 
    { 10003, "  Enabled (max 3 frames)      ", 0}, 
    { 10004, "  Enabled (max 4 frames)      ", 0}, 
    { -1, NULL, -1 }, 
    { -1, "Screen", -1 }, 
    { 11000, "  No stretch                  ", 1}, 
    { 11001, "  Stretch to fullscreen       ", 0}, 
    };


void menuPause()
{
    int emulatorMenuCount = sizeof(emulatorMenu) / sizeof(SMenuItem);
    int optionMenuCount = sizeof(optionMenu) / sizeof(SMenuItem);
    
    S9xClearMenuTabs();
    S9xAddTab("Emulator", emulatorMenu, emulatorMenuCount);
    S9xAddTab("Options", optionMenu, optionMenuCount);
    S9xAddTab("Select ROM", fileMenu, totalRomFileCount);
    S9xSetTabSubTitle(0, NULL);
    S9xSetTabSubTitle(2, cwd);

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
                snesLoadRom();
                return;
            }
        }
        else if (selection >= 2001 && selection <= 2010)
        {
            int slot = selection - 2000;
            char s[_MAX_PATH];
            sprintf(s, "Saving into slot %d...", slot);
            S9xShowWaitingMessage("Save State", s, "");

            sprintf(s, ".%d.frz", slot);
            Snapshot(S9xGetFilename (s)); 

            sprintf(s, "Slot %d save complete", slot);
            S9xAlertSuccess("Save State", s, "");
        }
        else if (selection >= 3001 && selection <= 3010)
        {
            int slot = selection - 3000;
            char s[_MAX_PATH];
            
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
                S9xAlertFailure("Load State", s, "");
            }
        }
        else if (selection == 4001)
        {
            S9xReset();
            cacheInit();
            gpu3dsClearAllRenderTargets();
            GPU3DS.emulatorState = EMUSTATE_EMULATE;
            consoleClear();
            return;

        }
        else if (selection / 1000 == 10)
        {
            settings3DS.MaxFrameSkips = selection % 1000;
            S9xUncheckGroup(optionMenu, optionMenuCount, selection);
            S9xCheckItemByID(optionMenu, optionMenuCount, selection);
        }
        else if (selection / 1000 == 11)
        {
            settings3DS.ScreenStretch = selection % 1000;
            S9xUncheckGroup(optionMenu, optionMenuCount, selection);
            S9xCheckItemByID(optionMenu, optionMenuCount, selection);
            settingsUpdateScreen();
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
    printf ("Initialization complete\n");
        
}


//--------------------------------------------------------
// Finalize the emulator.
//--------------------------------------------------------
void emulatorFinalize()
{
	hidExit();
	gfxExit();
	aptExit();
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
        
        consoleClear();

        if (framesSkippedCount)
            snprintf (frameCountBuffer, 69, "FPS: %2d.%1d (%d skipped)\n", fpsmul10 / 10, fpsmul10 % 10, framesSkippedCount);
        else
            snprintf (frameCountBuffer, 69, "FPS: %2d.%1d \n", fpsmul10 / 10, fpsmul10 % 10);

        ui3dsSetColor(0xffffff, 0);
        ui3dsDrawString(2, 2, 200, false, frameCountBuffer);

        frameCount60 = 60;
        framesSkippedCount = 0;


#ifndef RELEASE
        printf ("\n\n");
        for (int i=0; i<50; i++)
        {
            t3dsShowTotalTiming(i);
        } 
        t3dsResetTimings();
#endif
        frameCountTick = newTick;
    }
    
    frameCount60--;    
}


/*
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
*/



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


        if (GPU3DS.isReal3DS)
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

            //printf ("%ld %ld sk : %ld\n", snesFrameTotalAccurateTicks, snesFrameTotalActualTicks, skew);
            if (skew < 0)
            {
                // We've skewed out of the actual frame rate.
                // Once we skew beyond 0.2 frames slower, skip the frame.
                // 
                if (skew < -settings3DS.TicksPerFrame/5 && snesFramesSkipped < settings3DS.MaxFrameSkips)
                {
                    //printf ("s");
                    // Skewed beyond threshold. So now we skip.
                    //
                    IPPU.RenderThisFrame = false;
                    snesFramesSkipped++;

                    framesSkippedCount++;   // this is used for the stats display every 60 frames.
                }
                else
                {
                    //printf ("-");
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

                //printf ("+");
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
    //testUIFramework();

    emulatorInitialize();    
    clearTopScreenWithLogo();
   
    getcwd(cwd, 1023);
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
    
    emulatorFinalize();
	return 0;
}
