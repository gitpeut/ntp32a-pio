#ifndef NTPSERVER_H
#define NTPSERVER_H

#include <Arduino.h>
#include <AsyncUDP.h>
#include "nmea.h"
#include "time.h"
#include "globals.h"


/// based mostly on https://github.com/ElektorLabs/180662-mini-NTP-ESP32
typedef struct{
    uint8_t mode:3;               // mode. Three bits. Client will pick mode 3 for client.
    uint8_t vn:3;                 // vn.   Three bits. Version number of the protocol.
    uint8_t li:2;                 // li.   Two bits.   Leap indicator.
}ntp_flags_t;
  
typedef union {
    uint32_t data;
    uint8_t byte[4];
    char c_str[4];
} ntp_ref_t;

typedef struct{
  uint8_t ntpbytes[8];
}ntp_clock_t;

typedef struct{

  ntp_flags_t   flags;
  uint8_t       stratum;     
  uint8_t       polltime;        // Eight bits. Maximum interval between successive messages.
  uint8_t       precision;       // Eight bits. Precision of the local clock.

  uint32_t      rootDelay;      // 32 bits. Total round trip delay time.
  uint32_t      rootDispersion; // 32 bits. Max error aloud from primary clock source.
  ntp_ref_t     refId;           // 32 bits. Reference clock identifier.

  ntp_clock_t   reftime;
  ntp_clock_t   origtime;
  ntp_clock_t   receivetime;
  ntp_clock_t   sendtime;

  // uint32_t   keyid; // optional fields for v4 - https://www.meinbergglobal.com/english/info/ntp-packet.htm
  // uint32_t   digest;
  
}ntp_packet_t;

void ntp2tv(uint8_t *ntp, struct timeval *tv);
void tv2ntp(struct timeval *tv, uint8_t *ntp );
void initNTPServer();

extern int gpsLoopCount;
extern uint32_t  last_packetlength;
#endif
