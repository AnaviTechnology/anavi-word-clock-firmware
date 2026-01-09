/*
  ANAVI Word Clock - Network Functions Implementation
  Functions for WiFi, MQTT, Home Assistant and Configurations
*/

#include "network.h"
#include "config.h"
#include <WiFi.h>
#include <ArduinoJson.h>
#include <MD5Builder.h>
#include <nvs_flash.h>
#include <SPIFFS.h>
#include <Arduino.h>

void waitForFactoryReset()
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

void factoryReset()
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
            // Disconnect from any previously connected WiFi
            // true to erase the saved credentials
            WiFi.disconnect(true);

            Serial.println("Restarting...");
            // Erase the NVS (Non-Volatile Storage)
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
    StaticJsonDocument<JSON_SCALE_SIZE> data;
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
    md5.toString().toCharArray(machineId, MACHINE_ID_SIZE + 1);
}

void mqttReconnect()
{
  char clientId[18 + MACHINE_ID_SIZE + 1];
  snprintf(clientId, sizeof(clientId), "anavi-miracle-emitter-%s", machineId);

  // Loop until we're reconnected
  for (int attempt = 0; attempt < MQTT_RECONNECT_ATTEMPTS; ++attempt)
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
          delay(MQTT_RECONNECT_DELAY);
      }
  }
}

void publishState()
{
  //TODO
}

void publishSensorData(const char* subTopic, const char* key, const float value)
{
    StaticJsonDocument<JSON_SMALL_SIZE> json;
    json[key] = value;
    char payload[JSON_SMALL_SIZE];
    serializeJson(json, payload);
    char topic[TOPIC_BUFFER_SIZE];
    sprintf(topic,"%s/%s/%s", workgroup, machineId, subTopic);
    mqttClient.publish(topic, payload, true);
}

void publishSensorData(const char* subTopic, const char* key, const String& value)
{
    StaticJsonDocument<JSON_SMALL_SIZE> json;
    json[key] = value;
    char payload[JSON_SMALL_SIZE];
    serializeJson(json, payload);
    char topic[TOPIC_BUFFER_SIZE];
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
    DynamicJsonDocument json(JSON_CONFIG_SIZE);
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
