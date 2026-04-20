#include <WiFi.h>
#include <HTTPClient.h>
#include "network.h"
#include <ESPmDNS.h>
#include <filesystem.h>
#include <WiFiUdp.h>
#include <WiFiClient.h>
#include <bits/stdc++.h>
#include <ArduinoJson.h>
#include "FileFetcher.h"

static const char *TAG = "NET";

using namespace std;

WiFiClient client;
FileFetcher fileFetcher(client);

String WSSID = "SSID";
String WPASS = "PW";

String WIFItimeout = "20";
char *WIFICountry[2];

unsigned long lastTimeCore = 0;
unsigned long lastTimeSettings = 0;

unsigned long updateDelayCore = 1000;
unsigned long updateDelaySettings = 1000;

long screenTimeout = 10; // This can be overridden by server settings
long screenTimeoutSettings = 10;

//----------------------------------------------
void loadWiFiConfig()
{
    // Read WiFi Credentials
    File filehandle;
#ifdef USE_SPIFFS
    filehandle = SPIFFS.open("/config.json", FILE_READ);
#else
    filehandle = SD_MMC.open("/wifi.txt", FILE_READ);
#endif
    if (!filehandle)
    {
        writetextcentered("Could not open the file config.json", 30, DEFAULT_FONT, 0, YELLOW, false, "");
        ESP_LOGI(TAG, "Could not open the file config.json");
        while (1)
            delay(0);
    }
    else
    {
        String WIFIcountry = "GB";
        JsonDocument config;

        DeserializationError error = deserializeJson(config, filehandle);
        if (error)
        {
            ESP_LOGI(TAG, "JSON Deserialization Error - have you uploaded the SPIFFS yet?");
            delay(2000);
            return;
        }

        WSSID = config["SSID"].as<const char *>();
        WPASS = config["PASSWORD"].as<const char *>();
        WIFItimeout = config["TIMEOUT"].as<const char *>();
        WIFIcountry = config["LOCAL"].as<const char *>();
        if (WIFItimeout.isEmpty())
            WIFItimeout = "20";
        ESP_LOGI(TAG, "WSSID %s", WSSID);
        ESP_LOGI(TAG, "WPASS: %s", WPASS);
        ESP_LOGI(TAG, "WiFi Timeout: %s", WIFItimeout);

        if (WIFIcountry.isEmpty())
        {
            WIFIcountry = "GB";
        }
        else
        {
            WIFIcountry.toCharArray(*WIFICountry, 2);
        }

        if (WSSID == "SSID" && WPASS == "PW")
        {
            writetextcentered("No valid WiFi credentials, WiFi disabled.", 30, DEFAULT_FONT, 0, YELLOW, false, "");
            ESP_LOGI(TAG, "No valid WiFi credentials, WiFi disabled.");
            delay(1000);
        }
    }
    filehandle.close();
}

//----------------------------------------------
void connectWiFi()
{
    // https://github.com/esp8266/Arduino/blob/master/tools/sdk/include/user_interface.h#L750-L760
    // wifi_country_t WIFIcountry;    // https://github.com/esp8266/Arduino/issues/7083
    WiFi.mode(WIFI_STA); // Explicit use of STA mode
    WiFi.setAutoReconnect(true);

    for (int i = 1; i <= 10; i++)
    {
        int startTime = millis();
        WiFi.disconnect(); // Disconnect from AP if it was previously connected
        WiFi.begin(WSSID.c_str(), WPASS.c_str(), WIFI_ALL_CHANNEL_SCAN);
        WiFi.persistent(true); // Store Wifi configuration in Flash?
        while (WiFi.status() != WL_CONNECTED && (millis() - startTime) <= WIFItimeout.toInt() * 1000)
        { // Try to connect
            ESP_LOGI(TAG, "WiFi status: %d", WiFi.status());
            delay(500);
        }
        if (WiFi.status() == WL_CONNECTED)
        {
            ESP_LOGI(TAG, "Connected to WiFi network: %s", WSSID.c_str());
            ESP_LOGI(TAG, "IP address: %s", WiFi.localIP().toString().c_str());
            break;
        }
    }
}

//----------------------------------------------
void startFileManager()
{
    WiFi.waitForConnectResult(4000); // File system manager does not support delayed start
    ESP_LOGI(TAG, "HTTP server starting");
    configTzTime("GMT0BST,M3.5.0/1,M10.5.0", "pool.ntp.org", "");
    addFileSystems();
    setupFilemanager();
}

//----------------------------------------------
void startMdns()
{
    ESP_LOGI(TAG, "mDNS responder starting");
    const char *mdnsName = PRODUCT_NAME;
    if (!MDNS.begin(mdnsName))
    {
        ESP_LOGI(TAG, "Error setting up MDNS responder!");
    }
    MDNS.addService("http", "tcp", filemanagerport);
    MDNS.addService("ota", "tcp", 3232);
}

//----------------------------------------------
void startOTA()
{
    ESP_LOGI(TAG, "ArduinoOTA starting");
    ArduinoOTA.setHostname(PRODUCT_NAME);
    ArduinoOTA.setPassword(PASSWORD);
    ArduinoOTA.begin();

    ArduinoOTA.onStart([]()
                       {
  String type;
  if (ArduinoOTA.getCommand() == U_FLASH) type = "sketch";
  else type = "filesystem";
  Serial.println("Start updating " + type); });

    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total)
                          { Serial.printf("Progress: %u%%\n", (progress / (total / 100))); });
    ArduinoOTA.onEnd([]()
                     { Serial.println("\nEnd"); });
}

// ----------------------------------------------
void readSettings()
{
    if ((millis() - lastTimeSettings) > updateDelaySettings)
    {
        //        ESP_LOGI("Net", "read settings");
        // Check WiFi connection status
        if (WiFi.status() == WL_CONNECTED)
        {
            HTTPClient http;
            http.begin(MISTER_SETTINGS);

            int httpResponseCode = http.GET();
            //            ESP_LOGE("HTTP", "HTTP Response code: %d", httpResponseCode);

            if (httpResponseCode > 0)
            {
                JsonDocument settings;
                String settingsFile = http.getString();
                DeserializationError error = deserializeJson(settings, settingsFile);
                if (error)
                {
                    ESP_LOGI(TAG, "Error %s", error);
                    http.end();
                    return;
                }

                int rotation = settings["rotation"].as<int>();
                //                ESP_LOGI("Net", "Rotation = %i", rotation);
                screenRotation(rotation);

                videoPlay = settings["videoPlay"].as<bool>();
                //                ESP_LOGI("Net", "Play Video = %s", videoPlay ? "TRUE" : "FALSE");

                screenTimeoutSettings = settings["timeout"].as<long>();
                //                ESP_LOGI("Net", "Timeout = %l", screenTimeoutSettings);

                // portraitScreen = settings["portraitScreen"].as<bool>();
                //                 ESP_LOGI("Net", "Portrait Screen = %s", portraitScreen ? "TRUE" : "FALSE");

                bool debug = settings["debug"].as<bool>();
                //                ESP_LOGI("Net", "Debug mode = %s", debug ? "TRUE" : "FALSE");
            }
            else
            {
                ESP_LOGI(TAG, "HTTP Error code: %d", httpResponseCode);
            }
            http.end();
            lastTimeSettings = millis();
        }
    }
}

//----------------------------------------------
void processCore()
{
    // Send an HTTP POST request every few seconds
    if ((millis() - lastTimeCore) > updateDelayCore)
    {
        // Check WiFi connection status
        if (WiFi.status() == WL_CONNECTED)
        {
            // Using HTTP conenction in temp session mode to cater for mister being restarted
            HTTPClient http;
            http.begin(MISTER_CURRENT_STATUS);

            int httpResponseCode = http.GET();
            //            ESP_LOGE("HTTP", "HTTP Response code: %d", httpResponseCode);

            if (httpResponseCode > 0)
            {
                screenOn();
                screenTimeout = screenTimeoutSettings;
                String payload = http.getString();
                payload.trim(); // Fix the issue of ScummVM adding a \r to the end of the payload
                if (payload == "")
                    payload = "MENU";
                if (payload != currentCore)
                {
                    currentCore = payload;
                    if (payload == "MENU")
                    {
                        ESP_LOGI(TAG, "Showing local: %s", payload);
                        tft->fillScreen(BLACK);
                        showLocalImage(PIC_MENU);
                    }
                    else
                    {
                        ESP_LOGI(TAG, "Showing URL Core: %s ", payload);
                        tft->fillScreen(BLACK);
                        showCore(payload);
                    }
                }
            }
            else
            {
                ESP_LOGI(TAG, "ScreenTimeout: %d", screenTimeout);
                if (screenTimeout-- <= 0)
                {
                    screenOff();
                    screenTimeout = 0;
                }
            }
            http.end();
        }
        else
        {
            ESP_LOGI(TAG, "WiFi Disconnected");
            tft->fillScreen(BLACK);
            showLocalImage(PIC_NOWIFI);
        }
        lastTimeCore = millis();
    }
}