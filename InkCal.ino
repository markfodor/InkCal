// Next 3 lines are a precaution, you can ignore those, and the example would also work without them
#if !defined(ARDUINO_INKPLATE10) && !defined(ARDUINO_INKPLATE10V2)
#error "Wrong board selection for this example, please select e-radionica Inkplate10 or Soldered Inkplate10 in the boards menu."
#endif

#include "Inkplate.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <string.h>
#include "Network.h"
#include "NetworkConfig.h"

Inkplate display(INKPLATE_3BIT);
Network network;
WiFiClientSecure client;
char *data;

#define MAX_EVENTS 10  // Define the maximum number of events you want to store
int numEvents = 0;

typedef struct {
    char uid[50];        // Unique ID of the event
    char dtstart[20];    // Start date and time (YYYYMMDDTHHMMSS)
    char dtend[20];      // End date and time (YYYYMMDDTHHMMSS)
    char summary[100];   // Summary or title of the event
    char description[250];// Description of the event
    char location[100];  // Location of the event
} iCalEvent;

iCalEvent events[MAX_EVENTS];

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

void setup() {
  Serial.begin(115200);

  handleWakeup();
  data = (char *)ps_malloc(2000000LL);

  network.begin(ssid, pass);

  Serial.println("Getting data");
  while (!network.getData(calendarURL, data)) {
    Serial.print('.');
    delay(1000);
  }
  Serial.print("\n");

  Serial.print(data);
  
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
