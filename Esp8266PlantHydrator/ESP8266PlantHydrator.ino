#include <ArduinoOTA.h>
#include <WiFiManager.h> 
#include "index.h"
#include "telegram.h"
#include "prefs.h"
#include "tspeak.h"
#include <ESP8266mDNS.h>

//#define DEBUG

WiFiManager wm;
ESP8266WebServer server(80);

const char* AP_NAME = "ESP_PlantHydrator";

const int LED = 2;
const int ESP_CONFIG_PIN = 4; //If conencted to the ground - start config mode

const int SOIL_PIN = 5; //Soil moisture sensor pin
const int PUMP_PIN = 15; //Water pump pin

bool configMode = false;
bool connected = false;

short sensorMin = 1024;
short sensorMax = 0;

bool telegramEnabled() {
  return prefs.tg_enabled && sizeof(prefs.tg_bot_token) > 0 && sizeof(prefs.tg_chat_id);
}

void setup() {

  #ifdef DEBUG
  Serial.begin(115200);
  #endif

  setupPrefs();

  sensorMin = prefs.sensorMin;
  sensorMax = prefs.sensorMax;

  pinMode(LED, OUTPUT);
  
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
    println("Entering config mode");

    WiFi.mode(WIFI_STA);  // explicitly set mode, esp defaults to STA+AP
    //reset settings - wipe credentials for testing
    //wm.resetSettings();

    wm.setConfigPortalBlocking(false);
    wm.setConfigPortalTimeout(60);
    //automatically connect using saved credentials if they exist
    //If connection fails it starts an access point with the specified name
    if (wm.autoConnect(AP_NAME)) {
      connected = true;
      print("connected, IP address: ");
      String localIP = WiFi.localIP().toString();
      println(localIP);
      setupOTA();
      setupWebServer();
      bool mdsStarted = MDNS.begin(AP_NAME);
      if (mdsStarted) {
        println("MDNS responder started");
      }
      if(telegramEnabled()) {
          initTGbot(prefs.tg_bot_token, prefs.tg_chat_id);
          String msg = String(AP_NAME) + "started in config mode, connect to http://" + localIP;
          if(mdsStarted) {
            msg += " or http://" + String(AP_NAME);
          }
          sendTGMessage(msg);
      }
    } else {
      println("Configportal running");
    }
  } else {

    if(telegramEnabled() || prefs.ts_enabled) {
      // Set the WiFi transmit power to the maximum
      WiFi.setOutputPower(20.5); // Values are in dBm
      WiFi.mode(WIFI_STA);
      wm.setConfigPortalBlocking(false);
      wm.setConfigPortalTimeout(30);
      wm.setEnableConfigPortal(false);
      if (wm.autoConnect(AP_NAME)) {
        connected = true;
        print("connected, IP address: ");
        println(WiFi.localIP().toString());
        if(telegramEnabled()){
            initTGbot(prefs.tg_bot_token, prefs.tg_chat_id);
        }
        if(prefs.ts_enabled) {
          initTS();
        }
      }
    }
    delay(1000);
    performWatering();
    sleep(false);
  }
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
  ArduinoOTA.onStart([]() {
    println("Start");
  });
  ArduinoOTA.onEnd([]() {
    println("\nEnd");
  });
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

bool blink = false;
unsigned long previousMillis = 0;  // will store last time the loop was updated
const long interval = 500;         // interval at which to run (milliseconds

const int numReadings = 10;
short getSensorValue() {
    short sum = 0;
    short max = 0;
    short min = 1024;
    for (int i = 0; i < numReadings; i++) {
      short cur = analogRead(A0); // Sum up readings
      sum += cur;
      max = (cur > max) ? cur : max;
      min = (cur < min) ? cur : min;
      delay(20);
    }
    sum -= (max + min);
    short average = sum / (numReadings - 2); // Calculate the average
    return average;
}

short getMoisture(short sensorValue) {

    sensorMax = (sensorValue > sensorMax) ? sensorValue : sensorMax;
    sensorMin = (sensorValue < sensorMin) ? sensorValue : sensorMin;

    sensorValue -= sensorMin;
    short range = sensorMax - sensorMin;
    if(range <= 0) {
      range = 1;
    }
    short percent = (sensorValue * 100) / range;
    return 100 - percent;
}

void performWatering () {
    println("perform watering");
    int sensorValue = getSensorValue();
    int sensorMoisture = getMoisture(sensorValue);
    bool watering = sensorMoisture <= prefs.wateringPercent;

    if(connected && prefs.ts_enabled) {
        writeTSData(prefs.ts_channel_id, prefs.ts_write_key, sensorMoisture, sensorValue);
    }

    if(watering) {
        if(connected && telegramEnabled()) {
            sendTGMessage("Soil moisture " + String(sensorMoisture) + "%, watering: " + String(prefs.wateringTimeSec) + " sec");
        }
        pump_on();
        delay(1000 * prefs.wateringTimeSec);
        pump_off();
    }
}

void loop() {

  if(configMode || telegramEnabled() || prefs.ts_enabled) {
    wm.process();
  }

  if (configMode) {
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= interval) {
      previousMillis = currentMillis;
      
      if (digitalRead(ESP_CONFIG_PIN) == HIGH) {
        println("Config pin HIGH, restart...");
        ESP.restart();
      }

      if (blink) {
        digitalWrite(LED, HIGH);
      } else {
        digitalWrite(LED, LOW);
      }
      blink = !blink;
    }

    if (connected) {
      ArduinoOTA.handle();
      server.handleClient();
    }
  }
}

short lastValue = 0;
void sensor_data() {

  int currentValue = getSensorValue();

  if (currentValue != lastValue) {
    int currentMoisture = getMoisture(currentValue);

    StaticJsonDocument<200> jsonDoc;

    jsonDoc["adc_percent"] = currentMoisture;
    jsonDoc["adc_info"] = "(sensor: " + String(currentValue) + ", min: " + String(sensorMin) + ", max: " + String(sensorMax) + ")";

    // Serialize the JSON document to a string
    String jsonResponse;
    serializeJson(jsonDoc, jsonResponse);

    // Send the JSON response
    server.send(200, "application/json", jsonResponse);

    lastValue = currentValue;
  } else {
    server.send(304);  // Not Modified
  }
}

void sleep(bool isShort) {
  
    digitalWrite(SOIL_PIN, LOW);

    uint64_t deepSleepTimeMicros = isShort ? 60*1000000 : 0.98 * ESP.deepSleepMax();

    if (configMode) {
        uint64_t deepSleepTimeSeconds = deepSleepTimeMicros / 1000000;  // convert microseconds to seconds
        uint64_t hours = deepSleepTimeSeconds / 3600;
        uint64_t minutes = (deepSleepTimeSeconds % 3600) / 60;
        uint64_t seconds = deepSleepTimeSeconds % 60;
        char buffer[30]; // Buffer to hold the formatted time string
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

void test_tg_bot() {
    if (server.hasArg("tg_bot_token") && server.hasArg("tg_chat_id")){
        String token = server.arg("tg_bot_token");
        String chatId = server.arg("tg_chat_id");
        if(token.isEmpty() || chatId.isEmpty()) {
            server.send(200, "text/plain", "Incorrect Telegram bot token / chat id");
        } else {
            initTGbot(token, chatId);
            bool msgSent = sendTGMessage("Soil moisture " + String(getMoisture(analogRead(A0))) + "%, watering: " + String(prefs.wateringTimeSec) + " sec");
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
      prefs.sensorMax = server.arg("smax").toInt();
      prefs.sensorMin = server.arg("smin").toInt();
      prefs.wateringTimeSec = server.arg("wtime").toInt();
      
      prefs.ts_enabled = server.arg("ts_state") == "true";
      // Convert string to unsigned long
      prefs.ts_channel_id = strtoul(server.arg("ts_channel_id").c_str(), NULL, 10);
      storeInputString(server.arg("ts_write_key"), prefs.ts_write_key, sizeof(prefs.ts_write_key));

      prefs.tg_enabled = server.arg("tg_state") == "true";
      storeInputString(server.arg("tg_chat_id"), prefs.tg_chat_id, sizeof(prefs.tg_chat_id));
      storeInputString(server.arg("tg_bot_token"), prefs.tg_bot_token, sizeof(prefs.tg_bot_token));
      
      savePrefs();
      println("Saving settings...");
      println("TS Enabled = " + String(prefs.ts_enabled));
      println("TS Channel ID = " + prefs.ts_channel_id);

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
    if (server.hasArg("ts_channel_id") && server.hasArg("ts_write_key")){
      if(connected) {
          long channel_id = strtoul(server.arg("ts_channel_id").c_str(), NULL, 10);
          String wkey = server.arg("ts_write_key");
          int sensorValue = getSensorValue();
          int sensorMoisture = getMoisture(sensorValue);
          int result = writeTSData(channel_id, wkey.c_str(), sensorMoisture, sensorValue);
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

void pump_off() {
    println("Pump off");
    digitalWrite(PUMP_PIN, LOW);
    if (configMode) {
        server.send(200, "text/plain", "Pump OFF");
    }
}

void handleRoot() {
  //String s = webpage;
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
    // Ensure the input string does not exceed the buffer size minus 1 (for null terminator)
    size_t length = input.length();
    if (length >= bufferSize) {
        length = bufferSize - 1;  // Limit the length to the buffer size minus 1
    }
    // Copy the string into the char array and null-terminate it
    input.toCharArray(buffer, length + 1);
    buffer[length] = '\0';  // Ensure null termination
}