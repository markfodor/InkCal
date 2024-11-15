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

// TODO check for "error" field in the response and print that if there is any
Inkplate display(INKPLATE_1BIT); // Create object on Inkplate library and set library to work in monochorme mode
Network network;
WiFiClientSecure client;
String payload;
int currentUiStep = 0;

// UI variables
#define DATE_TEXT_SIZE 2
#define TIME_TEXT_SIZE 3
#define TITLE_TEXT_SIZE 4 // UI acts all good until up to 6 - it is all the display can fit
#define CHAR_WIDTH 6 // Define the width of a single character in pixels - Assuming a monospaced font
#define CHAR_HEIGHT 8 // Assuming a monospaced font
#define UI_STEP 20
#define UI_EVENT_STEP 40

#define MAX_ALL_DAY_EVENT_NUMBER 8
#define MAX_SHORT_EVENT_NUMBER 5

String currentTimestamp;
typedef struct {
    String startDate;    // Start date and time (YYYY-MM-DDTHH:MM)
    String endDate;      // End date and time 
    String name;   // Summary or title of the event
    bool allDayEvent;
} calEvent;

calEvent allDayEvents[MAX_ALL_DAY_EVENT_NUMBER];
calEvent shortEvents[MAX_SHORT_EVENT_NUMBER];
int shortEventNumber = 0;
int allDayEventNumber = 0;

void handleWakeup() 
{
  esp_sleep_wakeup_cause_t wakeup_reason;
  wakeup_reason = esp_sleep_get_wakeup_cause();

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

  currentTimestamp = doc["timestamp"].as<String>();
  JsonArray payloadallDayEvents = doc["events"].as<JsonArray>();
  for (JsonObject event : payloadallDayEvents) {
    bool allDayEvent = event["allDayEvent"];
  
    // TODO this does not work well
    if(allDayEvent && allDayEventNumber <= MAX_ALL_DAY_EVENT_NUMBER)
    {
      allDayEvents[allDayEventNumber].name = event["name"].as<String>();
      allDayEvents[allDayEventNumber].allDayEvent = event["allDayEvent"];
      allDayEvents[allDayEventNumber].startDate = event["startDate"].as<String>();
      allDayEvents[allDayEventNumber].endDate = event["endDate"].as<String>();
      allDayEventNumber++;
    }
    else if(shortEventNumber <= MAX_SHORT_EVENT_NUMBER)
    {
      shortEvents[shortEventNumber].name = event["name"].as<String>();
      shortEvents[shortEventNumber].allDayEvent = event["allDayEvent"];
      shortEvents[shortEventNumber].startDate = event["startDate"].as<String>();
      shortEvents[shortEventNumber].endDate = event["endDate"].as<String>();
      shortEventNumber++;
    }
  }
}

int getNextUiStep()
{
  currentUiStep = currentUiStep + UI_STEP;
  return currentUiStep;
}

void printTextFromTop(String text, double textSize, int charWidth, int charHeight) 
{
  display.setTextSize(textSize);
  int textWidth = text.length() * charWidth * textSize;
  int textHeight = charHeight * textSize;
  int x = (display.width() - textWidth) / 2;
  display.setCursor((display.width() - textWidth) / 2, getNextUiStep());
  display.println(text);
  getNextUiStep();
}

// If you ever need to change the UI here is the guide: https://inkplate.readthedocs.io/en/latest/arduino.html#system-functions
// It uses the "Adafruit GFX Graphics Library": https://learn.adafruit.com/adafruit-gfx-graphics-library/using-fonts
void printCalendar() 
{
  display.begin();        // Init library (you should call this function ONLY ONCE)
  display.clearDisplay(); // Clear any data that may have been in (software) frame buffer.

  // rotate display so the wake button is always up so user can easily refresh
  display.setRotation(-1);

  printTextFromTop(currentTimestamp, TITLE_TEXT_SIZE, CHAR_WIDTH, CHAR_HEIGHT);

  for (int i = 0; i < allDayEventNumber; i++) {
    printTextFromTop(allDayEvents[i].name, TIME_TEXT_SIZE, CHAR_WIDTH, CHAR_HEIGHT);
  }

  // draw one single line between all day and short events - fixed values so NOT scaled to other Inkplates
  int y = getNextUiStep();
  display.drawThickLine(50, y, E_INK_HEIGHT - 50, y, BLACK, 2);

  for (int i = 0; i < shortEventNumber; i++) {
    printTextFromTop(shortEvents[i].startDate + " - " + shortEvents[i].endDate, DATE_TEXT_SIZE, CHAR_WIDTH, CHAR_HEIGHT);
    printTextFromTop(shortEvents[i].name, TIME_TEXT_SIZE, CHAR_WIDTH, CHAR_HEIGHT);
  }


  display.display();
  delay(500000);
}

void setup() 
{
  Serial.begin(115200);
  Serial.println("Setup called");
  
  handleWakeup();

  network.begin(ssid, pass);
  // TODO 1 hour ahead -> needed to set the proper midnigh refresh
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
    // Never here, as deepsleep restarts esp32 and always begins with setup
}


// --- test functions for debugging
void drawHelperLines() 
{
  // helper lines for UI alignments - params: starting X, starting Y, length, color
  display.drawFastVLine(E_INK_WIDTH / 2, 0, E_INK_HEIGHT, BLACK);
  display.drawFastHLine(0, E_INK_HEIGHT / 2, E_INK_WIDTH, BLACK);
}

void printEvents() 
{
  Serial.println(currentTimestamp);
  Serial.println();

  Serial.println("--- All day events ---");
  for (int i = 0; i < allDayEventNumber; i++) {
    printEvent(allDayEvents[i]);
  }

  Serial.println("--- Short events ---");
  for (int i = 0; i < shortEventNumber; i++) {
    printEvent(shortEvents[i]);
  }
}

void printEvent(calEvent event)
{
  Serial.print(F("Name: "));
  Serial.println(event.name);
  Serial.print(F("Is All Day Event: "));
  Serial.println(event.allDayEvent ? "true" : "false");
  Serial.print(F("Start Date: "));
  Serial.println(event.startDate);
  Serial.print(F("End Date: "));
  Serial.println(event.endDate);
  Serial.println();
}
