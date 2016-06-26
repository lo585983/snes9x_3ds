
#include <3ds.h>

#include "snes9x.h"
#include "port.h"
#include "memmap.h"

#define TICKS_PER_SEC (268123)

//--------------------------------------------------------------------------------
// Initialize optimizations
//--------------------------------------------------------------------------------

char *t3dsClockName[50];
int t3dsTotalCount[50];
u64 t3dsStartTicks[50];
u64 t3dsTotalTicks[50];


char *emptyString = "";

// Optimization variables
//


//struct ST3DSOpt t3dsOpt;

void t3dsResetTimings()
{
	for (int i = 0; i < 50; i++)
    {
        t3dsTotalTicks[i] = 0; 
        t3dsTotalCount[i] = 0;
        t3dsClockName[i] = emptyString;
    }
}

void t3dsStartTiming(int bucket, char *clockName)
{
    t3dsStartTicks[bucket] = svcGetSystemTick(); 
    t3dsClockName[bucket] = clockName;
}

void t3dsCount(int bucket, char *clockName)
{
    t3dsStartTicks[bucket] = -1; 
    t3dsClockName[bucket] = clockName;
    t3dsTotalCount[bucket]++;
}

void t3dsEndTiming(int bucket)
{
    u64 endTicks = svcGetSystemTick(); 
    t3dsTotalTicks[bucket] += (endTicks - (u64)t3dsStartTicks[bucket]);
    t3dsTotalCount[bucket]++;
}

 void t3dsShowTotalTiming(int bucket)
{
    if (t3dsTotalTicks[bucket] > 0)
        printf ("%-20s: %2d %4d ms %d\n", t3dsClockName[bucket], bucket,
        (int)(t3dsTotalTicks[bucket] / TICKS_PER_SEC), 
        t3dsTotalCount[bucket]);
    else if (t3dsStartTicks[bucket] == -1 && t3dsTotalCount[bucket] > 0)
        printf ("%-20s: %2d %d\n", t3dsClockName[bucket], bucket,
        t3dsTotalCount[bucket]);
}
