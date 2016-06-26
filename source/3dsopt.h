
#ifndef _3DSOPT_H_
#define _3DSOPT_H_



struct ST3DSOpt
{
};
extern struct ST3DSOpt t3dsOpt;

void t3dsResetTimings();
void t3dsStartTiming(int bucket, char *name);
void t3dsCount(int bucket, char *name);
void t3dsEndTiming(int bucket);
void t3dsShowTotalTiming(int bucket);

#endif