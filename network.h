/*
  ANAVI Word Clock - Network Functions Header
  Functions for WiFi, MQTT, Home Assistant and Configurations
*/

#ifndef NETWORK_FUNCTIONS_H
#define NETWORK_FUNCTIONS_H

#include <PubSubClient.h>
#include <WiFiManager.h>
#include <Arduino.h>

// External references to global variables defined in main .ino
extern char mqtt_server[];
extern char mqtt_port[];
extern char workgroup[];
extern char username[];
extern char password[];
extern char configLed1[];
extern char ledType[];
extern char ledColorOrder[];
extern char temp_scale[];
extern bool configTempCelsius;
extern char machineId[];
extern bool shouldSaveConfig;
extern PubSubClient mqttClient;

extern char line1_topic[];
extern char line2_topic[];
extern char line3_topic[];
extern char cmnd_temp_coefficient_topic[];
extern char cmnd_temp_format[];
extern char stat_temp_coefficient_topic[];
extern char cmnd_led1_power_topic[];
extern char cmnd_led1_color_topic[];
extern char cmnd_reset_hue_topic[];
extern char stat_led1_power_topic[];
extern char stat_led1_color_topic[];

#ifdef HOME_ASSISTANT_DISCOVERY
extern char ha_name[];
#endif

#ifdef OTA_UPGRADES
extern char ota_server[];
extern char cmnd_update_topic[];
#endif

// Function declarations

void waitForFactoryReset();

void factoryReset();

void processMessageScale(const char* text);

void mqttCallback(char* topic, byte* payload, unsigned int length);

void calculateMachineId();

void mqttReconnect();

void publishState();

void publishSensorData(const char* subTopic, const char* key, const float value);

void publishSensorData(const char* subTopic, const char* key, const String& value);

float convertCelsiusToFahrenheit(float temperature);

float convertTemperature(float temperature);

String formatTemperature(float temperature);

//callback notifying us of the need to save config
void saveConfigCallback();

void apWiFiCallback(WiFiManager *myWiFiManager);

void saveConfig();

#ifdef HOME_ASSISTANT_DISCOVERY
void publishDiscoveryState();
#endif

#ifdef OTA_UPGRADES
void do_ota_upgrade(const char* text);
#endif

#endif // NETWORK_FUNCTIONS_H
