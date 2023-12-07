// Next 3 lines are a precaution, you can ignore those, and the example would also work without them
#if !defined(ARDUINO_INKPLATE10) && !defined(ARDUINO_INKPLATE10V2)
#error "Wrong board selection for this example, please select e-radionica Inkplate10 or Soldered Inkplate10 in the boards menu."
#endif

#include "Inkplate.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include "Network.h"
#include "NetworkConfig.h"

Inkplate display(INKPLATE_3BIT);
Network network;
WiFiClientSecure client;
char *data;

void setup()
{
  Serial.begin(115200);
  data = (char *)ps_malloc(2000000LL);

  Serial.print("Connecting to WiFi");
  network.begin(ssid, pass);

  Serial.print("Getting data");
  while (!network.getData(calendarURL, data))
  {
      Serial.print('.');
      delay(1000);
  }

  Serial.print(data);
  
  
  Serial.println("Going to sleep");
  delay(100);
  esp_sleep_enable_timer_wakeup(60ll * 2 * 1000 * 1000); //wakeup in 60min time - 60min * 60s * 1000ms * 1000us
  esp_deep_sleep_start();
}

void loop()
{
    // Never here, as deepsleep restarts esp32
}
