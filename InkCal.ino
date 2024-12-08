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

#define uS_TO_S_FACTOR 1000000 // Conversion factor for micro seconds to seconds

// UI variables
#define DATE_TEXT_SIZE 2
#define EVENT_TEXT_SIZE 3
#define TITLE_TEXT_SIZE 4 // UI acts all good until up to 6 - it is all the display can fit
#define CHAR_WIDTH 6 // Define the width of a single character in pixels - Assuming a monospaced font
#define CHAR_HEIGHT 8 // Assuming a monospaced font
#define UI_STEP 20
#define UI_EVENT_STEP 40

#define MAX_ALL_DAY_EVENT_NUMBER 8
#define MAX_SHORT_EVENT_NUMBER 5

String currentTimestamp;
int sleepInMins = 3600; // if error -> sleep for 6 hours
typedef struct {
    String startDate;    // (HH:MM)
    String endDate;      // TODO rename these to ...Time
    String name;   // Summary or title of the event
    bool allDayEvent;
} calEvent;

calEvent allDayEvents[MAX_ALL_DAY_EVENT_NUMBER];
calEvent shortEvents[MAX_SHORT_EVENT_NUMBER];
int shortEventNumber = 0;
int allDayEventNumber = 0;
String errorResponse = ""; // used for server errors and deserialization problems

void handleWakeup() {
  esp_sleep_wakeup_cause_t wakeup_reason;
  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch(wakeup_reason) {
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

bool processPayload(const String payload) {
  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, payload);
  
  if (error) {
    Serial.print("Parsing failed: ");
    errorResponse = error.c_str();
    Serial.println(errorResponse);
    return false;
  }

  if (doc.containsKey("error")) {
    errorResponse = doc["error"].as<String>();
    return false;
  }

  currentTimestamp = doc["date"].as<String>() + " - " + doc["time"].as<String>();
  sleepInMins = doc["sleep"];
  JsonArray payloadallDayEvents = doc["events"].as<JsonArray>();
  for (JsonObject event : payloadallDayEvents) {
    bool allDayEvent = event["allDayEvent"];

    if(allDayEvent) {
      allDayEvents[allDayEventNumber].name = event["name"].as<String>();
      allDayEvents[allDayEventNumber].allDayEvent = event["allDayEvent"];
      allDayEvents[allDayEventNumber].startDate = event["startDate"].as<String>();
      allDayEvents[allDayEventNumber].endDate = event["endDate"].as<String>();
      allDayEventNumber++;
    }
    else {
      shortEvents[shortEventNumber].name = event["name"].as<String>();
      shortEvents[shortEventNumber].allDayEvent = event["allDayEvent"];
      shortEvents[shortEventNumber].startDate = event["startDate"].as<String>();
      shortEvents[shortEventNumber].endDate = event["endDate"].as<String>();
      shortEventNumber++;
    }
  }

  return true;
}

int getNextUiStep() {
  currentUiStep = currentUiStep + UI_STEP;
  return currentUiStep;
}


String* splitStringToArray(String input, int maxLen, int &outputSize) {
  String* output = new String[3]; // can split into max 3 lines
  outputSize = 0;
  input.trim();   // Remove leading and trailing spaces
  int startIdx = 0; // Start of the current word
  int spaceIdx;     // Index of the next space
  String currentGroup = "";

  while (startIdx < input.length()) {
    spaceIdx = input.indexOf(' ', startIdx); // Find the next space
    if (spaceIdx == -1) spaceIdx = input.length(); // If no space, go to the end

    String word = input.substring(startIdx, spaceIdx);

    if (currentGroup.length() + word.length() + (currentGroup.isEmpty() ? 0 : 1) <= maxLen) {
      // Add the word to the current group if it fits (add 1 for the space)
      if (!currentGroup.isEmpty()) currentGroup += " ";
      currentGroup += word;
    } else {
      output[outputSize++] = currentGroup;
      currentGroup = word; // Start a new group with the current word
    }

    startIdx = spaceIdx + 1;
  }

  if (!currentGroup.isEmpty()) {
    output[outputSize++] = currentGroup;
  }

  return output;
}

void printTextFromTop(String text, double textSize, int charWidth, int charHeight) {
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
void printCalendar() {
  display.begin();        // Init library (you should call this function ONLY ONCE)
  display.clearDisplay(); // Clear any data that may have been in (software) frame buffer.

  // rotate display so the wake button is always up so user can easily refresh
  display.setRotation(-1);

  printTextFromTop(currentTimestamp, TITLE_TEXT_SIZE, CHAR_WIDTH, CHAR_HEIGHT);

  for (int i = 0; i < allDayEventNumber; i++) {
    // split too long all-day events into multiple rows
    const int maxLen = 30;       // Maximum length of each group --> TODO make it dynamic if possible
    int outputSize = 0;          // Number of strings in the output array
    String* result = splitStringToArray(allDayEvents[i].name, maxLen, outputSize);

    if (result != nullptr) {
        for (int i = 0; i < outputSize; i++) {
          printTextFromTop(result[i], EVENT_TEXT_SIZE, CHAR_WIDTH, CHAR_HEIGHT);
        }

        delete[] result;
    }
  }

  // draw one single line between all day and short events
  int y = getNextUiStep();
  display.drawThickLine(50, y, E_INK_HEIGHT - 50, y, BLACK, 2);

  for (int i = 0; i < shortEventNumber; i++) {
    printTextFromTop(shortEvents[i].startDate + " - " + shortEvents[i].endDate, DATE_TEXT_SIZE, CHAR_WIDTH, CHAR_HEIGHT);
    printTextFromTop(shortEvents[i].name, EVENT_TEXT_SIZE, CHAR_WIDTH, CHAR_HEIGHT);
  }


  display.display();
  delay(100);
}

void printError() {
  display.begin();
  display.clearDisplay();
  display.setRotation(-1);
  display.setTextSize(EVENT_TEXT_SIZE);
  display.println(errorResponse);
  display.display();
  delay(100);
}

void setup() {
  Serial.begin(115200);
  Serial.println("Setup called");
  
  handleWakeup();

  network.begin(ssid, pass);
  Serial.println("Getting data");
  while (!network.getData(calendarURL, payload)) {
    Serial.print('.');
    delay(1000);
  }

  bool success = processPayload(payload);
  Serial.println("success");
  Serial.println(success);

  if (success) {
    logEvents(); // for testing
    printCalendar();
  }
  else {
    printError();
  }

  Serial.print("Going to sleep for ");
  Serial.print(sleepInMins);
  Serial.print(" mins");
  delay(100);

  esp_sleep_enable_timer_wakeup(sleepInMins * 60ll * uS_TO_S_FACTOR); // needs to be multiplied with a long number!
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_36, 0);   // Enable wakeup from deep sleep on gpio 36 (wake button)
  esp_deep_sleep_start();
}

void loop() {
    // Never here, as deepsleep restarts esp32 and always begins with setup
}


// ----- test functions for debugging -----
void drawHelperLines() {
  // helper lines for UI alignments - params: starting X, starting Y, length, color
  display.drawFastVLine(E_INK_WIDTH / 2, 0, E_INK_HEIGHT, BLACK);
  display.drawFastHLine(0, E_INK_HEIGHT / 2, E_INK_WIDTH, BLACK);
}

void logEvents() {
  Serial.println(currentTimestamp);
  Serial.println(sleepInMins);
  Serial.println();

  Serial.println("--- All day events ---");
  for (int i = 0; i < allDayEventNumber; i++) {
    logEvent(allDayEvents[i]);
  }

  Serial.println("--- Short events ---");
  for (int i = 0; i < shortEventNumber; i++) {
    logEvent(shortEvents[i]);
  }
}

void logEvent(calEvent event) {
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
