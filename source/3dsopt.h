
#ifndef _3DSOPT_H_
#define _3DSOPT_H_


#include <3ds.h>

#include "snes9x.h"
#include "port.h"
#include "memmap.h"
#include "3dssnes9x.h"


#ifdef _3DSOPT_CPP_

char *t3dsClockName[50];
int t3dsTotalCount[50];
u64 t3dsStartTicks[50];
u64 t3dsTotalTicks[50];

#else

extern char **t3dsClockName;
extern int *t3dsTotalCount;
extern u64 *t3dsStartTicks;
extern u64 *t3dsTotalTicks;

#endif


void t3dsResetTimings();
void t3dsCount(int bucket, char *name);
void t3dsShowTotalTiming(int bucket);


inline void t3dsStartTiming(int bucket, char *clockName)
{
#ifndef RELEASE
    t3dsStartTicks[bucket] = svcGetSystemTick(); 
    t3dsClockName[bucket] = clockName;
#endif
}

inline void t3dsEndTiming(int bucket)
{
#ifndef RELEASE
    u64 endTicks = svcGetSystemTick(); 
    t3dsTotalTicks[bucket] += (endTicks - (u64)t3dsStartTicks[bucket]);
    t3dsTotalCount[bucket]++;
#endif
}

#endif