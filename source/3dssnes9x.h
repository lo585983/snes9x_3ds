
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
    { \
        uint32 prevkey = 1; \
        while (aptMainLoop()) \ 
        {  \
            hidScanInput(); \ 
            uint32 key = hidKeysHeld(); \
            if (key == KEY_L) break; \
            if (key == KEY_SELECT) { GPU3DS.enableDebug ^= 1; break; } \
            if (prevkey == 0 && key != 0) \
                break;  \
            prevkey = key; \
        } \ 
    }
