#include "3ds.h"

#ifndef _3DSSOUND_H_
#define _3DSSOUND_H_

#define SAMPLE_RATE         21600


typedef struct 
{
    short       *fullBuffers;
    short       *leftBuffer;
    short       *rightBuffer;
    short       *dummyBuffer;
    u64			startTick;
    u64         bufferPosition;
    u64         samplePosition;

    Handle      dspThreadHandle;
    u8          dspThreadStack[0x4000] __attribute__((aligned(8)));
    bool        terminateDSPThread;

    u64         startSamplePosition = 0;
    u64         upToSamplePosition = 0;

    CSND_ChnInfo*   channelInfo;

} SSND3DS;


#ifdef _3DSSOUND_CPP_
    SSND3DS snd3DS;
#else
    extern "C" SSND3DS snd3DS;
#endif


extern "C" bool snd3dsInitialize();
//extern "C" void snd3dsInsertSamples(short *leftSamples, short *rightSamples, int count);
extern "C" void snd3dsDeinitialize();
extern "C" void snd3dsMixSamples();

#endif