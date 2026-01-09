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
// BRIGHTNESS SETTINGS
// ============================================================================
#define DAYBRIGHTNESS 40
#define NIGHTBRIGHTNESS 20

// Cutoff times for day / night brightness
#define MORNINGCUTOFF 7   // when does daybrightness begin? 7am
#define NIGHTCUTOFF   22  // when does nightbrightness begin? 10pm

// ============================================================================
// TIMING DELAYS
// ============================================================================
#define FLASHDELAY 100   // delay for startup "flashWords" sequence
#define SHIFTDELAY 100   // controls color shifting speed

// ============================================================================
// WORD MASK DEFINITIONS (64-bit masks for 8x8 matrix)
// ============================================================================
// These are in hexadecimal, 00 for an empty row
#define MFIVE    mask |= 0xF0000000000
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
// BUFFER SIZES
// ============================================================================
#define MQTT_SERVER_SIZE 40
#define MQTT_PORT_SIZE 6
#define WORKGROUP_SIZE 32
#define USERNAME_SIZE 20
#define PASSWORD_SIZE 20
#define LED_CONFIG_SIZE 20
#define MACHINE_ID_SIZE 32
#define TEMP_SCALE_SIZE 40

#ifdef HOME_ASSISTANT_DISCOVERY
  #define HA_NAME_SIZE 32
#endif

#ifdef OTA_UPGRADES
  #define OTA_SERVER_SIZE 40
#endif

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
#define WIFI_AP_NAME_PREFIX "ANAVI Miracle Emitter "

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
