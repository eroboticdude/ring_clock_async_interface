
/*


/*********
  Rui Santos
  Complete instructions at https://RandomNerdTutorials.com/esp32-wi-fi-manager-asyncwebserver/
  
  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files.
  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
**

 * * ESP8266 template with phone config web page
   based on BVB_WebConfig_OTA_V7 from Andreas Spiess https://github.com/SensorsIot/Internet-of-Things-with-ESP8266

   Code by Elliott Bradley (mostly)
   but I pieced together code from others, mainly (insert the instructble/github link for the ring clock)

*/
#define FASTLED_INTERRUPT_RETRY_COUNT 1
#include <FastLED.h>
#define LED_TYPE WS2812B
#define COLOR_ORDER GRB
#define LED_PIN 32
#define NUM_LEDS  60
CRGB leds[NUM_LEDS];
CRGB HourColor = CRGB(145, 0, 0); //RED               //FastLED config
CRGB MinuteColor = CRGB(0, 118, 138); //GREEN
CRGB SecondColor = CRGB(144, 0, 112); //BLUE
const CRGB lightcolor(8, 5, 1);
uint8_t BRIGHTNESS = 255;

CRGBPalette16 currentPalette;


#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
 #include <SPIFFS.h>

const int wifiTimeout = 5000;
int wifiCount = 0;
int wifiPrev = 0;
bool  wifiCast = 0;
char ssid[] = "mnrbradley"; // WiFi name
char password[] = "Kr1st1n@"; // WiFi password

const char* ssidAP = "Ring_Clock";  // Soft AP
const char* passwordAP = "123456789";
// IP Address details
IPAddress local_ip(192, 168, 1, 1);
IPAddress gateway(192, 168, 1, 1);   //soft AP data
IPAddress subnet(255, 255, 255, 0);


WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "north-america.pool.ntp.org", -18000); // -5 hours offset, Chicago Time

// Set web server port number to 80
        //WiFiServer server(80);
AsyncWebServer server(80);
// Variable to store the HTTP request
String header;

// Current time
unsigned long currentTime = millis();
// Previous time
unsigned long previousTime = 0;
// Define timeout time in milliseconds (example: 2000ms = 2s)
const long timeoutTime = 2000;

// Auxiliar variables to store the current output state
String output26State = "off";
String output27State = "off";

// Assign output variables to GPIO pins
const int output26 = 26;
const int output27 = 27;                           //temporary for the html host code


int touch = 0;
int touchMax = 0;
int touchMin = 0;
int touchButton = 0;
int touchButtonLast = 1;
int touchRange = 0;

int modeCount = 5;  

int lightMode = 0;
int hour = 0;
int minute = 0;
int second = 0;

byte temp_second = 0;

int offlineTime = 0;
int offlinePrev = 0;

TaskHandle_t Main;
TaskHandle_t STA;
TaskHandle_t AP;


const char* PARAM_SSID = "inputSSID";
const char* PARAM_PSK = "inputPSK";
const char* PARAM_INT = "inputInt";
const char* PARAM_FLOAT = "inputFloat";

// HTML web page to handle 3 input fields (inputString, inputInt, inputFloat)
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html><head>
  <title>ESP Input Form</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <script>
    function submitMessage() {
      alert("Saved value to ESP SPIFFS");
      setTimeout(function(){ document.location.reload(false); }, 500);   
    }
  </script></head><body>
  <form action="/get" target="hidden-form">
    inputSSID (current value %inputSSID%): <input type="text" name="inputSSID">
    <input type="submit" value="Submit" onclick="submitMessage()">
  </form><br>
  <form action="/get" target="hidden-form">
    inputPSK (current value %inputPSK%): <input type="text" name="inputPSK">
    <input type="submit" value="Submit" onclick="submitMessage()">
  </form><br>
  <form action="/get" target="hidden-form">
    inputInt (current value %inputInt%): <input type="number " name="inputInt">
    <input type="submit" value="Submit" onclick="submitMessage()">
  </form><br>
  <form action="/get" target="hidden-form">
    inputFloat (current value %inputFloat%): <input type="number " name="inputFloat">
    <input type="submit" value="Submit" onclick="submitMessage()">
  </form>
  <iframe style="display:none" name="hidden-form"></iframe>
</body></html>)rawliteral";


void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}

String readFile(fs::FS &fs, const char * path){
  Serial.printf("Reading file: %s\r\n", path);
  File file = fs.open(path, "r");
  if(!file || file.isDirectory()){
    Serial.println("- empty file or failed to open file");
    return String();
  }
  Serial.println("- read from file:");
  String fileContent;
  while(file.available()){
    fileContent+=String((char)file.read());
  }
  file.close();
  Serial.println(fileContent);
  return fileContent;
}

void writeFile(fs::FS &fs, const char * path, const char * message){
  Serial.printf("Writing file: %s\r\n", path);
  File file = fs.open(path, "w");
  if(!file){
    Serial.println("- failed to open file for writing");
    return;
  }
  if(file.print(message)){
    Serial.println("- file written");
  } else {
    Serial.println("- write failed");
  }
  file.close();
}

// Replaces placeholder with stored values
String processor(const String& var){
  //Serial.println(var);
  if(var == "inputSSID"){
    return readFile(SPIFFS, "/wifiSSID.txt");
  }
  else if(var == "inputPSK"){
    return readFile(SPIFFS, "/wifiPSK.txt");
  }
  else if(var == "inputInt"){
    return readFile(SPIFFS, "/inputInt.txt");
  }
  else if(var == "inputFloat"){
    return readFile(SPIFFS, "/inputFloat.txt");
  }
  return String();
}



void setup() {

  Serial.begin(115200);
                                      currentPalette = CloudColors_p;
 if(!SPIFFS.begin(true)){
      Serial.println("An Error has occurred while mounting SPIFFS");
      return;
 }
                       
  pinMode(output26, OUTPUT);
  pinMode(output27, OUTPUT);
  // Set outputs to LOW
  digitalWrite(output26, LOW);
  digitalWrite(output27, LOW);

  // tell FastLED about the LED strip configuration
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalSMD5050);
  FastLED.setBrightness(BRIGHTNESS);
  Serial.println("FastLed Setup done on core ");
  Serial.println(xPortGetCoreID());
  delay(100);

  WiFi.mode(WIFI_STA);

  WiFi.begin(ssid, password);
  wifiCount = millis();
  wifiPrev = wifiCount;
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    //softtwinkles();
    pride();
    FastLED.show();
    wifiCount = millis();



    if (wifiCount - wifiPrev >= wifiTimeout) {
      wifiCast = true;
      hour = 10;
      minute = 30;          //set time from EEPROM here
      second = 45;
      Serial.println(".");
      Serial.println("WiFi not available, switching to Access Point mode");
      Serial.print("wifiCast ");
      Serial.println(wifiCast);
      WiFi.mode(WIFI_AP);
     // WiFi.disconnect();
      break;   //this should definitely make the loop stop and go do other stuff
    }
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("");
    Serial.println("WiFi connected.");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.print("connected with core ");
    Serial.println(xPortGetCoreID());
    Serial.print("time taken ");
    Serial.println(wifiCount - wifiPrev);

  }

   serverSetup();
  xTaskCreatePinnedToCore(Maincode, "Main", 50000, NULL, 1, &Main, 1); //hopefully I can name these whatever I want

  if (wifiCast == false) {

    xTaskCreatePinnedToCore(STAcode, "STA", 50000, NULL, 0, &STA, 0);  // im pretty sure this function actually calls the code like the void command should,
  }                                                 // so this should work just like any other pointers do except I control core and priority,
  else {                                              // because the esp32 can multi task! Just make sure to make the tasks with fastLED the highest

    xTaskCreatePinnedToCore(APcode, "AP", 50000, NULL, 0, &AP, 0); // priority because otherwise there will be interrupt issues that im not smart enough to fix
  }

}

// the loop function runs over and over again forever

void loop() {
  delay(1); //to hopefully reduce processing times if the loop is considered
}


void Maincode( void * parameter) {

  for (;;) {

    if (wifiCast == false) {
      if (temp_second != timeClient.getSeconds()) { //update the temp_second only when the time client second changes
        temp_second = timeClient.getSeconds();
        timeClient.update();                        //this might be redundant, but im not sure if the timeClient.get commands update the client or not
        Serial.print("24 hr time ");
        Serial.println(timeClient.getFormattedTime());
        Serial.print("looped on ");
        Serial.println(xPortGetCoreID());
        hour = timeClient.getHours();
        minute = timeClient.getMinutes();
        second = timeClient.getSeconds();
        //EEPROM.write?
      }
    }
    if (wifiCast == true) {
      //offlineTime = millis(); //offline time keeping with millis() function, I believe it is precise enough but i can look into that
      //if (offlineTime - offlinePrev >= 1000) { // every 1000ms increment the second variable

      EVERY_N_SECONDS(1) { //this should replace my timer code to make it look pretty
        second += 1;
        Serial.print("Offline time ");
        Serial.print(hour);
        Serial.print(":");
        Serial.print(minute);
        Serial.print(".");
        Serial.println(second);
        //offlinePrev = offlineTime;
      }
    }


    if (second >= 59) {
      second = 0;
      minute += 1;
    }
    if (minute >= 60) {
      minute = 0;
      hour += 1;
    }
    if (hour >= 24) {
      hour = 0;
    }


    touch = touchRead(T0);

    if (touch > touchMax) {
      touchMax = touch;
    }
    if (touch < touchMin) {
      touchMin = touch;
    }
    touchRange = touchMax - touchMin;
    if (touch < touchRange / 2) {
      touchButton = 1;
    }
    else {
      touchButton = 0;
    }

    if (touchButton == 0 && touchButton != touchButtonLast) {
      lightMode++;
    }
    if (lightMode > modeCount) {
      lightMode = 1;
    }
    touchButtonLast = touchButton;

    if (touchButton == 1) {
      //Serial.print("touch value ");
      // Serial.println(touch);
      Serial.print("button value ");
      Serial.println(touchButton);
      Serial.print("light mode ");
      Serial.println(lightMode);
      if (wifiCast == true) {
        Serial.print("offlineTime ");
        Serial.println(offlineTime);
        Serial.print("offlinePrev ");
        Serial.print(offlinePrev);
      }
    }


    if (lightMode == 1) {
      timeDisplay1(hour, minute, second);
    }
    else if (lightMode == 2) {
      timeDisplay2(hour, minute, second);
    }
    else if (lightMode == 3) {
      timeDisplay3(hour, minute, second);
    } //use if to choose between 'renderers'
    else if (lightMode == 4) {
      timeDisplay4(hour, minute, second);
    }
    else if (lightMode == 5) {
      timeDisplay5(hour, minute, second);
    }
      FastLED.show();

    }
  }
