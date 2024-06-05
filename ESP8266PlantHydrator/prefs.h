#include <EEPROM.h>

const unsigned int currentVersion = 0;

struct preferences {
  unsigned short sensorMin, sensorMax, wateringTimeSec;
  byte wateringPercent;
  bool tg_enabled;
  bool ts_enabled;
  char tg_chat_id[10];
  char tg_bot_token[50];
  char ts_write_key[30];
  unsigned long ts_channel_id;
  unsigned int version;
} prefs;

const unsigned int EEPROM_SIZE = sizeof(preferences);
const unsigned int EEPROM_ADDRESS = 0;

void savePrefs() {
  prefs.version = currentVersion;  // Update version to current
  EEPROM.put(EEPROM_ADDRESS, prefs);
  EEPROM.commit();       // Only needed for ESP8266/ESP32 to actually write to flash

  if (EEPROM.commit()) {
    #ifdef DEBUG
    Serial.println("EEPROM commit successful");
    #endif
  } else {
    #ifdef DEBUG
    Serial.println("EEPROM commit failed");
    #endif
  }
}

void setDefaultPrefs() {
  #ifdef DEBUG
  Serial.println("Loading default prefs");
  #endif
  // Set all your default values here
  prefs.sensorMin = 0;
  prefs.sensorMax = 1024;
  prefs.wateringTimeSec = 5;
  prefs.wateringPercent = 50;
  prefs.tg_enabled = false;
  prefs.ts_enabled = false;
  strncpy(prefs.tg_chat_id, "", sizeof(prefs.tg_chat_id) - 1);
  strncpy(prefs.tg_bot_token, "", sizeof(prefs.tg_bot_token) - 1);
  prefs.ts_channel_id = 12345678;
  strncpy(prefs.ts_write_key, "", sizeof(prefs.ts_write_key) - 1);
  prefs.version = currentVersion;  // Important to set version to current
}

void loadPrefs() {
  EEPROM.get(EEPROM_ADDRESS, prefs);  // Read the structure starting from address 0
  if (prefs.version != currentVersion) {
    // If version mismatch, initialize with default values
    setDefaultPrefs();
  }
  #ifdef DEBUG
  Serial.println("prefs.sensorMin = " + String(prefs.sensorMin));
  Serial.println("prefs.sensorMax = " + String(prefs.sensorMax));
  Serial.println("prefs.ts_enabled = " + String(prefs.ts_enabled));
  Serial.println("prefs.ts_channel_id = " + String(prefs.ts_channel_id));
  #endif
}

void setupPrefs() {
  EEPROM.begin(EEPROM_SIZE);
  loadPrefs();  // Optionally load preferences at startup
}