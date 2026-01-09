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

// Include configuration header with all constants
#include "config.h"

// Include function headers
#include "clock.h"
#include "network.h"

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
NTPClient timeClient(ntpUDP, NTP_SERVER, NTP_OFFSET);

// 64-bit "mask" for each pixel in the matrix- is it on or off?
uint64_t mask;

DateTime theTime; // Holds current clock time

// an integer for the color shifting effect
int j;

// configure for 8x8 neopixel matrix
Adafruit_NeoMatrix matrix = Adafruit_NeoMatrix(8, 8, NEOPIN,
                            NEO_MATRIX_TOP  + NEO_MATRIX_LEFT +
                            NEO_MATRIX_ROWS + NEO_MATRIX_ZIGZAG,
                            NEO_GRB         + NEO_KHZ800);

//define your default values here, if there are different values in config.json, they are overwritten.
char mqtt_server[MQTT_SERVER_SIZE] = DEFAULT_MQTT_SERVER;
char mqtt_port[MQTT_PORT_SIZE] = DEFAULT_MQTT_PORT;
char workgroup[WORKGROUP_SIZE] = DEFAULT_WORKGROUP;
// MQTT username and password
char username[USERNAME_SIZE] = "";
char password[PASSWORD_SIZE] = "";
// Configurations for number of LEDs in each strip
char configLed1[LED_CONFIG_SIZE] = DEFAULT_LED_COUNT;
// LED type
char ledType[LED_CONFIG_SIZE] = DEFAULT_LED_TYPE;
// Color order of the LEDs: GRB, RGB
char ledColorOrder[LED_CONFIG_SIZE] = DEFAULT_LED_COLOR_ORDER;
#ifdef HOME_ASSISTANT_DISCOVERY
char ha_name[HA_NAME_SIZE + 1] = "";        // Make sure the machineId fits.
#endif
#ifdef OTA_UPGRADES
char ota_server[OTA_SERVER_SIZE];
#endif
char temp_scale[TEMP_SCALE_SIZE] = DEFAULT_TEMP_SCALE;

// Set the temperature in Celsius or Fahrenheit
// true - Celsius, false - Fahrenheit
bool configTempCelsius = true;

// MD5 of chip ID.  If you only have a handful of Miracle Emitters and use
// your own MQTT broker (instead of iot.eclips.org) you may want to
// truncate the MD5 by changing the 32 to a smaller value.
char machineId[MACHINE_ID_SIZE + 1] = "";

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

char cmnd_led1_power_topic[TOPIC_SMALL_SIZE];
char cmnd_led1_color_topic[TOPIC_SMALL_SIZE];
char cmnd_reset_hue_topic[TOPIC_SMALL_SIZE];

char stat_led1_power_topic[TOPIC_SMALL_SIZE];
char stat_led1_color_topic[TOPIC_SMALL_SIZE];

void setup()
{
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
      if (SPIFFS.exists("/config.json"))
      {
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
              DynamicJsonDocument json(JSON_CONFIG_SIZE);
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
  WiFiManagerParameter custom_led_type("ledType", DEFAULT_LED_TYPE, ledType, sizeof(ledType));
  WiFiManagerParameter custom_led_color_order("ledColorOrder", DEFAULT_LED_COLOR_ORDER, ledColorOrder, sizeof(ledColorOrder));
  WiFiManagerParameter custom_led1("led1", "LED", configLed1, sizeof(configLed1));
  #ifdef HOME_ASSISTANT_DISCOVERY
    WiFiManagerParameter custom_mqtt_ha_name("ha_name", "Device name for Home Assistant", ha_name, sizeof(ha_name));
  #endif
  #ifdef OTA_UPGRADES
    WiFiManagerParameter custom_ota_server("ota_server", "OTA server", ota_server, sizeof(ota_server));
  #endif
  WiFiManagerParameter custom_temperature_scale("temp_scale", "Temperature scale", temp_scale, sizeof(temp_scale));

  char htmlMachineId[TOPIC_BUFFER_SIZE];
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
  wifiManager.setTimeout(WIFI_CONFIG_TIMEOUT);

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
  String accessPointName = String(WIFI_AP_NAME_PREFIX) + apId;
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
  char hiddenpass[PASSWORD_SIZE] = "";
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

void loop()
{

  timeClient.update();
  
  unsigned long epochTime = timeClient.getEpochTime();
  theTime = DateTime(epochTime);

  adjustBrightness();
  displayTime();
  delay(5);
}
