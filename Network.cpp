/*
Network.cpp
Inkplate 6 Arduino library
David Zovko, Borna Biro, Denis Vajak, Zvonimir Haramustek @ Soldered
September 24, 2020
https://github.com/e-radionicacom/Inkplate-6-Arduino-library

For support, please reach over forums: forum.e-radionica.com/en
For more info about the product, please check: www.inkplate.io

This code is released under the GNU Lesser General Public License v3.0: https://www.gnu.org/licenses/lgpl-3.0.en.html
Please review the LICENSE file included with this example.
If you have any questions about licensing, please contact techsupport@e-radionica.com
Distributed as-is; no warranty is given.
*/

#include <string.h>
#include "Network.h"

// Connect Inkplate to the WiFi
void Network::begin(char *ssid, char *pass)
{
    // Initiating wifi, like in BasicHttpClient example
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, pass);

    // Waiting to WiFi connect
    int cnt = 0;
    Serial.print(F("Waiting for WiFi to connect..."));
    while ((WiFi.status() != WL_CONNECTED))
    {
        // Prints a dot every second that wifi isn't connected
        Serial.print(F("."));
        delay(1000);
        ++cnt;

        // If it doesn't connect to wifi in 10 seconds, reset the ESP
        if (cnt == 10)
        {
            Serial.println("Can't connect to WIFI, restarting");
            delay(100);
            ESP.restart();
        }
    }
    Serial.println(F(" connected"));
}

// Gets time from ntp server
void Network::getTime(char *timeStr, long offSet, int timeZone)
{
    // Get seconds since 1.1.1970. and add timezone
    time_t nowSecs = time(nullptr) + (long)timeZone * 3600L + offSet;

    // Used to store time
    struct tm timeinfo;
    gmtime_r(&nowSecs, &timeinfo);

    // Copies time string into timeStr
    strcpy(timeStr, asctime(&timeinfo));
}

// Function to get all raw data from the web
bool Network::getData(char *calendarURL, String& payload)
{
    // If not connected to WiFi, reconnect wifi
    if (WiFi.status() != WL_CONNECTED)
    {
        WiFi.reconnect();

        // Waiting to WiFi connect again
        int cnt = 0;
        Serial.println(F("Waiting for WiFi to reconnect..."));
        while ((WiFi.status() != WL_CONNECTED))
        {
            // Prints a dot every second that wifi isn't connected
            Serial.print(F("."));
            delay(1000);
            ++cnt;

            // If it doesn't connect to wifi in 10 seconds, reset the ESP
            if (cnt == 10)
            {
                Serial.println("Can't connect to WIFI, restart initiated.");
                delay(100);
                ESP.restart();
            }
        }
    }

    // Http object used to make get request
    HTTPClient http;
    http.setTimeout(20000);
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

    if (http.begin(calendarURL)) {
      int httpCode = http.GET();

      if (httpCode != 200) {
        Serial.println("Failed request. HTTP response:");
        Serial.println(httpCode);
        http.end();

        return 0;
      }

      else {
        payload = http.getString();
        http.end();

        return 1;
      }
    }

    return 0;
}

// Find internet time
void Network::setTime(int timezoneOffset)
{
    // Used for setting correct time
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");

    Serial.print(F("Waiting for NTP time sync: "));
    time_t nowSecs = time(nullptr);
    while (nowSecs < 8 * 3600 * 2)
    {
        delay(500);
        Serial.print(F("."));
        yield();
        nowSecs = time(nullptr);
    }
    Serial.println();

    nowSecs += timezoneOffset * 3600;

    // Used to store time info
    struct tm timeinfo;
    gmtime_r(&nowSecs, &timeinfo);

    // Print the current time without adding a timezone
    Serial.print(F("Current time: "));
    Serial.print(asctime(&timeinfo));
}