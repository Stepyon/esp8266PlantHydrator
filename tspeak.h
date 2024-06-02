// ThingSpeak information
#include <ThingSpeak.h>
char thingSpeakAddress[] = "api.thingspeak.com";
WiFiClient client;
bool inited = false;

void initTS() {
    ThingSpeak.begin(client);
}

int writeTSData(long TSChannel, const char* writeAPIKey, int moisturePercent, int sensorData){

  if(!inited) {
    inited = true;
    initTS();
  }

  ThingSpeak.setField(1, moisturePercent);
  ThingSpeak.setField(2, sensorData);
   
  int writeSuccess = ThingSpeak.writeFields(TSChannel, writeAPIKey);
  return writeSuccess;
}