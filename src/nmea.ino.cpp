#include "nmea.h"
#include "gpstime.h"

TaskHandle_t    GPSreaderTask;

SemaphoreHandle_t valueSemaphore;
volatile struct nmeainfo gpsvalues;


SemaphoreHandle_t statusSemaphore;
char            statusbuffer[512];

extern void getFS();
extern void loseFS();
//---------------------------------------------------------------

void setGPSstatus(){
  int   position;

  xSemaphoreTake(valueSemaphore, portMAX_DELAY);
  xSemaphoreTake(statusSemaphore, portMAX_DELAY);

  position = sprintf(statusbuffer, "\t\"fix\" : %d,\r\n\t\"valid\" : %d,\r\n", gpsvalues.fixed, gpsvalues.valid );
  position += sprintf(statusbuffer + position,"\t\"date\": \"%d-%d-%4d\",\r\n\t\"time\": \"%2d:%02d:%02d\",\r\n ",
      gpsvalues.gpstime.tm_mday,
      gpsvalues.gpstime.tm_mon + 1,
      gpsvalues.gpstime.tm_year + 1900,
      gpsvalues.gpstime.tm_hour,
      gpsvalues.gpstime.tm_min,
      gpsvalues.gpstime.tm_sec);
  position += sprintf(statusbuffer + position,"\t\"latitude\" : %f,\r\n\t\"longitude\" : %f,\r\n",gpsvalues.lat, gpsvalues.lon );
  position += sprintf(statusbuffer + position,"\t\"GPScount\" : %d,\r\n\t\"GLONASScount\" :%d\r\n",gpsvalues.gpssatcount, gpsvalues.glosatcount );

  xSemaphoreGive(statusSemaphore);
  xSemaphoreGive(valueSemaphore);

}
//---------------------------------------------------------------

bool isGPSvalid(){
    bool valid;
    xSemaphoreTake( valueSemaphore, portMAX_DELAY);
    valid = gpsvalues.valid;
    xSemaphoreGive( valueSemaphore);  

    return valid;
}
//---------------------------------------------------------------

bool isGPSfixed(){
    bool fixed;
    xSemaphoreTake( valueSemaphore, portMAX_DELAY);
    fixed = gpsvalues.fixed;
    xSemaphoreGive( valueSemaphore);  

    return fixed;
}
//---------------------------------------------------------------

char *getGPSstatus( char *getbuffer){
  xSemaphoreTake(statusSemaphore, portMAX_DELAY);

  strcpy( getbuffer, statusbuffer );

  xSemaphoreGive(statusSemaphore);  
  return( getbuffer );
}

//---------------------------------------------------------------
void printgps(){
  char *getbuffer = (char *)calloc(512,1);
  Serial.printf( "%s\n", getGPSstatus(getbuffer) ); 
  free( getbuffer );
}

//---------------------------------------------------------------

int startGPSReader(){
    
    statusSemaphore = xSemaphoreCreateMutex(); 
    xSemaphoreTake(statusSemaphore, 10);
    xSemaphoreGive(statusSemaphore);

    valueSemaphore = xSemaphoreCreateMutex(); 
    xSemaphoreTake(valueSemaphore, 10);
    xSemaphoreGive(valueSemaphore);
     
    xTaskCreatePinnedToCore( 
         GPSReader,                                      // Task to handle special functions.
         "GPSReader",                                    // name of task.
         16*1024,                                        // Stack size of task
         NULL,                                           // parameter of the task
         GPSREADERPRIO,                                  // priority of the task
         &GPSreaderTask,                                 // Task handle to keep track of created task 
         GPSREADERCORE);                                 // processor core
                
return(0);
}

//---------------------------------------------------------------

void GPSReader( void *param ){
  char linebuffer[256];
  char *lineptr = linebuffer; 
  bool nmea=false;
  char c;

  Serial.printf("GPS reader starting...\n");
  Serial1.begin(9600, SERIAL_8N1, RXPin, TXPin);
  
  while(1){
   
    getFS();
    loseFS();
     
    while (Serial1.available()) {
     
  
      c = (char)Serial1.read();
      switch(c ){
        case '$':
                //Serial1.print('a'); 
                nmea = true; 
                break;
        case '*':
                *lineptr  = 0;
                if ( nmea ){
                  gpsline( linebuffer );
                  lineptr = linebuffer; 
                  nmea    = 0;
                }  
                break;
        default:
                if ( nmea ){
                  *lineptr++ = c;        
                }
                
                break;
                
      }   
      
      //Serial.print(c); 
      delay(1); //make the task watchdog happy-needed when running on core 0 as Arduino disables the task watchdor on core 1
    }
   
   
 } 
}




//---------------------------------------------------------------

void gpsline( char *line ){
  if ( strncmp( line + 2 , "RMC", 3 ) == 0 ){
      nmea_GNRMC( line ); 
      setGPStime();        
      setGPSstatus();        
      //printgps();
  }
  
  // satellite count gp = gps, gl = glonass
  // https://receiverhelp.trimble.com/alloy-gnss/en-us/NMEA-0183messages_GNS.html
  //
  if ( strncmp( line + 2, "GPGSV", 3 ) == 0 )nmea_GPGSV( line );  
  if ( strncmp( line + 2, "GLGSV", 5 ) == 0 )nmea_GLGSV( line );  
  
}
//---------------------------------------------------------------

#define TIMEIDX   1
#define FIXEDIDX  2
#define LATIDX    3
#define LONGIDX   5
#define DATEIDX   9
#define SATCNTIDX 3 
//---------------------------------------------------------------
void todecimaldegrees( char *centigrade, float &decdegrees ){
  float d, m;
  char  *ms, sav;

  for ( ms = centigrade; *ms != '.'; ++ms );
 
  --ms;
  --ms;
  sav = *ms;
  *ms = 0;

  d = String( centigrade ).toFloat();
  *ms = sav;
  
  m = String( ms ).toFloat() /60;

  decdegrees = d + m;   
    
}
//---------------------------------------------------------------

void nmea_GNRMC( char *line ){
  char  *s = line;
  int   datidx=0;    
  char  sav, tmp[32];
  char  *t = &tmp[0];
  float decdegrees;

  xSemaphoreTake(valueSemaphore, portMAX_DELAY);
  
  gpsvalues.valid = false;
  
  while (*s){
      switch( *s ){
        case ',':
          switch( datidx){
            case TIMEIDX:
                *t = 0;
                if ( strlen(tmp) == 0 )break;

                sav = tmp[2];
                tmp[2] = 0;
                gpsvalues.gpstime.tm_hour = atoi( tmp );  
                tmp[2] = sav;
                
                sav = tmp[4];
                tmp[4] = 0;
                gpsvalues.gpstime.tm_min = atoi( &tmp[2] );
                tmp[4] = sav;

                sav = tmp[6];
                tmp[6] = 0;
                gpsvalues.gpstime.tm_sec = atoi( &tmp[4] );
                tmp[6] = sav;
                break;
                
            case FIXEDIDX:
                *t = 0;
                if ( tmp[0] == 'A' ){
                  gpsvalues.fixed = true;
                }else{
                  gpsvalues.fixed = false; 
                }
                break;
                
            case LATIDX:
                *t = 0;
                if ( !gpsvalues.fixed ) {gpsvalues.lat = 0; break;}

                todecimaldegrees( tmp, decdegrees );
                if ( *(s+1) == 'S' ) decdegrees = decdegrees * -1;

                gpsvalues.lat = decdegrees;
                break;
            case LONGIDX:
                *t = 0;
                if ( !gpsvalues.fixed ) {gpsvalues.lon = 0; break;}

                todecimaldegrees( tmp, decdegrees );
                if ( *(s+1) == 'W' ) decdegrees = decdegrees * -1;

                gpsvalues.lon = decdegrees;
                break;
            case DATEIDX:
                *t = 0;
                
                if ( strlen(tmp) == 0 )break;
                
                sav = tmp[2];
                tmp[2] = 0;
                gpsvalues.gpstime.tm_mday = atoi( tmp );
                tmp[2] = sav;
                
                sav = tmp[4];
                tmp[4] = 0;
                gpsvalues.gpstime.tm_mon = atoi( &tmp[2] ) - 1; // months are 0 -11
                tmp[4] = sav;

                sav = tmp[6];
                tmp[6] = 0;
                gpsvalues.gpstime.tm_year = atoi( &tmp[4] ) + 100; // years since 1900
                tmp[6] = sav;
                gpsvalues.valid = true;
                break;
          }

          t = tmp;
          *t = 0;
          ++datidx;
          break;
       default:
          *t++ = *s;
          break;   

      }
    
  ++s;
  }  
  xSemaphoreGive( valueSemaphore); 
}
//---------------------------------------------------------------

void nmea_GPGSV( char *line ){
  char  *s = line;
  int   datidx=0;    
  char  tmp[32];
  char  *t = tmp;

  xSemaphoreTake(valueSemaphore, portMAX_DELAY);

  while(*s){
    switch ( *s ){
        case ',':
          *t = 0;
          if( datidx == SATCNTIDX ) {
            gpsvalues.gpssatcount = atoi ( tmp );
            xSemaphoreGive( valueSemaphore); 
            return;            
          }
          t = tmp;
          *t = 0;
          ++datidx;
          break;
        default:
          *t++ = *s ;
          break;
    }
    ++s;
  }
  xSemaphoreGive( valueSemaphore); 
}
//---------------------------------------------------------------

void nmea_GLGSV( char *line ){
  char  *s = line;
  int   datidx=0;    
  char  tmp[32];
  char  *t = &tmp[0];

  xSemaphoreTake(valueSemaphore, portMAX_DELAY);
  
  while(*s){
    switch ( *s ){
        case ',':
          *t = 0;
          if( datidx == SATCNTIDX ) {
            gpsvalues.glosatcount = atoi ( tmp );
            xSemaphoreGive( valueSemaphore);             
            return;
          }
          t = &tmp[0];
          *t = 0;
          ++datidx;
          break;
        default:
          *t++ = *s ;
          break;
    }
    ++s;
  }
  xSemaphoreGive( valueSemaphore); 
}
