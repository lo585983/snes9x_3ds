
#define _CLIPHW_CPP_
#include "cliphw.h"
#include "3dsgpu.h"

//----------------------------------------------------------------
// Adapted from BlargSNES
//----------------------------------------------------------------

#define WINMASK_OUT (3|(3<<2))
#define WINMASK_1   (2|(3<<2))
#define WINMASK_2   (3|(2<<2))
#define WINMASK_12  (2|(2<<2))


const u16 PPU_WindowCombine[] = 
{
	/* OR
	1 1 1 1
	1 0 0 0
	1 0 0 0
	1 0 0 0 */
	0x111F,
	
	/* AND
	1 0 1 1
	0 0 0 0
	1 0 0 0
	1 0 0 0 */
	0x110D,
	
	/* XOR
	0 1 1 1
	1 0 0 0
	1 0 0 0
	1 0 0 0 */
	0x111E,
	
	/* XNOR
	1 0 1 1
	0 1 0 0
	1 0 0 0
	1 0 0 0 */
	0x112D
};


inline void PPU_ComputeSingleWindow(PPU_WindowSegment* s, u32 x1, u32 x2, u32 mask)
{
	if (x1 < x2)
	{
		s->EndOffset = x1;
		s->WindowMask = WINMASK_OUT;
		s++;
		
		s->EndOffset = x2;
		s->WindowMask = mask;
		s++;
	}
	
	s->EndOffset = 256;
	s->WindowMask = WINMASK_OUT;
}


void PPU_ComputerDoubleWindow(PPU_WindowSegment* s)
{
	if (PPU.Window1Left < PPU.Window2Left)
	{
		// window 1 first
		
		// border to win1 x1
		s->EndOffset = PPU.Window1Left;
		s->WindowMask = WINMASK_OUT;
		s++;
		
		if (PPU.Window2Left < PPU.Window1Right)
		{
			// windows overlapped
			
			// win1 x1 to win2 x1
			s->EndOffset = PPU.Window2Left;
			s->WindowMask = WINMASK_1;
			s++;

			if(PPU.Window2Right <= PPU.Window1Right)
			{
				// win2 fully in win1

				// close win2
				s->EndOffset = PPU.Window2Right;
				s->WindowMask = WINMASK_12;
				s++;

				// close win1
				s->EndOffset = PPU.Window1Right;
				s->WindowMask = WINMASK_1;
				s++;

				// finish
				s->EndOffset = 256;
				s->WindowMask = WINMASK_OUT;
				return;
			}
			
			// windows intersect

			// win2 x1 to win1 x2
			s->EndOffset = PPU.Window1Right;
			s->WindowMask = WINMASK_12;
			s++;
		}
		else
		{
			// windows separate
				
			// win1 x1 to win1 x2
			s->EndOffset = PPU.Window1Right;
			s->WindowMask = WINMASK_1;
			s++;
				
			// win1 x2 to win2 x1
			s->EndOffset = PPU.Window2Left;
			s->WindowMask = WINMASK_OUT;
			s++;
		}
			
		// to win2 x2
		s->EndOffset = PPU.Window2Right;
		s->WindowMask = WINMASK_2;
		s++;
			
		// win2 x2 to border
		s->EndOffset = 256;
		s->WindowMask = WINMASK_OUT;
	}
	else
	{
		// window 2 first
		
		// border to win2 x1
		s->EndOffset = PPU.Window2Left;
		s->WindowMask = WINMASK_OUT;
		s++;
			
		if (PPU.Window1Left < PPU.Window2Right)
		{
			// windows overlapped
			
			// win2 x1 to win1 x1
			s->EndOffset = PPU.Window1Left;
			s->WindowMask = WINMASK_2;
			s++;

			if(PPU.Window1Right <= PPU.Window2Right)
			{
				// win1 fully in win2
				
				// close win1
				s->EndOffset = PPU.Window1Right;
				s->WindowMask = WINMASK_12;
				s++;

				// close win2
				s->EndOffset = PPU.Window2Right;
				s->WindowMask = WINMASK_2;
				s++;

				// finish
				s->EndOffset = 256;
				s->WindowMask = WINMASK_OUT;
				return;
			}

			// win1 x1 to win2 x2
			s->EndOffset = PPU.Window2Right;
			s->WindowMask = WINMASK_12;
			s++;
		}
		else
		{
			// windows separate
				
			// win2 x1 to win2 x2
			s->EndOffset = PPU.Window2Right;
			s->WindowMask = WINMASK_2;
			s++;
				
			// win2 x2 to win1 x1
			s->EndOffset = PPU.Window1Left;
			s->WindowMask = WINMASK_OUT;
			s++;
		}
			
		// to win1 x2
		s->EndOffset = PPU.Window1Right;
		s->WindowMask = WINMASK_1;
		s++;
			
		// win1 x2 to border
		s->EndOffset = 256;
		s->WindowMask = WINMASK_OUT;
	}
}


void PPU_ComputeWindows(PPU_WindowSegment* s)
{
	PPU_WindowSegment* first_s = s;
	//bprintf("-0- (%d,%d), -1- (%d,%d)\n", PPU.Window1Left, PPU.Window1Right, PPU.Window2Left, PPU.Window2Right);
	// check for cases that would disable windows fully
    
	//if ((!((PPU.MainScreen|PPU.SubScreen) & 0x1F00)) && 
	//	(((PPU.ColorMath1 & 0x30) == 0x00) || ((PPU.ColorMath1 & 0x30) == 0x30)))
    if ( 
            // Window disabled for OBJ + BG0 to 3
            !((Memory.FillRAM [0x212e] | Memory.FillRAM [0x212f]) & 0x1f) &&    

            // Never or always prevent color math 
            (
                ((Memory.FillRAM [0x2130] & 0x30) == 0x00) ||
                ((Memory.FillRAM [0x2130] & 0x30) == 0x30)
            )
        )
	{
		//bprintf("Disabled\n");
		s->EndOffset = 256;
		s->WindowMask = 0x0F;
		s->ColorMath = 0x10;
		return;
	}
	
	// first, check single-window cases

	if (PPU.Window2Left >= PPU.Window2Right)
	{
		PPU_ComputeSingleWindow(s, PPU.Window1Left, PPU.Window1Right, WINMASK_1);
	}
	else if (PPU.Window1Left >= PPU.Window1Right)
	{
		PPU_ComputeSingleWindow(s, PPU.Window2Left, PPU.Window2Right, WINMASK_2);
	}
	else
	{
		// okay, we have two windows
		PPU_ComputerDoubleWindow(s);
	}
	
	// precompute the final window for color math
	s = first_s;


	for (;;)
	{
		//u16 isinside = PPU.ColorMathWindowCombine & (1 << (s->WindowMask ^ PPU.ColorMathWindowMask));
        u16 isinside = PPU_WindowCombine[(Memory.FillRAM[0x212b] & 0x0C) >> 2] & 
            (1 << (s->WindowMask ^ (Memory.FillRAM[0x2125] >> 4)));
		s->ColorMath = isinside ? 0x20 : 0x10;
		
		if (s->EndOffset >= 256) break;
		s++;
	}
}


void PPU_ComputeWindows_Hard(PPU_WindowSegment* s)
{
	PPU_ComputeWindows(s);
	
    // TODO: We may have to shift this into the PPU/IPPU structure later
    //
    u16 mainScreen = Memory.FillRAM[0x212e] | (Memory.FillRAM[0x212c] << 8);
    u16 subScreen = Memory.FillRAM[0x212f] | (Memory.FillRAM[0x212d] << 8);

    u8 val = 0;
    val = Memory.FillRAM[0x212a];
    u8 windowCombine[4] = {0, 0, 0, 0};
    windowCombine[0] = PPU_WindowCombine[(val & 0x03)];
    windowCombine[1] = PPU_WindowCombine[(val & 0x0C) >> 2];
    windowCombine[2] = PPU_WindowCombine[(val & 0x30) >> 4];
    windowCombine[3] = PPU_WindowCombine[(val & 0xC0) >> 6];    

    val = Memory.FillRAM[0x212b];
    u8 objWindowCombine = PPU_WindowCombine[(val & 0x03)];
    u8 colorMathWindowCombine = PPU_WindowCombine[(val & 0x0C) >> 2];

    u8 windowMask[4] = {0, 0, 0, 0};

    val = Memory.FillRAM[0x2123];
    windowMask[0] = val & 0x0F;
    windowMask[1] = val >> 4;

    val = Memory.FillRAM[0x2124];
    windowMask[2] = val & 0x0F;
    windowMask[3] = val >> 4;

    val = Memory.FillRAM[0x2125];
    u8 objWindowMask = val & 0x0F;
    u8 colorMathWindowMask = val >> 4;


	// compute final window masks
	for (;;)
	{
		//u16 allenable = PPU.MainScreen|PPU.SubScreen;

        u16 allenable = mainScreen | subScreen;

		allenable &= (allenable >> 8);
		u8 finalmask = 0;
		
		if (allenable & 0x01)
			finalmask |= ((windowCombine[0] & (1 << (s->WindowMask ^ windowMask[0]))) ? 0x01 : 0);
		if (allenable & 0x02)
			finalmask |= ((windowCombine[1] & (1 << (s->WindowMask ^ windowMask[1]))) ? 0x02 : 0);
		if (allenable & 0x04)
			finalmask |= ((windowCombine[2] & (1 << (s->WindowMask ^ windowMask[2]))) ? 0x04 : 0);
		if (allenable & 0x08)
			finalmask |= ((windowCombine[3] & (1 << (s->WindowMask ^ windowMask[3]))) ? 0x08 : 0);
		
		if (allenable & 0x10)
			finalmask |= ((objWindowCombine & (1 << (s->WindowMask ^ objWindowMask))) ? 0x10 : 0);
		
		//if (s->ColorMath & PPU.ColorMath1)
        if (s->ColorMath & Memory.FillRAM [0x2130])
			finalmask |= 0x20;
			
		//s->FinalMaskMain = finalmask & (PPU.MainWindowEnable|0x20);
		//s->FinalMaskSub = finalmask & (PPU.SubWindowEnable|0x20);
		s->FinalMaskMain = finalmask & (Memory.FillRAM[0x212e]|0x20);
		s->FinalMaskSub = finalmask & (Memory.FillRAM[0x212f]|0x20);
		
		if (s->EndOffset >= 256) break;
		s++;
	}
}



void PPU_DrawWindowMask(u32 snum)
{
	// a bit of trickery is used here to easily fill the stencil buffer
	//
	// color buffer:  RRGGBBAA  (8-bit RGBA)
	// depth buffer:  SSDDDDDD  (8-bit stencil, 24-bit depth)
	//
	// thus we can use the depth buffer as a color buffer and write red
	
	//u8* vptr = (u8*)vertexPtr;
	
#define ADDVERTEX(x, y, a) \
	*(u16*)vptr = x; vptr += 2; \
	*(u16*)vptr = y; vptr += 2; \
	*vptr++ = a; vptr++;
	

	//bglUseShader(&windowMaskShaderP);

	
	//bglOutputBuffers(OBJDepthBuffer, OBJDepthBuffer);
	
	//bglEnableStencilTest(false);
	//bglStencilOp(GPU_KEEP, GPU_KEEP, GPU_KEEP);
	
	//bglEnableDepthTest(false);
	//bglEnableAlphaTest(false);
	//bglBlendEquation(GPU_BLEND_ADD, GPU_BLEND_ADD);
	//bglBlendFunc(GPU_ONE, GPU_ZERO, GPU_ONE, GPU_ZERO);
	
	//bglColorDepthMask(GPU_WRITE_RED);
	
	//bglUniformMatrix(GPU_VERTEX_SHADER, 0, snesProjMatrix);
	
	//bglNumAttribs(2);
	//bglAttribType(0, GPU_SHORT, 2);	// vertex
	//bglAttribType(1, GPU_UNSIGNED_BYTE, 2);	// color
	//bglAttribBuffer(vptr);
		
	int nvtx = 0;
	int ystart = 1;
	PPU_WindowSection* s = &IPPU.WindowSections[0];
	for (;;)
	{
		int xstart = 0;
		PPU_WindowSegment* ws = &s->Window[0];
		for (;;)
		{
			if (xstart < ws->EndOffset)
			{
				u8 alpha = snum ? ws->FinalMaskSub : ws->FinalMaskMain;
				//ADDVERTEX(xstart, ystart,       	    alpha);
				//ADDVERTEX(ws->EndOffset, s->EndOffset,  alpha);
				nvtx += 2;
			}
			
			if (ws->EndOffset >= 256) break;
			xstart = ws->EndOffset;
			ws++;
		}
		
		if (s->EndOffset >= PPU.ScreenHeight) break;
		ystart = s->EndOffset;
		s++;
	}
	
	//vptr = (u8*)((((u32)vptr) + 0x1F) & ~0x1F);
	
	//bglDrawArrays(GPU_UNKPRIM, nvtx);
	
	//vertexPtr = vptr;
	
#undef ADDVERTEX
}