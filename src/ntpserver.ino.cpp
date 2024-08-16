#include "ntpserver.h"
#include "lwip/err.h"
//#include "lwip/apps/sntp.h"
#include "esp_sntp.h"

AsyncUDP udp;

uint8_t timeoverhead = 0;

uint32_t  requestcount = 0;
uint32_t  invalid_requestcount = 0;
uint32_t  last_packetlength;
uint32_t  unanswered_requestcount = 0;
int       packet_write_error;
size_t    write_result;


TaskHandle_t  NTPServerTask;  
QueueHandle_t AnswerQ;


//---------------------------------------------------------------------------------------------
void calctimeoverhead() {

  struct timeval tv;

  uint32_t start = micros();
  for (uint32_t i = 0; i < 1024; i++) {
    gettimeofday( &tv, NULL);
  }
  uint32_t end = micros();
  Serial.printf("Overhead micros: %ld\n", end - start);
  double runtime = ( ((double)(end) - (double)(start) ) / ( (double)(1000000.0) * (double)(1024) ) ) + ((double)(1)) ; // One second as we don't keep track on the fractions
  timeoverhead = log2(runtime);

  Serial.printf("Overhead: %d\n", timeoverhead);
}


//---------------------------------------------------------------------------------------------

void IRAM_ATTR udpCallback(AsyncUDPPacket &packet) {

  requestcount++;

  if ( !isGPSvalid() || gpsLoopCount == 0) {
    unanswered_requestcount++;
    return; // do not answer if GPS has no fix
  }

  if (packet.length() != sizeof( ntp_packet_t ) ) {
    last_packetlength = packet.length();
    invalid_requestcount++;
    return; // do not answer if this is not an NTP request
  }

  xQueueSend( AnswerQ, &packet, portMAX_DELAY);

}

//---------------------------------------------------------------------------------------------
void IRAM_ATTR answerNTP( AsyncUDPPacket &packet){
  
  struct timeval  receivetv, nowtv;
  gettimeofday( &receivetv, NULL);
  ntp_packet_t ntprequest;

  Serial.printf("sizeof ntp_packet_t %d,  sizeof ntprequest %d\n", 
                  sizeof( ntp_packet_t ), sizeof( ntprequest ));   
  
 
  memcpy(&ntprequest, packet.data(), sizeof(ntp_packet_t));
  
  // fill packet with answer. Multibyte field in network byte order

  ntprequest.flags.li   = 0; //NONE
  ntprequest.flags.vn   = 4; // NTP Version 4
  ntprequest.flags.mode = 4; // Server

  ntprequest.stratum        = 1; 
  ntprequest.polltime       = 3;
  ntprequest.precision      = 0xF7;// timeoverhead;
  ntprequest.rootDelay      = 0; //unspecified
  ntprequest.rootDispersion = htonl(1);

  strncpy(ntprequest.refId.c_str , "ESP", sizeof(ntprequest.refId.c_str) );

  // fill the four time stamps
  // Reference Timestamp: 0xE4BCEA14E2D3A604 (153623 12:05:40.8860420s - 10-8-2021 14:05:38)
  // Originate Timestamp: 0xE4BCEA1476C5AECB (153623 12:05:40.4639539s - 10-8-2021 14:05:40)
  // Receive Timestamp: 0xE4BCEA14E20E8427 (153623   12:05:40.8830340s - 10-8-2021 14:05:40)
  // Transmit Timestamp: 0xE4BCEA14E2D3A604 (153623  12:05:40.8860420s - 10-8-2021 14:05:40)

  // 4. originate time : (with sendtime)
  // do this before original sendtime gets overwritten
  
  for ( uint8_t i = 0; i < 8 ; ++i ) {
    // first try ntprequest.origtime.ntpbytes[i] = ntprequest.sendtime.ntpbytes[i];
    ntprequest.origtime.ntpbytes[i] = ntprequest.sendtime.ntpbytes[i];
  }

  // 1. receive time
  tv2ntp( &receivetv, ntprequest.receivetime.ntpbytes);

  for ( int writeCount = 0; writeCount < 5; writeCount++) {

    gettimeofday( &nowtv, NULL);
    tv2ntp( &nowtv, ntprequest.reftime.ntpbytes);

    // for sendtime, collect current time

    //    gettimeofday( &nowtv, NULL); //4.2 same 
    tv2ntp( &nowtv, ntprequest.sendtime.ntpbytes);

    write_result = packet.write((uint8_t*)&ntprequest, sizeof(ntp_packet_t));
    delay(2);
    if ( write_result ) break;
    packet_write_error = errno;
  }
  
  if ( write_result == 0 ) {
    ++unanswered_requestcount;
  } else {
    packet.flush();
  }
}

//---------------------------------------------------------------------------------------------

void RunNTPServer(void *param){

  AsyncUDPPacket *packet = (AsyncUDPPacket *) calloc( 1 , sizeof( AsyncUDPPacket ) );
  AnswerQ = xQueueCreate( 10, sizeof( AsyncUDPPacket ) );

  if (udp.listen(123)) {
    udp.onPacket( udpCallback );
  } 

  for (;;) {
    if ( pdTRUE == xQueueReceive( AnswerQ, packet, portMAX_DELAY)) {
      answerNTP( *packet );
    }
  }
}

//---------------------------------------------------------------------------------------------


void initNTPServer() {

  sntp_set_sync_mode( SNTP_SYNC_MODE_SMOOTH );
  calctimeoverhead();

    xTaskCreatePinnedToCore( 
         RunNTPServer,                                 // Task to handle special functions.
         "NTPServer",                                  // name of task.
         1024*8,                                       // Stack size of task
         NULL,                                         // parameter of the task
         4,                                            // priority of the task
         &NTPServerTask,                               // Task handle to keep track of created task 
         tskNO_AFFINITY );                                          //core to run it on


  //https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/system_time.html

}





//---------------------------------------------------------------------------------------------

//https://stackoverflow.com/questions/29112071/how-to-convert-ntp-time-to-unix-epoch-time-in-c-language-linux
// modified with hard define of number of bytes of NTP time

#define NTOFFSET 2208988800ULL // difference between Unix (1-1-1970) and NTP Epoch (1-1-1900)
#define NTIMESIZE 8            // 8 bytes in NTP time format




void ntp2tv(uint8_t *ntp, struct timeval *tv)
{
  uint64_t aux = 0;
  uint8_t *p = ntp;
  int i;

  /* we get the ntp in network byte order, so we must
     convert it to host byte order. */
  for (i = 0; i < NTIMESIZE / 2; i++) {
    aux <<= 8;
    aux |= *p++;
  } /* for */

  /* now we have in aux the NTP seconds offset */
  aux -= NTOFFSET;
  tv->tv_sec = aux;

  /* let's go with the fraction of second */
  aux = 0;
  for (; i < NTIMESIZE ; i++) {
    aux <<= 8;
    aux |= *p++;
  } /* for */

  /* now we have in aux the NTP fraction (0..2^32-1) */
  aux *= 1000000; /* multiply by 1e6 */
  aux >>= 32;     /* and divide by 2^32 */
  tv->tv_usec = aux;
} /* ntp2tv */

//----------------------------------------------------------------

void tv2ntp(struct timeval *tv, uint8_t *ntp )
{
  uint64_t aux = 0;
  uint8_t *p = ntp + NTIMESIZE;
  int i;

  aux = tv->tv_usec;
  aux <<= 32;
  aux /= 1000000;

  /* we set the ntp in network byte order */
  for (i = 0; i < NTIMESIZE / 2; i++) {
    *--p = aux & 0xff;
    aux >>= 8;
  } /* for */

  aux = tv->tv_sec;
  aux += NTOFFSET;

  /* let's go with the fraction of second */
  for (; i < NTIMESIZE; i++) {
    *--p = aux & 0xff;
    aux >>= 8;
  } /* for */

} /* ntp2tv */

//-------------------------------------------------------------

size_t print_tv(struct timeval *t)
{
  return printf("%lld.%06ld\n", t->tv_sec, t->tv_usec);
}

//-------------------------------------------------------------

size_t print_ntp(uint8_t ntp[8])
{
  int i;
  int res = 0;
  for (i = 0; i < NTIMESIZE; i++) {
    if (i == NTIMESIZE / 2)
      res += printf(".");
    res += printf("%02x", ntp[i]);
  } /* for */
  res += printf("\n");
  return res;
} /* print_ntp */
