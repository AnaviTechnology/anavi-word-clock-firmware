/*
  ANAVI Word Clock Configuration Header
  Contains all constants and definitions
*/

#ifndef CONFIG_H
#define CONFIG_H

// ============================================================================
// HARDWARE PIN DEFINITIONS
// ============================================================================
#define NEOPIN 10  // connect to DIN on NeoMatrix 8x8

// Configure pins
const int pinAlarm = D3;
const int pinButton = D8;
const int pinExtra = D2;

// ============================================================================
// NTP CONFIGURATION
// ============================================================================
const char NTP_SERVER[] = "pool.ntp.org";
const long NTP_OFFSET = 2 * 3600;  // UTC+2 for Bulgaria

// ============================================================================
// DST RULES
// ============================================================================
// Define US or EU rules for DST comment out as required
// More countries could be added with different rules in DST_RTC.cpp
const char rulesDST[] = "US"; // US DST rules
// const char rulesDST[] = "EU";   // EU DST rules

// ============================================================================
// DEFAULT MQTT CONFIGURATION
// ============================================================================
#define DEFAULT_MQTT_SERVER "mqtt.eclipseprojects.io"
#define DEFAULT_MQTT_PORT "1883"
#define DEFAULT_WORKGROUP "workgroup"

// ============================================================================
// DEFAULT LED CONFIGURATION
// ============================================================================
#define DEFAULT_LED_COUNT "10"
#define DEFAULT_LED_TYPE "WS2812B"
#define DEFAULT_LED_COLOR_ORDER "GRB"

// ============================================================================
// TEMPERATURE CONFIGURATION
// ============================================================================
#define DEFAULT_TEMP_SCALE "celsius"

// ============================================================================
// WIFI CONFIGURATION
// ============================================================================
#define WIFI_CONFIG_TIMEOUT 300  // seconds
#define WIFI_AP_NAME_PREFIX "ANAVI Word Clock "

// ============================================================================
// MQTT RECONNECTION SETTINGS
// ============================================================================
#define MQTT_RECONNECT_ATTEMPTS 3
#define MQTT_RECONNECT_DELAY 5000  // milliseconds

// ============================================================================
// FACTORY RESET TIMING
// ============================================================================
#define FACTORY_RESET_WAIT_ITERATIONS 40
#define FACTORY_RESET_HOLD_ITERATIONS 30
#define FACTORY_RESET_BLINK_DELAY 50
#define FACTORY_RESET_HOLD_DELAY 100

// ============================================================================
// JSON DOCUMENT SIZES
// ============================================================================
#define JSON_CONFIG_SIZE 1024
#define JSON_SMALL_SIZE 100
#define JSON_SCALE_SIZE 200

// ============================================================================
// TOPIC BUFFER SIZES
// ============================================================================
#define TOPIC_BUFFER_SIZE 200
#define TOPIC_SMALL_SIZE 50

#endif // CONFIG_H
