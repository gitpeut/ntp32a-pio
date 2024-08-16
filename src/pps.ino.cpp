#include "pps.h"
#include "nmea.h"

int64_t ppsmicros;

//ISR for PPS interrupt
void IRAM_ATTR ppsISR() {

   if ( gpsLoopCount == 0 || gpsLoopCount >= 100 ) return;
    
   struct timeval nowval, newval; 
   bool doSet = false;   
   
   gettimeofday(&nowval, NULL);

   newval.tv_sec  = nowval.tv_sec;
   newval.tv_usec = MAGICFIXDELAY;
   
   if ( nowval.tv_usec < 450000 ){ 
      doSet = true;   
   }
   if ( nowval.tv_usec > 550000 ){ 
      newval.tv_sec += 1;
      doSet = true;   
   }

   if ( doSet ){ 
    settimeofday( &newval, NULL );  
   } 
} 

void initPPS(){

 pinMode(PULSE_PIN, INPUT_PULLDOWN);
 attachInterrupt( PULSE_PIN , ppsISR, RISING);
  
}
 
