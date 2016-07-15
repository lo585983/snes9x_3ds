
#define _3DSSOUND_CPP_
#include <stdio.h>

#include "snes9x.h"
#include "spc700.h"
#include "apu.h"
#include "soundux.h"

#include "3ds.h"
#include "3dsgpu.h"
#include "3dssound.h"
#include "3dsopt.h"

#define LEFT_CHANNEL        10
#define RIGHT_CHANNEL       11
//#define DUMMY_CHANNEL       12

#define BUFFER_SIZE         21600


uint8 mixedSamples[BUFFER_SIZE];



//#define TICKS_PER_SEC_LL 268111856LL
//#define TICKS_PER_SEC_LL 268090000LL
#define TICKS_PER_SEC_LL 268100000LL

u64 snd3dsGetSamplePosition() {
	u64 delta = (svcGetSystemTick() - snd3DS.startTick);
	u64 samplePosition = delta * SAMPLE_RATE / TICKS_PER_SEC_LL;

    snd3DS.samplePosition = samplePosition;
	return samplePosition;
}

int blockCount = 0;

void snd3dsMixSamples()
{
    #define SAMPLES_TO_GENERATE         360
    #define MIN_FORWARD_BLOCKS          4
    #define MAX_FORWARD_BLOCKS          6

    t3dsStartTiming(44, "Mix-S9xMix");
    if (GPU3DS.emulatorState == EMUSTATE_EMULATE)
    {
        S9xMixSamplesIntoTempBuffer(SAMPLES_TO_GENERATE * 2);
    }
    else
    {
        S9xGenerateSilenceIntoTempBuffer(SAMPLES_TO_GENERATE * 2);
    }
    t3dsEndTiming(44);

    t3dsStartTiming(41, "Mix-Timing");
    long generateAtSamplePosition = 0;
    while (true)
    {
        //snd3DS.channelInfo = csndGetChnInfo(DUMMY_CHANNEL);
        u64 nowSamplePosition = snd3dsGetSamplePosition();
        long deltaTimeAhead = snd3DS.upToSamplePosition - nowSamplePosition;
        long blocksAhead = deltaTimeAhead / SAMPLES_TO_GENERATE;
        
        if (blocksAhead < MIN_FORWARD_BLOCKS)
        {
            // buffer is about to underrun.
            //
            generateAtSamplePosition = 
                ((u64)((nowSamplePosition + SAMPLES_TO_GENERATE - 1) / SAMPLES_TO_GENERATE)) * SAMPLES_TO_GENERATE + 
                MIN_FORWARD_BLOCKS * SAMPLES_TO_GENERATE;
            break;
        }
        else if (blocksAhead < MAX_FORWARD_BLOCKS)
        {
            // play head is still within acceptable range.
            // so we place the generated samples at where 
            // we left off previously
            //
            generateAtSamplePosition = snd3DS.upToSamplePosition;
            break;
        }

        // blocksAhead >= MAX_FORWARD_BLOCKS
        // although we've already mixed the previous block,
        // but we are too ahead of time, so let's wait a while.
    }
    
    snd3DS.startSamplePosition = generateAtSamplePosition;
    snd3DS.upToSamplePosition = generateAtSamplePosition + SAMPLES_TO_GENERATE;
    t3dsEndTiming(41);

    t3dsStartTiming(42, "Mix-Copy+Vol");
    int p = generateAtSamplePosition % BUFFER_SIZE;
    S9xApplyMasterVolumeOnTempBufferIntoLeftRightBuffers(&snd3DS.leftBuffer[p], &snd3DS.rightBuffer[p], SAMPLES_TO_GENERATE * 2);
    t3dsEndTiming(42);

    // Now that we have the samples, we have to copy it back into our buffers
    // for the 3DS to playback
    //
    t3dsStartTiming(43, "Mix-Flush");    
    blockCount++;
    if (blockCount % MIN_FORWARD_BLOCKS == 0)
        GSPGPU_FlushDataCache(snd3DS.fullBuffers, BUFFER_SIZE * 2 * 2);
    t3dsEndTiming(43);
}

void snd3dsDSPThread(void *p)
{
    int mask = BUFFER_SIZE - 1;
    snd3DS.upToSamplePosition = snd3dsGetSamplePosition();
    snd3DS.startSamplePosition = snd3DS.upToSamplePosition;
    //svcExitThread();
    //return;

    while (!snd3DS.terminateDSPThread)
    {
        if (!GPU3DS.isReal3DS)
            svcSleepThread(1000000 * 1);
        snd3dsMixSamples();
    }
    svcExitThread();
}


bool snd3dsInitialize()
{
    Result ret = 0;
    ret = csndInit();
	if (ret != 0) {
        printf ("Unable to initialize 3DS CSND service\n");
		return false;
	}

    
    snd3DS.fullBuffers = (short *)linearAlloc(BUFFER_SIZE * 2 * 2); 
	snd3DS.leftBuffer = &snd3DS.fullBuffers[0];
	snd3DS.rightBuffer = &snd3DS.fullBuffers[BUFFER_SIZE];
    memset(snd3DS.fullBuffers, 0, sizeof(BUFFER_SIZE * 2 * 2));

    if (!snd3DS.fullBuffers)
    {
        printf ("Unable to allocate sound buffers\n");
        return false;
    }

    printf ("snd3dsInit - Allocate L/R buffers\n");

	ret = csndPlaySound(LEFT_CHANNEL, SOUND_REPEAT | SOUND_FORMAT_16BIT, SAMPLE_RATE, 1.0f, -1.0f, (u32*)snd3DS.leftBuffer, (u32*)snd3DS.leftBuffer, BUFFER_SIZE * 2);
	ret = csndPlaySound(RIGHT_CHANNEL, SOUND_REPEAT | SOUND_FORMAT_16BIT, SAMPLE_RATE, 1.0f, 1.0f, (u32*)snd3DS.rightBuffer, (u32*)snd3DS.rightBuffer, BUFFER_SIZE * 2);
	//ret = csndPlaySound(DUMMY_CHANNEL, SOUND_REPEAT | SOUND_FORMAT_ADPCM, SAMPLE_RATE, 1.0f, 0.0f, (u32*)snd3DS.dummyBuffer, (u32*)snd3DS.dummyBuffer, BUFFER_SIZE * 2);

	//try to start stalled channels 
	u8 playing = 0;
	csndIsPlaying(LEFT_CHANNEL, &playing);
	if (playing == 0) {
		CSND_SetPlayState(LEFT_CHANNEL, 1);
	}
	csndIsPlaying(RIGHT_CHANNEL, &playing);
	if (playing == 0) {
		CSND_SetPlayState(RIGHT_CHANNEL, 1);
	} 
    /*
	csndIsPlaying(DUMMY_CHANNEL, &playing);
	if (playing == 0) {
		CSND_SetPlayState(DUMMY_CHANNEL, 1);
	} 
    */

	//flush csnd command buffers
	csndExecCmds(true);
	snd3DS.startTick = svcGetSystemTick();

    printf ("snd3dsInit - Start playing buffers\n");

	// DSP thread
    snd3DS.terminateDSPThread = false;

    if (GPU3DS.isReal3DS)
    {
        aptOpenSession();
        APT_SetAppCpuTimeLimit(30); // enables syscore usage
        aptCloseSession();    

        printf ("snd3dsInit - DSP Stack: %x\n", snd3DS.dspThreadStack);
        printf ("snd3dsInit - DSP ThreadFunc: %x\n", &snd3dsDSPThread);
        ret = svcCreateThread(&snd3DS.dspThreadHandle, snd3dsDSPThread, 0, 
            (u32*)(snd3DS.dspThreadStack+0x4000), 0x18, 1);
        if (ret)
        {
            printf("Unable to start DSP thread:\n");
            snd3DS.dspThreadHandle = NULL;
        } 
        printf ("snd3dsInit - Create DSP thread %x\n", snd3DS.dspThreadHandle);
    }
    
    printf ("snd3DSInit complete\n");
	return true;
}



void snd3dsDeinitialize()
{
     snd3DS.terminateDSPThread = true;
     
    if (snd3DS.fullBuffers)  linearFree(snd3DS.fullBuffers);

    CSND_SetPlayState(LEFT_CHANNEL, 0);
    CSND_SetPlayState(RIGHT_CHANNEL, 0);
	csndExecCmds(true);
}

