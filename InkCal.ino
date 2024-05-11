// Next 3 lines are a precaution, you can ignore those, and the example would also work without them
#ifndef ARDUINO_INKPLATE5
#error "Wrong board selection for this example, please select Soldered Inkplate5 in the boards menu."
#endif

#include "Inkplate.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <string.h>
#include "Network.h"
#include "NetworkConfig.h"

Inkplate display(INKPLATE_1BIT); // Create object on Inkplate library and set library to work in monochorme mode
Network network;
WiFiClientSecure client;
String payload;

#define MAX_EVENTS 10  // Define the maximum number of events you want to store
int numEvents = 0; // Initialize the number of events

typedef struct {
    String startTime;    // Start date and time (YYYYMMDDTHHMMSS)
    String endTime;      // End date and time (YYYYMMDDTHHMMSS)
    String title;   // Summary or title of the event
    bool isAllDayEvent;
} calEvent;

calEvent events[MAX_EVENTS];
int eventNumber = -1;

// Store int in rtc data, to remain persistent during deep sleep
RTC_DATA_ATTR int bootCount = 0;

void handleWakeup() {
  ++bootCount;

  esp_sleep_wakeup_cause_t wakeup_reason;
  wakeup_reason = esp_sleep_get_wakeup_cause();

  Serial.print(F("Boot count: "));
  Serial.println(bootCount, DEC);

  switch(wakeup_reason)
  {
    case ESP_SLEEP_WAKEUP_EXT0:
      Serial.println("Wakeup caused by external signal using RTC_IO"); 

      break;
    case ESP_SLEEP_WAKEUP_TIMER : 
      Serial.println("Wakeup caused by timer");

      break;
    default:
      Serial.printf("Unhandled wakeup. Reason: %d\n", wakeup_reason); 
      break;
  }
}

void parseEventData(const String& eventData) {
    if (numEvents < MAX_EVENTS) {
        // Split the eventData string into tokens using semicolon (;) as delimiter
        int startPos = 0;
        int endPos = eventData.indexOf(';');
        events[numEvents].title = eventData.substring(startPos, endPos);
        startPos = endPos + 1;

        endPos = eventData.indexOf(';', startPos);
        events[numEvents].isAllDayEvent = (eventData.substring(startPos, endPos) == "true");
        startPos = endPos + 1;

        endPos = eventData.indexOf(';', startPos);
        events[numEvents].startTime = eventData.substring(startPos, endPos);
        startPos = endPos + 1;

        endPos = eventData.indexOf(';', startPos);
        events[numEvents].endTime = eventData.substring(startPos, endPos);

        // Increment the number of events
        numEvents++;
    } else {
        Serial.println(F("Maximum number of events reached!"));
    }
}

void processPayload(const String& allEventData) {
  int startPos = 0;
  int endPos = allEventData.indexOf('\n');
  while (endPos != -1) {
      String eventData = allEventData.substring(startPos, endPos);
      parseEventData(eventData);
      startPos = endPos + 1;
      endPos = allEventData.indexOf('\n', startPos);
  }
}

void printEvents() {
  for (int i = 0; i < numEvents; i++) {
    Serial.print(F("Event "));
    Serial.println(i + 1);
    Serial.print(F("Title: "));
    Serial.println(events[i].title);
    Serial.print(F("Is All Day Event: "));
    Serial.println(events[i].isAllDayEvent ? "true" : "false");
    Serial.print(F("Start Time: "));
    Serial.println(events[i].startTime);
    Serial.print(F("End Time: "));
    Serial.println(events[i].endTime);
    Serial.println();
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("Setup called");

  handleWakeup();

  network.begin(ssid, pass);
  network.setTime(timezoneOffset);

  Serial.println("Getting data");
  while (!network.getData(calendarURL, payload)) {
    Serial.print('.');
    delay(1000);
  }
  Serial.print("\n");
  Serial.println("payload:");
  //Serial.print(payload);

  processPayload(payload);
  printEvents();

  Serial.println("Going to sleep");
  delay(100);
  esp_sleep_enable_timer_wakeup(60ll * 2 * 1000 * 1000); //wakeup in 60min time - 60min * 60s * 1000ms * 1000us
  // Enable wakeup from deep sleep on gpio 36 (wake button)
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_36, LOW);
  esp_deep_sleep_start();
}

void loop() {
    // Never here, as deepsleep restarts esp32
}
