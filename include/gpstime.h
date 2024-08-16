#ifndef GPSTIME_H
#define GPSTIME_H

#include "time.h"
#include "lwip/err.h"
//#include "lwip/apps/sntp.h"

//const char* ntpTimezone   = "CET-1CEST,M3.5.0/2,M10.5.0/3";
extern int32_t MAGICFIXDELAY; 
extern int32_t MAGICNOFIXDELAY; 
extern int gpsLoopCount;

void initGPStime();
void tellTime();
int setGPStime();


#endif
