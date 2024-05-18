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
int currentUiStep = 0;

#define DATE_TEXT_SIZE 2
#define TITLE_TEXT_SIZE 4 // UI acts all good until up to 6 - it is all the display can fit
#define CHAR_WIDTH 6 // Define the width of a single character in pixels - Assuming a monospaced font
#define CHAR_HEIGHT 8 // Assuming a monospaced font
#define UI_STEP 30

#define MAX_EVENTS 10  // Define the maximum number of events you want to store


typedef struct {
    String startDate;    // Start date and time (YYYYMMDDTHHMMSS)
    String endDate;      // End date and time (YYYYMMDDTHHMMSS)
    String name;   // Summary or title of the event
    bool allDayEvent;
} calEvent;

calEvent events[MAX_EVENTS];
int numEvents = 0;

// Store int in rtc data, to remain persistent during deep sleep
RTC_DATA_ATTR int bootCount = 0;

void handleWakeup() 
{
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

void processPayload(const String payload) 
{
  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, payload);
  
  // TODO add proper error handling - print on the board if response has an error field
  if (error) {
    Serial.print("Parsing failed: ");
    Serial.println(error.c_str());
    return;
  }

  JsonArray payloadEvents = doc["events"].as<JsonArray>();
  for (JsonObject event : payloadEvents) {
    events[numEvents].name = event["name"].as<String>();
    events[numEvents].allDayEvent = event["allDayEvent"];
    events[numEvents].startDate = event["startDate"].as<String>();
    events[numEvents].endDate = event["endDate"].as<String>();
  }
}

// test function
void printEvents() 
{
  for (int i = 0; i < numEvents; i++) {
    Serial.print(F("Event "));
    Serial.println(i + 1);
    Serial.print(F("Name: "));
    Serial.println(events[i].name);
    Serial.print(F("Is All Day Event: "));
    Serial.println(events[i].allDayEvent ? "true" : "false");
    Serial.print(F("Start Date: "));
    Serial.println(events[i].startDate);
    Serial.print(F("End Date: "));
    Serial.println(events[i].endDate);
    Serial.println();
  }
}

int getNextUiStep()
{
  currentUiStep = currentUiStep + UI_STEP;
  return currentUiStep;
}

void printTextFromTop(String text, int textSize, int charWidth, int charHeight) 
{
  display.setTextSize(textSize);
  int textWidth = text.length() * charWidth * textSize;
  int textHeight = charHeight * textSize;
  int x = (display.width() - textWidth) / 2;
  display.setCursor((display.width() - textWidth) / 2, getNextUiStep());
  display.println(text);
}

void printCalendar() 
{
  display.begin();        // Init library (you should call this function ONLY ONCE)
  display.clearDisplay(); // Clear any data that may have been in (software) frame buffer.

  // helper lines for UI alignments - params: starting X, starting Y, length, color
  display.drawFastVLine(E_INK_WIDTH / 2, 0, E_INK_HEIGHT, BLACK);
  display.drawFastHLine(0, E_INK_HEIGHT / 2, E_INK_WIDTH, BLACK);

  // these are used during the UI calculations
  // int textWidth, textHeight, x, y;

  // rotate display so the wake button is always up so user can easily refresh
  display.setRotation(-1);

  String date = network.getDate("%04d-%02d-%02d");
  printTextFromTop(date, DATE_TEXT_SIZE, CHAR_WIDTH, CHAR_HEIGHT);

  printTextFromTop("All-day events", TITLE_TEXT_SIZE, CHAR_WIDTH, CHAR_HEIGHT);

  // TODO move the format to the config
  // String date = network.getDate("%04d-%02d-%02d");
  // display.setTextSize(DATE_TEXT_SIZE);
  // textWidth = date.length() * CHAR_WIDTH * DATE_TEXT_SIZE;
  // textHeight = CHAR_HEIGHT * DATE_TEXT_SIZE;
  // // x and y centered
  // x = (display.width() - textWidth) / 2;
  // y = (display.height() - textHeight) / 2;
  // display.setCursor(x, getNextUiStep());
  // display.println(date);


  // String allDayEventsString = "All-day events";
  // display.setTextSize(TITLE_TEXT_SIZE);
  // textWidth = allDayEventsString.length() * CHAR_WIDTH * TITLE_TEXT_SIZE;
  // textHeight = CHAR_HEIGHT * TITLE_TEXT_SIZE;
  // x = (display.width() - textWidth) / 2;
  // display.setCursor(x, getNextUiStep());
  // display.println(allDayEventsString);


  // String shortEventsString = "Short events";
  // textWidth = shortEventsString.length() * CHAR_WIDTH * TITLE_TEXT_SIZE;
  // x = (display.width() - textWidth) / 2;
  // display.setCursor(x, y + UI_STEP);
  // display.println(shortEventsString);


  display.display(); // Write hello message on the screen
  delay(500000);       // Wait a little bit
}

void setup() 
{
  Serial.begin(115200);
  Serial.println("Setup called");
  
  handleWakeup();

  network.begin(ssid, pass);
  network.setTimeInfo(timezoneOffset); // set it after every wakeup so the daylight saving won't cause any problem

  Serial.println("Getting data");
  while (!network.getData(calendarURL, payload)) {
    Serial.print('.');
    delay(1000);
  }

  processPayload(payload);
  printEvents();
  printCalendar();

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
