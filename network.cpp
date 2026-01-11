/*
  ANAVI Word Clock - Network Functions Implementation
  NetworkConnector class for WiFi, MQTT, Home Assistant and Configurations
*/
#include "network.h"
#include "config.h"
#include <WiFi.h>
#include <ArduinoJson.h>
#include <MD5Builder.h>
#include <nvs_flash.h>
#include <SPIFFS.h>
#include <Arduino.h>
// Initialize static instance pointer
NetworkConnector* NetworkConnector::instance = nullptr;
NetworkConnector::NetworkConnector()
    : timeClient(ntpUDP, NTP_SERVER, NTP_OFFSET)
    , mqttClient(espClient)
    , configTempCelsius(true)
    , shouldSaveConfig(false)
    , timezoneOffset(NTP_OFFSET)
{
    // Initialize configuration with defaults
    strcpy(mqtt_server, DEFAULT_MQTT_SERVER);
    strcpy(mqtt_port, DEFAULT_MQTT_PORT);
    strcpy(workgroup, DEFAULT_WORKGROUP);
    username[0] = '\0';
    password[0] = '\0';
    strcpy(temp_scale, DEFAULT_TEMP_SCALE);
    strcpy(timezone, "+2");  // Default UTC+2 for Bulgaria
    machineId[0] = '\0';
    #ifdef HOME_ASSISTANT_DISCOVERY
    ha_name[0] = '\0';
    #endif
    #ifdef OTA_UPGRADES
    ota_server[0] = '\0';
    #endif
    // Set static instance for callbacks
    instance = this;
}
void NetworkConnector::begin()
{
    // Calculate machine ID first
    calculateMachineId();
    // Set MQTT topics
    sprintf(cmnd_led1_power_topic, "cmnd/%s/power", machineId);
    sprintf(cmnd_led1_color_topic, "cmnd/%s/color", machineId);
    sprintf(cmnd_reset_hue_topic, "cmnd/%s/resethue", machineId);
    sprintf(stat_led1_power_topic, "stat/%s/power", machineId);
    sprintf(stat_led1_color_topic, "stat/%s/color", machineId);
    sprintf(line1_topic, "cmnd/%s/line1", machineId);
    sprintf(line2_topic, "cmnd/%s/line2", machineId);
    sprintf(line3_topic, "cmnd/%s/line3", machineId);
    sprintf(cmnd_temp_coefficient_topic, "cmnd/%s/tempcoef", machineId);
    sprintf(stat_temp_coefficient_topic, "stat/%s/tempcoef", machineId);
    sprintf(cmnd_temp_format, "cmnd/%s/tempformat", machineId);
    #ifdef OTA_UPGRADES
    sprintf(cmnd_update_topic, "cmnd/%s/update", machineId);
    #endif
    // Load configuration from file system
    loadConfig();
    // Update timezone offset based on loaded config
    updateTimezoneOffset();
    // Wait for factory reset option
    waitForFactoryReset();
}
void NetworkConnector::updateTimezoneOffset()
{
    // Convert timezone string (e.g., "+2" or "-5" or "5.5") to seconds
    float hours = atof(timezone);
    timezoneOffset = (long)(hours * 3600);
    // Update NTP client with new offset
    timeClient.setTimeOffset(timezoneOffset);
    Serial.print("Timezone offset set to: ");
    Serial.print(hours);
    Serial.print(" hours (");
    Serial.print(timezoneOffset);
    Serial.println(" seconds)");
}
const char* NetworkConnector::buildTimezoneDropdown()
{
    static char dropdown[1600];
    // Build a dropdown that updates a hidden field via onchange
    strcpy(dropdown, "<br/><label>Timezone</label>");
    strcat(dropdown, "<select id='tz' onchange=\"document.getElementById('timezone').value=this.value\">");
    // Define timezone options
    const char* options[][2] = {
        {"-12", "UTC-12"},
        {"-11", "UTC-11"},
        {"-10", "UTC-10"},
        {"-9", "UTC-9"},
        {"-8", "UTC-8"},
        {"-7", "UTC-7"},
        {"-6", "UTC-6"},
        {"-5", "UTC-5"},
        {"-4", "UTC-4"},
        {"-3", "UTC-3"},
        {"-2", "UTC-2"},
        {"-1", "UTC-1"},
        {"0", "UTC+0"},
        {"1", "UTC+1"},
        {"2", "UTC+2"},
        {"3", "UTC+3"},
        {"4", "UTC+4"},
        {"5", "UTC+5"},
        {"5.5", "UTC+5:30"},
        {"6", "UTC+6"},
        {"7", "UTC+7"},
        {"8", "UTC+8"},
        {"9", "UTC+9"},
        {"9.5", "UTC+9:30"},
        {"10", "UTC+10"},
        {"11", "UTC+11"},
        {"12", "UTC+12"},
        {"13", "UTC+13"},
        {"14", "UTC+14"}
    };
    // Add each option
    for (int i = 0; i < 29; i++) {
        char opt[50];
        if (strcmp(timezone, options[i][0]) == 0) {
            sprintf(opt, "<option value='%s' selected>%s</option>", options[i][0], options[i][1]);
        } else {
            sprintf(opt, "<option value='%s'>%s</option>", options[i][0], options[i][1]);
        }
        strcat(dropdown, opt);
    }
    strcat(dropdown, "</select>");
    return dropdown;
}
void NetworkConnector::setupWiFi()
{
    // WiFiManager parameters

    // Build and add timezone dropdown as custom HTML
    WiFiManagerParameter custom_timezone_dropdown(buildTimezoneDropdown());

    // Home Assistant and MQTT
    WiFiManagerParameter custom_mqtt_server("server", "mqtt server", mqtt_server, sizeof(mqtt_server));
    WiFiManagerParameter custom_mqtt_port("port", "mqtt port", mqtt_port, sizeof(mqtt_port));
    WiFiManagerParameter custom_workgroup("workgroup", "workgroup", workgroup, sizeof(workgroup));
    WiFiManagerParameter custom_mqtt_user("user", "MQTT username", username, sizeof(username));
    WiFiManagerParameter custom_mqtt_pass("pass", "MQTT password", password, sizeof(password));
    WiFiManagerParameter custom_temperature_scale("temp_scale", "Temperature scale", temp_scale, sizeof(temp_scale));
    #ifdef HOME_ASSISTANT_DISCOVERY
    WiFiManagerParameter custom_mqtt_ha_name("ha_name", "Device name for Home Assistant", ha_name, sizeof(ha_name));
    #endif
    #ifdef OTA_UPGRADES
    WiFiManagerParameter custom_ota_server("ota_server", "OTA server", ota_server, sizeof(ota_server));
    #endif
    char htmlMachineId[200];
    sprintf(htmlMachineId,"<p style=\"color: red;\">Machine ID:</p><p><b>%s</b></p><p>Copy and save the machine ID because you will need it to control the device.</p>", machineId);
    WiFiManagerParameter custom_text_machine_id(htmlMachineId);
    // WiFiManager setup
    WiFiManager wifiManager;
    wifiManager.setSaveConfigCallback(saveConfigCallbackWrapper);
    // Add timezone dropdown
    wifiManager.addParameter(&custom_timezone_dropdown);
    // Add all other parameters
    wifiManager.addParameter(&custom_mqtt_server);
    wifiManager.addParameter(&custom_mqtt_port);
    wifiManager.addParameter(&custom_workgroup);
    wifiManager.addParameter(&custom_mqtt_user);
    wifiManager.addParameter(&custom_mqtt_pass);
    wifiManager.addParameter(&custom_temperature_scale);
    #ifdef HOME_ASSISTANT_DISCOVERY
    wifiManager.addParameter(&custom_mqtt_ha_name);
    #endif
    #ifdef OTA_UPGRADES
    wifiManager.addParameter(&custom_ota_server);
    #endif
    wifiManager.addParameter(&custom_text_machine_id);
    wifiManager.setTimeout(WIFI_CONFIG_TIMEOUT);
    wifiManager.setAPCallback(apWiFiCallbackWrapper);
    digitalWrite(pinAlarm, HIGH);
    // Create access point name
    String apId(machineId);
    apId = apId.substring(apId.length() - 5);
    String accessPointName = String(WIFI_AP_NAME_PREFIX) + apId;
    if (!wifiManager.autoConnect(accessPointName.c_str(), ""))
    {
        digitalWrite(pinAlarm, LOW);
        Serial.println("failed to connect and hit timeout");
        delay(3000);
    }
    Serial.println("connected!)");
    digitalWrite(pinAlarm, LOW);
    // Read updated parameters
    strcpy(mqtt_server, custom_mqtt_server.getValue());
    strcpy(mqtt_port, custom_mqtt_port.getValue());
    strcpy(workgroup, custom_workgroup.getValue());
    strcpy(username, custom_mqtt_user.getValue());
    strcpy(password, custom_mqtt_pass.getValue());
    // Get timezone from the form submission
    // WiFiManager doesn't provide getValue() for custom HTML, so we need to read it differently
    // The value will be in the POST data with name 'timezone'
    if (shouldSaveConfig) {
        // Timezone will be read from HTTP POST - WiFiManager handles this internally
        // For now, keep existing value - we'll get it after save
    }
    strcpy(temp_scale, custom_temperature_scale.getValue());
    #ifdef HOME_ASSISTANT_DISCOVERY
    strcpy(ha_name, custom_mqtt_ha_name.getValue());
    #endif
    #ifdef OTA_UPGRADES
    strcpy(ota_server, custom_ota_server.getValue());
    #endif
    // Update timezone offset based on new value
    updateTimezoneOffset();
    // Save config if needed
    if (shouldSaveConfig)
    {
        saveConfig();
    }
    Serial.println("local ip");
    Serial.println(WiFi.localIP());
    // Start NTP client
    timeClient.begin();
    timeClient.update();
}
void NetworkConnector::setupMQTT()
{
    const int mqttPort = atoi(mqtt_port);
    mqttClient.setServer(mqtt_server, mqttPort);
    mqttClient.setCallback(mqttCallbackWrapper);
    mqttReconnect();
}
void NetworkConnector::printConfiguration()
{
    Serial.println("");
    Serial.println("-----");
    Serial.print("Machine ID: ");
    Serial.println(machineId);
    Serial.println("-----");
    Serial.print("MQTT Server: ");
    Serial.println(mqtt_server);
    Serial.print("MQTT Port: ");
    Serial.println(mqtt_port);
    Serial.print("MQTT Username: ");
    Serial.println(username);
    // Hide password
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
    Serial.println(configTempCelsius ? "Celsius" : "Fahrenheit");
    Serial.print("Timezone: UTC");
    Serial.print(timezone);
    Serial.print(" (");
    Serial.print(timezoneOffset / 3600.0);
    Serial.println(" hours)");
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
    #ifdef OTA_SERVER
    Serial.print("Hardcoded OTA server: ");
    Serial.println(OTA_SERVER);
    #endif
    #endif
    Serial.println("");
}
void NetworkConnector::updateTime()
{
    timeClient.update();
}
unsigned long NetworkConnector::getEpochTime()
{
    return timeClient.getEpochTime();
}
void NetworkConnector::waitForFactoryReset()
{
    Serial.println("Press button within 4 seconds for factory reset...");
    for (int iter = 0; iter < FACTORY_RESET_WAIT_ITERATIONS; iter++)
    {
        digitalWrite(pinAlarm, HIGH);
        delay(FACTORY_RESET_BLINK_DELAY);
        if (false == digitalRead(pinButton))
        {
            factoryReset();
            return;
        }
        digitalWrite(pinAlarm, LOW);
        delay(FACTORY_RESET_BLINK_DELAY);
        if (false == digitalRead(pinButton))
        {
            factoryReset();
            return;
        }
    }
}
void NetworkConnector::factoryReset()
{
    if (false == digitalRead(pinButton))
    {
        Serial.println("Hold the button to reset to factory defaults...");
        bool cancel = false;
        for (int iter=0; iter<FACTORY_RESET_HOLD_ITERATIONS; iter++)
        {
            digitalWrite(pinAlarm, HIGH);
            delay(FACTORY_RESET_HOLD_DELAY);
            if (true == digitalRead(pinButton))
            {
                cancel = true;
                break;
            }
            digitalWrite(pinAlarm, LOW);
            delay(FACTORY_RESET_HOLD_DELAY);
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
            WiFi.disconnect(true);
            Serial.println("Restarting...");
            esp_err_t result = nvs_flash_erase();
            if (ESP_OK == result)
            {
                Serial.println("NVS erased successfully.");
            }
            else
            {
                Serial.print("Failed to erase NVS. Error code: ");
                Serial.println(result);
            }
            ESP.restart();
        }
        else
        {
            Serial.println("Reset to factory defaults cancelled.");
            digitalWrite(pinAlarm, LOW);
        }
    }
}
void NetworkConnector::processMessageScale(const char* text)
{
    StaticJsonDocument<JSON_SCALE_SIZE> data;
    deserializeJson(data, text);
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
    saveConfig();
}
void NetworkConnector::mqttCallback(char* topic, byte* payload, unsigned int length)
{
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
        return;
    }
    #endif
}
void NetworkConnector::mqttCallbackWrapper(char* topic, byte* payload, unsigned int length)
{
    if (instance)
    {
        instance->mqttCallback(topic, payload, length);
    }
}
void NetworkConnector::calculateMachineId()
{
    MD5Builder md5;
    md5.begin();
    uint64_t chipId = ESP.getEfuseMac();
    char chipIdStr[13];
    snprintf(chipIdStr, sizeof(chipIdStr), "%04X%08X", (uint16_t)(chipId >> 32), (uint32_t)chipId);
    md5.add(chipIdStr);
    md5.calculate();
    md5.toString().toCharArray(machineId, 33);
}
void NetworkConnector::mqttReconnect()
{
    char clientId[51];
    snprintf(clientId, sizeof(clientId), "anavi-word-clock-%s", machineId);
    for (int attempt = 0; attempt < MQTT_RECONNECT_ATTEMPTS; ++attempt)
    {
        Serial.print("Attempting MQTT connection...");
        if (true == mqttClient.connect(clientId, username, password))
        {
            Serial.println("connected");
            // Subscribe to topics
            mqttClient.subscribe(cmnd_led1_power_topic);
            mqttClient.subscribe(cmnd_led1_color_topic);
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
            publishDiscoveryState();
            #endif
            publishState();
            break;
        }
        else
        {
            Serial.print("failed, rc=");
            Serial.print(mqttClient.state());
            Serial.println(" try again in 5 seconds");
            delay(MQTT_RECONNECT_DELAY);
        }
    }
}
void NetworkConnector::publishState()
{
    //TODO
}
void NetworkConnector::publishSensorData(const char* subTopic, const char* key, const float value)
{
    StaticJsonDocument<JSON_SMALL_SIZE> json;
    json[key] = value;
    char payload[JSON_SMALL_SIZE];
    serializeJson(json, payload);
    char topic[TOPIC_BUFFER_SIZE];
    sprintf(topic,"%s/%s/%s", workgroup, machineId, subTopic);
    mqttClient.publish(topic, payload, true);
}
void NetworkConnector::publishSensorData(const char* subTopic, const char* key, const String& value)
{
    StaticJsonDocument<JSON_SMALL_SIZE> json;
    json[key] = value;
    char payload[JSON_SMALL_SIZE];
    serializeJson(json, payload);
    char topic[TOPIC_BUFFER_SIZE];
    sprintf(topic,"%s/%s/%s", workgroup, machineId, subTopic);
    mqttClient.publish(topic, payload, true);
}
float NetworkConnector::convertCelsiusToFahrenheit(float temperature)
{
    return (temperature * 9/5 + 32);
}
float NetworkConnector::convertTemperature(float temperature)
{
    return (true == configTempCelsius) ? temperature : convertCelsiusToFahrenheit(temperature);
}
String NetworkConnector::formatTemperature(float temperature)
{
    String unit = (true == configTempCelsius) ? "°C" : "°F";
    return String(convertTemperature(temperature), 1) + unit;
}
void NetworkConnector::saveConfigCallback()
{
    Serial.println("Should save config");
    shouldSaveConfig = true;
}
void NetworkConnector::saveConfigCallbackWrapper()
{
    if (instance)
    {
        instance->saveConfigCallback();
    }
}
void NetworkConnector::apWiFiCallback(WiFiManager *myWiFiManager)
{
    String configPortalSSID = myWiFiManager->getConfigPortalSSID();
    Serial.print("Created access point for configuration: ");
    Serial.println(configPortalSSID);
}
void NetworkConnector::apWiFiCallbackWrapper(WiFiManager *myWiFiManager)
{
    if (instance)
    {
        instance->apWiFiCallback(myWiFiManager);
    }
}
void NetworkConnector::loadConfig()
{
    Serial.println("mounting FS...");
    if (SPIFFS.begin(true))
    {
        Serial.println("mounted file system");
        if (SPIFFS.exists("/config.json"))
        {
            Serial.println("reading config file");
            File configFile = SPIFFS.open("/config.json", "r");
            if (configFile)
            {
                Serial.println("opened config file");
                const size_t size = configFile.size();
                std::unique_ptr<char[]> buf(new char[size]);
                configFile.readBytes(buf.get(), size);
                DynamicJsonDocument json(JSON_CONFIG_SIZE);
                if (DeserializationError::Ok == deserializeJson(json, buf.get()))
                {
                    #ifdef DEBUG
                    serializeJson(json, Serial);
                    Serial.println("\nparsed json");
                    #endif
                    strcpy(mqtt_server, json["mqtt_server"]);
                    strcpy(mqtt_port, json["mqtt_port"]);
                    strcpy(workgroup, json["workgroup"]);
                    strcpy(username, json["username"]);
                    strcpy(password, json["password"]);
                    strcpy(temp_scale, json["temp_scale"]);
                    // Load timezone
                    const char *tz = json["timezone"];
                    if (tz) {
                        strncpy(timezone, tz, sizeof(timezone) - 1);
                        timezone[sizeof(timezone) - 1] = '\0';
                    }
                    #ifdef HOME_ASSISTANT_DISCOVERY
                    const char *s = json["ha_name"];
                    if (!s) s = machineId;
                    snprintf(ha_name, sizeof(ha_name), "%s", s);
                    #endif
                    #ifdef OTA_UPGRADES
                    const char *s2 = json["ota_server"];
                    if (!s2) s2 = "";
                    snprintf(ota_server, sizeof(ota_server), "%s", s2);
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
}
void NetworkConnector::saveConfig()
{
    Serial.println("saving config");
    DynamicJsonDocument json(JSON_CONFIG_SIZE);
    json["mqtt_server"] = mqtt_server;
    json["mqtt_port"] = mqtt_port;
    json["workgroup"] = workgroup;
    json["username"] = username;
    json["password"] = password;
    json["temp_scale"] = temp_scale;
    json["timezone"] = timezone;
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
}
#ifdef HOME_ASSISTANT_DISCOVERY
void NetworkConnector::publishDiscoveryState()
{
    // TODO: Implement Home Assistant discovery
}
#endif
#ifdef OTA_UPGRADES
void NetworkConnector::do_ota_upgrade(const char* text)
{
    // TODO: Implement OTA upgrade
}
#endif
