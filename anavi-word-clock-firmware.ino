/*
  ANAVI Word Clock
  8x8 WS2812B LEDs using XIAO ESP32C3

  grid pattern

  A T W E N T Y D
  Q U A R T E R Y
  F I V E H A L F
  D P A S T O R O
  F I V E I G H T
  S I X T H R E E
  T W E L E V E N
  F O U R N I N E


  Acknowledgements:
  - Thanks to Andy Doro and Dano for the inspiration:
  https://andydoro.com/wordclockdesktop/

*/

// include the library code:
#include <Wire.h>
#include <Adafruit_GFX.h>
// Adafruit NeoMatrix Library
#include <Adafruit_NeoMatrix.h>
// Adafruit NeoPixel
#include <Adafruit_NeoPixel.h>
#include <WiFi.h>
#include <WiFiUdp.h>
// NTPClient Library
#include <NTPClient.h>
// RTClib Library by Adafruit
#include <RTClib.h>
// ArduinoJson Library
#include <ArduinoJson.h>
// WiFiManager Library
#include <WiFiManager.h>
// PubSubClient Library for MQTT
#include <PubSubClient.h>
// From Arduino core for ESP32
#include <MD5Builder.h>
#include <nvs_flash.h>
#include <SPIFFS.h>

// NTP server setup
WiFiUDP ntpUDP;
// UTC+2 for Bulgaria
NTPClient timeClient(ntpUDP, "pool.ntp.org", 2 * 3600); 

// define how to write each of the words

// 64-bit "mask" for each pixel in the matrix- is it on or off?
uint64_t mask;

// define masks for each word. we add them with "bitwise or" to generate a mask for the entire "phrase".
#define MFIVE    mask |= 0xF0000000000 // these are in hexadecimal, 00 for an empty row
#define MTEN     mask |= 0x1A00000000000000
#define AQUARTER mask |= 0x1FE000000000000
#define TWENTY   mask |= 0x7E00000000000000
#define HALF     mask |= 0xF00000000000
#define PAST     mask |= 0x7800000000
#define TO       mask |= 0xC00000000
#define ONE      mask |= 0x43
#define TWO      mask |= 0x340
#define THREE    mask |= 0x1F0000
#define FOUR     mask |= 0xF0
#define FIVE     mask |= 0xF000000
#define SIX      mask |= 0xE00000
#define SEVEN    mask |= 0x80F000
#define EIGHT    mask |= 0xF8000000
#define NINE     mask |= 0xF
#define TEN      mask |= 0x80018000
#define ELEVEN   mask |= 0xFC00
#define TWELVE   mask |= 0x6F00
#define ANAVI    mask |= 0x1100200000002004
#define WIFI     mask |= 0x400020003000000
#define HA       mask |= 0x300000000000

// define pins
#define NEOPIN 10  // connect to DIN on NeoMatrix 8x8

// brightness based on time of day- could try warmer colors at night?
#define DAYBRIGHTNESS 40
#define NIGHTBRIGHTNESS 20

// cutoff times for day / night brightness. feel free to modify.
#define MORNINGCUTOFF 7  // when does daybrightness begin?   7am
#define NIGHTCUTOFF   22 // when does nightbrightness begin? 10pm


// define delays
#define FLASHDELAY 100  // delay for startup "flashWords" sequence
#define SHIFTDELAY 100   // controls color shifting speed

// Define US or EU rules for DST comment out as required. More countries could be added with different rules in DST_RTC.cpp
const char rulesDST[] = "US"; // US DST rules
// const char rulesDST[] = "EU";   // EU DST rules

DateTime theTime; // Holds current clock time

// an integer for the color shifting effect
int j;

// configure for 8x8 neopixel matrix
Adafruit_NeoMatrix matrix = Adafruit_NeoMatrix(8, 8, NEOPIN,
                            NEO_MATRIX_TOP  + NEO_MATRIX_LEFT +
                            NEO_MATRIX_ROWS + NEO_MATRIX_ZIGZAG,
                            NEO_GRB         + NEO_KHZ800);

// Configure pins
const int pinAlarm = D3;
const int pinButton = D8;
//extra pins: 0, 1 and 2
const int pinExtra = D2;

//define your default values here, if there are different values in config.json, they are overwritten.
char mqtt_server[40] = "mqtt.eclipseprojects.io";
char mqtt_port[6] = "1883";
char workgroup[32] = "workgroup";
// MQTT username and password
char username[20] = "";
char password[20] = "";
// Configurations for number of LEDs in each strip
char configLed1[20] = "10";
// LED type
char ledType[20] = "WS2812B";
// Color order of the LEDs: GRB, RGB
char ledColorOrder[20] = "GRB";
#ifdef HOME_ASSISTANT_DISCOVERY
char ha_name[32+1] = "";        // Make sure the machineId fits.
#endif
#ifdef OTA_UPGRADES
char ota_server[40];
#endif
char temp_scale[40] = "celsius";

// Set the temperature in Celsius or Fahrenheit
// true - Celsius, false - Fahrenheit
bool configTempCelsius = true;

// MD5 of chip ID.  If you only have a handful of Miracle Emitters and use
// your own MQTT broker (instead of iot.eclips.org) you may want to
// truncate the MD5 by changing the 32 to a smaller value.
char machineId[32+1] = "";

//flag for saving data
bool shouldSaveConfig = false;

// MQTT
WiFiClient espClient;
PubSubClient mqttClient(espClient);

char line1_topic[11 + sizeof(machineId)];
char line2_topic[11 + sizeof(machineId)];
char line3_topic[11 + sizeof(machineId)];
char cmnd_temp_coefficient_topic[14 + sizeof(machineId)];
char cmnd_temp_format[16 + sizeof(machineId)];

char stat_temp_coefficient_topic[14 + sizeof(machineId)];

char cmnd_led1_power_topic[49];
char cmnd_led1_color_topic[49];
char cmnd_reset_hue_topic[47];

char stat_led1_power_topic[50];
char stat_led1_color_topic[50];

// change brightness based on the time of day.

// show colorshift through the phrase mask. for each NeoPixel either show a color or show nothing!
void applyMask() {

  for (byte i = 0; i < 64; i++) {
    boolean masker = bitRead(mask, 63 - i); // bitread is backwards because bitRead reads rightmost digits first. could have defined the word masks differently
    switch (masker) {
      case 0:
        matrix.setPixelColor(i, 0, 0, 0);
        break;
      case 1:
        matrix.setPixelColor(i, Wheel(((i * 256 / matrix.numPixels()) + j) & 255));
        break;
    }
  }

  matrix.show(); // show it!
  delay(SHIFTDELAY);
  j++; // move the colors forward
  j = j % (256 * 5);

  // reset mask for next time
  mask = 0;
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {

  WheelPos = 255 - WheelPos;
  uint32_t wheelColor;

  if (WheelPos < 85) {
    wheelColor = matrix.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else if (WheelPos < 170) {
    WheelPos -= 85;
    wheelColor = matrix.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  } else {
    WheelPos -= 170;
    wheelColor = matrix.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  }

  // convert from 24-bit to 16-bit color - NeoMatrix requires 16-bit. perhaps there's a better way to do this.
  uint32_t bits = wheelColor;
  uint32_t blue = bits & 0x001F;     // 5 bits blue
  uint32_t green = bits & 0x07E0;    // 6 bits green
  uint32_t red = bits & 0xF800;      // 5 bits red

  // Return shifted bits with alpha set to 0xFF
  return (red << 8) | (green << 5) | (blue << 3) | 0xFF000000;
}


// Slightly different, this makes the rainbow equally distributed throughout
void rainbowCycle(uint8_t wait) {
  
  uint16_t i, j;

  // Cycly through all colors
  for (j = 0; j < 256; j++) {
    for (i = 0; i < matrix.numPixels(); i++) {
      matrix.setPixelColor(i, Wheel(((i * 256 / matrix.numPixels()) + j) & 255));
    }
    matrix.show();
    delay(wait);
  }
}

void adjustBrightness() {
  
  //change brightness if it's night time
  if (theTime.hour() < MORNINGCUTOFF || theTime.hour() > NIGHTCUTOFF) {
    matrix.setBrightness(NIGHTBRIGHTNESS);
  } else {
    matrix.setBrightness(DAYBRIGHTNESS);
  }
}

// function to generate the right "phrase" based on the time

void displayTime(void) {
  
  // time we display the appropriate theTime.minute() counter
  if ((theTime.minute() > 4) && (theTime.minute() < 10)) {
    MFIVE;
  }
  if ((theTime.minute() > 9) && (theTime.minute() < 15)) {
    MTEN;
  }
  if ((theTime.minute() > 14) && (theTime.minute() < 20)) {
    AQUARTER;
  }
  if ((theTime.minute() > 19) && (theTime.minute() < 25)) {
    TWENTY;
  }
  if ((theTime.minute() > 24) && (theTime.minute() < 30)) {
    TWENTY;
    MFIVE;
  }
  if ((theTime.minute() > 29) && (theTime.minute() < 35)) {
    HALF;
  }
  if ((theTime.minute() > 34) && (theTime.minute() < 40)) {
    TWENTY;
    MFIVE;
  }
  if ((theTime.minute() > 39) && (theTime.minute() < 45)) {
    TWENTY;
  }
  if ((theTime.minute() > 44) && (theTime.minute() < 50)) {
    AQUARTER;
  }
  if ((theTime.minute() > 49) && (theTime.minute() < 55)) {
    MTEN;
  }
  if (theTime.minute() > 54) {
    MFIVE;
  }

  if ((theTime.minute() < 5))
  {
    switch (theTime.hour()) {
      case 1:
      case 13:
        ONE;
        break;
      case 2:
      case 14:
        TWO;
        break;
      case 3:
      case 15:
        THREE;
        break;
      case 4:
      case 16:
        FOUR;
        break;
      case 5:
      case 17:
        FIVE;
        break;
      case 6:
      case 18:
        SIX;
        break;
      case 7:
      case 19:
        SEVEN;
        break;
      case 8:
      case 20:
        EIGHT;
        break;
      case 9:
      case 21:
        NINE;
        break;
      case 10:
      case 22:
        TEN;
        break;
      case 11:
      case 23:
        ELEVEN;
        break;
      case 0:
      case 12:
        TWELVE;
        break;
    }

  }
  else if ((theTime.minute() < 35) && (theTime.minute() > 4))
  {
    PAST;
    Serial.print(" past ");
    switch (theTime.hour()) {
      case 1:
      case 13:
        ONE;
        break;
      case 2:
      case 14:
        TWO;
        break;
      case 3:
      case 15:
        THREE;
        break;
      case 4:
      case 16:
        FOUR;
        break;
      case 5:
      case 17:
        FIVE;
        break;
      case 6:
      case 18:
        SIX;
        break;
      case 7:
      case 19:
        SEVEN;
        break;
      case 8:
      case 20:
        EIGHT;
        break;
      case 9:
      case 21:
        NINE;
        break;
      case 10:
      case 22:
        TEN;
        break;
      case 11:
      case 23:
        ELEVEN;
        break;
      case 0:
      case 12:
        TWELVE;
        break;
    }
  }
  else
  {
    // if we are greater than 34 minutes past the hour then display
    // the next hour, as we will be displaying a 'to' sign
    TO;
    Serial.print(" to ");
    switch (theTime.hour()) {
      case 1:
      case 13:
        TWO;
        break;
      case 14:
      case 2:
        THREE;
        break;
      case 15:
      case 3:
        FOUR;
        break;
      case 4:
      case 16:
        FIVE;
        break;
      case 5:
      case 17:
        SIX;
        break;
      case 6:
      case 18:
        SEVEN;
        break;
      case 7:
      case 19:
        EIGHT;
        break;
      case 8:
      case 20:
        NINE;
        break;
      case 9:
      case 21:
        TEN;
        break;
      case 10:
      case 22:
        ELEVEN;
        break;
      case 11:
      case 23:
        TWELVE;
        break;
      case 0:
      case 12:
        ONE;
        break;
    }
  }

  // Apply phrase mask to colorshift function
  applyMask();

}

void flashWords(void) {

  ANAVI;
  applyMask();
  delay(FLASHDELAY * 2);

  MFIVE;
  applyMask();
  delay(FLASHDELAY);
  
  MTEN;
  applyMask();
  delay(FLASHDELAY);

  AQUARTER;
  applyMask();
  delay(FLASHDELAY);

  TWENTY;
  applyMask();
  delay(FLASHDELAY);

  HALF;
  applyMask();
  delay(FLASHDELAY);

  TO;
  applyMask();
  delay(FLASHDELAY);

  PAST;
  applyMask();
  delay(FLASHDELAY);
  

  ONE;
  applyMask();
  delay(FLASHDELAY);

  TWO;
  applyMask();
  delay(FLASHDELAY);
  
  THREE;
  applyMask();
  delay(FLASHDELAY);
  
  FOUR;
  applyMask();
  delay(FLASHDELAY);

  FIVE;
  applyMask();
  delay(FLASHDELAY);

  SIX;
  applyMask();
  delay(FLASHDELAY);

  SEVEN;
  applyMask();
  delay(FLASHDELAY);

  EIGHT;
  applyMask();
  delay(FLASHDELAY);


  NINE;
  applyMask();
  delay(FLASHDELAY);

  TEN;
  applyMask();
  delay(FLASHDELAY);

  ELEVEN;
  applyMask();
  delay(FLASHDELAY);

  TWELVE;
  applyMask();
  delay(FLASHDELAY);

  // blank for a bit
  applyMask();
  delay(FLASHDELAY);

}

void waitForFactoryReset()
{
    Serial.println("Press button within 4 seconds for factory reset...");
    for (int iter = 0; iter < 40; iter++)
    {
        digitalWrite(pinAlarm, HIGH);
        delay(50);
        if (false == digitalRead(pinButton))
        {
            factoryReset();
            return;
        }
        digitalWrite(pinAlarm, LOW);
        delay(50);
        if (false == digitalRead(pinButton))
        {
            factoryReset();
            return;
        }
    }
}

void factoryReset()
{
    if (false == digitalRead(pinButton))
    {
        Serial.println("Hold the button to reset to factory defaults...");
        bool cancel = false;
        for (int iter=0; iter<30; iter++)
        {
            digitalWrite(pinAlarm, HIGH);
            delay(100);
            if (true == digitalRead(pinButton))
            {
                cancel = true;
                break;
            }
            digitalWrite(pinAlarm, LOW);
            delay(100);
            if (true == digitalRead(pinButton))
            {
                cancel = true;
                break;
            }
        }
        if (false == digitalRead(pinButton) && !cancel)
        {
            digitalWrite(pinAlarm, HIGH);
            Serial.println("Disconnecting...");
            // Disconnect from any previously connected WiFi
            // true to erase the saved credentials
            WiFi.disconnect(true);

            Serial.println("Restarting...");
            // Erase the NVS (Non-Volatile Storage)
            esp_err_t result = nvs_flash_erase();
            if (ESP_OK == result)
            {
              Serial.println("NVS erased successfully.");
            } else
            {
              Serial.print("Failed to erase NVS. Error code: ");
              Serial.println(result);
            }
            // Restart the board
            ESP.restart();
        }
        else
        {
            // Cancel reset to factory defaults
            Serial.println("Reset to factory defaults cancelled.");
            digitalWrite(pinAlarm, LOW);
        }
    }
}

void processMessageScale(const char* text)
{
    StaticJsonDocument<200> data;
    deserializeJson(data, text);
    // Set temperature to Celsius or Fahrenheit and redraw screen
    Serial.print("Changing the temperature scale to: ");
    if (data.containsKey("scale") && (0 == strcmp(data["scale"], "celsius")) )
    {
        Serial.println("Celsius");
        configTempCelsius = true;
        strcpy(temp_scale, "celsius");
    }
    else
    {
        Serial.println("Fahrenheit");
        configTempCelsius = false;
        strcpy(temp_scale, "fahrenheit");
    }
    // Save configurations to file
    saveConfig();
}

void mqttCallback(char* topic, byte* payload, unsigned int length)
{
    // Convert received bytes to a string
    char text[length + 1];
    snprintf(text, length + 1, "%s", payload);

    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("] ");
    Serial.println(text);

    if (strcmp(topic, cmnd_temp_format) == 0)
    {
        processMessageScale(text);
    }

#ifdef OTA_UPGRADES
    if (strcmp(topic, cmnd_update_topic) == 0)
    {
        Serial.println("OTA request seen.\n");
        do_ota_upgrade(text);
        // Any OTA upgrade will stop the mqtt client, so if the
        // upgrade failed and we get here publishState() will fail.
        // Just return here, and we will reconnect from within the
        // loop().
        return;
    }
#endif
}

void calculateMachineId()
{
    MD5Builder md5;
    md5.begin();
    uint64_t chipId = ESP.getEfuseMac();
    char chipIdStr[13];
    snprintf(chipIdStr, sizeof(chipIdStr), "%04X%08X", (uint16_t)(chipId >> 32), (uint32_t)chipId);
    md5.add(chipIdStr);
    md5.calculate();
    md5.toString().toCharArray(machineId, sizeof(machineId));
}

void mqttReconnect()
{
  char clientId[18 + sizeof(machineId)];
  snprintf(clientId, sizeof(clientId), "anavi-miracle-emitter-%s", machineId);

  // Loop until we're reconnected
  for (int attempt = 0; attempt < 3; ++attempt)
  {
      Serial.print("Attempting MQTT connection...");
      // Attempt to connect
      if (true == mqttClient.connect(clientId, username, password))
      {
          Serial.println("connected");

          // Subscribe to MQTT topics

          // LED1
          mqttClient.subscribe(cmnd_led1_power_topic);
          mqttClient.subscribe(cmnd_led1_color_topic);
          // Topic to reset hue
          mqttClient.subscribe(cmnd_reset_hue_topic);

          mqttClient.subscribe(line1_topic);
          mqttClient.subscribe(line2_topic);
          mqttClient.subscribe(line3_topic);
          mqttClient.subscribe(cmnd_temp_coefficient_topic);
          mqttClient.subscribe(cmnd_temp_format);
  #ifdef OTA_UPGRADES
          mqttClient.subscribe(cmnd_update_topic);
  #endif

  #ifdef HOME_ASSISTANT_DISCOVERY
          // Publish discovery messages
          publishDiscoveryState();
  #endif

          // Publish initial status of both LED strips
          publishState();
          break;

      }
      else
      {
          Serial.print("failed, rc=");
          Serial.print(mqttClient.state());
          Serial.println(" try again in 5 seconds");
          // Wait 5 seconds before retrying
          delay(5000);
      }
  }
}

void publishState()
{
  //TODO
}

void publishSensorData(const char* subTopic, const char* key, const float value)
{
    StaticJsonDocument<100> json;
    json[key] = value;
    char payload[100];
    serializeJson(json, payload);
    char topic[200];
    sprintf(topic,"%s/%s/%s", workgroup, machineId, subTopic);
    mqttClient.publish(topic, payload, true);
}

void publishSensorData(const char* subTopic, const char* key, const String& value)
{
    StaticJsonDocument<100> json;
    json[key] = value;
    char payload[100];
    serializeJson(json, payload);
    char topic[200];
    sprintf(topic,"%s/%s/%s", workgroup, machineId, subTopic);
    mqttClient.publish(topic, payload, true);
}

float convertCelsiusToFahrenheit(float temperature)
{
    return (temperature * 9/5 + 32);
}

float convertTemperature(float temperature)
{
    return (true == configTempCelsius) ? temperature : convertCelsiusToFahrenheit(temperature);
}

String formatTemperature(float temperature)
{
    String unit = (true == configTempCelsius) ? "°C" : "°F";
    return String(convertTemperature(temperature), 1) + unit;
}

//callback notifying us of the need to save config
void saveConfigCallback ()
{
    Serial.println("Should save config");
    shouldSaveConfig = true;
}

void apWiFiCallback(WiFiManager *myWiFiManager)
{
    String configPortalSSID = myWiFiManager->getConfigPortalSSID();
    // Print information in the serial output
    Serial.print("Created access point for configuration: ");
    Serial.println(configPortalSSID);
}

void saveConfig()
{
    Serial.println("saving config");
    DynamicJsonDocument json(1024);
    json["mqtt_server"] = mqtt_server;
    json["mqtt_port"] = mqtt_port;
    json["workgroup"] = workgroup;
    json["username"] = username;
    json["password"] = password;
    json["led_type"] = ledType;
    json["led_color_order"] = ledColorOrder;
    json["temp_scale"] = temp_scale;
#ifdef HOME_ASSISTANT_DISCOVERY
    json["ha_name"] = ha_name;
#endif
#ifdef OTA_UPGRADES
    json["ota_server"] = ota_server;
#endif

    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile)
    {
        Serial.println("failed to open config file for writing");
    }

    serializeJson(json, Serial);
    Serial.println("");
    serializeJson(json, configFile);
    configFile.close();
    //end save
}

void setup() {
  // put your setup code here, to run once:

  // set pinmodes
  pinMode(NEOPIN, OUTPUT);

  //LED
  pinMode(pinAlarm, OUTPUT);
  //Button
  pinMode(pinButton, INPUT);
  // Set the extra pin to low in output mode
  pinMode(pinExtra, OUTPUT);
  digitalWrite(pinExtra, LOW);

  Serial.begin(115200);
  Serial.println();

  matrix.begin();
  matrix.setBrightness(DAYBRIGHTNESS);
  // Initialize all pixels to 'off'
  matrix.fillScreen(0); 
  matrix.show();

  // Startup
  rainbowCycle(5);
  delay(100);
  flashWords();
  delay(100);

  // Power-up safety delay and a chance for resetting the board
  waitForFactoryReset();

  // Machine ID
  calculateMachineId();

  // Set MQTT topics
  sprintf(cmnd_led1_power_topic, "cmnd/%s/power", machineId);
  sprintf(cmnd_led1_color_topic, "cmnd/%s/color", machineId);
  sprintf(cmnd_reset_hue_topic, "cmnd/%s/resethue", machineId);

  sprintf(stat_led1_power_topic, "stat/%s/power", machineId);
  sprintf(stat_led1_color_topic, "stat/%s/color", machineId);

  //read configuration from FS json
  Serial.println("mounting FS...");

  if (SPIFFS.begin(true))
  {
      Serial.println("mounted file system");
      if (SPIFFS.exists("/config.json")) {
          //file exists, reading and loading
          Serial.println("reading config file");
          File configFile = SPIFFS.open("/config.json", "r");
          if (configFile)
          {
              Serial.println("opened config file");
              const size_t size = configFile.size();
              // Allocate a buffer to store contents of the file.
              std::unique_ptr<char[]> buf(new char[size]);

              configFile.readBytes(buf.get(), size);
              DynamicJsonDocument json(1024);
              if (DeserializationError::Ok == deserializeJson(json, buf.get()))
              {
  #ifdef DEBUG
                  // Content stored in the memory of the microcontroller contains
                  // sensitive data such as username and passwords therefore
                  // should be printed only during debugging
                  serializeJson(json, Serial);
                  Serial.println("\nparsed json");
  #endif

                  strcpy(mqtt_server, json["mqtt_server"]);
                  strcpy(mqtt_port, json["mqtt_port"]);
                  strcpy(workgroup, json["workgroup"]);
                  strcpy(username, json["username"]);
                  strcpy(password, json["password"]);
                  strcpy(ledType, json["led_type"]);
                  strcpy(ledColorOrder, json["led_color_order"]);

                  strcpy(temp_scale, json["temp_scale"]);

  #ifdef HOME_ASSISTANT_DISCOVERY
                  {
                      const char *s = json["ha_name"];
                      if (!s)
                          s = machineId;
                      snprintf(ha_name, sizeof(ha_name), "%s", s);
                  }
  #endif
  #ifdef OTA_UPGRADES
                  {
                      const char *s = json["ota_server"];
                      if (!s)
                          s = ""; // The empty string never matches.
                      snprintf(ota_server, sizeof(ota_server), "%s", s);
                  }
  #endif
              }
              else
              {
                  Serial.println("failed to load json config");
              }
          }
      }
  }
  else
  {
      Serial.println("failed to mount FS");
  }
  //end read

  // Set MQTT topics
  sprintf(line1_topic, "cmnd/%s/line1", machineId);
  sprintf(line2_topic, "cmnd/%s/line2", machineId);
  sprintf(line3_topic, "cmnd/%s/line3", machineId);
  sprintf(cmnd_temp_coefficient_topic, "cmnd/%s/tempcoef", machineId);
  sprintf(stat_temp_coefficient_topic, "stat/%s/tempcoef", machineId);
  sprintf(cmnd_temp_format, "cmnd/%s/tempformat", machineId);
  #ifdef OTA_UPGRADES
    sprintf(cmnd_update_topic, "cmnd/%s/update", machineId);
  #endif

  // The extra parameters to be configured (can be either global or just in the setup)
  // After connecting, parameter.getValue() will get you the configured value
  // id/name placeholder/prompt default length
  WiFiManagerParameter custom_mqtt_server("server", "mqtt server", mqtt_server, sizeof(mqtt_server));
  WiFiManagerParameter custom_mqtt_port("port", "mqtt port", mqtt_port, sizeof(mqtt_port));
  WiFiManagerParameter custom_workgroup("workgroup", "workgroup", workgroup, sizeof(workgroup));
  WiFiManagerParameter custom_mqtt_user("user", "MQTT username", username, sizeof(username));
  WiFiManagerParameter custom_mqtt_pass("pass", "MQTT password", password, sizeof(password));
  WiFiManagerParameter custom_led_type("ledType", "WS2812B", ledType, sizeof(ledType));
  WiFiManagerParameter custom_led_color_order("ledColorOrder", "GRB", ledColorOrder, sizeof(ledColorOrder));
  WiFiManagerParameter custom_led1("led1", "LED", configLed1, sizeof(configLed1));
  #ifdef HOME_ASSISTANT_DISCOVERY
    WiFiManagerParameter custom_mqtt_ha_name("ha_name", "Device name for Home Assistant", ha_name, sizeof(ha_name));
  #endif
  #ifdef OTA_UPGRADES
    WiFiManagerParameter custom_ota_server("ota_server", "OTA server", ota_server, sizeof(ota_server));
  #endif
  WiFiManagerParameter custom_temperature_scale("temp_scale", "Temperature scale", temp_scale, sizeof(temp_scale));

  char htmlMachineId[200];
  sprintf(htmlMachineId,"<p style=\"color: red;\">Machine ID:</p><p><b>%s</b></p><p>Copy and save the machine ID because you will need it to control the device.</p>", machineId);
  WiFiManagerParameter custom_text_machine_id(htmlMachineId);

  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;

  //set config save notify callback
  wifiManager.setSaveConfigCallback(saveConfigCallback);

  //add all your parameters here
  wifiManager.addParameter(&custom_mqtt_server);
  wifiManager.addParameter(&custom_mqtt_port);
  wifiManager.addParameter(&custom_workgroup);
  wifiManager.addParameter(&custom_mqtt_user);
  wifiManager.addParameter(&custom_mqtt_pass);
  wifiManager.addParameter(&custom_led_type);
  wifiManager.addParameter(&custom_led_color_order);
  wifiManager.addParameter(&custom_led1);
  wifiManager.addParameter(&custom_temperature_scale);
  #ifdef HOME_ASSISTANT_DISCOVERY
    wifiManager.addParameter(&custom_mqtt_ha_name);
  #endif
  #ifdef OTA_UPGRADES
    wifiManager.addParameter(&custom_ota_server);
  #endif
  wifiManager.addParameter(&custom_text_machine_id);

  //sets timeout until configuration portal gets turned off
  //useful to make it all retry or go to sleep
  //in seconds
  wifiManager.setTimeout(300);

  digitalWrite(pinAlarm, HIGH);

  WIFI;
  applyMask();

  //fetches ssid and pass and tries to connect
  //if it does not connect it starts an access point
  //and goes into a blocking loop awaiting configuration
  wifiManager.setAPCallback(apWiFiCallback);
  // Append the last 5 character of the machine id to the access point name
  String apId(machineId);
  apId = apId.substring(apId.length() - 5);
  String accessPointName = "ANAVI Miracle Emitter " + apId;
  if (!wifiManager.autoConnect(accessPointName.c_str(), ""))
  {
    digitalWrite(pinAlarm, LOW);
    Serial.println("failed to connect and hit timeout");
    delay(3000);
  }

  //if you get here you have connected to the WiFi
  Serial.println("connected!)");
  digitalWrite(pinAlarm, LOW);

  //read updated parameters
  strcpy(mqtt_server, custom_mqtt_server.getValue());
  strcpy(mqtt_port, custom_mqtt_port.getValue());
  strcpy(workgroup, custom_workgroup.getValue());
  strcpy(username, custom_mqtt_user.getValue());
  strcpy(password, custom_mqtt_pass.getValue());
  strcpy(ledType, custom_led_type.getValue());
  strcpy(ledColorOrder, custom_led_color_order.getValue());
  int saveLed1 = atoi(custom_led1.getValue());
  if (0 > saveLed1)
  {
    saveLed1 = 10;
  }
  strcpy(temp_scale, custom_temperature_scale.getValue());
  #ifdef HOME_ASSISTANT_DISCOVERY
    strcpy(ha_name, custom_mqtt_ha_name.getValue());
  #endif
  #ifdef OTA_UPGRADES
    strcpy(ota_server, custom_ota_server.getValue());
  #endif

  //save the custom parameters to FS
  if (shouldSaveConfig)
  {
      saveConfig();
  }

  Serial.println("local ip");
  Serial.println(WiFi.localIP());

  timeClient.begin();
  timeClient.update();

  HA;
  applyMask();

  // MQTT
  Serial.print("MQTT Server: ");
  Serial.println(mqtt_server);
  Serial.print("MQTT Port: ");
  Serial.println(mqtt_port);
  // Print MQTT Username
  Serial.print("MQTT Username: ");
  Serial.println(username);
  // Hide password from the log and show * instead
  char hiddenpass[20] = "";
  for (size_t charP=0; charP < strlen(password); charP++)
  {
      hiddenpass[charP] = '*';
  }
  hiddenpass[strlen(password)] = '\0';
  Serial.print("MQTT Password: ");
  Serial.println(hiddenpass);
      Serial.print("Saved temperature scale: ");
  Serial.println(temp_scale);
  configTempCelsius = String(temp_scale).equalsIgnoreCase("celsius");
  Serial.print("Temperature scale: ");
  if (true == configTempCelsius)
  {
    Serial.println("Celsius");
  }
  else
  {
    Serial.println("Fahrenheit");
  }
  #ifdef HOME_ASSISTANT_DISCOVERY
    Serial.print("Home Assistant device name: ");
    Serial.println(ha_name);
  #endif
  #ifdef OTA_UPGRADES
    if (ota_server[0] != '\0')
    {
        Serial.print("OTA server: ");
        Serial.println(ota_server);
    }
    else
    {
    #  ifndef OTA_SERVER
        Serial.println("No OTA server");
    #  endif
    }

    #  ifdef OTA_SERVER
    Serial.print("Hardcoded OTA server: ");
    Serial.println(OTA_SERVER);
    #  endif

  #endif

  const int mqttPort = atoi(mqtt_port);
  mqttClient.setServer(mqtt_server, mqttPort);
  mqttClient.setCallback(mqttCallback);

  mqttReconnect();

  Serial.println("");
  Serial.println("-----");
  Serial.print("Machine ID: ");
  Serial.println(machineId);
  Serial.println("-----");
  Serial.println("");

}

void loop() {

  timeClient.update();
  
  unsigned long epochTime = timeClient.getEpochTime();
  theTime = DateTime(epochTime);

  adjustBrightness();
  displayTime();
  delay(5);
}


