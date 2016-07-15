/*******************************************************************************
  Snes9x - Portable Super Nintendo Entertainment System (TM) emulator.
 
  (c) Copyright 1996 - 2002 Gary Henderson (gary.henderson@ntlworld.com) and
                            Jerremy Koot (jkoot@snes9x.com)

  (c) Copyright 2001 - 2004 John Weidman (jweidman@slip.net)

  (c) Copyright 2002 - 2004 Brad Jorsch (anomie@users.sourceforge.net),
                            funkyass (funkyass@spam.shaw.ca),
                            Joel Yliluoma (http://iki.fi/bisqwit/)
                            Kris Bleakley (codeviolation@hotmail.com),
                            Matthew Kendora,
                            Nach (n-a-c-h@users.sourceforge.net),
                            Peter Bortas (peter@bortas.org) and
                            zones (kasumitokoduck@yahoo.com)

  C4 x86 assembler and some C emulation code
  (c) Copyright 2000 - 2003 zsKnight (zsknight@zsnes.com),
                            _Demo_ (_demo_@zsnes.com), and Nach

  C4 C++ code
  (c) Copyright 2003 Brad Jorsch

  DSP-1 emulator code
  (c) Copyright 1998 - 2004 Ivar (ivar@snes9x.com), _Demo_, Gary Henderson,
                            John Weidman, neviksti (neviksti@hotmail.com),
                            Kris Bleakley, Andreas Naive

  DSP-2 emulator code
  (c) Copyright 2003 Kris Bleakley, John Weidman, neviksti, Matthew Kendora, and
                     Lord Nightmare (lord_nightmare@users.sourceforge.net

  OBC1 emulator code
  (c) Copyright 2001 - 2004 zsKnight, pagefault (pagefault@zsnes.com) and
                            Kris Bleakley
  Ported from x86 assembler to C by sanmaiwashi

  SPC7110 and RTC C++ emulator code
  (c) Copyright 2002 Matthew Kendora with research by
                     zsKnight, John Weidman, and Dark Force

  S-DD1 C emulator code
  (c) Copyright 2003 Brad Jorsch with research by
                     Andreas Naive and John Weidman
 
  S-RTC C emulator code
  (c) Copyright 2001 John Weidman
  
  ST010 C++ emulator code
  (c) Copyright 2003 Feather, Kris Bleakley, John Weidman and Matthew Kendora

  Super FX x86 assembler emulator code 
  (c) Copyright 1998 - 2003 zsKnight, _Demo_, and pagefault 

  Super FX C emulator code 
  (c) Copyright 1997 - 1999 Ivar, Gary Henderson and John Weidman


  SH assembler code partly based on x86 assembler code
  (c) Copyright 2002 - 2004 Marcus Comstedt (marcus@mc.pp.se) 

 
  Specific ports contains the works of other authors. See headers in
  individual files.
 
  Snes9x homepage: http://www.snes9x.com
 
  Permission to use, copy, modify and distribute Snes9x in both binary and
  source form, for non-commercial purposes, is hereby granted without fee,
  providing that this license information and copyright notice appear with
  all copies and any derived work.
 
  This software is provided 'as-is', without any express or implied
  warranty. In no event shall the authors be held liable for any damages
  arising from the use of this software.
 
  Snes9x is freeware for PERSONAL USE only. Commercial users should
  seek permission of the copyright holders first. Commercial use includes
  charging money for Snes9x or software derived from Snes9x.
 
  The copyright holders request that bug fixes and improvements to the code
  should be forwarded to them so everyone can benefit from the modifications
  in future versions.
 
  Super NES and Super Nintendo Entertainment System are trademarks of
  Nintendo Co., Limited and its subsidiary companies.
*******************************************************************************/

#include "snes9x.h"

#include "memmap.h"
#include "ppu.h"
#include "cpuexec.h"
#include "display.h"
#include "gfx.h"
#include "apu.h"
#include "cheats.h"
#include "screenshot.h"

#include "3dsopt.h"
#include "3dsgpu.h"
#include <3ds.h>

#define M7 19
#define M8 19

void output_png();
void ComputeClipWindows ();
static void S9xDisplayFrameRate ();
static void S9xDisplayString (const char *string);

extern uint8 BitShifts[8][4];
extern uint8 TileShifts[8][4];
extern uint8 PaletteShifts[8][4];
extern uint8 PaletteMasks[8][4];
extern uint8 Depths[8][4];
extern uint8 BGSizes [2];

extern NormalTileRenderer DrawTilePtr;
extern ClippedTileRenderer DrawClippedTilePtr;
extern NormalTileRenderer DrawHiResTilePtr;
extern ClippedTileRenderer DrawHiResClippedTilePtr;
extern LargePixelRenderer DrawLargePixelPtr;

extern struct SBG BG;

extern struct SLineData LineData[240];
extern struct SLineMatrixData LineMatrixData [240];

extern uint8  Mode7Depths [2];

#define ON_MAIN(N) \
(GFX.r212c & (1 << (N)) && \
 !(PPU.BG_Forced & (1 << (N))))

#define SUB_OR_ADD(N) \
(GFX.r2131 & (1 << (N)))

#define ON_SUB(N) \
((GFX.r2130 & 0x30) != 0x30 && \
 (GFX.r2130 & 2) && \
 (GFX.r212d & (1 << N)) && \
 !(PPU.BG_Forced & (1 << (N))))

#define ANYTHING_ON_SUB \
((GFX.r2130 & 0x30) != 0x30 && \
 (GFX.r2130 & 2) && \
 (GFX.r212d & 0x1f))

#define ADD_OR_SUB_ON_ANYTHING \
(GFX.r2131 & 0x3f)


			
#define DrawTileLater(Tile, Offset, StartLine, LineCount) \
	{ \
		BG.DrawTileParameters[bg][BG.DrawTileCount[bg]][0] = 0; \
		BG.DrawTileParameters[bg][BG.DrawTileCount[bg]][1] = Tile; \
		BG.DrawTileParameters[bg][BG.DrawTileCount[bg]][2] = Offset; \
		BG.DrawTileParameters[bg][BG.DrawTileCount[bg]][3] = StartLine; \
		BG.DrawTileParameters[bg][BG.DrawTileCount[bg]][4] = LineCount; \
		BG.DrawTileCount[bg]++; \
	}
	
#define DrawClippedTileLater(Tile, Offset, StartPixel, Width, StartLine, LineCount) \
	{ \
		BG.DrawTileParameters[bg][BG.DrawTileCount[bg]][0] = 1; \
		BG.DrawTileParameters[bg][BG.DrawTileCount[bg]][1] = Tile; \
		BG.DrawTileParameters[bg][BG.DrawTileCount[bg]][2] = Offset; \ 
		BG.DrawTileParameters[bg][BG.DrawTileCount[bg]][3] = StartPixel; \
		BG.DrawTileParameters[bg][BG.DrawTileCount[bg]][4] = Width; \
		BG.DrawTileParameters[bg][BG.DrawTileCount[bg]][5] = StartLine; \
		BG.DrawTileParameters[bg][BG.DrawTileCount[bg]][6] = LineCount; \
		BG.DrawTileCount[bg]++; \
	}
	
#define DrawOBJTileLater(bg, Tile, ScreenX, ScreenY, TextureYOffset) \
	{ \
		int newIndex = BG.DrawTileLaterParametersCount++; \
		int newBGIndex = BG.DrawTileLaterBGIndexCount[bg]++; \
		BG.DrawTileLaterBGIndex[bg][newBGIndex] = newIndex; \
		BG.DrawTileLaterParameters[newIndex][0] = 0; \
		BG.DrawTileLaterParameters[newIndex][1] = Tile; \
		BG.DrawTileLaterParameters[newIndex][2] = ScreenX; \
		BG.DrawTileLaterParameters[newIndex][3] = ScreenY; \
		BG.DrawTileLaterParameters[newIndex][4] = TextureYOffset; \
	}
	
#define DrawClippedOBJTileLater(bg, Tile, Offset, StartPixel, Width, StartLine, LineCount) \
	{ \
		int newIndex = BG.DrawTileLaterParametersCount++; \
		int newBGIndex = BG.DrawTileLaterBGIndexCount[bg]++; \
		BG.DrawTileLaterBGIndex[bg][newBGIndex] = newIndex; \
		BG.DrawTileLaterParameters[newIndex][0] = 1; \
		BG.DrawTileLaterParameters[newIndex][1] = Tile; \
		BG.DrawTileLaterParameters[newIndex][2] = Offset; \ 
		BG.DrawTileLaterParameters[newIndex][3] = StartPixel; \
		BG.DrawTileLaterParameters[newIndex][4] = Width; \
		BG.DrawTileLaterParameters[newIndex][5] = StartLine; \
		BG.DrawTileLaterParameters[newIndex][6] = LineCount; \
	}
		

//-------------------------------------------------------------------
// Render the backdrop
//-------------------------------------------------------------------
void S9xDrawBackdropHardware(bool sub, int depth)
{
    uint32 starty = GFX.StartY;
    uint32 endy = GFX.EndY;

	
	uint32 backColor = IPPU.ScreenColors [0];
	if (PPU.ForcedBlanking)
		backColor = 0x000000ff;			// black
	else
	{
		if (!sub)
		{
			backColor = 
				((backColor & (0x1F << 11)) << 16) | 
				((backColor & (0x1F << 6)) << 13)| 
				((backColor & (0x1F << 1)) << 10) | 0xFF;
		}
		else
		{
			backColor = 
				(PPU.FixedColourRed << (3 + 24)) |
				(PPU.FixedColourGreen << (3 + 16)) |
				(PPU.FixedColourBlue << (3 + 8)) |
				0xff;
			
		}
	}

	gpu3dsDrawRectangle(0, starty, 256, endy + 1, depth, backColor);
}


//-------------------------------------------------------------------
// Convert tile to 8-bit.
//-------------------------------------------------------------------
uint8 S9xConvertTileTo8Bit (uint8 *pCache, uint32 TileAddr)
{
    //printf ("Tile Addr: %04x\n", TileAddr);
    uint8 *tp = &Memory.VRAM[TileAddr];
    uint32 *p = (uint32 *) pCache;
    uint32 non_zero = 0;
    uint8 line;

    switch (BG.BitShift)
    {
    case 8:
		for (line = 8; line != 0; line--, tp += 2)
		{
			uint32 p1 = 0;
			uint32 p2 = 0;
			uint8 pix;

			if ((pix = *(tp + 0)))
			{
				p1 |= odd_high[0][pix >> 4];
				p2 |= odd_low[0][pix & 0xf];
			}
			if ((pix = *(tp + 1)))
			{
				p1 |= even_high[0][pix >> 4];
				p2 |= even_low[0][pix & 0xf];
			}
			if ((pix = *(tp + 16)))
			{
				p1 |= odd_high[1][pix >> 4];
				p2 |= odd_low[1][pix & 0xf];
			}
			if ((pix = *(tp + 17)))
			{
				p1 |= even_high[1][pix >> 4];
				p2 |= even_low[1][pix & 0xf];
			}
			if ((pix = *(tp + 32)))
			{
				p1 |= odd_high[2][pix >> 4];
				p2 |= odd_low[2][pix & 0xf];
			}
			if ((pix = *(tp + 33)))
			{
				p1 |= even_high[2][pix >> 4];
				p2 |= even_low[2][pix & 0xf];
			}
			if ((pix = *(tp + 48)))
			{
				p1 |= odd_high[3][pix >> 4];
				p2 |= odd_low[3][pix & 0xf];
			}
			if ((pix = *(tp + 49)))
			{
				p1 |= even_high[3][pix >> 4];
				p2 |= even_low[3][pix & 0xf];
			}
			*p++ = p1;
			*p++ = p2;
			non_zero |= p1 | p2;
		}
		break;

    case 4:
		for (line = 8; line != 0; line--, tp += 2)
		{
			uint32 p1 = 0;
			uint32 p2 = 0;
			uint8 pix;
			if ((pix = *(tp + 0)))
			{
				p1 |= odd_high[0][pix >> 4];
				p2 |= odd_low[0][pix & 0xf];
			}
			if ((pix = *(tp + 1)))
			{
				p1 |= even_high[0][pix >> 4];
				p2 |= even_low[0][pix & 0xf];
			}
			if ((pix = *(tp + 16)))
			{
				p1 |= odd_high[1][pix >> 4];
				p2 |= odd_low[1][pix & 0xf];
			}
			if ((pix = *(tp + 17)))
			{
				p1 |= even_high[1][pix >> 4];
				p2 |= even_low[1][pix & 0xf];
			}
			*p++ = p1;
			*p++ = p2;
			non_zero |= p1 | p2;
		}
		break;

    case 2:
		for (line = 8; line != 0; line--, tp += 2)
		{
			uint32 p1 = 0;
			uint32 p2 = 0;
			uint8 pix;
			if ((pix = *(tp + 0)))
			{
				p1 |= odd_high[0][pix >> 4];
				p2 |= odd_low[0][pix & 0xf];
			}
			if ((pix = *(tp + 1)))
			{
				p1 |= even_high[0][pix >> 4];
				p2 |= even_low[0][pix & 0xf];
			}
			*p++ = p1;
			*p++ = p2;
			non_zero |= p1 | p2;
		}
		break;
    }
    return (non_zero ? TRUE : BLANK_TILE);
}

//-------------------------------------------------------------------
// Draw a clipped BG tile using 3D hardware.
//-------------------------------------------------------------------
inline void __attribute__((always_inline)) S9xDrawBGClippedTileHardware (
	uint32 snesTile, uint32 screenOffset,
	uint32 startX, uint32 width,
	uint32 startLine, uint32 height)
{
	// Prepare tile for rendering
	//
    uint8 *pCache;  
    uint16 *pCache16; 

    uint32 TileAddr = BG.TileAddress + ((snesTile & 0x3ff) << BG.TileShift); 
    
    uint32 TileNumber; 
    pCache = &BG.Buffer[(TileNumber = (TileAddr >> BG.TileShift)) << 6]; 

    if (!BG.Buffered [TileNumber]) 
    { 
	    BG.Buffered[TileNumber] = S9xConvertTileTo8Bit (pCache, TileAddr); 
        if (BG.Buffered [TileNumber] == BLANK_TILE) 
            return; 
			
        GFX.VRAMPaletteFrame[TileAddr][0] = 0; 
        GFX.VRAMPaletteFrame[TileAddr][1] = 0; 
        GFX.VRAMPaletteFrame[TileAddr][2] = 0; 
        GFX.VRAMPaletteFrame[TileAddr][3] = 0; 
        GFX.VRAMPaletteFrame[TileAddr][4] = 0; 
        GFX.VRAMPaletteFrame[TileAddr][5] = 0; 
        GFX.VRAMPaletteFrame[TileAddr][6] = 0; 
        GFX.VRAMPaletteFrame[TileAddr][7] = 0; 
        GFX.VRAMPaletteFrame[TileAddr][8] = 0; 
        GFX.VRAMPaletteFrame[TileAddr][9] = 0; 
        GFX.VRAMPaletteFrame[TileAddr][10] = 0; 
        GFX.VRAMPaletteFrame[TileAddr][11] = 0; 
        GFX.VRAMPaletteFrame[TileAddr][12] = 0; 
        GFX.VRAMPaletteFrame[TileAddr][13] = 0; 
        GFX.VRAMPaletteFrame[TileAddr][14] = 0; 
        GFX.VRAMPaletteFrame[TileAddr][15] = 0; 
    } 

    if (BG.Buffered [TileNumber] == BLANK_TILE) 
	    return; 
		
	int texturePos = 0;
    
    uint32 l; 
    uint8 pal; 
    if (BG.DirectColourMode) 
    { 
        if (IPPU.DirectColourMapsNeedRebuild) 
            S9xBuildDirectColourMaps (); 
        pal = (snesTile >> 10) & BG.PaletteMask; 
		GFX.ScreenColors = DirectColourMaps [pal]; 
		texturePos = cacheGetTexturePositionFast(COMPOSE_HASH(TileAddr, pal));
        if (GFX.VRAMPaletteFrame[TileAddr][pal] != GFX.PaletteFrame[pal + BG.StartPalette / 16]) 
        { 
            GFX.VRAMPaletteFrame[TileAddr][pal] = GFX.PaletteFrame[pal + BG.StartPalette / 16]; 
			
			gpu3dsCacheToTexturePosition(pCache, GFX.ScreenColors, texturePos);
        } 
    } 
    else 
    { 
        pal = (snesTile >> 10) & BG.PaletteMask; 
        GFX.ScreenColors = &IPPU.ScreenColors [(pal << BG.PaletteShift) + BG.StartPalette]; 
		texturePos = cacheGetTexturePositionFast(COMPOSE_HASH(TileAddr, pal));
		//printf ("%d\n", texturePos);
        if (GFX.VRAMPaletteFrame[TileAddr][pal] != GFX.PaletteFrame[pal + BG.StartPalette / 16]) 
        { 
            GFX.VRAMPaletteFrame[TileAddr][pal] = GFX.PaletteFrame[pal + BG.StartPalette / 16]; 

			//printf ("cache %d\n", texturePos);
			gpu3dsCacheToTexturePosition(pCache, GFX.ScreenColors, texturePos);
        } 
    }
	
	// Render tile
	//
	screenOffset += startX;
	int x0 = screenOffset & 0xFF;
	int y0 = screenOffset >> 8;
	int x1 = x0 + width;
	int y1 = y0 + height;
	
	int txBase = (texturePos & 0x7F) << 3;
	int tyBase = (texturePos >> 7) << 3;
	
	int tx0 = startX;
	int ty0 = startLine >> 3;
	int tx1 = tx0 + width;
	int ty1 = ty0 + height;
	
	if (snesTile & V_FLIP)
	{
		ty0 = 8 - ty0;
		ty1 = 8 - ty1;
	} 
	if (snesTile & H_FLIP)
	{
		tx0 = 8 - tx0; 
		tx1 = 8 - tx1;
	}
	
	//printf ("Draw: %d %d %d, %d %d %d %d - %d %d %d %d (%d)\n", screenOffset, startX, startLine, x0, y0, x1, y1, txBase + tx0, tyBase + ty0, txBase + tx1, tyBase + ty1, texturePos);
	gpu3dsAddTileVertexes(
		x0, y0, x1, y1, 
		txBase + tx0, tyBase + ty0, 
		txBase + tx1, tyBase + ty1, BG.Depth);
}


//-------------------------------------------------------------------
// Draw tile using 3D hardware
//-------------------------------------------------------------------
inline void __attribute__((always_inline)) S9xDrawBGTileHardware (uint32 snesTile, uint32 screenOffset, uint32 startLine, uint32 height)
{
	S9xDrawBGClippedTileHardware (snesTile, screenOffset, 0, 8, startLine, height);
}



//-------------------------------------------------------------------
// Draw an SNES background priority 1 tiles using 3D hardware
//-------------------------------------------------------------------
void S9xDrawBackgroundHardwarePriority1 (uint32 BGMode, uint32 bg, bool sub, int depth)
{
    GFX.PixSize = 1;
	
    BG.TileSize = BGSizes [PPU.BG[bg].BGSize];
    BG.BitShift = BitShifts[BGMode][bg];
    BG.TileShift = TileShifts[BGMode][bg];
    BG.TileAddress = PPU.BG[bg].NameBase << 1;
    BG.NameSelect = 0;
    BG.Buffer = IPPU.TileCache [Depths [BGMode][bg]];
    BG.Buffered = IPPU.TileCached [Depths [BGMode][bg]];
    BG.PaletteShift = PaletteShifts[BGMode][bg];
    BG.PaletteMask = PaletteMasks[BGMode][bg];
    BG.DirectColourMode = (BGMode == 3 || BGMode == 4) && bg == 0 &&
		(GFX.r2130 & 1);
	BG.Depth = depth;
	BG.SubY = sub ? 256 : 0;
	
	/*
	We may need to add this back later...
    if (PPU.BGMosaic [bg] && PPU.Mosaic > 1)
    {
		DrawBackgroundMosaic (BGMode, bg, Z1, Z2);
		return;
    }
    switch (BGMode)
    {
		case 2:
		case 4: // Used by Puzzle Bobble
			DrawBackgroundOffset (BGMode, bg, Z1, Z2);
			return;
			
		case 5:
		case 6: // XXX: is also offset per tile.
			if (Settings.SupportHiRes)
			{
				DrawBackgroundMode5 (BGMode, bg, Z1, Z2);
				return;
			}
			break;
    }
    CHECK_SOUND();
	*/
    if (BGMode == 0)
		BG.StartPalette = bg << 5;
    else BG.StartPalette = 0;

	for (int i = 0; i < BG.DrawTileCount[bg]; i++)
	{
		if (BG.DrawTileParameters[bg][i][0] == 0)
		{
			// unclipped tile.
			S9xDrawBGTileHardware (
				BG.DrawTileParameters[bg][i][1], BG.DrawTileParameters[bg][i][2], 
				BG.DrawTileParameters[bg][i][3], BG.DrawTileParameters[bg][i][4]);
		}
		else
		{
			// clipped tile.
			S9xDrawBGClippedTileHardware (
				BG.DrawTileParameters[bg][i][1], BG.DrawTileParameters[bg][i][2], BG.DrawTileParameters[bg][i][3], 
				BG.DrawTileParameters[bg][i][4], BG.DrawTileParameters[bg][i][5], BG.DrawTileParameters[bg][i][6]);
		}
	}
	//printf ("BG %d P1 count (%d)\n", bg, BG.DrawTileCount[bg]);
}


//-------------------------------------------------------------------
// Draw an SNES background priority 0 tiles using 3D hardware
//-------------------------------------------------------------------
void S9xDrawBackgroundHardware (uint32 BGMode, uint32 bg, bool sub, int depth, int priority)
{
    GFX.PixSize = 1;
	
    BG.TileSize = BGSizes [PPU.BG[bg].BGSize];
    BG.BitShift = BitShifts[BGMode][bg];
    BG.TileShift = TileShifts[BGMode][bg];
    BG.TileAddress = PPU.BG[bg].NameBase << 1;
    BG.NameSelect = 0;
    BG.Buffer = IPPU.TileCache [Depths [BGMode][bg]];
    BG.Buffered = IPPU.TileCached [Depths [BGMode][bg]];
    BG.PaletteShift = PaletteShifts[BGMode][bg];
    BG.PaletteMask = PaletteMasks[BGMode][bg];
    BG.DirectColourMode = (BGMode == 3 || BGMode == 4) && bg == 0 &&
		(GFX.r2130 & 1);
	BG.DrawTileCount[bg] = 0;
	BG.Depth = depth;
	BG.SubY = sub ? 256 : 0;
	
	/*
    if (PPU.BGMosaic [bg] && PPU.Mosaic > 1)
    {
		DrawBackgroundMosaic (BGMode, bg, Z1, Z2);
		return;
    }
	*/
	/*
    switch (BGMode)
    {
		case 2:
		case 4: // Used by Puzzle Bobble
			DrawBackgroundOffset (BGMode, bg, Z1, Z2);
			return;
			
		case 5:
		case 6: // XXX: is also offset per tile.
			if (Settings.SupportHiRes)
			{
				DrawBackgroundMode5 (BGMode, bg, Z1, Z2);
				return;
			}
			break;
    }
	*/
	
    uint32 Tile;
    uint16 *SC0;
    uint16 *SC1;
    uint16 *SC2;
    uint16 *SC3;
    uint32 Width;
    //uint8 depths [2] = {Z1, Z2};
    
    if (BGMode == 0)
		BG.StartPalette = bg << 5;
    else BG.StartPalette = 0;
	
    SC0 = (uint16 *) &Memory.VRAM[PPU.BG[bg].SCBase << 1];
	
    if (PPU.BG[bg].SCSize & 1)
		SC1 = SC0 + 1024;
    else
		SC1 = SC0;
	
	if(SC1>=(unsigned short*)(Memory.VRAM+0x10000))
		SC1=(uint16*)&Memory.VRAM[((uint8*)SC1-&Memory.VRAM[0])%0x10000];
	
    if (PPU.BG[bg].SCSize & 2)
		SC2 = SC1 + 1024;
    else
		SC2 = SC0;
	
	if(((uint8*)SC2-Memory.VRAM)>=0x10000)
		SC2-=0x08000;
	
    if (PPU.BG[bg].SCSize & 1)
		SC3 = SC2 + 1024;
    else
		SC3 = SC2;
    
	if(((uint8*)SC3-Memory.VRAM)>=0x10000)
		SC3-=0x08000;
	
    int Lines;
    int OffsetMask;
    int OffsetShift;
	
    if (BG.TileSize == 16)
    {
		OffsetMask = 0x3ff;
		OffsetShift = 4;
    }
    else
    {
		OffsetMask = 0x1ff;
		OffsetShift = 3;
    }
	
    for (uint32 Y = GFX.StartY; Y <= GFX.EndY; Y += Lines)
    {
		uint32 VOffset = LineData [Y].BG[bg].VOffset;
		uint32 HOffset = LineData [Y].BG[bg].HOffset;
		
		int VirtAlign = (Y + VOffset) & 7;
		
		for (Lines = 1; Lines < 8 - VirtAlign; Lines++)
			if ((VOffset != LineData [Y + Lines].BG[bg].VOffset) ||
				(HOffset != LineData [Y + Lines].BG[bg].HOffset))
				break;
			
		if (Y + Lines > GFX.EndY)
			Lines = GFX.EndY + 1 - Y;
		
		//if (GFX.EndY - GFX.StartY < 10)
		//	printf ("bg:%d Y/L:%3d/%3d OFS:%d,%d\n", bg, Y, Lines, HOffset, VOffset);  
			
		VirtAlign <<= 3;
		
		uint32 ScreenLine = (VOffset + Y) >> OffsetShift;
		uint32 t1;
		uint32 t2;
		if (((VOffset + Y) & 15) > 7)
		{
			t1 = 16;
			t2 = 0;
		}
		else
		{
			t1 = 0;
			t2 = 16;
		}
		uint16 *b1;
		uint16 *b2;
		
		if (ScreenLine & 0x20)
			b1 = SC2, b2 = SC3;
		else
			b1 = SC0, b2 = SC1;
		
		b1 += (ScreenLine & 0x1f) << 5;
		b2 += (ScreenLine & 0x1f) << 5;
		
		int clipcount = GFX.pCurrentClip->Count [bg];
		if (!clipcount)
			clipcount = 1;
		for (int clip = 0; clip < clipcount; clip++)
		{
			uint32 Left;
			uint32 Right;
			
			if (!GFX.pCurrentClip->Count [bg])
			{
				Left = 0;
				Right = 256;
			}
			else
			{
				Left = GFX.pCurrentClip->Left [clip][bg];
				Right = GFX.pCurrentClip->Right [clip][bg];
				
				if (Right <= Left)
					continue;
			}
			
			//uint32 s = Left * GFX.PixSize + Y * GFX.PPL;
			uint32 s = Left * GFX.PixSize + Y * 256;		// Once hardcoded, Hires mode no longer supported.
			//printf ("s = %d, Lines = %d\n", s, Lines);
			uint32 HPos = (HOffset + Left) & OffsetMask;
			
			uint32 Quot = HPos >> 3;
			uint32 Count = 0;
			
			uint16 *t;
			if (BG.TileSize == 8)
			{
				if (Quot > 31)
					t = b2 + (Quot & 0x1f);
				else
					t = b1 + Quot;
			}
			else
			{
				if (Quot > 63)
					t = b2 + ((Quot >> 1) & 0x1f);
				else
					t = b1 + (Quot >> 1);
			}
			
			Width = Right - Left;
			// Left hand edge clipped tile
			if (HPos & 7)
			{
				uint32 Offset = (HPos & 7);
				Count = 8 - Offset;
				if (Count > Width)
					Count = Width;
				s -= Offset * GFX.PixSize;
				Tile = READ_2BYTES(t);
				
				int tpriority = (Tile & 0x2000) >> 13;
				//if (tpriority == priority) 
				{
					//GFX.Z1 = GFX.Z2 = depths [(Tile & 0x2000) >> 13];
					
					if (BG.TileSize == 8)
					{
						if (tpriority == 0)
							S9xDrawBGClippedTileHardware (Tile, s, Offset, Count, VirtAlign, Lines);
						else 
							DrawClippedTileLater (Tile, s, Offset, Count, VirtAlign, Lines);
					}
					else
					{
						if (!(Tile & (V_FLIP | H_FLIP)))
						{
							// Normal, unflipped
							if (tpriority == 0)
								S9xDrawBGClippedTileHardware (Tile + t1 + (Quot & 1), s, Offset, Count, VirtAlign, Lines);
							else
								DrawClippedTileLater (Tile + t1 + (Quot & 1), s, Offset, Count, VirtAlign, Lines);
						}
						else
							if (Tile & H_FLIP)
							{
								if (Tile & V_FLIP)
								{
									// H & V flip
									if (tpriority == 0)
										S9xDrawBGClippedTileHardware (Tile + t2 + 1 - (Quot & 1), s, Offset, Count, VirtAlign, Lines);
									else
										DrawClippedTileLater (Tile + t2 + 1 - (Quot & 1), s, Offset, Count, VirtAlign, Lines);
								}
								else
								{
									// H flip only
									if (tpriority == 0)
										S9xDrawBGClippedTileHardware (Tile + t1 + 1 - (Quot & 1), s, Offset, Count, VirtAlign, Lines);
									else
										DrawClippedTileLater (Tile + t1 + 1 - (Quot & 1), s, Offset, Count, VirtAlign, Lines);
								}
							}
							else
							{
								// V flip only
								if (tpriority == 0)
									S9xDrawBGClippedTileHardware (Tile + t2 + (Quot & 1), s, Offset, Count, VirtAlign, Lines);
								else
									DrawClippedTileLater (Tile + t2 + (Quot & 1), s, Offset, Count, VirtAlign, Lines);
							}
					}
				}
				
				if (BG.TileSize == 8)
				{
					t++;
					if (Quot == 31)
						t = b2;
					else if (Quot == 63)
						t = b1;
				}
				else
				{
					t += Quot & 1;
					if (Quot == 63)
						t = b2;
					else if (Quot == 127)
						t = b1;
				}
				Quot++;
				s += 8 * GFX.PixSize;
				
			}
			
			// Middle, unclipped tiles
			Count = Width - Count;
			int Middle = Count >> 3;
			Count &= 7;
			for (int C = Middle; C > 0; s += 8 * GFX.PixSize, Quot++, C--)
			{
				Tile = READ_2BYTES(t);
				
				int tpriority = (Tile & 0x2000) >> 13;
				//if (tpriority == priority) 
				{
				
					//GFX.Z1 = GFX.Z2 = depths [(Tile & 0x2000) >> 13];
					
					if (BG.TileSize != 8)
					{
						if (Tile & H_FLIP)
						{
							// Horizontal flip, but what about vertical flip ?
							if (Tile & V_FLIP)
							{
								// Both horzontal & vertical flip
								if (tpriority == 0)
									S9xDrawBGTileHardware (Tile + t2 + 1 - (Quot & 1), s, VirtAlign, Lines);
								else
									DrawTileLater (Tile + t2 + 1 - (Quot & 1), s, VirtAlign, Lines);
							}
							else
							{
								// Horizontal flip only
								if (tpriority == 0)
									S9xDrawBGTileHardware (Tile + t1 + 1 - (Quot & 1), s, VirtAlign, Lines);
								else
									DrawTileLater (Tile + t1 + 1 - (Quot & 1), s, VirtAlign, Lines);
							}
						}
						else
						{
							// No horizontal flip, but is there a vertical flip ?
							if (Tile & V_FLIP)
							{
								// Vertical flip only
								if (tpriority == 0)
									S9xDrawBGTileHardware (Tile + t2 + (Quot & 1), s, VirtAlign, Lines);
								else
									DrawTileLater (Tile + t2 + (Quot & 1), s, VirtAlign, Lines);
							}
							else
							{
								// Normal unflipped
								if (tpriority == 0)
									S9xDrawBGTileHardware (Tile + t1 + (Quot & 1), s, VirtAlign, Lines);
								else
									DrawTileLater (Tile + t1 + (Quot & 1), s, VirtAlign, Lines);
							}
						}
					}
					else
					{
						if (tpriority == 0)
							S9xDrawBGTileHardware (Tile, s, VirtAlign, Lines);
						else
							DrawTileLater (Tile, s, VirtAlign, Lines);
					}
				}
									
				if (BG.TileSize == 8)
				{
					t++;
					if (Quot == 31)
						t = b2;
					else
						if (Quot == 63)
							t = b1;
				}
				else
				{
					t += Quot & 1;
					if (Quot == 63)
						t = b2;
					else
						if (Quot == 127)
							t = b1;
				}
			}
			// Right-hand edge clipped tiles
			if (Count)
			{
				Tile = READ_2BYTES(t);
				
				int tpriority = (Tile & 0x2000) >> 13;
				//if (tpriority == priority) 
				{
					//GFX.Z1 = GFX.Z2 = depths [(Tile & 0x2000) >> 13];
					
					if (BG.TileSize == 8)
					{
						if (tpriority == 0)
							S9xDrawBGClippedTileHardware (Tile, s, 0, Count, VirtAlign, Lines);
						else
							DrawClippedTileLater (Tile, s, 0, Count, VirtAlign, Lines);
					}
					else
					{
						if (!(Tile & (V_FLIP | H_FLIP)))
						{
							// Normal, unflipped
							if (tpriority == 0)
								S9xDrawBGClippedTileHardware (Tile + t1 + (Quot & 1), s, 0, Count, VirtAlign, Lines);
							else
								DrawClippedTileLater (Tile + t1 + (Quot & 1), s, 0, Count, VirtAlign, Lines);
						}
						else if (Tile & H_FLIP)
						{
							if (Tile & V_FLIP)
							{
								// H & V flip
								if (tpriority == 0)
									S9xDrawBGClippedTileHardware (Tile + t2 + 1 - (Quot & 1), s, 0, Count, VirtAlign, Lines);
								else
									DrawClippedTileLater (Tile + t2 + 1 - (Quot & 1), s, 0, Count, VirtAlign, Lines);
							}
							else
							{
								// H flip only
								if (tpriority == 0)
									S9xDrawBGClippedTileHardware (Tile + t1 + 1 - (Quot & 1), s, 0, Count, VirtAlign, Lines);
								else
									DrawClippedTileLater (Tile + t1 + 1 - (Quot & 1), s, 0, Count, VirtAlign, Lines);
							}
						}
						else
						{
							// V flip only
							if (tpriority == 0)
								S9xDrawBGClippedTileHardware (Tile + t2 + (Quot & 1), s, 0, Count, VirtAlign, Lines);
							else
								DrawClippedTileLater (Tile + t2 + (Quot & 1), s, 0, Count, VirtAlign, Lines);	
						}
					}
				}
			}
		}
    }
	
	//printf ("BG %d P0\n", bg);
}




//-------------------------------------------------------------------
// Draw a clipped BG tile using 3D hardware.
//-------------------------------------------------------------------
inline void __attribute__((always_inline)) S9xDrawBGClippedTileHardwareInline (
    int tileSize, int tileShift, int paletteShift, int paletteMask, int startPalette, bool directColourMode,
	uint32 snesTile, uint32 screenOffset,
	uint32 startX, uint32 width,
	uint32 startLine, uint32 height)
{
	// Prepare tile for rendering
	//
    uint8 *pCache;  
    uint16 *pCache16; 

    uint32 TileAddr = BG.TileAddress + ((snesTile & 0x3ff) << tileShift); 
    
    uint32 TileNumber; 
    pCache = &BG.Buffer[(TileNumber = (TileAddr >> tileShift)) << 6]; 

    if (!BG.Buffered [TileNumber]) 
    { 
	    BG.Buffered[TileNumber] = S9xConvertTileTo8Bit (pCache, TileAddr); 
        if (BG.Buffered [TileNumber] == BLANK_TILE) 
            return; 
			
        GFX.VRAMPaletteFrame[TileAddr][0] = 0; 
        GFX.VRAMPaletteFrame[TileAddr][1] = 0; 
        GFX.VRAMPaletteFrame[TileAddr][2] = 0; 
        GFX.VRAMPaletteFrame[TileAddr][3] = 0; 
        GFX.VRAMPaletteFrame[TileAddr][4] = 0; 
        GFX.VRAMPaletteFrame[TileAddr][5] = 0; 
        GFX.VRAMPaletteFrame[TileAddr][6] = 0; 
        GFX.VRAMPaletteFrame[TileAddr][7] = 0; 
        GFX.VRAMPaletteFrame[TileAddr][8] = 0; 
        GFX.VRAMPaletteFrame[TileAddr][9] = 0; 
        GFX.VRAMPaletteFrame[TileAddr][10] = 0; 
        GFX.VRAMPaletteFrame[TileAddr][11] = 0; 
        GFX.VRAMPaletteFrame[TileAddr][12] = 0; 
        GFX.VRAMPaletteFrame[TileAddr][13] = 0; 
        GFX.VRAMPaletteFrame[TileAddr][14] = 0; 
        GFX.VRAMPaletteFrame[TileAddr][15] = 0; 
    } 

    if (BG.Buffered [TileNumber] == BLANK_TILE) 
	    return; 
		
	int texturePos = 0;
    
    uint32 l; 
    uint8 pal; 
    if (directColourMode) 
    { 
        if (IPPU.DirectColourMapsNeedRebuild) 
            S9xBuildDirectColourMaps (); 
        pal = (snesTile >> 10) & paletteMask; 
		GFX.ScreenColors = DirectColourMaps [pal]; 
		texturePos = cacheGetTexturePositionFast(COMPOSE_HASH(TileAddr, pal));
        if (GFX.VRAMPaletteFrame[TileAddr][pal] != GFX.PaletteFrame[pal + startPalette / 16]) 
        { 
            GFX.VRAMPaletteFrame[TileAddr][pal] = GFX.PaletteFrame[pal + startPalette / 16]; 
			
			gpu3dsCacheToTexturePosition(pCache, GFX.ScreenColors, texturePos);
        } 
    } 
    else 
    { 
        pal = (snesTile >> 10) & paletteMask; 
        GFX.ScreenColors = &IPPU.ScreenColors [(pal << paletteShift) + startPalette]; 
		texturePos = cacheGetTexturePositionFast(COMPOSE_HASH(TileAddr, pal));
		//printf ("%d\n", texturePos);
        if (GFX.VRAMPaletteFrame[TileAddr][pal] != GFX.PaletteFrame[pal + startPalette / 16]) 
        { 
            GFX.VRAMPaletteFrame[TileAddr][pal] = GFX.PaletteFrame[pal + startPalette / 16]; 

			//printf ("cache %d\n", texturePos);
			gpu3dsCacheToTexturePosition(pCache, GFX.ScreenColors, texturePos);
        } 
    }
	
	// Render tile
	//
	screenOffset += startX;
	int x0 = screenOffset & 0xFF;
	int y0 = screenOffset >> 8;
	int x1 = x0 + width;
	int y1 = y0 + height;
	
	int txBase = (texturePos & 0x7F) << 3;
	int tyBase = (texturePos >> 7) << 3;
	
	int tx0 = startX;
	int ty0 = startLine >> 3;
	int tx1 = tx0 + width;
	int ty1 = ty0 + height;
	
	if (snesTile & V_FLIP)
	{
		ty0 = 8 - ty0;
		ty1 = 8 - ty1;
	} 
	if (snesTile & H_FLIP)
	{
		tx0 = 8 - tx0; 
		tx1 = 8 - tx1;
	}
	
	//printf ("Draw: %d %d %d, %d %d %d %d - %d %d %d %d (%d)\n", screenOffset, startX, startLine, x0, y0, x1, y1, txBase + tx0, tyBase + ty0, txBase + tx1, tyBase + ty1, texturePos);
	gpu3dsAddTileVertexes(
		x0, y0, x1, y1, 
		txBase + tx0, tyBase + ty0, 
		txBase + tx1, tyBase + ty1, BG.Depth);
}


//-------------------------------------------------------------------
// Draw tile using 3D hardware
//-------------------------------------------------------------------
inline void __attribute__((always_inline)) S9xDrawBGTileHardwareInline (
    int tileSize, int tileShift, int paletteShift, int paletteMask, int startPalette, bool directColourMode,
    uint32 snesTile, uint32 screenOffset, uint32 startLine, uint32 height)
{
	S9xDrawBGClippedTileHardwareInline (
        tileSize, tileShift, paletteShift, paletteMask, startPalette, directColourMode,
        snesTile, screenOffset, 0, 8, startLine, height);
}


inline void __attribute__((always_inline)) S9xDrawBackgroundHardwarePriority1Inline (
    int tileSize, int tileShift, int paletteShift, int paletteMask, int startPalette, bool directColourMode,
    uint32 BGMode, uint32 bg, bool sub, int depth)
{
    GFX.PixSize = 1;
	
    BG.TileSize = tileSize;
    BG.BitShift = BitShifts[BGMode][bg];
    BG.TileShift = tileShift;
    BG.TileAddress = PPU.BG[bg].NameBase << 1;
    BG.NameSelect = 0;
    BG.Buffer = IPPU.TileCache [Depths [BGMode][bg]];
    BG.Buffered = IPPU.TileCached [Depths [BGMode][bg]];
    BG.PaletteShift = paletteShift;
    BG.PaletteMask = paletteMask;
    BG.DirectColourMode = (BGMode == 3 || BGMode == 4) && bg == 0 &&
		(GFX.r2130 & 1);
	BG.Depth = depth;
	BG.SubY = sub ? 256 : 0;
	
	/*
	We may need to add this back later...
    if (PPU.BGMosaic [bg] && PPU.Mosaic > 1)
    {
		DrawBackgroundMosaic (BGMode, bg, Z1, Z2);
		return;
    }
    switch (BGMode)
    {
		case 2:
		case 4: // Used by Puzzle Bobble
			DrawBackgroundOffset (BGMode, bg, Z1, Z2);
			return;
			
		case 5:
		case 6: // XXX: is also offset per tile.
			if (Settings.SupportHiRes)
			{
				DrawBackgroundMode5 (BGMode, bg, Z1, Z2);
				return;
			}
			break;
    }
    CHECK_SOUND();
	*/

    if (BGMode == 0)
		BG.StartPalette = startPalette;
    else BG.StartPalette = 0;

	for (int i = 0; i < BG.DrawTileCount[bg]; i++)
	{
		if (BG.DrawTileParameters[bg][i][0] == 0)
		{
			// unclipped tile.
			S9xDrawBGTileHardwareInline (
                tileSize, tileShift, paletteShift, paletteMask, startPalette, directColourMode,
				BG.DrawTileParameters[bg][i][1], BG.DrawTileParameters[bg][i][2], 
				BG.DrawTileParameters[bg][i][3], BG.DrawTileParameters[bg][i][4]);
		}
		else
		{
			// clipped tile.
			S9xDrawBGClippedTileHardwareInline (
                tileSize, tileShift, paletteShift, paletteMask, startPalette, directColourMode,
				BG.DrawTileParameters[bg][i][1], BG.DrawTileParameters[bg][i][2], BG.DrawTileParameters[bg][i][3], 
				BG.DrawTileParameters[bg][i][4], BG.DrawTileParameters[bg][i][5], BG.DrawTileParameters[bg][i][6]);
		}
	}
}


// Draw non-offset-per-tile backgrounds
// 
inline void __attribute__((always_inline)) S9xDrawBackgroundHardwarePriority0Inline (
    int tileSize, int tileShift, int paletteShift, int paletteMask, int startPalette, bool directColourMode,
    uint32 BGMode, uint32 bg, bool sub, int depth)
{
    GFX.PixSize = 1;
	
    BG.TileSize = tileSize;
    BG.BitShift = BitShifts[BGMode][bg];
    BG.TileShift = tileShift;
    BG.TileAddress = PPU.BG[bg].NameBase << 1;
    BG.NameSelect = 0;
    BG.Buffer = IPPU.TileCache [Depths [BGMode][bg]];
    BG.Buffered = IPPU.TileCached [Depths [BGMode][bg]];
    BG.PaletteShift = paletteShift;
    BG.PaletteMask = paletteMask;
    BG.DirectColourMode = directColourMode;
	BG.DrawTileCount[bg] = 0;
	BG.Depth = depth;
	BG.SubY = sub ? 256 : 0;
	
	/*
    if (PPU.BGMosaic [bg] && PPU.Mosaic > 1)
    {
		DrawBackgroundMosaic (BGMode, bg, Z1, Z2);
		return;
    }
	*/
	/*
    switch (BGMode)
    {
		case 2:
		case 4: // Used by Puzzle Bobble
			DrawBackgroundOffset (BGMode, bg, Z1, Z2);
			return;
			
		case 5:
		case 6: // XXX: is also offset per tile.
			if (Settings.SupportHiRes)
			{
				DrawBackgroundMode5 (BGMode, bg, Z1, Z2);
				return;
			}
			break;
    }
	*/
	
    uint32 Tile;
    uint16 *SC0;
    uint16 *SC1;
    uint16 *SC2;
    uint16 *SC3;
    uint32 Width;
    //uint8 depths [2] = {Z1, Z2};
    
    if (BGMode == 0)
		BG.StartPalette = startPalette;
    else BG.StartPalette = 0;
	
    SC0 = (uint16 *) &Memory.VRAM[PPU.BG[bg].SCBase << 1];
	
    if (PPU.BG[bg].SCSize & 1)
		SC1 = SC0 + 1024;
    else
		SC1 = SC0;
	
	if(SC1>=(unsigned short*)(Memory.VRAM+0x10000))
		SC1=(uint16*)&Memory.VRAM[((uint8*)SC1-&Memory.VRAM[0])%0x10000];
	
    if (PPU.BG[bg].SCSize & 2)
		SC2 = SC1 + 1024;
    else
		SC2 = SC0;
	
	if(((uint8*)SC2-Memory.VRAM)>=0x10000)
		SC2-=0x08000;
	
    if (PPU.BG[bg].SCSize & 1)
		SC3 = SC2 + 1024;
    else
		SC3 = SC2;
    
	if(((uint8*)SC3-Memory.VRAM)>=0x10000)
		SC3-=0x08000;
	
    int Lines;
    int OffsetMask;
    int OffsetShift;
	
    if (tileSize == 16)
    {
		OffsetMask = 0x3ff;
		OffsetShift = 4;
    }
    else
    {
		OffsetMask = 0x1ff;
		OffsetShift = 3;
    }
	
    for (uint32 Y = GFX.StartY; Y <= GFX.EndY; Y += Lines)
    {
		uint32 VOffset = LineData [Y].BG[bg].VOffset;
		uint32 HOffset = LineData [Y].BG[bg].HOffset;
		
		int VirtAlign = (Y + VOffset) & 7;
		
		for (Lines = 1; Lines < 8 - VirtAlign; Lines++)
			if ((VOffset != LineData [Y + Lines].BG[bg].VOffset) ||
				(HOffset != LineData [Y + Lines].BG[bg].HOffset))
				break;
			
		if (Y + Lines > GFX.EndY)
			Lines = GFX.EndY + 1 - Y;
		
		//if (GFX.EndY - GFX.StartY < 10)
		//	printf ("bg:%d Y/L:%3d/%3d OFS:%d,%d\n", bg, Y, Lines, HOffset, VOffset);  
			
		VirtAlign <<= 3;
		
		uint32 ScreenLine = (VOffset + Y) >> OffsetShift;
		uint32 t1;
		uint32 t2;
		if (((VOffset + Y) & 15) > 7)
		{
			t1 = 16;
			t2 = 0;
		}
		else
		{
			t1 = 0;
			t2 = 16;
		}
		uint16 *b1;
		uint16 *b2;
		
		if (ScreenLine & 0x20)
			b1 = SC2, b2 = SC3;
		else
			b1 = SC0, b2 = SC1;
		
		b1 += (ScreenLine & 0x1f) << 5;
		b2 += (ScreenLine & 0x1f) << 5;
		
		int clipcount = GFX.pCurrentClip->Count [bg];
		if (!clipcount)
			clipcount = 1;
		for (int clip = 0; clip < clipcount; clip++)
		{
			uint32 Left;
			uint32 Right;
			
			if (!GFX.pCurrentClip->Count [bg])
			{
				Left = 0;
				Right = 256;
			}
			else
			{
				Left = GFX.pCurrentClip->Left [clip][bg];
				Right = GFX.pCurrentClip->Right [clip][bg];
				
				if (Right <= Left)
					continue;
			}
			
			//uint32 s = Left * GFX.PixSize + Y * GFX.PPL;
			uint32 s = Left * GFX.PixSize + Y * 256;		// Once hardcoded, Hires mode no longer supported.
			//printf ("s = %d, Lines = %d\n", s, Lines);
			uint32 HPos = (HOffset + Left) & OffsetMask;
			
			uint32 Quot = HPos >> 3;
			uint32 Count = 0;
			
			uint16 *t;
			if (tileSize == 8)
			{
				if (Quot > 31)
					t = b2 + (Quot & 0x1f);
				else
					t = b1 + Quot;
			}
			else
			{
				if (Quot > 63)
					t = b2 + ((Quot >> 1) & 0x1f);
				else
					t = b1 + (Quot >> 1);
			}
			
			Width = Right - Left;
			// Left hand edge clipped tile
			if (HPos & 7)
			{
				uint32 Offset = (HPos & 7);
				Count = 8 - Offset;
				if (Count > Width)
					Count = Width;
				s -= Offset * GFX.PixSize;
				Tile = READ_2BYTES(t);
				
				int tpriority = (Tile & 0x2000) >> 13;
				//if (tpriority == priority) 
				{
					//GFX.Z1 = GFX.Z2 = depths [(Tile & 0x2000) >> 13];
					
					if (tileSize == 8)
					{
						if (tpriority == 0)
							S9xDrawBGClippedTileHardwareInline (
                                tileSize, tileShift, paletteShift, paletteMask, startPalette, directColourMode,
                                Tile, s, Offset, Count, VirtAlign, Lines);
						else 
							DrawClippedTileLater (Tile, s, Offset, Count, VirtAlign, Lines);
					}
					else
					{
						if (!(Tile & (V_FLIP | H_FLIP)))
						{
							// Normal, unflipped
							if (tpriority == 0)
                                S9xDrawBGClippedTileHardwareInline (
                                    tileSize, tileShift, paletteShift, paletteMask, startPalette, directColourMode,
                                    Tile + t1 + (Quot & 1), s, Offset, Count, VirtAlign, Lines);
							else
								DrawClippedTileLater (Tile + t1 + (Quot & 1), s, Offset, Count, VirtAlign, Lines);
						}
						else
							if (Tile & H_FLIP)
							{
								if (Tile & V_FLIP)
								{
									// H & V flip
									if (tpriority == 0)
                                        S9xDrawBGClippedTileHardwareInline (
                                            tileSize, tileShift, paletteShift, paletteMask, startPalette, directColourMode,
                                            Tile + t2 + 1 - (Quot & 1), s, Offset, Count, VirtAlign, Lines);
									else
										DrawClippedTileLater (Tile + t2 + 1 - (Quot & 1), s, Offset, Count, VirtAlign, Lines);
								}
								else
								{
									// H flip only
									if (tpriority == 0)
                                        S9xDrawBGClippedTileHardwareInline (
                                            tileSize, tileShift, paletteShift, paletteMask, startPalette, directColourMode,
                                            Tile + t1 + 1 - (Quot & 1), s, Offset, Count, VirtAlign, Lines);
									else
										DrawClippedTileLater (Tile + t1 + 1 - (Quot & 1), s, Offset, Count, VirtAlign, Lines);
								}
							}
							else
							{
								// V flip only
								if (tpriority == 0)
                                    S9xDrawBGClippedTileHardwareInline (
                                        tileSize, tileShift, paletteShift, paletteMask, startPalette, directColourMode,
                                        Tile + t2 + (Quot & 1), s, Offset, Count, VirtAlign, Lines);
								else
									DrawClippedTileLater (Tile + t2 + (Quot & 1), s, Offset, Count, VirtAlign, Lines);
							}
					}
				}
				
				if (tileSize == 8)
				{
					t++;
					if (Quot == 31)
						t = b2;
					else if (Quot == 63)
						t = b1;
				}
				else
				{
					t += Quot & 1;
					if (Quot == 63)
						t = b2;
					else if (Quot == 127)
						t = b1;
				}
				Quot++;
				s += 8 * GFX.PixSize;
				
			}
			
			// Middle, unclipped tiles
			Count = Width - Count;
			int Middle = Count >> 3;
			Count &= 7;
			for (int C = Middle; C > 0; s += 8 * GFX.PixSize, Quot++, C--)
			{
				Tile = READ_2BYTES(t);
				
				int tpriority = (Tile & 0x2000) >> 13;
				//if (tpriority == priority) 
				{
				
					//GFX.Z1 = GFX.Z2 = depths [(Tile & 0x2000) >> 13];
					
					if (tileSize != 8)
					{
						if (Tile & H_FLIP)
						{
							// Horizontal flip, but what about vertical flip ?
							if (Tile & V_FLIP)
							{
								// Both horzontal & vertical flip
								if (tpriority == 0)
                                    S9xDrawBGTileHardwareInline (
                                        tileSize, tileShift, paletteShift, paletteMask, startPalette, directColourMode,
                                        Tile + t2 + 1 - (Quot & 1), s, VirtAlign, Lines);
								else
									DrawTileLater (Tile + t2 + 1 - (Quot & 1), s, VirtAlign, Lines);
							}
							else
							{
								// Horizontal flip only
								if (tpriority == 0)
                                    S9xDrawBGTileHardwareInline (
                                        tileSize, tileShift, paletteShift, paletteMask, startPalette, directColourMode,
                                        Tile + t1 + 1 - (Quot & 1), s, VirtAlign, Lines);
								else
									DrawTileLater (Tile + t1 + 1 - (Quot & 1), s, VirtAlign, Lines);
							}
						}
						else
						{
							// No horizontal flip, but is there a vertical flip ?
							if (Tile & V_FLIP)
							{
								// Vertical flip only
								if (tpriority == 0)
                                    S9xDrawBGTileHardwareInline (
                                        tileSize, tileShift, paletteShift, paletteMask, startPalette, directColourMode,
                                        Tile + t2 + (Quot & 1), s, VirtAlign, Lines);
								else
									DrawTileLater (Tile + t2 + (Quot & 1), s, VirtAlign, Lines);
							}
							else
							{
								// Normal unflipped
								if (tpriority == 0)
                                    S9xDrawBGTileHardwareInline (
                                        tileSize, tileShift, paletteShift, paletteMask, startPalette, directColourMode,
                                        Tile + t1 + (Quot & 1), s, VirtAlign, Lines);
								else
									DrawTileLater (Tile + t1 + (Quot & 1), s, VirtAlign, Lines);
							}
						}
					}
					else
					{
						if (tpriority == 0)
							S9xDrawBGTileHardwareInline (
                                tileSize, tileShift, paletteShift, paletteMask, startPalette, directColourMode,
                                Tile, s, VirtAlign, Lines);
						else
							DrawTileLater (Tile, s, VirtAlign, Lines);
					}
				}
									
				if (tileSize == 8)
				{
					t++;
					if (Quot == 31)
						t = b2;
					else
						if (Quot == 63)
							t = b1;
				}
				else
				{
					t += Quot & 1;
					if (Quot == 63)
						t = b2;
					else
						if (Quot == 127)
							t = b1;
				}
			}
			// Right-hand edge clipped tiles
			if (Count)
			{
				Tile = READ_2BYTES(t);
				
				int tpriority = (Tile & 0x2000) >> 13;
				//if (tpriority == priority) 
				{
					//GFX.Z1 = GFX.Z2 = depths [(Tile & 0x2000) >> 13];
					
					if (tileSize == 8)
					{
						if (tpriority == 0)
							S9xDrawBGClippedTileHardwareInline (
                                tileSize, tileShift, paletteShift, paletteMask, startPalette, directColourMode,
                                Tile, s, 0, Count, VirtAlign, Lines);
						else
							DrawClippedTileLater (Tile, s, 0, Count, VirtAlign, Lines);
					}
					else
					{
						if (!(Tile & (V_FLIP | H_FLIP)))
						{
							// Normal, unflipped
							if (tpriority == 0)
                                S9xDrawBGClippedTileHardwareInline (
                                    tileSize, tileShift, paletteShift, paletteMask, startPalette, directColourMode,
                                    Tile + t1 + (Quot & 1), s, 0, Count, VirtAlign, Lines);
							else
								DrawClippedTileLater (Tile + t1 + (Quot & 1), s, 0, Count, VirtAlign, Lines);
						}
						else if (Tile & H_FLIP)
						{
							if (Tile & V_FLIP)
							{
								// H & V flip
								if (tpriority == 0)
                                    S9xDrawBGClippedTileHardwareInline (
                                        tileSize, tileShift, paletteShift, paletteMask, startPalette, directColourMode,
                                        Tile + t2 + 1 - (Quot & 1), s, 0, Count, VirtAlign, Lines);
								else
									DrawClippedTileLater (Tile + t2 + 1 - (Quot & 1), s, 0, Count, VirtAlign, Lines);
							}
							else
							{
								// H flip only
								if (tpriority == 0)
                                    S9xDrawBGClippedTileHardwareInline (
                                        tileSize, tileShift, paletteShift, paletteMask, startPalette, directColourMode,
                                        Tile + t1 + 1 - (Quot & 1), s, 0, Count, VirtAlign, Lines);
								else
									DrawClippedTileLater (Tile + t1 + 1 - (Quot & 1), s, 0, Count, VirtAlign, Lines);
							}
						}
						else
						{
							// V flip only
							if (tpriority == 0)
                                S9xDrawBGClippedTileHardwareInline (
                                    tileSize, tileShift, paletteShift, paletteMask, startPalette, directColourMode,
                                    Tile + t2 + (Quot & 1), s, 0, Count, VirtAlign, Lines);
							else
								DrawClippedTileLater (Tile + t2 + (Quot & 1), s, 0, Count, VirtAlign, Lines);	
						}
					}
				}
			}
		}
    }
	
	//printf ("BG %d P0\n", bg);
}


//-------------------------------------------------------------------
// 4-color BGs, priority 0
//-------------------------------------------------------------------

void S9xDrawBackgroundHardwarePriority0Inline_4Color_8x8
    (uint32 BGMode, uint32 bg, bool sub, int depth)
{
    S9xDrawBackgroundHardwarePriority0Inline(
        8,              // tileSize 
        4,              // tileShift
        2,              // paletteShift
        7,              // paletteMask 
        0,              // startPalette
        FALSE,          // directColourMode
        BGMode, bg, sub, depth);
}

void S9xDrawBackgroundHardwarePriority0Inline_4Color_16x16
    (uint32 BGMode, uint32 bg, bool sub, int depth)
{
    S9xDrawBackgroundHardwarePriority0Inline(
        16,             // tileSize 
        4,              // tileShift
        2,              // paletteShift
        7,              // paletteMask 
        0,              // startPalette
        FALSE,          // directColourMode
        BGMode, bg, sub, depth);
}

void S9xDrawBackgroundHardwarePriority0Inline_Mode0_4Color_8x8
    (uint32 BGMode, uint32 bg, bool sub, int depth)
{
    S9xDrawBackgroundHardwarePriority0Inline(
        8,              // tileSize 
        4,              // tileShift
        2,              // paletteShift
        7,              // paletteMask 
        bg << 5,        // startPalette
        FALSE,          // directColourMode
        BGMode, bg, sub, depth);
}

void S9xDrawBackgroundHardwarePriority0Inline_Mode0_4Color_16x16
    (uint32 BGMode, uint32 bg, bool sub, int depth)
{
    S9xDrawBackgroundHardwarePriority0Inline(
        16,             // tileSize 
        4,              // tileShift
        2,              // paletteShift
        7,              // paletteMask 
        bg << 5,        // startPalette
        FALSE,          // directColourMode
        BGMode, bg, sub, depth);
}


void S9xDrawBackgroundHardwarePriority0Inline_4Color
    (uint32 BGMode, uint32 bg, bool sub, int depth)
{
    if (BGMode != 0)
    {
        if (BGSizes [PPU.BG[bg].BGSize] == 8)
            S9xDrawBackgroundHardwarePriority0Inline_4Color_8x8(
                BGMode, bg, sub, depth);
        else
            S9xDrawBackgroundHardwarePriority0Inline_4Color_16x16(
                BGMode, bg, sub, depth);
    }
    else
    {
        if (BGSizes [PPU.BG[bg].BGSize] == 8)
            S9xDrawBackgroundHardwarePriority0Inline_Mode0_4Color_8x8(
                BGMode, bg, sub, depth);
        else
            S9xDrawBackgroundHardwarePriority0Inline_Mode0_4Color_16x16(
                BGMode, bg, sub, depth);
    }
}


//-------------------------------------------------------------------
// 4-color BGs, priority 1
//-------------------------------------------------------------------

void S9xDrawBackgroundHardwarePriority1Inline_4Color_8x8
    (uint32 BGMode, uint32 bg, bool sub, int depth)
{
    S9xDrawBackgroundHardwarePriority1Inline(
        8,              // tileSize 
        4,              // tileShift
        2,              // paletteShift
        7,              // paletteMask 
        0,              // startPalette
        FALSE,          // directColourMode
        BGMode, bg, sub, depth);
}

void S9xDrawBackgroundHardwarePriority1Inline_4Color_16x16
    (uint32 BGMode, uint32 bg, bool sub, int depth)
{
    S9xDrawBackgroundHardwarePriority1Inline(
        16,             // tileSize 
        4,              // tileShift
        2,              // paletteShift
        7,              // paletteMask 
        0,              // startPalette
        FALSE,          // directColourMode
        BGMode, bg, sub, depth);
}


void S9xDrawBackgroundHardwarePriority1Inline_Mode0_4Color_8x8
    (uint32 BGMode, uint32 bg, bool sub, int depth)
{
    S9xDrawBackgroundHardwarePriority1Inline(
        8,              // tileSize 
        4,              // tileShift
        2,              // paletteShift
        7,              // paletteMask 
        bg << 5,        // startPalette
        FALSE,          // directColourMode
        BGMode, bg, sub, depth);
}

void S9xDrawBackgroundHardwarePriority1Inline_Mode0_4Color_16x16
    (uint32 BGMode, uint32 bg, bool sub, int depth)
{
    S9xDrawBackgroundHardwarePriority1Inline(
        16,             // tileSize 
        4,              // tileShift
        2,              // paletteShift
        7,              // paletteMask 
        bg << 5,        // startPalette
        FALSE,          // directColourMode
        BGMode, bg, sub, depth);
}


void S9xDrawBackgroundHardwarePriority1Inline_4Color
    (uint32 BGMode, uint32 bg, bool sub, int depth)
{
    if (BGMode != 0)
    {
        if (BGSizes [PPU.BG[bg].BGSize] == 8)
            S9xDrawBackgroundHardwarePriority1Inline_4Color_8x8(
                BGMode, bg, sub, depth);
        else
            S9xDrawBackgroundHardwarePriority1Inline_4Color_16x16(
                BGMode, bg, sub, depth);
    }
    else
    {
        if (BGSizes [PPU.BG[bg].BGSize] == 8)
            S9xDrawBackgroundHardwarePriority1Inline_Mode0_4Color_8x8(
                BGMode, bg, sub, depth);
        else
            S9xDrawBackgroundHardwarePriority1Inline_Mode0_4Color_16x16(
                BGMode, bg, sub, depth);
    }
}




//-------------------------------------------------------------------
// 16-color BGs, priority 0
//-------------------------------------------------------------------

void S9xDrawBackgroundHardwarePriority0Inline_16Color_8x8
    (uint32 BGMode, uint32 bg, bool sub, int depth)
{
    S9xDrawBackgroundHardwarePriority0Inline(
        8,              // tileSize 
        5,              // tileShift
        4,              // paletteShift
        7,              // paletteMask 
        0,              // startPalette
        FALSE,          // directColourMode
        BGMode, bg, sub, depth);
}

void S9xDrawBackgroundHardwarePriority0Inline_16Color_16x16
    (uint32 BGMode, uint32 bg, bool sub, int depth)
{
    S9xDrawBackgroundHardwarePriority0Inline(
        16,             // tileSize 
        5,              // tileShift
        4,              // paletteShift
        7,              // paletteMask 
        0,              // startPalette
        FALSE,          // directColourMode
        BGMode, bg, sub, depth);
}

void S9xDrawBackgroundHardwarePriority0Inline_16Color
    (uint32 BGMode, uint32 bg, bool sub, int depth)
{
    if (BGSizes [PPU.BG[bg].BGSize] == 8)
    {
        S9xDrawBackgroundHardwarePriority0Inline_16Color_8x8(
            BGMode, bg, sub, depth);
    }
    else
    {
        S9xDrawBackgroundHardwarePriority0Inline_16Color_16x16(
            BGMode, bg, sub, depth);
    }
}


//-------------------------------------------------------------------
// 16-color BGs, priority 1
//-------------------------------------------------------------------

void S9xDrawBackgroundHardwarePriority1Inline_16Color_8x8
    (uint32 BGMode, uint32 bg, bool sub, int depth)
{
    S9xDrawBackgroundHardwarePriority1Inline(
        8,              // tileSize 
        5,              // tileShift
        4,              // paletteShift
        7,              // paletteMask 
        0,              // startPalette
        FALSE,          // directColourMode
        BGMode, bg, sub, depth);
}

void S9xDrawBackgroundHardwarePriority1Inline_16Color_16x16
    (uint32 BGMode, uint32 bg, bool sub, int depth)
{
    S9xDrawBackgroundHardwarePriority1Inline(
        16,             // tileSize 
        5,              // tileShift
        4,              // paletteShift
        7,              // paletteMask 
        0,              // startPalette
        FALSE,          // directColourMode
        BGMode, bg, sub, depth);
}

void S9xDrawBackgroundHardwarePriority1Inline_16Color
    (uint32 BGMode, uint32 bg, bool sub, int depth)
{

    if (BGSizes [PPU.BG[bg].BGSize] == 8)
    {
        S9xDrawBackgroundHardwarePriority1Inline_16Color_8x8(
            BGMode, bg, sub, depth);
    }
    else
    {
        S9xDrawBackgroundHardwarePriority1Inline_16Color_16x16(
            BGMode, bg, sub, depth);
    }
}



//-------------------------------------------------------------------
// 256-color BGs, priority 0
//-------------------------------------------------------------------

void S9xDrawBackgroundHardwarePriority0Inline_256NormalColor_8x8
    (uint32 BGMode, uint32 bg, bool sub, int depth)
{
    S9xDrawBackgroundHardwarePriority0Inline(
        8,              // tileSize 
        6,              // tileShift
        0,              // paletteShift
        0,              // paletteMask 
        0,              // startPalette
        FALSE,          // directColourMode
        BGMode, bg, sub, depth);
}

void S9xDrawBackgroundHardwarePriority0Inline_256NormalColor_16x16
    (uint32 BGMode, uint32 bg, bool sub, int depth)
{
    S9xDrawBackgroundHardwarePriority0Inline(
        16,             // tileSize 
        6,              // tileShift
        0,              // paletteShift
        0,              // paletteMask 
        0,              // startPalette
        FALSE,          // directColourMode
        BGMode, bg, sub, depth);
}

void S9xDrawBackgroundHardwarePriority0Inline_256DirectColor_8x8
    (uint32 BGMode, uint32 bg, bool sub, int depth)
{
    S9xDrawBackgroundHardwarePriority0Inline(
        8,              // tileSize 
        6,              // tileShift
        0,              // paletteShift
        0,              // paletteMask 
        0,              // startPalette
        TRUE,          // directColourMode
        BGMode, bg, sub, depth);
}

void S9xDrawBackgroundHardwarePriority0Inline_256DirectColor_16x16
    (uint32 BGMode, uint32 bg, bool sub, int depth)
{
    S9xDrawBackgroundHardwarePriority0Inline(
        16,             // tileSize 
        6,              // tileShift
        0,              // paletteShift
        0,              // paletteMask 
        0,              // startPalette
        TRUE,          // directColourMode
        BGMode, bg, sub, depth);
}

void S9xDrawBackgroundHardwarePriority0Inline_256Color
    (uint32 BGMode, uint32 bg, bool sub, int depth)
{
    if (BGSizes [PPU.BG[bg].BGSize] == 8)
    {
        if (!(GFX.r2130 & 1))
            S9xDrawBackgroundHardwarePriority0Inline_256NormalColor_8x8(
                BGMode, bg, sub, depth);
        else
            S9xDrawBackgroundHardwarePriority0Inline_256DirectColor_8x8(
                BGMode, bg, sub, depth);
    }
    else
    {
        if (!(GFX.r2130 & 1))
            S9xDrawBackgroundHardwarePriority0Inline_256NormalColor_16x16(
                BGMode, bg, sub, depth);
        else
            S9xDrawBackgroundHardwarePriority0Inline_256DirectColor_16x16(
                BGMode, bg, sub, depth);
    }
}


//-------------------------------------------------------------------
// 256-color BGs, priority 1
//-------------------------------------------------------------------

void S9xDrawBackgroundHardwarePriority1Inline_256NormalColor_8x8
    (uint32 BGMode, uint32 bg, bool sub, int depth)
{
    S9xDrawBackgroundHardwarePriority1Inline(
        8,              // tileSize 
        6,              // tileShift
        0,              // paletteShift
        0,              // paletteMask 
        0,              // startPalette
        FALSE,          // directColourMode
        BGMode, bg, sub, depth);
}

void S9xDrawBackgroundHardwarePriority1Inline_256NormalColor_16x16
    (uint32 BGMode, uint32 bg, bool sub, int depth)
{
    S9xDrawBackgroundHardwarePriority1Inline(
        16,             // tileSize 
        6,              // tileShift
        0,              // paletteShift
        0,              // paletteMask 
        0,              // startPalette
        FALSE,          // directColourMode
        BGMode, bg, sub, depth);
}

void S9xDrawBackgroundHardwarePriority1Inline_256DirectColor_8x8
    (uint32 BGMode, uint32 bg, bool sub, int depth)
{
    S9xDrawBackgroundHardwarePriority1Inline(
        8,              // tileSize 
        6,              // tileShift
        0,              // paletteShift
        0,              // paletteMask 
        0,              // startPalette
        TRUE,          // directColourMode
        BGMode, bg, sub, depth);
}

void S9xDrawBackgroundHardwarePriority1Inline_256DirectColor_16x16
    (uint32 BGMode, uint32 bg, bool sub, int depth)
{
    S9xDrawBackgroundHardwarePriority1Inline(
        16,             // tileSize 
        6,              // tileShift
        0,              // paletteShift
        0,              // paletteMask 
        0,              // startPalette
        TRUE,          // directColourMode
        BGMode, bg, sub, depth);
}

void S9xDrawBackgroundHardwarePriority1Inline_256Color
    (uint32 BGMode, uint32 bg, bool sub, int depth)
{
    if (BGSizes [PPU.BG[bg].BGSize] == 8)
    {
        if (GFX.r2130 & 1)
            S9xDrawBackgroundHardwarePriority1Inline_256NormalColor_8x8(
                BGMode, bg, sub, depth);
        else
            S9xDrawBackgroundHardwarePriority1Inline_256DirectColor_8x8(
                BGMode, bg, sub, depth);
    }
    else
    {
        if (GFX.r2130 & 1)
            S9xDrawBackgroundHardwarePriority1Inline_256NormalColor_16x16(
                BGMode, bg, sub, depth);
        else
            S9xDrawBackgroundHardwarePriority1Inline_256DirectColor_16x16(
                BGMode, bg, sub, depth);
    }
}



//-------------------------------------------------------------------
// Draw a clipped OBJ tile using 3D hardware.
//-------------------------------------------------------------------
inline void __attribute__((always_inline)) S9xDrawOBJClippedTileHardware (
	uint32 snesTile, uint32 screenOffset,
	uint32 startX, uint32 width,
	uint32 startLine, uint32 height)
{
	/*
	BG.BitShift = 4;
	BG.TileShift = 5;
	BG.TileAddress = PPU.OBJNameBase;
	BG.StartPalette = 128;
	BG.PaletteShift = 4;
	BG.PaletteMask = 7;
	BG.Buffer = IPPU.TileCache [TILE_4BIT];
	BG.Buffered = IPPU.TileCached [TILE_4BIT];
	BG.NameSelect = PPU.OBJNameSelect;
	BG.DirectColourMode = FALSE; */
	
	// Prepare tile for rendering
	//
    uint8 *pCache;  
    uint16 *pCache16; 

    uint32 TileAddr = BG.TileAddress + ((snesTile & 0x3ff) << 5); 
    
	// OBJ tiles can be name-selected.
	if ((snesTile & 0x1ff) >= 256)
		TileAddr += BG.NameSelect; 
	TileAddr &= 0xffff;
	
    uint32 TileNumber; 
    pCache = &BG.Buffer[(TileNumber = (TileAddr >> 5)) << 6]; 

    if (!BG.Buffered [TileNumber]) 
    { 
	    BG.Buffered[TileNumber] = S9xConvertTileTo8Bit (pCache, TileAddr); 
        if (BG.Buffered [TileNumber] == BLANK_TILE) 
            return; 
			
        GFX.VRAMPaletteFrame[TileAddr][0] = 0; 
        GFX.VRAMPaletteFrame[TileAddr][1] = 0; 
        GFX.VRAMPaletteFrame[TileAddr][2] = 0; 
        GFX.VRAMPaletteFrame[TileAddr][3] = 0; 
        GFX.VRAMPaletteFrame[TileAddr][4] = 0; 
        GFX.VRAMPaletteFrame[TileAddr][5] = 0; 
        GFX.VRAMPaletteFrame[TileAddr][6] = 0; 
        GFX.VRAMPaletteFrame[TileAddr][7] = 0; 
        GFX.VRAMPaletteFrame[TileAddr][8] = 0; 
        GFX.VRAMPaletteFrame[TileAddr][9] = 0; 
        GFX.VRAMPaletteFrame[TileAddr][10] = 0; 
        GFX.VRAMPaletteFrame[TileAddr][11] = 0; 
        GFX.VRAMPaletteFrame[TileAddr][12] = 0; 
        GFX.VRAMPaletteFrame[TileAddr][13] = 0; 
        GFX.VRAMPaletteFrame[TileAddr][14] = 0; 
        GFX.VRAMPaletteFrame[TileAddr][15] = 0; 
    } 

    if (BG.Buffered [TileNumber] == BLANK_TILE) 
	    return; 
		
	int texturePos = 0;
    
    uint32 l; 
    uint8 pal; 
     
	{
        pal = (snesTile >> 10) & 7; 
        GFX.ScreenColors = &IPPU.ScreenColors [(pal << 4) + 128]; 
		texturePos = cacheGetTexturePositionFast(COMPOSE_HASH(TileAddr, pal));
		//printf ("%d\n", texturePos);
        if (GFX.VRAMPaletteFrame[TileAddr][pal] != GFX.PaletteFrame[pal + 128 / 16]) 
        { 
            GFX.VRAMPaletteFrame[TileAddr][pal] = GFX.PaletteFrame[pal + 128 / 16]; 

			//printf ("cache %d\n", texturePos);
			gpu3dsCacheToTexturePosition(pCache, GFX.ScreenColors, texturePos);
        } 
    }
	
	// Render tile
	//
	int x0 = screenOffset & 0xFF;
	int y0 = screenOffset >> 8;
	x0 += startX;
	int x1 = x0 + width;
	int y1 = y0 + height;
	
	int txBase = (texturePos & 0x7F) << 3;
	int tyBase = (texturePos >> 7) << 3;
	
	int tx0 = startX;
	int ty0 = startLine >> 3;
	int tx1 = tx0 + width;
	int ty1 = ty0 + height;
	
	if (snesTile & V_FLIP)
	{
		ty0 = 8 - ty0;
		ty1 = 8 - ty1;
	} 
	if (snesTile & H_FLIP)
	{
		tx0 = 8 - tx0; 
		tx1 = 8 - tx1;
	}
	
	//printf ("Draw: %d %d %d, %d %d %d %d - %d %d %d %d (%d)\n", screenOffset, startX, startY, x0, y0, x1, y1, txBase + tx0, tyBase + ty0, txBase + tx1, tyBase + ty1, texturePos);
	gpu3dsAddTileVertexes(
		x0, y0, x1, y1, 
		txBase + tx0, tyBase + ty0, 
		txBase + tx1, tyBase + ty1, pal <= 3 ? 1 : BG.Depth);
}


inline void __attribute__((always_inline)) S9xDrawOBJTileHardware2 (
	uint32 snesTile, 
	int screenX, int screenY, uint32 textureYOffset)
{
	/*
	BG.BitShift = 4;
	BG.TileShift = 5;
	BG.TileAddress = PPU.OBJNameBase;
	BG.StartPalette = 128;
	BG.PaletteShift = 4;
	BG.PaletteMask = 7;
	BG.Buffer = IPPU.TileCache [TILE_4BIT];
	BG.Buffered = IPPU.TileCached [TILE_4BIT];
	BG.NameSelect = PPU.OBJNameSelect;
	BG.DirectColourMode = FALSE; */
	
	// Prepare tile for rendering
	//
    uint8 *pCache;  
    uint16 *pCache16; 

    uint32 TileAddr = BG.TileAddress + ((snesTile & 0x3ff) << 5); 
    
	// OBJ tiles can be name-selected.
	if ((snesTile & 0x1ff) >= 256)
		TileAddr += BG.NameSelect; 
	TileAddr &= 0xffff;
	
    uint32 TileNumber; 
    pCache = &BG.Buffer[(TileNumber = (TileAddr >> 5)) << 6]; 

    if (!BG.Buffered [TileNumber]) 
    { 
	    BG.Buffered[TileNumber] = S9xConvertTileTo8Bit (pCache, TileAddr); 
        if (BG.Buffered [TileNumber] == BLANK_TILE) 
            return; 
			
        GFX.VRAMPaletteFrame[TileAddr][0] = 0; 
        GFX.VRAMPaletteFrame[TileAddr][1] = 0; 
        GFX.VRAMPaletteFrame[TileAddr][2] = 0; 
        GFX.VRAMPaletteFrame[TileAddr][3] = 0; 
        GFX.VRAMPaletteFrame[TileAddr][4] = 0; 
        GFX.VRAMPaletteFrame[TileAddr][5] = 0; 
        GFX.VRAMPaletteFrame[TileAddr][6] = 0; 
        GFX.VRAMPaletteFrame[TileAddr][7] = 0; 
        GFX.VRAMPaletteFrame[TileAddr][8] = 0; 
        GFX.VRAMPaletteFrame[TileAddr][9] = 0; 
        GFX.VRAMPaletteFrame[TileAddr][10] = 0; 
        GFX.VRAMPaletteFrame[TileAddr][11] = 0; 
        GFX.VRAMPaletteFrame[TileAddr][12] = 0; 
        GFX.VRAMPaletteFrame[TileAddr][13] = 0; 
        GFX.VRAMPaletteFrame[TileAddr][14] = 0; 
        GFX.VRAMPaletteFrame[TileAddr][15] = 0; 
    } 

    if (BG.Buffered [TileNumber] == BLANK_TILE) 
	    return; 
		
	int texturePos = 0;
    
    uint32 l; 
    uint8 pal; 
     
	{
        pal = (snesTile >> 10) & 7; 
        GFX.ScreenColors = &IPPU.ScreenColors [(pal << 4) + 128]; 
		texturePos = cacheGetTexturePositionFast(COMPOSE_HASH(TileAddr, pal));
		//printf ("%d\n", texturePos);
        if (GFX.VRAMPaletteFrame[TileAddr][pal] != GFX.PaletteFrame[pal + 128 / 16]) 
        { 
            GFX.VRAMPaletteFrame[TileAddr][pal] = GFX.PaletteFrame[pal + 128 / 16]; 

			//printf ("cache %d\n", texturePos);
			gpu3dsCacheToTexturePosition(pCache, GFX.ScreenColors, texturePos);
        } 
    }
	
	// Render tile
	//
	int x0 = screenX;
	int y0 = screenY;
	int x1 = x0 + 8;
	int y1 = y0 + 1;
	
	int txBase = (texturePos & 0x7F) << 3;
	int tyBase = (texturePos >> 7) << 3;
	
	int tx0 = 0;
	int ty0 = textureYOffset >> 3;
	int tx1 = tx0 + 8;
	int ty1 = ty0 + 1;
	
	if (snesTile & V_FLIP)
	{
		ty0 = 8 - ty0;
		ty1 = 8 - ty1;
	} 
	if (snesTile & H_FLIP)
	{
		tx0 = 8 - tx0; 
		tx1 = 8 - tx1;
	}
	
	//printf ("Draw: %d %d %d, %d %d %d %d - %d %d %d %d (%d)\n", screenOffset, startX, startY, x0, y0, x1, y1, txBase + tx0, tyBase + ty0, txBase + tx1, tyBase + ty1, texturePos);
	gpu3dsAddTileVertexes(
		x0, y0, x1, y1, 
		txBase + tx0, tyBase + ty0, 
		txBase + tx1, tyBase + ty1, pal <= 3 ? 1 : BG.Depth);
}


//-------------------------------------------------------------------
// Draw OBJ tile using 3D hardware
//-------------------------------------------------------------------
inline void __attribute__((always_inline)) S9xDrawOBJTileHardware (uint32 snesTile, uint32 screenOffset, uint32 startLine, uint32 height)
{
	S9xDrawOBJClippedTileHardware (snesTile, screenOffset, 0, 8, startLine, height);
}



//-------------------------------------------------------------------
// Draw the OBJ layers using 3D hardware.
//-------------------------------------------------------------------
void S9xDrawOBJSHardwarePriority (bool8 sub, int depth = 0, int priority = 0)
{
#ifdef MK_DEBUG_RTO
	if(Settings.BGLayering) fprintf(stderr, "Entering DrawOBJS() for %d-%d\n", GFX.StartY, GFX.EndY);
#endif
	CHECK_SOUND();

	int p = 4 + priority - 1; 
	
	BG.BitShift = 4;
	BG.TileShift = 5;
	BG.TileAddress = PPU.OBJNameBase;
	BG.StartPalette = 128;
	BG.PaletteShift = 4;
	BG.PaletteMask = 7;
	BG.Buffer = IPPU.TileCache [TILE_4BIT];
	BG.Buffered = IPPU.TileCached [TILE_4BIT];
	BG.NameSelect = PPU.OBJNameSelect;
	BG.DirectColourMode = FALSE;
	BG.Depth = depth;
	BG.SubY = sub ? 256 : 0;

	GFX.PixSize = 1;
	
	//printf ("OBJ p%d count = %d\n", p, BG.DrawTileLaterBGIndexCount[p]);
	for (int i = 0; i < BG.DrawTileLaterBGIndexCount[p]; i++)
	{
		int index = BG.DrawTileLaterBGIndex[p][i];
		if (BG.DrawTileLaterParameters[index][0] == 0)
		{
			S9xDrawOBJTileHardware2 (
				BG.DrawTileLaterParameters[index][1], BG.DrawTileLaterParameters[index][2], BG.DrawTileLaterParameters[index][3], 
				BG.DrawTileLaterParameters[index][4]);
		}
		else if (BG.DrawTileLaterParameters[index][0] == 1)
		{
			// clipped tile.
			//printf ("clip OBJ: %d %d %d %d %d %d\n", BG.DrawOBJTileParameters[p][i][1], BG.DrawOBJTileParameters[p][i][2], 
			//	BG.DrawOBJTileParameters[p][i][3], BG.DrawOBJTileParameters[p][i][4],
			//	BG.DrawOBJTileParameters[p][i][5], BG.DrawOBJTileParameters[p][i][6]);
			
			S9xDrawOBJClippedTileHardware (
				BG.DrawTileLaterParameters[index][1], BG.DrawTileLaterParameters[index][2], BG.DrawTileLaterParameters[index][3], 
				BG.DrawTileLaterParameters[index][4], BG.DrawTileLaterParameters[index][5], BG.DrawTileLaterParameters[index][6]);
		}
	}	
}


//-------------------------------------------------------------------
// Draw the OBJ layers using 3D hardware.
//-------------------------------------------------------------------
void S9xDrawOBJSHardware (bool8 sub, int depth = 0, int priority = 0)
{
#ifdef MK_DEBUG_RTO
	if(Settings.BGLayering) fprintf(stderr, "Entering DrawOBJS() for %d-%d\n", GFX.StartY, GFX.EndY);
#endif
	CHECK_SOUND();

	//printf ("--------------------\n");
	int p = 0;			// To be used in the DrawTileLater/DrawClippedTileLater macros.
	
	BG.BitShift = 4;
	BG.TileShift = 5;
	BG.TileAddress = PPU.OBJNameBase;
	BG.StartPalette = 128;
	BG.PaletteShift = 4;
	BG.PaletteMask = 7;
	BG.Buffer = IPPU.TileCache [TILE_4BIT];
	BG.Buffered = IPPU.TileCached [TILE_4BIT];
	BG.NameSelect = PPU.OBJNameSelect;
	BG.DirectColourMode = FALSE;
	BG.Depth = depth;
	BG.SubY = sub ? 256 : 0;

	GFX.PixSize = 1;

	struct {
		uint16 Pos;
		bool8 Value;
	} Windows[7];
	int clipcount = GFX.pCurrentClip->Count [4];
	if (!clipcount){
		Windows[0].Pos=0;
		Windows[0].Value=TRUE;
		Windows[1].Pos=256;
		Windows[1].Value=FALSE;
		Windows[2].Pos=1000;
		Windows[2].Value=FALSE;
	} else {
		Windows[0].Pos=1000;
		Windows[0].Value=FALSE;
		for(int clip=0, i=1; clip<clipcount; clip++){
			if(GFX.pCurrentClip->Right[clip][4]<=GFX.pCurrentClip->Left[clip][4]) continue;
			int j;
			for(j=0; j<i && Windows[j].Pos<GFX.pCurrentClip->Left[clip][4]; j++);
			if(j<i && Windows[j].Pos==GFX.pCurrentClip->Left[clip][4]){
				Windows[j].Value = TRUE;
			} else {
				if(j<i) memmove(&Windows[j+1], &Windows[j], sizeof(Windows[0])*(i-j));
				Windows[j].Pos = GFX.pCurrentClip->Left[clip][4];
				Windows[j].Value = TRUE;
				i++;
			}
			for(j=0; j<i && Windows[j].Pos<GFX.pCurrentClip->Right[clip][4]; j++);
			if(j>=i || Windows[j].Pos!=GFX.pCurrentClip->Right[clip][4]){
				if(j<i) memmove(&Windows[j+1], &Windows[j], sizeof(Windows[0])*(i-j));
				Windows[j].Pos = GFX.pCurrentClip->Right[clip][4];
				Windows[j].Value = FALSE;
				i++;
			}
		}
	}

#ifdef MK_DEBUG_RTO
if(Settings.BGLayering) {
	fprintf(stderr, "Windows:\n");
	for(int xxx=0; xxx<6; xxx++){ fprintf(stderr, "%d: %d = %d\n", xxx, Windows[xxx].Pos, Windows[xxx].Value); }
}
#endif

	/* We will not support Hires 
	if (Settings.SupportHiRes)
	{
		if (PPU.BGMode == 5 || PPU.BGMode == 6)
		{
			// Bah, OnMain is never used except to determine if calling
			// SelectTileRenderer is necessary. So let's hack it to false here
			// to stop SelectTileRenderer from being called when it causes
			// problems.
			OnMain = FALSE;
			GFX.PixSize = 2;
			if (IPPU.DoubleHeightPixels)

			{
				if (Settings.SixteenBit)
				{
					DrawTilePtr = DrawTile16x2x2;
					DrawClippedTilePtr = DrawClippedTile16x2x2;
				}
				else
				{
					DrawTilePtr = DrawTilex2x2;
					DrawClippedTilePtr = DrawClippedTilex2x2;
				}
			}
			else
			{
				if (Settings.SixteenBit)
				{
					DrawTilePtr = DrawTile16x2;
					DrawClippedTilePtr = DrawClippedTile16x2;
				}
				else
				{
					DrawTilePtr = DrawTilex2;
					DrawClippedTilePtr = DrawClippedTilex2;
				}
			}
		}
		else
		{
			if (Settings.SixteenBit)
			{
				DrawTilePtr = DrawTile16;
				DrawClippedTilePtr = DrawClippedTile16;
			}
			else
			{
				DrawTilePtr = DrawTile;
				DrawClippedTilePtr = DrawClippedTile;
			}
		}
	}
	//GFX.Z1 = D + 2;*/

	for(uint32 Y=GFX.StartY, Offset=Y*GFX.PPL; Y<=GFX.EndY; Y++, Offset+=GFX.PPL)
	{
#ifdef MK_DEBUG_RTO
		bool8 Flag=0;
#endif
		int I = 0;
#ifdef MK_DISABLE_TIME_OVER
		int tiles=0;
#else
		int tiles=GFX.OBJLines[Y].Tiles;
#endif
		//for (int S = GFX.OBJLines[Y].OBJ[I].Sprite; S >= 0 && I<32; S = GFX.OBJLines[Y].OBJ[++I].Sprite)
		for (int I = GFX.OBJLines[Y].OBJCount - 1; I >= 0; I --)
		{
			int S = GFX.OBJLines[Y].OBJ[I].Sprite;
			if (S < 0) continue;
			
			//if (PPU.OBJ[S].Priority != priority)
			//	continue;
			
			tiles+=GFX.OBJVisibleTiles[S];
			if(tiles<=0){
				continue;
			}

			int BaseTile = (((GFX.OBJLines[Y].OBJ[I].Line<<1) + (PPU.OBJ[S].Name&0xf0))&0xf0) | (PPU.OBJ[S].Name&0x100) | (PPU.OBJ[S].Palette << 10);
			int TileX = PPU.OBJ[S].Name&0x0f;
			int TileLine = (GFX.OBJLines[Y].OBJ[I].Line&7)*8;
			int TileInc = 1;

			if (PPU.OBJ[S].HFlip)
			{
				TileX = (TileX + (GFX.OBJWidths[S] >> 3) - 1) & 0x0f;
				BaseTile |= H_FLIP;
				TileInc = -1;
			}

			//GFX.Z2 = (PPU.OBJ[S].Priority + 1) * 4 + D;

			bool8 WinStat=TRUE;
			int WinIdx=0, NextPos=-1000;
			int X=PPU.OBJ[S].HPos; if(X==-256) X=256;

			if (!clipcount)
			{
				// No clipping at all.
				//
				for (int t=tiles, O=Offset+X*GFX.PixSize; X<=256 && X<PPU.OBJ[S].HPos+GFX.OBJWidths[S]; TileX=(TileX+TileInc)&0x0f, X+=8, O+=8*GFX.PixSize)
				{
					DrawOBJTileLater (4 + PPU.OBJ[S].Priority - 1, BaseTile|TileX, X, Y, TileLine);
				} // end for
			}
			else
			{
				// Clip with windows.
				//
				for (int t=tiles, O=Offset+X*GFX.PixSize; X<=256 && X<PPU.OBJ[S].HPos+GFX.OBJWidths[S]; TileX=(TileX+TileInc)&0x0f, X+=8, O+=8*GFX.PixSize)
				{
	#ifdef MK_DEBUG_RTO
	if(Settings.BGLayering) {
					if(X<-7) continue;
					if((t-1)<0) fprintf(stderr, "-[%d]", 35-t);
					else fprintf(stderr, "-%d", 35-t);
	}
	#endif
					if(X<-7 || --t<0 || X==256) continue;
					if(X>=NextPos){
						for(; WinIdx<7 && Windows[WinIdx].Pos<=X; WinIdx++);
						if(WinIdx==0) WinStat=FALSE;
						else WinStat=Windows[WinIdx-1].Value;
						NextPos=(WinIdx<7)?Windows[WinIdx].Pos:1000;
					}

					if(X+8<NextPos)
					{
						if(WinStat) 
						{
							//printf ("  p%d : %x (full) @ %x, %d\n", PPU.OBJ[S].Priority, BaseTile|TileX, O, TileLine);
							DrawOBJTileLater (4 + PPU.OBJ[S].Priority - 1, BaseTile|TileX, X, Y, TileLine);
						}
					} 
					else 
					{
						int x=X;
						while(x<X+8)
						{
							if(WinStat) 
							{
								DrawClippedOBJTileLater (4 + PPU.OBJ[S].Priority - 1, BaseTile|TileX, O, x-X, NextPos-x, TileLine, 1);
							}
							x=NextPos;
							for(; WinIdx<7 && Windows[WinIdx].Pos<=x; WinIdx++);
							if(WinIdx==0) WinStat=FALSE;
							else WinStat=Windows[WinIdx-1].Value;
							NextPos=(WinIdx<7)?Windows[WinIdx].Pos:1000;
							if(NextPos>X+8) NextPos=X+8;
						}
					}
				} // end for				
			}
		}
#ifdef MK_DEBUG_RTO
		if(Settings.BGLayering) if(Flag) fprintf(stderr, "\n");
#endif
	}
#ifdef MK_DEBUG_RTO
	if(Settings.BGLayering) fprintf(stderr, "Exiting DrawOBJS() for %d-%d\n", GFX.StartY, GFX.EndY);
#endif
}

void S9xRenderScreenHardware (bool8 sub, bool8 force_no_add, uint8 D)
{
    bool8 BG0;
    bool8 BG1;
    bool8 BG2;
    bool8 BG3;
    bool8 OB;
	
	int BGDepth0 = 0, BGDepth1 = 0, BGDepth2 = 0, BGDepth3 = 0, OBDepth = 0, BackDepth = 0;
	
    if (!sub)
    {
		// Main Screen
		GFX.pCurrentClip = &IPPU.Clip [0];
		BG0 = ON_MAIN (0);
		BG1 = ON_MAIN (1);
		BG2 = ON_MAIN (2);
		BG3 = ON_MAIN (3);
		OB  = ON_MAIN (4);

		BGDepth0 = SUB_OR_ADD (0) ? 0 : 1;		
		BGDepth1 = SUB_OR_ADD (1) ? 0 : 1;		
		BGDepth2 = SUB_OR_ADD (2) ? 0 : 1;		
		BGDepth3 = SUB_OR_ADD (3) ? 0 : 1;		
		OBDepth = SUB_OR_ADD (4) ? 0 : 1;		
		BackDepth = SUB_OR_ADD (5) ? 0 : 1;		
    }
    else
    {
		// Sub Screen
		GFX.pCurrentClip = &IPPU.Clip [1];
		BG0 = ON_SUB (0);
		BG1 = ON_SUB (1);
		BG2 = ON_SUB (2);
		BG3 = ON_SUB (3);
		OB  = ON_SUB (4);
    }
	
    sub |= force_no_add;
	
	int depth = 0;
	
	/*
	#define DRAW_OBJS(p)  \
		if (OB) \
		{ \
			t3dsStartTiming(26, "DrawOBJS"); \
			S9xDrawOBJSHardware (!sub, depth, p); \
			t3dsEndTiming(26); \
		}	
		*/
	
	#define DRAW_OBJS(p)  \
		if (OB) \
		{ \
			t3dsStartTiming(26, "DrawOBJS"); \
			if (p == 0) \
			{ \
				S9xDrawOBJSHardware (sub, OBDepth, p); \
				S9xDrawOBJSHardwarePriority (sub, OBDepth, p); \
			} \
			else \
				S9xDrawOBJSHardwarePriority (sub, OBDepth, p); \
			t3dsEndTiming(26); \
		}	
		
	#define DRAW_BG(bg, d1, d2, p) \
		if (BG##bg) \
		{ \
			if (bg == 0) { t3dsStartTiming(21, "DrawBG0"); } \
			if (bg == 1) { t3dsStartTiming(22, "DrawBG1"); } \
			if (bg == 2) { t3dsStartTiming(23, "DrawBG2"); } \
			if (bg == 3) { t3dsStartTiming(24, "DrawBG3"); } \
			if (p == 0) \
				S9xDrawBackgroundHardware (PPU.BGMode, bg, sub, BGDepth##bg, p); \
			else \
				S9xDrawBackgroundHardwarePriority1 (PPU.BGMode, bg, sub, BGDepth##bg); \
			t3dsEndTiming(21 + bg); \
		}
	
	#define DRAW_4COLOR_BG_INLINE(bg, p) \
		if (BG##bg) \
		{ \
			if (bg == 0) { t3dsStartTiming(21, "DrawBG0"); } \
			if (bg == 1) { t3dsStartTiming(22, "DrawBG1"); } \
			if (bg == 2) { t3dsStartTiming(23, "DrawBG2"); } \
			if (bg == 3) { t3dsStartTiming(24, "DrawBG3"); } \
			if (p == 0) \
				S9xDrawBackgroundHardwarePriority0Inline_4Color (PPU.BGMode, bg, sub, BGDepth##bg); \
			else \
				S9xDrawBackgroundHardwarePriority1Inline_4Color (PPU.BGMode, bg, sub, BGDepth##bg); \
			t3dsEndTiming(21 + bg); \
		}
	
	#define DRAW_16COLOR_BG_INLINE(bg, p) \
		if (BG##bg) \
		{ \
			if (bg == 0) { t3dsStartTiming(21, "DrawBG0"); } \
			if (bg == 1) { t3dsStartTiming(22, "DrawBG1"); } \
			if (bg == 2) { t3dsStartTiming(23, "DrawBG2"); } \
			if (bg == 3) { t3dsStartTiming(24, "DrawBG3"); } \
			if (p == 0) \
				S9xDrawBackgroundHardwarePriority0Inline_16Color (PPU.BGMode, bg, sub, BGDepth##bg); \
			else \
				S9xDrawBackgroundHardwarePriority1Inline_16Color (PPU.BGMode, bg, sub, BGDepth##bg); \
			t3dsEndTiming(21 + bg); \
		}
	
	#define DRAW_256COLOR_BG_INLINE(bg, p) \
		if (BG##bg) \
		{ \
			if (bg == 0) { t3dsStartTiming(21, "DrawBG0"); } \
			if (bg == 1) { t3dsStartTiming(22, "DrawBG1"); } \
			if (bg == 2) { t3dsStartTiming(23, "DrawBG2"); } \
			if (bg == 3) { t3dsStartTiming(24, "DrawBG3"); } \
			if (p == 0) \
				S9xDrawBackgroundHardwarePriority0Inline_256Color (PPU.BGMode, bg, sub, BGDepth##bg); \
			else \
				S9xDrawBackgroundHardwarePriority1Inline_256Color (PPU.BGMode, bg, sub, BGDepth##bg); \
			t3dsEndTiming(21 + bg); \
		}
	
	
	// Initialize the draw later parameters 
	// (Maybe creating the actual vertexes might be a better idea instead?)
	//
	BG.DrawTileLaterParametersCount = 0;
	BG.DrawTileLaterBGIndexCount[0] = 0;
	BG.DrawTileLaterBGIndexCount[1] = 0;
	BG.DrawTileLaterBGIndexCount[2] = 0;
	BG.DrawTileLaterBGIndexCount[3] = 0;
	BG.DrawTileLaterBGIndexCount[4] = 0;
	BG.DrawTileLaterBGIndexCount[5] = 0;
	BG.DrawTileLaterBGIndexCount[6] = 0;
	
 	if (PPU.BGMode <= 6)
	{
		switch (PPU.BGMode)
		{
			case 0:
		        gpu3dsSetTextureEnvironmentReplaceColor();
				//gpu3dsUseShader(0);
				S9xDrawBackdropHardware(sub, depth);

	            gpu3dsSetTextureEnvironmentReplaceTexture0();
				//gpu3dsUseShader(1);
				DRAW_4COLOR_BG_INLINE (3, 0);
				DRAW_4COLOR_BG_INLINE (2, 0);
				DRAW_OBJS(0);

				DRAW_4COLOR_BG_INLINE (3, 1);
				DRAW_4COLOR_BG_INLINE (2, 1);
				DRAW_OBJS(1);

				DRAW_4COLOR_BG_INLINE (1, 0);
				DRAW_4COLOR_BG_INLINE (0, 0);
				DRAW_OBJS(2);

				DRAW_4COLOR_BG_INLINE (1, 1);
				DRAW_4COLOR_BG_INLINE (0, 1);
				DRAW_OBJS(3);
				gpu3dsDrawVertexes();				
				break;
			case 1:
		        gpu3dsSetTextureEnvironmentReplaceColor();
				//gpu3dsUseShader(0);
				S9xDrawBackdropHardware(sub, depth);

	            gpu3dsSetTextureEnvironmentReplaceTexture0();
				//gpu3dsUseShader(1);
				DRAW_4COLOR_BG_INLINE(2, 0);
				DRAW_OBJS(0);
				if (!PPU.BG3Priority)
				{
					DRAW_4COLOR_BG_INLINE(2, 1);
				}	
				DRAW_OBJS(1);
				DRAW_16COLOR_BG_INLINE(1, 0);
				DRAW_16COLOR_BG_INLINE(0, 0);
				DRAW_OBJS(2);
				DRAW_16COLOR_BG_INLINE(1, 1);
				DRAW_16COLOR_BG_INLINE(0, 1);
				DRAW_OBJS(3);
				if (PPU.BG3Priority)
				{
					DRAW_4COLOR_BG_INLINE(2, 1);
				}
				gpu3dsDrawVertexes();				
				
				break;
			case 2: 
		        gpu3dsSetTextureEnvironmentReplaceColor();
				//gpu3dsUseShader(0);
				S9xDrawBackdropHardware(sub, depth);

	            gpu3dsSetTextureEnvironmentReplaceTexture0();
				//gpu3dsUseShader(1);
				DRAW_OBJS(0);
				DRAW_16COLOR_BG_INLINE (1, 0);
				
				DRAW_OBJS(1);
				DRAW_16COLOR_BG_INLINE (0, 0);
				
				DRAW_OBJS(2);
				DRAW_16COLOR_BG_INLINE (1, 1);
				
				DRAW_OBJS(3);
				DRAW_16COLOR_BG_INLINE (0, 1);
				
				gpu3dsDrawVertexes();				
				
				break;
			case 3: 
		        gpu3dsSetTextureEnvironmentReplaceColor();
				//gpu3dsUseShader(0);
				S9xDrawBackdropHardware(sub, depth);

	            gpu3dsSetTextureEnvironmentReplaceTexture0();
				//gpu3dsUseShader(1);
				DRAW_OBJS(0);
				DRAW_16COLOR_BG_INLINE (1, 0);
				
				DRAW_OBJS(1);
				DRAW_256COLOR_BG_INLINE (0, 0);
				
				DRAW_OBJS(2);
				DRAW_16COLOR_BG_INLINE (1, 1);
				
				DRAW_OBJS(3);
				DRAW_256COLOR_BG_INLINE (0, 1);
				
				gpu3dsDrawVertexes();				
				
				break;
			case 4: 
		        gpu3dsSetTextureEnvironmentReplaceColor();
				//gpu3dsUseShader(0);
				S9xDrawBackdropHardware(sub, depth);

	            gpu3dsSetTextureEnvironmentReplaceTexture0();
				//gpu3dsUseShader(1);
				DRAW_OBJS(0);
				DRAW_16COLOR_BG_INLINE (1, 0);
				
				DRAW_OBJS(1);
				DRAW_256COLOR_BG_INLINE (0, 0);
				
				DRAW_OBJS(2);
				DRAW_16COLOR_BG_INLINE (1, 1);
				
				DRAW_OBJS(3);
				DRAW_256COLOR_BG_INLINE (0, 1);
				
				gpu3dsDrawVertexes();				
				
				break;
			case 5:
		        gpu3dsSetTextureEnvironmentReplaceColor();
				//gpu3dsUseShader(0);
				S9xDrawBackdropHardware(sub, depth);

	            gpu3dsSetTextureEnvironmentReplaceTexture0();
				//gpu3dsUseShader(1);
				DRAW_OBJS(0);
				DRAW_4COLOR_BG_INLINE (1, 0);
				
				DRAW_OBJS(1);
				DRAW_16COLOR_BG_INLINE (0, 0);
				
				DRAW_OBJS(2);
				DRAW_4COLOR_BG_INLINE (1, 1);
				
				DRAW_OBJS(3);
				DRAW_16COLOR_BG_INLINE (0, 1);
				
				gpu3dsDrawVertexes();				
				
				break;
			case 6: 
		        gpu3dsSetTextureEnvironmentReplaceColor();
				//gpu3dsUseShader(0);
				S9xDrawBackdropHardware(sub, depth);
				
	            gpu3dsSetTextureEnvironmentReplaceTexture0();
				//gpu3dsUseShader(1);
				DRAW_OBJS(0);
				DRAW_16COLOR_BG_INLINE (0, 0);
				DRAW_OBJS(1);
				
				DRAW_OBJS(2);
				DRAW_16COLOR_BG_INLINE (0, 1);
				DRAW_OBJS(3);
				
				gpu3dsDrawVertexes();				
				
				break;
			case 7:
                // TODO: Mode 7 graphics.
                //
				break;
		}
	}
	/*
    else
    {
		// Mode 7
		if (OB)
		{
			SelectTileRenderer (sub || !SUB_OR_ADD(4));
			DrawOBJS (!sub, D, 0);
		}
		if (BG0 || ((Memory.FillRAM [0x2133] & 0x40) && BG1))
		{
			int bg;
			
			if ((Memory.FillRAM [0x2133] & 0x40)&&BG1)
			{
				GFX.Mode7Mask = 0x7f;
				GFX.Mode7PriorityMask = 0x80;
				Mode7Depths [0] = (BG0?5:1) + D;
				Mode7Depths [1] = 9 + D;
				bg = 1;
			}
			else
			{
				GFX.Mode7Mask = 0xff;
				GFX.Mode7PriorityMask = 0;
				Mode7Depths [0] = 5 + D;
				Mode7Depths [1] = 5 + D;
				bg = 0;
			}
			if (sub || !SUB_OR_ADD(0))
			{
				if (!Settings.Mode7Interpolate)
					DrawBGMode7Background16 (Screen, bg);
				else
					DrawBGMode7Background16_i (Screen, bg);
			}
			else
			{
				if (GFX.r2131 & 0x80)
				{
					if (GFX.r2131 & 0x40)
					{
						if (!Settings.Mode7Interpolate)
							DrawBGMode7Background16Sub1_2 (Screen, bg);
						else
							DrawBGMode7Background16Sub1_2_i (Screen, bg);
					}
					else
					{
						if (!Settings.Mode7Interpolate)
							DrawBGMode7Background16Sub (Screen, bg);
						else
							DrawBGMode7Background16Sub_i (Screen, bg);
					}
				}
				else
				{
					if (GFX.r2131 & 0x40)
					{
						if (!Settings.Mode7Interpolate)
							DrawBGMode7Background16Add1_2 (Screen, bg);
						else
							DrawBGMode7Background16Add1_2_i (Screen, bg);
					}
					else
					{
						if (!Settings.Mode7Interpolate)
							DrawBGMode7Background16Add (Screen, bg);
						else
							DrawBGMode7Background16Add_i (Screen, bg);
					}
				}
			}
		}
    }*/
}


// ********************************************************************************************

//-----------------------------------------------------------
// Render color math.
//-----------------------------------------------------------
inline void S9xRenderColorMath(int left, int right)
{
	if (GFX.r2130 & 2)
	{
		if (ANYTHING_ON_SUB)
		{
			gpu3dsEnableDepthTest();
			
			// Subscreen math
			//
			//gpu3dsUseShader(0);
			gpu3dsSetTextureEnvironmentReplaceTexture0();
			gpu3dsBindTextureSubScreen(GPU_TEXUNIT0);
			gpu3dsSetRenderTarget(1);
			//printf ("Drawing sub\n");

			if (GFX.r2131 & 0x80)
			{
				// Subtractive
				if (GFX.r2131 & 0x40) gpu3dsEnableSubtractiveDiv2Blending();	// div 2
				else gpu3dsEnableSubtractiveBlending();						// no div
			}
			else
			{
				// Additive
				if (GFX.r2131 & 0x40) gpu3dsEnableAdditiveDiv2Blending();	// div 2
				else gpu3dsEnableAdditiveBlending();					// no div
			}
			
			gpu3dsAddTileVertexes(left, GFX.StartY, right, GFX.EndY + 1, 
				left, GFX.StartY, right, GFX.EndY + 1, 0);
			gpu3dsDrawVertexes();

			gpu3dsDisableDepthTest();
			
		}
	}
	else
	{
		// Fixed color math
		//
		int fixedColour = 
			(PPU.FixedColourRed << (3 + 24)) |
			(PPU.FixedColourGreen << (3 + 16)) |
			(PPU.FixedColourBlue << (3 + 8)) |
			0xff;
		
		if (fixedColour != 0xff)
		{
			gpu3dsEnableDepthTest();

			//gpu3dsUseShader(0);
			gpu3dsSetTextureEnvironmentReplaceColor();
			gpu3dsSetRenderTarget(1);

			if (GFX.r2131 & 0x80)
			{
				// Subtractive
				if (GFX.r2131 & 0x40) gpu3dsEnableSubtractiveDiv2Blending();	// div 2
				else gpu3dsEnableSubtractiveBlending();						// no div
			}
			else
			{
				// Additive
				if (GFX.r2131 & 0x40) gpu3dsEnableAdditiveDiv2Blending();	// div 2
				else gpu3dsEnableAdditiveBlending();					// no div
			}
			
			gpu3dsDrawRectangle(left, GFX.StartY, right, GFX.EndY + 1, 0, fixedColour);
			gpu3dsDisableDepthTest();
			
		}
	}
}

inline void S9xRenderColorMath()
{
	// TODO: Settle windowing 
	if (!IPPU.Clip[1].Count[5])
	{
		S9xRenderColorMath(0, 256);
	}
	else
	{
		for (int i = 0; i < IPPU.Clip[1].Count[5]; i++)
		{
			if (IPPU.Clip[1].Right[i][5] > IPPU.Clip[1].Left[i][5])
				S9xRenderColorMath(IPPU.Clip[1].Left[i][5], IPPU.Clip[1].Right[i][5]);
		}
	}

}


//-----------------------------------------------------------
// Updates the screen using the 3D hardware.
//-----------------------------------------------------------
void S9xUpdateScreenHardware ()
{
	t3dsStartTiming(11, "S9xUpdateScreen");
    int32 x2 = 1;
	
    GFX.S = GFX.Screen;
    GFX.r2131 = Memory.FillRAM [0x2131];
    GFX.r212c = Memory.FillRAM [0x212c];
    GFX.r212d = Memory.FillRAM [0x212d];
    GFX.r2130 = Memory.FillRAM [0x2130];

#ifdef JP_FIX

    GFX.Pseudo = (Memory.FillRAM [0x2133] & 8) != 0 &&
				 (GFX.r212c & 15) != (GFX.r212d & 15) &&
				 (GFX.r2131 == 0x3f);

#else

    GFX.Pseudo = (Memory.FillRAM [0x2133] & 8) != 0 &&
		(GFX.r212c & 15) != (GFX.r212d & 15) &&
		(GFX.r2131 & 0x3f) == 0;

#endif
	
    if (IPPU.OBJChanged)
		S9xSetupOBJ ();
	
    if (PPU.RecomputeClipWindows)
    {
		ComputeClipWindows ();
		PPU.RecomputeClipWindows = FALSE;
    }
	
    GFX.StartY = IPPU.PreviousLine;
    if ((GFX.EndY = IPPU.CurrentLine - 1) >= PPU.ScreenHeight)
		GFX.EndY = PPU.ScreenHeight - 1;

	// XXX: Check ForceBlank? Or anything else?
	PPU.RangeTimeOver |= GFX.OBJLines[GFX.EndY].RTOFlags;
	
    uint32 starty = GFX.StartY;
    uint32 endy = GFX.EndY;
	
	/*if (GFX.EndY - GFX.StartY < 40)
	{
		if (starty == 0)
			printf ("-----------------------\n");
		printf ("Render %d-%d Bl = %d Brt = %d\n", starty, endy, PPU.ForcedBlanking, PPU.Brightness);
	}*/
	
	if (!PPU.ForcedBlanking && PPU.Brightness != 0)
	{
		GPU_SetDepthTestAndWriteMask(false, GPU_NOTEQUAL, GPU_WRITE_ALL);
		gpu3dsEnableAlphaBlending();
		if (ANYTHING_ON_SUB)
		{
			// Render the subscreen
			//
			gpu3dsBindTextureSnesTileCache(GPU_TEXUNIT0);
			gpu3dsSetRenderTarget(2);
			S9xRenderScreenHardware (TRUE, TRUE, SUB_SCREEN_DEPTH);
		}
		
		// Render the main screen.
		//
		gpu3dsSetRenderTarget(1);
		gpu3dsBindTextureSnesTileCache(GPU_TEXUNIT0);
		S9xRenderScreenHardware (FALSE, FALSE, MAIN_SCREEN_DEPTH);

		/*
		// Testing only.
		gpu3dsBindTextureSnesTileCache(GPU_TEXUNIT0);
		gpu3dsSetTextureEnvironmentReplaceTexture0();
		gpu3dsAddTileVertexes(0, 0, 8, 8, 80, 0, 88, 8, 0.1f);
		*/
		
		// Do color math here
		//
		// ...
		S9xRenderColorMath();
		
		// Set master brightness here by overlaying the screen
		// with a translucent black rectangle. 
		// 
		if (PPU.Brightness != 0xF)
		{
			int32 alpha = 0xF - PPU.Brightness;
			alpha = alpha | (alpha << 4);
			//gpu3dsUseShader(0);
			gpu3dsEnableAlphaBlending();
			gpu3dsSetTextureEnvironmentReplaceColor();
			gpu3dsDrawRectangle(0, GFX.StartY, 256, GFX.EndY + 1, 0, alpha);
			//printf ("Brightness: %d - %d, %8x\n", GFX.StartY, GFX.EndY + 1, alpha);
		}
	}
	else
	{
		// Forced blank, or black brightness, so we clear the area with black.
		//gpu3dsUseShader(0);
		gpu3dsSetRenderTarget(1);
		gpu3dsSetTextureEnvironmentReplaceColor();
		gpu3dsDrawRectangle(0, GFX.StartY, 256, GFX.EndY + 1, 0, 0xff);
	}
	
    IPPU.PreviousLine = IPPU.CurrentLine;	
	t3dsEndTiming(11);
}
