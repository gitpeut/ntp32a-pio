#ifndef NMEA_H
#define NMEA_H

#include <Arduino.h>
#include "soc/timer_group_struct.h"
#include "soc/timer_group_reg.h"
#include <esp_task_wdt.h>
#include "freertos/semphr.h"
#include <time.h>
#include <string.h>
#include <pins.h>

#define GPSREADERCORE 1
#define GPSREADERPRIO 4

struct nmeainfo{
  bool   valid;
  bool   fixed;
  struct tm gpstime;
  float  lat;
  float  lon;
  int    gpssatcount;
  int    glosatcount;  
};


int startGPSReader();
void GPSReader( void *param );
void setGPSstatus();
char *getGPSstatus( char *);
void printgps();
bool isGPSvalid();

void gpsline( char *line );

extern volatile struct nmeainfo gpsvalues;
extern SemaphoreHandle_t valueSemaphore;
extern SemaphoreHandle_t statusSemaphore;

char *getGPSstatus( char *getbuffer);
void nmea_GNRMC( char *line );
void nmea_GPGSV( char *line );
void nmea_GLGSV( char *line );

#endif
