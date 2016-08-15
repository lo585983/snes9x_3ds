
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


int debugSoundCounter = 0;

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
        // we are too ahead of time, so let's wait for 0.1 millisecs.
        // 
        // That may help to save some battery.
        //
        svcSleepThread(100000);
    }
    
    snd3DS.startSamplePosition = generateAtSamplePosition;
    snd3DS.upToSamplePosition = generateAtSamplePosition + SAMPLES_TO_GENERATE;
    t3dsEndTiming(41);

    t3dsStartTiming(42, "Mix-Copy+Vol");
    int p = generateAtSamplePosition % BUFFER_SIZE;

    
    if (snd3DS.audioType==1)
    {
        S9xApplyMasterVolumeOnTempBufferIntoLeftRightBuffers(&snd3DS.leftBuffer[p], &snd3DS.rightBuffer[p], SAMPLES_TO_GENERATE * 2);


        /*FILE *fp = fopen("sample.dat", "ab");
        for (int i = 0; i < SAMPLES_TO_GENERATE; i++)
        {
            printf ("%7d ", snd3DS.leftBuffer[p + i]);
            fwrite (&snd3DS.leftBuffer[p + i], 2, 1, fp);
        }
        fclose(fp);*/
    }
    else
        S9xApplyMasterVolumeOnTempBufferIntoLeftRightBuffersNDSP(&snd3DS.fullBuffers[p], SAMPLES_TO_GENERATE * 2);

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
    snd3DS.terminateDSPThread = -1;
    svcExitThread();
}


bool snd3dsInitialize()
{
    snd3DS.audioType = 0;
    Result ret = 0;
    ret = csndInit();
    printf ("Trying to initialize CSND, ret = %x\n", ret);
	if (!R_FAILED(ret))
    {
        snd3DS.audioType = 1;
        printf ("CSND Initialized\n");
    }
    else 
    {
        printf ("Unable to initialize 3DS CSND service\n");
        return false;
        /*
        -----------------------------------------------
          NDSP isn't really fully tested yet.
        -----------------------------------------------
        ret = ndspInit();
        printf ("Trying to initialize NDSP, ret = %x\n", ret);

        if (!R_FAILED(ret))
        {
            snd3DS.audioType = 2;
            printf ("NDSP Initialized\n"); 
        }
        else
        {
            printf ("Unable to initialize 3DS CSND/NDSP service\n");
            return false;
        }*/
    }

    // Initialize the sound buffers
    //
    snd3DS.fullBuffers = (short *)linearAlloc(BUFFER_SIZE * 2 * 2); 
	snd3DS.leftBuffer = &snd3DS.fullBuffers[0];
	snd3DS.rightBuffer = &snd3DS.fullBuffers[BUFFER_SIZE];
    memset(snd3DS.fullBuffers, 0, sizeof(BUFFER_SIZE * 2 * 2));

    if (!snd3DS.fullBuffers)
    {
        printf ("Unable to allocate sound buffers\n");
        snd3dsFinalize();
        return false;
    }
    printf ("snd3dsInit - Allocate L/R buffers\n");


    if (snd3DS.audioType == 1)
    {
        // CSND
        ret = csndPlaySound(LEFT_CHANNEL, SOUND_REPEAT | SOUND_FORMAT_16BIT, SAMPLE_RATE, 1.0f, -1.0f, (u32*)snd3DS.leftBuffer, (u32*)snd3DS.leftBuffer, BUFFER_SIZE * 2);
        ret = csndPlaySound(RIGHT_CHANNEL, SOUND_REPEAT | SOUND_FORMAT_16BIT, SAMPLE_RATE, 1.0f, 1.0f, (u32*)snd3DS.rightBuffer, (u32*)snd3DS.rightBuffer, BUFFER_SIZE * 2);

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
    
        // Flush CSND command buffers
        csndExecCmds(true);
        snd3DS.startTick = svcGetSystemTick();

        printf ("snd3dsInit - Start playing CSND buffers\n");

    }
    else
    {
        float stereoMix[12] = { 1.0f, 1.0f, 0, 0, 0,   0, 0, 0, 0, 0,   0, 0 };

        ndspSetOutputMode(NDSP_OUTPUT_STEREO);
        ndspSetOutputCount(1);
        ndspSetMasterVol(1.0f);        

        // Both left/right channels
        ndspChnReset(0);
        ndspChnSetInterp(0, NDSP_INTERP_LINEAR);
        ndspChnSetRate(0, SAMPLE_RATE);
        ndspChnSetFormat(0, NDSP_FORMAT_STEREO_PCM16);
        ndspChnSetMix(0, stereoMix);
        printf ("snd3dsInit - Set channel state\n");

        memset(&snd3DS.waveBuf, 0, sizeof(ndspWaveBuf));
        snd3DS.waveBuf.data_vaddr = (u32*)snd3DS.fullBuffers;
        snd3DS.waveBuf.nsamples = BUFFER_SIZE;
        snd3DS.waveBuf.looping  = true;
        snd3DS.waveBuf.status = NDSP_WBUF_FREE;        

        ndspChnWaveBufAdd(0, &snd3DS.waveBuf);    
        printf ("snd3dsInit - Start playing NDSP buffers\n");
    }

    // SNES DSP thread
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
            snd3dsFinalize();
            return false;
        } 
        printf ("snd3dsInit - Create DSP thread %x\n", snd3DS.dspThreadHandle);
    }
    
    printf ("snd3DSInit complete\n");

	return true;
}



void snd3dsFinalize()
{
     snd3DS.terminateDSPThread = true;

     if (snd3DS.dspThreadHandle)
     {
         // Wait (at most 1 second) for the sound thread to finish, 
         printf ("Join dspThreadHandle\n");
         svcWaitSynchronization(snd3DS.dspThreadHandle, 1000 * 1000000);
         svcCloseHandle(snd3DS.dspThreadHandle);
     }

    if (snd3DS.fullBuffers)  linearFree(snd3DS.fullBuffers);

    if (snd3DS.audioType == 1)
    {
        CSND_SetPlayState(LEFT_CHANNEL, 0);
        CSND_SetPlayState(RIGHT_CHANNEL, 0);
        csndExecCmds(true);
        csndExit();
    }
    else if(snd3DS.audioType == 2)
    {
        ndspChnWaveBufClear(0);
        ndspExit();        
    }
}

