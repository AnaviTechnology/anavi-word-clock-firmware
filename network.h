/*
  ANAVI Word Clock - Network Functions Header
  NetworkConnector class for WiFi, MQTT, Home Assistant and Configurations
*/
#ifndef NETWORK_FUNCTIONS_H
#define NETWORK_FUNCTIONS_H
#include <PubSubClient.h>
#include <WiFiManager.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <Arduino.h>
class NetworkConnector {
public:
    // Constructor
    NetworkConnector();
    // Public methods used in setup()
    void begin();
    void setupWiFi();
    void setupMQTT();
    void printConfiguration();
    // Public methods used in loop()
    void updateTime();
    unsigned long getEpochTime();
    // Getters for configuration
    bool isTempCelsius() const { return configTempCelsius; }
    const char* getMachineId() const { return machineId; }
    long getTimezoneOffset() const { return timezoneOffset; }
private:
    // WiFi and NTP
    WiFiUDP ntpUDP;
    NTPClient timeClient;
    long timezoneOffset;  // Timezone offset in seconds
    // MQTT
    WiFiClient espClient;
    PubSubClient mqttClient;
    // Configuration variables
    char mqtt_server[40];
    char mqtt_port[6];
    char workgroup[32];
    char username[20];
    char password[20];
    char temp_scale[40];
    char timezone[10];  // Stores timezone offset as string (e.g., "+2" or "-5")
    bool configTempCelsius;
    char machineId[33];
    bool shouldSaveConfig;
    #ifdef HOME_ASSISTANT_DISCOVERY
    char ha_name[33];
    #endif
    #ifdef OTA_UPGRADES
    char ota_server[40];
    #endif
    // MQTT Topics
    char line1_topic[44];
    char line2_topic[44];
    char line3_topic[44];
    char cmnd_temp_coefficient_topic[47];
    char cmnd_temp_format[49];
    char stat_temp_coefficient_topic[47];
    char cmnd_led1_power_topic[50];
    char cmnd_led1_color_topic[50];
    char cmnd_reset_hue_topic[50];
    char stat_led1_power_topic[50];
    char stat_led1_color_topic[50];
    #ifdef OTA_UPGRADES
    char cmnd_update_topic[50];
    #endif
    // Private methods - Factory reset
    void waitForFactoryReset();
    void factoryReset();
    // Private methods - MQTT
    void mqttCallback(char* topic, byte* payload, unsigned int length);
    static void mqttCallbackWrapper(char* topic, byte* payload, unsigned int length);
    void processMessageScale(const char* text);
    void mqttReconnect();
    void publishState();
    void publishSensorData(const char* subTopic, const char* key, const float value);
    void publishSensorData(const char* subTopic, const char* key, const String& value);
    // Private methods - Configuration
    void calculateMachineId();
    void loadConfig();
    void saveConfig();
    void saveConfigCallback();
    static void saveConfigCallbackWrapper();
    void apWiFiCallback(WiFiManager *myWiFiManager);
    static void apWiFiCallbackWrapper(WiFiManager *myWiFiManager);
    // Private methods - Timezone
    void updateTimezoneOffset();
    const char* buildTimezoneDropdown();
    const char* buildTimezoneDetectJS();
    // Private methods - Temperature conversion
    float convertCelsiusToFahrenheit(float temperature);
    float convertTemperature(float temperature);
    String formatTemperature(float temperature);
    #ifdef HOME_ASSISTANT_DISCOVERY
    void publishDiscoveryState();
    #endif
    #ifdef OTA_UPGRADES
    void do_ota_upgrade(const char* text);
    #endif
    // Static pointer for callbacks
    static NetworkConnector* instance;
};
#endif // NETWORK_FUNCTIONS_H
