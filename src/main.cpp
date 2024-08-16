#include "config_ethernet.h"
#include "nmea.h"
#include "pins.h"
#include "gpstime.h"
#include "pps.h"
#include "web.h"
#include "ntpserver.h"
#include "globals.h"

#include <ArduinoNvs.h>

const char* esphostname = "ntp32a";
const char* project = "ntp32a-pio/1.1";
const char* compile_time = __DATE__ " " __TIME__;

String ip;

//---------------------------------------------------------------

void setup()
{
  Serial.begin(115200);

  NVS.begin();
  MAGICNOFIXDELAY = NVS.getInt( "MAGICNOFIXDELAY");
  MAGICFIXDELAY = NVS.getInt( "MAGICFIXDELAY");
  USEWIFI  = NVS.getInt( "USEWIFI");
  USEETHERNET  = NVS.getInt( "USEETHERNET");

  if ( ! USEWIFI && ! USEETHERNET ) {
    USEWIFI = 1;
  }

  if ( !USEWIFI ) {
    WiFi.mode( WIFI_MODE_NULL );
    WiFi.mode( WIFI_OFF );
  } else {
    startWiFi();
  }

  Serial.printf( "Use Ethernet %ld, use WiFi %ld\n", USEETHERNET, USEWIFI);
  if (USEETHERNET) {
    StartEthernet();
  }

  Serial.printf( "0 Start FSXServer\n");
  startFSXServer();
  Serial.printf( "2 Init GPS\n");
  initGPStime();      // set timezone to UTC
  Serial.printf( "3 Init PPS\n");
  initPPS();          // define pulse pin and attach interrupt
  Serial.printf( "4 Start GPS Reader\n");
  startGPSReader();   // start reading NMEA lines/packets
  Serial.printf( "6 Init NTPServer\n");
  initNTPServer();
}

//---------------------------------------------------------------

void loop() {
  delay(5000);
  vTaskDelete( NULL ); 

}
