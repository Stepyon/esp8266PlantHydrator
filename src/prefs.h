#include <EEPROM.h>

const unsigned int currentVersion = 5;

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

void savePrefs() {
  prefs.version = currentVersion;  // Update version to current
  EEPROM.put(0, prefs);  // Write the structure to EEPROM starting from address 0
  EEPROM.commit();       // Only needed for ESP8266/ESP32 to actually write to flash
}

void setDefaultPrefs() {
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
  EEPROM.get(0, prefs);  // Read the structure starting from address 0
  if (prefs.version != currentVersion) {
    // If version mismatch, initialize with default values
    setDefaultPrefs();
  }
}

void setupPrefs() {
  EEPROM.begin(sizeof(prefs));  // Initialize EEPROM with the size of `prefs`
  loadPrefs();  // Optionally load preferences at startup
}