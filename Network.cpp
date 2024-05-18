/*
This is a modified version of Network.cpp

Original source:
Inkplate 6 Arduino library
David Zovko, Borna Biro, Denis Vajak, Zvonimir Haramustek @ Soldered
September 24, 2020
https://github.com/e-radionicacom/Inkplate-6-Arduino-library
*/

#include <string.h>
#include "Network.h"

struct tm timeinfo;

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
void Network::setTimeInfo(int timezoneOffset)
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
    gmtime_r(&nowSecs, &timeinfo);

    // Print the current time without adding a timezone
    Serial.print(F("Current time: "));
    Serial.print(asctime(&timeinfo));
}

String Network::getDate(String format)
{
    char dateStr[20];
    snprintf(dateStr, sizeof(dateStr), format.c_str(), timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday);
    return String(dateStr);
}

String Network::getTime()
{
    char timeStr[10];
    snprintf(timeStr, sizeof(timeStr), "%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);
    return String(timeStr);
}