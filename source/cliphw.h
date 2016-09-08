
#include "3ds.h"
#include "snes9x.h"
#include "ppu.h"

#ifndef _CLIPHW_H_
#define _CLIPHW_H_

//----------------------------------------------------------------
// Adapted from BlargSNES
//----------------------------------------------------------------

typedef struct
{
	// no start offset in here; start offset is the end offset of the previous segment
	u16 EndOffset;	// 256 = final segment
	u8 WindowMask;	// each 2 bits: 2=inside, 3=outside
	u8 ColorMath;	// 0x20 = inside color math window, 0x10 = outside
	u8 FinalMaskMain, FinalMaskSub; // for use by the hardware renderer
	
} PPU_WindowSegment;

typedef struct
{
	u8 EndOffset;
	PPU_WindowSegment Window[5];
} PPU_WindowSection;

#ifdef _CLIPHW_CPP_
#else
#endif

void PPU_ComputeWindows_Hard(PPU_WindowSegment* s);

#endif