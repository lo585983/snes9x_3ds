
// Uncomment this to convert before releasing this to remove
// all the debugging stuff.
//
#define RELEASE 

// Uncomment this to use 2 point geometry shaders on a real 3DS.
//
#define RELEASE_SHADER

// Uncomment this to allow user to break into debug mode (for the 65816 CPU)
//
//#define DEBUG_CPU

// Uncomment this to allow user to break into debug mode (for the SPC700 APU)
//
//#define DEBUG_APU



#define DEBUG_WAIT_L_KEY 	\
    while (aptMainLoop()) { hidScanInput(); if (hidKeysHeld() == 0) break; } \ 
    while (aptMainLoop()) { hidScanInput(); if (hidKeysHeld() == KEY_L) break; }
