#ifndef WEB_H
#define WEB_H

#include <freertos/FreeRTOS.h>
#include <Arduino.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoNvs.h>
#include <Update.h>
#include <LittleFS.h>

#include "nmea.h"
#include "ntpserver.h"
#include "globals.h"
#include "config_ethernet.h"

extern AsyncWebServer  fsxserver;
extern SemaphoreHandle_t updateSemaphore;
extern int32_t USEWIFI; 
extern int32_t USEETHERNET; 

static const char* updateform PROGMEM= "<form method='POST' action='/update' enctype='multipart/form-data'><input type='file' name='update'><input type='submit' value='Update'></form>";
static const char uploadpage[] PROGMEM =
  R"(
<!DOCTYPE html>
<html>
<head>
 <meta charset='UTF-8'>
 <meta name='viewport' content='width=device-width, initial-scale=1.0'>

<style>
 body {
  font-family: Arial, Helvetica, sans-serif;  
  background: lightgrey;  
}
</style>
</head>
<html>
<h2>Upload file</h2>
<form method='POST' action='/upload' enctype='multipart/form-data' id='f' '>
<table>
<tr><td>Local file to upload </td><td><input type='file' name='blob' style='width: 300px;' id='pt'></td></tr>
<tr><td colspan=2> <input type='submit' value='Upload'></td></tr> 
</table>
</form>
 </html>)";

void getFS();
void loseFS();
void startFSXServer(); 
void startWiFi();
unsigned char h2int(char c);

extern uint32_t  requestcount;
extern uint32_t  invalid_requestcount;
extern uint32_t  unanswered_requestcount;
extern size_t    write_result;
extern int       packet_write_error;

void handleUpdate(AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data, size_t len, bool final);
void UpdateTask(void *param);

#endif
