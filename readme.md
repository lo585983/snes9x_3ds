To use:

1. Copy the Snes9x_3ds.3dsx into the \3ds\snes9x\ on your SD card. 
2. Place your SNES ROMs inside the same folder.
3. Go to your Homebrew Launcher (through Cubic Ninja) and launch the snes9x_3ds emulator.

When in-game,

1. Tap the bottom screen for the menu.
2. Use Up/Down to choose option, and A to confirm. 
3. Use Left/Right to change between ROM selection and emulator options.

Feedback and bug reports are welcome. Help with development is also welcome!


-------------------------------------------------------------------------------------------------------

Longer version, for those interested in the read:

I've spent over 4 months heavily optimising Snes9x for the 3DS (more specifically the old 3DS). I think it's reached a point where I can release it. Chose Snes9x v1.43 with the hope that compatibility will be better. 

Many SNES games run at 60fps on the old 3DS with this emulator. But the old 3DS, being old, has a terribly low under-powered CPU, so at some time the frame rates can still fall to 40fps or lower when the on-screen effects get heavy.

Optimisations:
1. Loop unrolls and appropriate global register uses.
2. Aggressive inline of innermost loop functions and memory load/store functions.
3. Merged structs. 
4. Generally optimised some 65816c instructions to avoid saving to global variables 
(if they are not going to be used at all, Snes9x does this a lot!)
4. SPC700 catch up only at HBlank, and when reading to/from $214x registers.
5. Caching of BRR samples
6. Use of the 3DS GPU for hardware acceleration
7. Use of the 3DS GPU hardware for some computation related to tile flips / texture coordinates.
8. The hardware rendering was derived from the original Snes9X software renderer. 
Thus, they don't scale very well during very heavy HDMA effects.
9. DSP runs off concurrently and independently on the syscore thread. 
Even without syncing with the 65816c/SPC700, it produces reasonable sound and music.

What's supported:
1. Graphic modes 0 - 5 (even offset-per-tile modes).
2. Save states!
3. Currently uses CSND for audio. So your entry point to home-brew must be able to use CSND.
(if you can play BlargSNES with sound, you should be able to play Snes9X_3DS with sound)

What's missing / needs to be improved:
1. Mode 7 (not yet, but coming soon!)
2. Sound interpolation / echo (will not be supported)
3. Some sound samples sound very wonky (don't know why)
4. Hi-res support (will not be supported)
5. All the other add-on chips. (not likely to support in the future)
6. Mosaics. (will not be supported)
7. Frame skipping toggling (subsequent versions)
8. Turbo buttons (subsequent versions)
9. Ugly menus... :D (great if someone can help improve on it)

Credits to:
1. Snes9x team for the fantastic SNES emulator
2. StapleButter for his work on BlargSNES. 
Some shader-related optimisations ideas came from him. 
Will likely rely on his codes for Mode 7 implementation too!
3. Author of SF2D library. Some codes for hardware acceleration evolved from here.