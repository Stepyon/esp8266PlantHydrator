//#define DEBUG

#include <ArduinoOTA.h>
#include <WiFiManager.h> 
#include "index.h"
#include "telegram.h"
#include "prefs.h"
#include "tspeak.h"
#include <ESP8266mDNS.h>

WiFiManager wm;
ESP8266WebServer server(80);

const char* AP_NAME = "ESP_PlantHydrator";

const int LED_PIN = 2;
const int ESP_CONFIG_PIN = 4; // If connected to the ground - start config mode
const int SOIL_PIN = 5; // Soil moisture sensor pin
const int PUMP_PIN = 15; // Water pump pin

bool configMode = false;
bool connected = false;

short sensorMin = 1024;
short sensorMax = 0;

short lastValue = 0;
bool blink = false;
unsigned long previousMillis = 0;

bool telegramEnabled() {
  return prefs.tg_enabled && strlen(prefs.tg_bot_token) > 0 && strlen(prefs.tg_chat_id) > 0;
}

bool thingSpeakEnabled() {
  return prefs.ts_enabled && prefs.ts_channel_id != 0 && strlen(prefs.ts_write_key) > 0;
}

void setup() {

  #ifdef DEBUG
  Serial.begin(115200);
  #endif

  setupPrefs();
  sensorMin = prefs.sensorMin;
  sensorMax = prefs.sensorMax;

  pinMode(LED_PIN, OUTPUT);
  pinMode(SOIL_PIN, OUTPUT);
  digitalWrite(SOIL_PIN, HIGH);
  pinMode(PUMP_PIN, OUTPUT);
  digitalWrite(PUMP_PIN, LOW);
  pinMode(ESP_CONFIG_PIN, INPUT_PULLUP);

  if (digitalRead(ESP_CONFIG_PIN) == LOW) {
    delay(50);
    if (digitalRead(ESP_CONFIG_PIN) == LOW) {
      configMode = true;
    }
  }

  if (configMode) {
    enterConfigMode();
  } else {
    normalOperationMode();
  }
}

void enterConfigMode() {
  println("Entering config mode");

  WiFi.mode(WIFI_STA);
  wm.setConfigPortalBlocking(false);
  wm.setConfigPortalTimeout(60);

  if (wm.autoConnect(AP_NAME)) {
    connected = true;
    print("Connected, IP address: ");
    String localIP = WiFi.localIP().toString();
    println(localIP);
    setupOTA();
    setupWebServer();
    bool mdsStarted = MDNS.begin(AP_NAME);
    if (mdsStarted) {
      println("MDNS responder started");
    }
    if (telegramEnabled()) {
      initTGbot(prefs.tg_bot_token, prefs.tg_chat_id);
      String msg = String(AP_NAME) + " started in config mode, connect to http://" + localIP;
      if (mdsStarted) {
        msg += " or http://" + String(AP_NAME);
      }
      sendTGMessage(msg);
    }
  } else {
    println("Config portal running");
  }
}

void normalOperationMode() {
  if (telegramEnabled() || thingSpeakEnabled()) {
    WiFi.setOutputPower(20.5); // Values are in dBm
    WiFi.mode(WIFI_STA);
    wm.setConfigPortalBlocking(false);
    wm.setConfigPortalTimeout(30);
    wm.setEnableConfigPortal(false);
    if (wm.autoConnect(AP_NAME)) {
      connected = true;
      print("Connected, IP address: ");
      println(WiFi.localIP().toString());
      if (telegramEnabled()) {
        initTGbot(prefs.tg_bot_token, prefs.tg_chat_id);
      }
      if (thingSpeakEnabled()) {
        initTS();
      }
    }
  }
  delay(1000);
  startWatering();
  sleep(false);
}

void setupWebServer() {
  server.on("/", handleRoot);
  server.on("/style.css", handleCss);
  server.on("/script.js", handleJs);
  server.on("/adcread", sensor_data);
  server.on("/sleep", sleep_min);
  server.on("/pump_on", pump_on);
  server.on("/pump_off", pump_off);
  server.on("/save_settings", HTTP_POST, save_settings);
  server.on("/reset", reset_settings);
  server.on("/reset_wifi", reset_wifi);
  server.on("/test_tg_bot", test_tg_bot);
  server.on("/send_ts_data", send_ts_data);
  server.begin();
  println("HTTP server started");
}

void setupOTA() {
  ArduinoOTA.onStart([]() { println("Start"); });
  ArduinoOTA.onEnd([]() { println("\nEnd"); });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) println("Receive Failed");
    else if (error == OTA_END_ERROR) println("End Failed");
  });
  ArduinoOTA.setHostname(AP_NAME);
  ArduinoOTA.begin();
}

const int sensorNumReadings = 12; // Number of readings for moving average

short getSensorValue() {
  short sum = 0, max = 0, min = 1024;
  for (int i = 0; i < sensorNumReadings; i++) {
    short cur = analogRead(A0);
    sum += cur;
    if (cur > max) max = cur;
    if (cur < min) min = cur;
    delay(20);
  }
  sum -= (max + min);
  return sum / (sensorNumReadings - 2);
}

float getMoisture(short sensorValue) {
  if (sensorValue > sensorMax) sensorMax = sensorValue;
  if (sensorValue < sensorMin) sensorMin = sensorValue;

  sensorValue -= sensorMin;
  short range = sensorMax - sensorMin;
  if (range <= 0) range = 1;

  float percent = 100.0 - (sensorValue * 100.0 / range);
  return round(percent * 10) / 10.0;
}

void startWatering() {
  println("Starting watering");
  int sensorValue = getSensorValue();
  float sensorMoisture = getMoisture(sensorValue);
  bool watering = sensorMoisture <= prefs.wateringPercent;

  if (connected && thingSpeakEnabled()) {
    writeTSData(prefs.ts_channel_id, prefs.ts_write_key, sensorMoisture, sensorValue);
  }

  if (watering) {
    if (connected && telegramEnabled()) {
      sendTGMessage("Soil moisture " + String(sensorMoisture) + "%, watering: " + String(prefs.wateringTimeSec) + " sec");
    }
    pump_on();
    delay(1000 * prefs.wateringTimeSec);
    pump_off();
  }
}

void loop() {
  if (configMode || telegramEnabled() || thingSpeakEnabled()) {
    wm.process();
  }

  if (configMode) {
    handleConfigMode();
  }
}

short count = 0;
short sen_value = 0;

short updateAverage(short newReading, short currentAverage, int currentCount) {
  // Calculate the new average using a cumulative moving average formula
  int effectiveCount = min(currentCount + 1, sensorNumReadings); // Limit the denominator to sensorNumReadings
  return currentAverage + (newReading - currentAverage) / effectiveCount;
}

void updateSensorData() {
    sen_value = updateAverage(analogRead(A0), sen_value, count);
    if (count < sensorNumReadings) {
      count++;
    }
}

void handleConfigMode() {
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= 200) {
    previousMillis = currentMillis;
    if (digitalRead(ESP_CONFIG_PIN) == HIGH) {
      println("Config pin HIGH, restart...");
      ESP.restart();
    }
    updateSensorData();
    digitalWrite(LED_PIN, blink ? HIGH : LOW);
    blink = !blink;
  }

  if (connected) {
    ArduinoOTA.handle();
    server.handleClient();
  }
}

bool sensorForceUpdate = false;
String getSensorsInfoStr() {
    return "(sensor: " + String(sen_value) + ", min: " + String(sensorMin) + ", max: " + String(sensorMax) + ")";
}
void sensor_data() {
  if (sen_value != lastValue || sensorForceUpdate) {
    float currentMoisture = getMoisture(sen_value);
    StaticJsonDocument<200> jsonDoc;
    jsonDoc["adc_percent"] = currentMoisture;
    jsonDoc["adc_info"] = getSensorsInfoStr();
    String jsonResponse;
    serializeJson(jsonDoc, jsonResponse);
    server.send(200, "application/json", jsonResponse);
    lastValue = sen_value;
    sensorForceUpdate = false;
  } else {
    server.send(304);  // Not Modified
  }
}

void sleep(bool isShort) {
  digitalWrite(SOIL_PIN, LOW);
  uint64_t deepSleepTimeMicros = isShort ? 60 * 1000000 : 0.98 * ESP.deepSleepMax();
  if (configMode) {
    uint64_t deepSleepTimeSeconds = deepSleepTimeMicros / 1000000;
    uint64_t hours = deepSleepTimeSeconds / 3600;
    uint64_t minutes = (deepSleepTimeSeconds % 3600) / 60;
    uint64_t seconds = deepSleepTimeSeconds % 60;
    char buffer[30];
    sprintf(buffer, "%02llu:%02llu:%02llu", hours, minutes, seconds);
    server.send(200, "text/plain", "In deep sleep for " + String(buffer));
    delay(1000);
  }
  #ifdef DEBUG
  Serial.flush();
  #endif
  ESP.deepSleep(deepSleepTimeMicros);
}

void sleep_min() {
  sleep(true);
}

void pump_on() {
  println("Pump on");
  digitalWrite(PUMP_PIN, HIGH);
  if (configMode) {
    server.send(200, "text/plain", "Pump ON");
  }
}

void pump_off() {
  println("Pump off");
  digitalWrite(PUMP_PIN, LOW);
  if (configMode) {
    server.send(200, "text/plain", "Pump OFF");
  }
}

void test_tg_bot() {
  if (server.hasArg("tg_bot_token") && server.hasArg("tg_chat_id")) {
    String token = server.arg("tg_bot_token");
    String chatId = server.arg("tg_chat_id");
    if (token.isEmpty() || chatId.isEmpty()) {
      server.send(200, "text/plain", "Incorrect Telegram bot token / chat id");
    } else {
      initTGbot(token, chatId);
      bool msgSent = sendTGMessage("Soil moisture " + String(getMoisture(sen_value)) + "%, watering: " + String(prefs.wateringTimeSec) + " sec");
      server.send(200, "text/plain", msgSent ? "Test message sent" : "Error sending message");
    }
  } else {
    server.send(200, "text/plain", "Incorrect request parameters");
  }
}

void save_settings() {
  if (server.hasArg("wpc") && server.hasArg("smax") && server.hasArg("smin") && server.hasArg("wtime") 
    && server.hasArg("ts_state") && server.hasArg("ts_channel_id") && server.hasArg("ts_write_key") 
    && server.hasArg("tg_state") && server.hasArg("tg_chat_id") && server.hasArg("tg_bot_token")) {
    
    prefs.wateringPercent = server.arg("wpc").toInt();
    sensorMax = server.arg("smax").toInt();
    sensorMin = server.arg("smin").toInt();
    prefs.sensorMax = sensorMax;
    prefs.sensorMin = sensorMin;

    prefs.wateringTimeSec = server.arg("wtime").toInt();
    
    prefs.ts_enabled = server.arg("ts_state") == "true";
    // Convert string to unsigned long
    prefs.ts_channel_id = strtoul(server.arg("ts_channel_id").c_str(), NULL, 10);
    storeInputString(server.arg("ts_write_key"), prefs.ts_write_key, sizeof(prefs.ts_write_key));

    prefs.tg_enabled = server.arg("tg_state") == "true";
    storeInputString(server.arg("tg_chat_id"), prefs.tg_chat_id, sizeof(prefs.tg_chat_id));
    storeInputString(server.arg("tg_bot_token"), prefs.tg_bot_token, sizeof(prefs.tg_bot_token));

    savePrefs();
    println("Settings saved");
    sensorForceUpdate = true;
    server.send(200, "text/plain", "Settings saved");
  } else {
    server.send(400, "text/plain", "Incorrect settings");
  }
}

void reset_settings() {
  setDefaultPrefs();
  savePrefs();
  server.send(200, "text/plain", "The settings cleared");
}

void send_ts_data() {
  String msg;
  if (server.hasArg("ts_channel_id") && server.hasArg("ts_write_key")) {
    if (connected) {
      long channel_id = strtoul(server.arg("ts_channel_id").c_str(), NULL, 10);
      String wkey = server.arg("ts_write_key");
      float sensorMoisture = getMoisture(sen_value);
      int result = writeTSData(channel_id, wkey.c_str(), sensorMoisture, sen_value);
      msg = result == 200 ? "Sensor data has been sent successfully" : "Failed to send data, HTTP Error code: " + String(result);
    } else {
      msg = "Failed to send data to Thing Speak server, not connected to WIFI";
    }
  } else {
    msg = "Failed to send data to Thing Speak server, Channel ID or Write Key is empty";
  }
  server.send(200, "text/plain", msg);
}

void reset_wifi() {
  server.send(200, "text/plain", "The wifi settings reset, connect to " + String(AP_NAME) + " AP");
  delay(200);
  wm.resetSettings();
  #ifdef DEBUG
  Serial.flush();
  #endif
  ESP.restart();
}

void handleRoot() {
  String s = htmlContent;
  s.replace("{wtime_value}", String(prefs.wateringTimeSec));
  s.replace("{wpc_value}", String(prefs.wateringPercent));
  s.replace("{smax_value}", String(prefs.sensorMax));
  s.replace("{smin_value}", String(prefs.sensorMin));
  s.replace("{ts_enabled_value}", prefs.ts_enabled ? "checked" : "");
  s.replace("{ts_channel_id_value}", String(prefs.ts_channel_id));
  s.replace("{ts_write_key_value}", prefs.ts_write_key);
  s.replace("{tg_enabled_value}", prefs.tg_enabled ? "checked" : "");
  s.replace("{tg_chat_id_value}", String(prefs.tg_chat_id));
  s.replace("{tg_bot_token_value}", prefs.tg_bot_token);
  s.replace("{sen_value}", String(getMoisture(sen_value)));
  s.replace("{sen_info}", getSensorsInfoStr());
  server.send(200, "text/html", s);
}

void handleCss() {
  server.send_P(200, "text/css", cssContent);
}

void handleJs() {
  server.send_P(200, "application/javascript", jsContent);
}

void println(const String& message) {
  #ifdef DEBUG
  Serial.println(message);
  #endif
}

void print(const String& message) {
  #ifdef DEBUG
  Serial.print(message);
  #endif
}

void storeInputString(const String& input, char* buffer, size_t bufferSize) {
  size_t length = input.length();
  if (length >= bufferSize) {
    length = bufferSize - 1;
  }
  input.toCharArray(buffer, length + 1);
  buffer[length] = '\0';
}
