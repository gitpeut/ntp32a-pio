#ifndef PPS_H
#define PPS_H

#include <Arduino.h>
#include <esp_timer.h>
#include "gpstime.h"

void initPPS();
extern int64_t ppsmicros;


#endif
