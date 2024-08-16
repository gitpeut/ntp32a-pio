#include "gpstime.h"
#include "nmea.h"
#include <vector>

int gpsLoopCount;

//----------------------------------------------------------------

void tellTime(){
  time_t now;
  now = time(nullptr);

  Serial.printf(" localtime : %s", asctime( localtime(&now) ) );
  Serial.printf("    gmtime : %s", asctime( gmtime(&now) ) );
}
//----------------------------------------------------------------

void initGPStime(){
 // set timezone; run this system at UTC time
  setenv( "TZ" , "UTC+0", 1);
  tzset();

}
//----------------------------------------------------------------
void print_timeval( struct timeval &tv ){
  time_t nowtime;
  struct tm *nowtm;
  char   tmpbuf[128];
  size_t buflen;
   
  nowtime = tv.tv_sec;
  nowtm  = localtime(&nowtime);
  buflen = strftime(tmpbuf, sizeof(tmpbuf), "%Y-%m-%d %H:%M:%S", nowtm);
  snprintf(tmpbuf + buflen, sizeof(tmpbuf) - buflen, ".%06ld",tv.tv_usec);

  Serial.println( tmpbuf );
  
}

  

//---------------------------------------------------------------------
int setGPStime(){
  struct timeval currentNmeaTv, nowval;

  xSemaphoreTake(valueSemaphore, portMAX_DELAY);

      if ( !gpsvalues.valid ){
          gpsLoopCount = 0;
          xSemaphoreGive(valueSemaphore);
          return 0;
      }

      
      currentNmeaTv.tv_sec  = mktime( ( struct tm *)&gpsvalues.gpstime );

      gettimeofday(&nowval, NULL);
      currentNmeaTv.tv_usec = nowval.tv_usec + 50;
      
  xSemaphoreGive(valueSemaphore);

  gpsLoopCount++;
  
  if ( gpsLoopCount > 100 && gpsvalues.fixed ){   
    settimeofday( &currentNmeaTv, NULL);
    gpsLoopCount = 1;
  }

  return 0;
}
