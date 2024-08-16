#include "web.h"
#include "nmea.h"
#include "Z:/Users/Peut/Documents/Arduino/libraries/wificredentials/wificredentials.h"


struct UPacket{
  public:
  size_t index;
  uint8_t Data[4096]; 
  size_t len; 
  bool final; 
};


AsyncWebServer  fsxserver(80);
TaskHandle_t    FSXServerTask;
TaskHandle_t    UpdateTaskHandle;
SemaphoreHandle_t updateSemaphore;
QueueHandle_t UpdateQ;

int32_t MAGICNOFIXDELAY = 460621;
int32_t MAGICFIXDELAY   = 155502;
int32_t USEWIFI   = 1;
int32_t USEETHERNET = 0;

int     packetCount = 0;
bool    UpdateError = false;

int32_t oldUseWifi = 0;
//------------------------------------------------------------------------------------
// added esp_task_wdt_reset(); in some tight loops against task watchdog panics.

void getFS(){
    xSemaphoreTake( updateSemaphore, portMAX_DELAY);
}

void loseFS(){
    xSemaphoreGive( updateSemaphore);
}


//==============================================================
//                     URL Encode Decode Functions
//https://circuits4you.com/2019/03/21/esp8266-url-encode-decode-example/
//==============================================================
String urldecode(String str)
{
    
    String encodedString="";
    char c;
    char code0;
    char code1;
    for (int i =0; i < str.length(); i++){
        c=str.charAt(i);
      if (c == '+'){
        encodedString+=' ';  
      }else if (c == '%') {
        i++;
        code0=str.charAt(i);
        i++;
        code1=str.charAt(i);
        c = (h2int(code0) << 4) | h2int(code1);
        encodedString+=c;
      } else{
        
        encodedString+=c;  
      }
      
      yield();
    }
    
   return encodedString;
}

//------------------------------------------------------------------------------------

String urlencode(String str)
{
    String encodedString="";
    char c;
    char code0;
    char code1;
    char code2=0;
    for (int i =0; i < str.length(); i++){
      c=str.charAt(i);
      if (c == ' '){
        encodedString+= '+';
      } else if (isalnum(c)){
        encodedString+=c;
      } else{
        code1=(c & 0xf)+'0';
        if ((c & 0xf) >9){
            code1=(c & 0xf) - 10 + 'A';
        }
        c=(c>>4)&0xf;
        code0=c+'0';
        if (c > 9){
            code0=c - 10 + 'A';
        }
        code2=0;
        encodedString+='%';
        encodedString+=code0;
        encodedString+=code1;
        encodedString+=code2;
      }
      yield();
    }
    return encodedString;
    
}
//------------------------------------------------------------------------------------
 
unsigned char h2int(char c)
{
    if (c >= '0' && c <='9'){
        return((unsigned char)c - '0');
    }
    if (c >= 'a' && c <='f'){
        return((unsigned char)c - 'a' + 10);
    }
    if (c >= 'A' && c <='F'){
        return((unsigned char)c - 'A' + 10);
    }
    return(0);
}
//------------------------------------------------------------------------------------
//format bytes
String formatBytes(size_t bytes) {
  if (bytes < 1024) {
    return String(bytes) + "B";
  } else if (bytes < (1024 * 1024)) {
    return String(bytes / 1024.0) + "KB";
  } else if (bytes < (1024 * 1024 * 1024)) {
    return String(bytes / 1024.0 / 1024.0) + "MB";
  } else {
    return String(bytes / 1024.0 / 1024.0 / 1024.0) + "GB";
  }
}
//---------------------------------------------------------------------------

String getContentType(String filename) {
  
  if (filename.endsWith(".htm")) {
    return "text/html";
  } else if (filename.endsWith(".html")) {
    return "text/html";
  } else if (filename.endsWith(".css")) {
    return "text/css";
  } else if (filename.endsWith(".js")) {
    return "application/javascript";
  } else if (filename.endsWith(".png")) {
    return "image/png";
  } else if (filename.endsWith(".gif")) {
    return "image/gif";
   } else if (filename.endsWith(".bmp")) {
    return "image/bmp";
  } else if (filename.endsWith(".jpg")) {
    return "image/jpeg";
  } else if (filename.endsWith(".ico")) {
    return "image/x-icon";
  } else if (filename.endsWith(".xml")) {
    return "text/xml";
  } else if (filename.endsWith(".pdf")) {
    return "application/x-pdf";
  } else if (filename.endsWith(".zip")) {
    return "application/x-zip";
  } else if (filename.endsWith(".gz")) {
    return "application/x-gzip";
  }else if (filename.endsWith(".wav")) {
    return "audio/wav";
  }else if (filename.endsWith(".mp3")) {
    return "audio/mp3";
  }else if (filename.endsWith(".m4a")) {
    return "audio/m4a";
  }else if (filename.endsWith(".flac")) {
    return "audio/flac";
  }
  return "text/plain";
}


//--------------------------------------------------------------------------------

void handleFileRead(  AsyncWebServerRequest *request ) {

  String path = request->url();
  
  Serial.println("handleFileRead: " + path);
  if (path.endsWith("/")) {
    path += "index.html";
  }
  path = urldecode( path);
  
  fs::FS  *ff = &LittleFS; // fsfromfile( path.c_str() );

  String fspath;
  
  
  fspath = path;
    
  String contentType = getContentType(path);
  String pathWithGz = fspath + ".gz";
  
  getFS();
  
  if ( ff->exists(pathWithGz) || ff->exists(fspath)) {
    if ( ff->exists(pathWithGz)) {
      fspath += ".gz";
    }

    if ( request->hasParam("download") )contentType = "application/octet-stream";

    request->send( *ff, fspath, contentType );

    loseFS();
    Serial.printf("File has been streamed\n");
    
    return;
  }
  loseFS();
  request->send( 404, "text/plain", "FileNotFound"); 
}


//----------------------------------------------------------------------------
void showUploadform(AsyncWebServerRequest *request) {
  request->send(200, "text/html", uploadpage);
}

//---------------------------------------------------------------------------

void handleFileUpload(AsyncWebServerRequest * request, String filename, size_t index, uint8_t *data, size_t len, bool final){
 
  static File fsUploadFile;
  String      fspath;
  static int  bcount, bmultiplier;
  
  if(!index){

        getFS();
                
        Serial.printf("UploadStart: %s\n", filename.c_str());
        

        String path = filename;
        if (!path.startsWith("/")) {
          path = "/" + path;
        }
        
        Serial.print("handleFileUpload Name: "); 
        Serial.println(path);
    
        fs::FS  *ff = &LittleFS;
        fspath = path;
                
        if (fspath == "/") {
          loseFS();
          request->send(500, "text/plain", "/ is not a valid filename");
          return;
        }
         
        bcount=0; bmultiplier = 1;
        
        fsUploadFile = ff->open(fspath, "w");
        if ( !fsUploadFile  ){
              loseFS();
              Serial.println("Error opening file " + fspath + " " + String (strerror(errno)) );
              request->send(400, "text/plain", String("Error opening ") + fspath + String(" ") + String ( strerror(errno) ) );
              return;
        }
  }
  int olderrno = errno;
  if (fsUploadFile) {
      if ( fsUploadFile.write( data, len) < len ){
           fsUploadFile.close();
           if ( olderrno != errno ){
            loseFS();
            request->send(500, "text/plain", "Error during write : " + String ( strerror(errno) ) );
            Serial.println("Error writing file " +fspath + " " + String (strerror(errno)) );
            return;
           }
      }
     
      bcount +=len;
      while ( bcount > (bmultiplier * 4096) ){ 
        Serial.print("*");
        if ( (bmultiplier%60) == 0) Serial.println("");
        bmultiplier++;
      }
      
  }

  if ( final ){
    if (fsUploadFile) {
      fsUploadFile.close();        
    }
    loseFS();

    Serial.printf("Uploaded file of %u bytes\n", bcount);
  }
  
}

//---------------------------------------------------------------------
void send_json_status(AsyncWebServerRequest *request)
{
 
char uptime[32];
int sec = millis() / 1000;
int upsec,upminute,uphr,updays;
 
upminute = (sec / 60) % 60;
uphr     = (sec / (60*60)) % 24;
updays   = sec  / (24*60*60);
upsec    = sec % 60;

char  *getbuffer = ( char *)calloc( 1, 512);
getGPSstatus( getbuffer);

if ( strlen(getbuffer) < 1 ){
  sprintf( getbuffer, "\t\"GPSNotConnected\": true");
}
sprintf( uptime, "%d %02d:%02d:%02d", updays, uphr, upminute,upsec);

  String output = "{\r\n";

  output += "\t\"Hostname\" : \"";
  output += esphostname;
  output += "\",\r\n";

  output += "\t\"Compiletime\" : \"";
  output += compile_time;
  output += "\",\r\n";

  String srcfile = String( sourcefile );
  srcfile.replace("\\", "\\\\");
  output += "\t\"Sourcefile\" : \"";
  output += srcfile;
  output += "\",\r\n";

  output += "\t\"uptime\" : \"";
  output += uptime;
  output += "\",\r\n";
    
  output += "\t\"RAMsize\" : ";
  output += ESP.getHeapSize();
  output += ",\r\n";
  
  output += "\t\"RAMfree\" : ";
  output += ESP.getFreeHeap();
  output += ",\r\n";

  output += "\t\"MagicNoFixDelay\" : ";
  output += MAGICNOFIXDELAY;
  output += ",\r\n";

  output += "\t\"MagicFixDelay\" : ";
  output += MAGICFIXDELAY;
  output += ",\r\n";

  output += "\t\"UseWifi\" : ";
  output += USEWIFI?"\"yes\"":"\"no\"";
  output += ",\r\n";

  output += "\t\"UseEthernet\" : ";
  output += USEETHERNET?"\"yes\"":"\"no\"";
  output += ",\r\n";

  output += "\t\"ethernetIP\" : \"";
  output += ETH.localIP().toString();
  output += "\",\r\n";
   
  output += "\t\"WiFiIP\" : \"";
  output += WiFi.localIP().toString();
  output += "\",\r\n";

  output += "\t\"requestcount\" : ";
  output += requestcount;
  output += ",\r\n";

  output += "\t\"unanswered\" : ";
  output += unanswered_requestcount;
  output += ",\r\n";

  output += "\t\"invalid\" : ";
  output += invalid_requestcount;
  output += ",\r\n";

  output += "\t\"ntp_packetlength\" : ";
  output += sizeof(ntp_packet_t);
  output += ",\r\n";

  output += "\t\"last_packetlength\" : ";
  output += last_packetlength;
  output += ",\r\n";

   
  output += "\t\"writeresult\" : ";
  output += write_result;
  output += ",\r\n";

  output += "\t\"writeerror\" : ";
  output += packet_write_error;
  output += ",\r\n";

  output += getbuffer;
  output += "\r\n";

   
  output += "}" ;
    
  request->send(200, "application/json;charset=UTF-8", output);
  free( getbuffer );
}


//---------------------------------------------------------------------------------------
void printProgress(size_t progress, size_t size) {
  size_t percent = (progress*1000)/size;
  if ( (percent%100)  == 0 )Serial.printf(" %u%%\r", percent/10);
}
//-------------------------------------------------------------------------
void setMagicDelay( AsyncWebServerRequest *request ){

   if ( request->params() == 0) {
    request->send(500, "text/plain", "BAD ARGS for magic");
    return;
  }
  String mstring;
  
  if(request->hasParam("fix")){
    mstring = request->getParam("fix")->value();
    MAGICFIXDELAY = atol( mstring.c_str() );
    NVS.setInt( "MAGICFIXDELAY", MAGICFIXDELAY);
  }

  if(request->hasParam("nofix")){
    mstring  = request->getParam("nofix")->value();
    MAGICNOFIXDELAY = atol( mstring.c_str() );
    NVS.setInt( "MAGICNOFIXDELAY", MAGICNOFIXDELAY);
  }

  request->send(200, "text/plain", "Magic set" );
  
}
//-------------------------------------------------------------------------
void setWifi( AsyncWebServerRequest *request ){

   if ( request->params() == 0) {
    request->send(500, "text/plain", "BAD ARGS for wifi");
    return;
  }

  oldUseWifi = USEWIFI;
  if(request->hasParam("on") || request->hasParam("yes")|| request->hasParam("true") || request->hasParam("1") ){
   
    if ( !oldUseWifi ){
      USEWIFI = 1;
      NVS.setInt( "USEWIFI", USEWIFI);

      startWiFi();
    }
  }

  if(request->hasParam("off") || request->hasParam("no")|| request->hasParam("false") || request->hasParam("0") ){
    if (oldUseWifi ){
      USEWIFI = 0;
      NVS.setInt( "USEWIFI", USEWIFI);
  
      WiFi.mode( WIFI_MODE_NULL );
      WiFi.mode( WIFI_OFF );
    }  
  }

  String output =  USEWIFI?"Wifi turned on\n":"Wifi turned off\n";
  request->send(200, "text/plain", output );
  
}
//-------------------------------------------------------------------------
void setEthernet( AsyncWebServerRequest *request ){

   if ( request->params() == 0) {
    request->send(500, "text/plain", "BAD ARGS for wifi");
    return;
  }

  int oldUseEthernet = USEETHERNET;
  if(request->hasParam("on") || request->hasParam("yes")|| request->hasParam("true") || request->hasParam("1") ){
   
    if ( !oldUseEthernet ){
      USEETHERNET = 1;
      NVS.setInt( "USEETHERNET", USEETHERNET);      
      StartEthernet();
    }
  }

  if(request->hasParam("off") || request->hasParam("no")|| request->hasParam("false") || request->hasParam("0") ){
    if (oldUseWifi ){
      USEETHERNET = 0;
      NVS.setInt( "USEETHERNET", USEETHERNET);

      request->send(200, "text/plain", "Restart to disable Ethernet\n");
      delay(100);
      ESP.restart();
      
    }  
  }

  String output =  USEETHERNET?"Ethernet turned on\n":"Ethernet turned off\n";
  request->send(200, "text/plain", output );
}
//---------------------------------------------------------------------------------------
void FSXServer(void *param){

  Serial.printf("Starting FSXServer\n");
  fsxserver.on("/", HTTP_GET, send_json_status);
    
  fsxserver.on("/upload", HTTP_GET, showUploadform);
  fsxserver.on("/upload", HTTP_POST, [](AsyncWebServerRequest *request){ request->send(200, "text/html", uploadpage); }, handleFileUpload);

  fsxserver.on("/status", HTTP_GET, send_json_status );
  fsxserver.on("/magic", HTTP_GET, setMagicDelay );
  fsxserver.on("/wifi", HTTP_GET, setWifi );
  fsxserver.on("/ethernet", HTTP_GET, setEthernet );
  


  fsxserver.on("/reset", HTTP_GET, []( AsyncWebServerRequest *request ) {
        request->send(200, "text/plain", "Doej!");
        delay(20);
        ESP.restart();
  });

  fsxserver.on("/update", HTTP_GET, []( AsyncWebServerRequest *request ) {
        request->send( 200, "text/html", updateform );
  });  
  fsxserver.on("/update", HTTP_POST, [](AsyncWebServerRequest *request){ request->send( 200, "text/html",updateform); }, handleUpdate);

  fsxserver.onNotFound( handleFileRead );

  fsxserver.begin();
  Update.onProgress(printProgress);

  
  while(1){
    delay(100);
  }
}


//----------------------------------------------------------------------------
void stopFSXServer(){
  
  fsxserver.end();
  vTaskDelete( FSXServerTask ); 

  return;
}

//----------------------------------------------------------------------------
void startWiFi(){

     static int started = 0;

     if ( WiFi.status() != WL_CONNECTED && started == 0){

        //WiFiEventId_t eventID = 
        WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info){
            WiFi.setHostname( esphostname );
            //tcpip_adapter_set_hostname(TCPIP_ADAPTER_IF_STA, esphostname);
        //2.0.0 doesn't recognize SYSTEM_EVENTxxxx?    }, WiFiEvent_t::SYSTEM_EVENT_STA_CONNECTED);
         }, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_CONNECTED);
        started++;
        Serial.printf( "Connecting to Wifi network %s, password %s, started %d\n", ssid, password, started);

        WiFi.setSleep(WIFI_PS_NONE);
        WiFi.begin(ssid, password);
    }

    int point = 0;
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        ++point;
        Serial.print(".");
        if ( point%32 == 0 )Serial.printf("%d\n",point);

    }

        
    Serial.println("");
    Serial.println("WiFi connected.");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
}    

//------------------------------------------------------------------------
void UpdateTask(void *param){
    UPacket *upacket = (UPacket *) calloc( 1 , sizeof( UPacket ) );
    size_t written;
    
    for (;;) {
      
      if ( pdTRUE == xQueueReceive( UpdateQ, upacket, portMAX_DELAY)) {

          if ( upacket->index == 0 ){
                if (!Update.begin( UPDATE_SIZE_UNKNOWN, U_FLASH ) ) {
                  Update.printError(Serial);
                }
                
                getFS();
          }

          if ( upacket->len > sizeof(upacket->Data) ){
            Serial.printf("Erroneous size of data: %d, ignored\n", sizeof(upacket->Data) );
            continue;
          }

          UpdateError = false;
          written = Update.write(upacket->Data, upacket->len);
          if ( written != upacket->len) {
               UpdateError = true;
               Serial.printf("written %d bytes, should be %d\n", written, upacket->len);
               Update.printError(Serial);               
          }else{
               Serial.printf("Written %d\n", written );
          }

          if ( UpdateError ) {
            loseFS();
            break;
          }
      }

      if ( upacket->final ){
              if (!Update.end(true)){
                Update.printError(Serial);
              } else {
                Serial.println("\nUpdate complete");
                Serial.flush();
              }
              loseFS();
      }

      delay(1);
    }

    vTaskDelete( NULL );

}

//--------------------------------------------------------------------------

void handleUpdate(AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data, size_t len, bool final) {
  
  //uint32_t free_space = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
  UPacket upacket;
  
  if (!index){
  
    Serial.println("Update");
    packetCount = 0;
        

     xTaskCreatePinnedToCore( 
       UpdateTask,                                   // Task to handle special functions.
       "Update Task",                                           // name of task.
       1024*8,                                       // Stack size of task
       NULL,                                         // parameter of the task
       5,                                            // priority of the task
       &UpdateTaskHandle,                               // Task handle to keep track of created task 
       1 );                                          //core to run it on
        
  }

  //UPacket upacket = UPacket( index, data, len, final);

  upacket.index = index;
  upacket.len = len;
  upacket.final = final;

  for(int i=0; i< len && i < sizeof( upacket.Data ); ++ i){
    upacket.Data[i] = *(data+i);
  }
  
  packetCount++;
  //Serial.printf("Packet #%d Update index %d, size %d, final %d\n", packetCount, index, len, final);
  
  if (pdTRUE != xQueueSend( UpdateQ, &upacket, portMAX_DELAY) ){
    Serial.printf("Update que send error\n");
  }

  if (final || UpdateError ) {

    getFS();
    loseFS();
   
    AsyncWebServerResponse *response = request->beginResponse(302, "text/plain", "Please wait while the device reboots");
    response->addHeader("Refresh", "20");  
    response->addHeader("Location", "/");
    request->send(response);

    delay(100);
    ESP.restart();
  }
}


//------------------------------------------------------------------------
void startFSXServer(){
  
    updateSemaphore = xSemaphoreCreateMutex(); 
    xSemaphoreTake(updateSemaphore, 10);
    xSemaphoreGive(updateSemaphore);

    UpdateQ = xQueueCreate( 5, sizeof( UPacket ) );
     
    LittleFS.begin();

    if ( USEWIFI ){
      startWiFi();
    }
    
    //FSXServer( NULL );

    Serial.printf("Started FSX Server task\n");
   
    xTaskCreatePinnedToCore( 
         FSXServer,                                    // Task to handle special functions.
         "NTP Webserver",                              // name of task.
         1024*8,                                       // Stack size of task
         NULL,                                         // parameter of the task
         2,                                            // priority of the task
         &FSXServerTask,                               // Task handle to keep track of created task 
         1 );                                          //core to run it on
  
}
