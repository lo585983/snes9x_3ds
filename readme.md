To use:

*Homebrew Launcher:*

1. Copy Snes9x_3ds.3dsx and snes9x_3ds_top.png into any folder on your SD card. 
2. Place your SNES ROMs inside any folder.
3. Go to your Homebrew Launcher (through Cubic Ninja) and launch the snes9x_3ds emulator.

*CIA Version:*

1. Use your favorite CIA installer to install snes9x_3ds.cia into your CFW.
2. Place your SNES ROMs inside any folder.
3. Copy snes9x_3ds_top.png to ROOT of your SD card.
4. Exit your CIA installer and go to your CFW's home screen to launch the app.

When in-game,

1. Tap the bottom screen for the menu.
2. Use Up/Down to choose option, and A to confirm. 
3. Use Left/Right to change between ROM selection and emulator options.
4. You can quit the emulator to your homebrew launcher / your CFW's home screen.

Feedback and bug reports are welcome. Help with development is also welcome!


-------------------------------------------------------------------------------------------------------

Longer version, for those interested in the read:

I've spent over 4 months heavily optimising Snes9x for the 3DS (more specifically the old 3DS). I think it's reached a point where I can release it. Chose Snes9x v1.43 with the hope that compatibility will be better. 

Many SNES games run at 60fps on the old 3DS with this emulator. But the old 3DS, being old, has a terribly low under-powered CPU, so at some time the frame rates can still fall to 40fps or lower when the on-screen effects get heavy.


Optimisations:

1. Loop unrolls and appropriate global register uses.
2. Aggressive inline of innermost loop functions and memory load/store functions.
3. Merged structs. 
4. Generally optimised some 65816c instructions to avoid saving to global variables (if they are not going to be used at all, Snes9x does this a lot!)
5. SPC700 catch up only at HBlank, and when reading to/from $214x registers.
6. Caching of BRR samples
7. Use of the 3DS GPU for hardware acceleration
8. Use of the 3DS GPU hardware for some computation related to tile flips / texture coordinates.
9. The hardware rendering was derived from the original Snes9X software renderer. Thus, they don't scale very well during very heavy HDMA effects.
10. The SNES DSP runs off concurrently and independently on the syscore thread. Even without syncing with the 65816c/SPC700, it produces reasonable sound and music.


What's supported:

1. Graphic modes 0 - 4 (even offset-per-tile modes), mode 7.
2. Save states of up to 4 slots
3. Currently uses CSND for audio. So your entry point to home-brew must be able to use CSND. (if you can play BlargSNES with sound, you should be able to play Snes9X_3DS with sound)
4. Frame skipping
5. Full screen 

What's missing / needs to be improved:

1. Sound interpolation / echo (will not be supported)
2. Some sound samples sound very wonky (don't know why)
3. Hi-res support (will not be supported)
4. All the other add-on chips. (not likely to support in the future)
5. Mosaics. (will not be supported)
6. Turbo buttons (subsequent versions)
7. CIA version doesn't not properly support resume of the emulator.


Credits to:

1. Snes9x team for the fantastic SNES emulator
2. StapleButter for his work on BlargSNES. Some shader-related optimisations ideas came from him. Will likely rely on his codes for Mode 7 implementation too!
3. Author of SF2D library. Some codes for hardware acceleration evolved from here.
4. Authors of the Citra 3DS Emulator team. Without them, this project would have been extremely difficult.
