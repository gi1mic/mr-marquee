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
#include <WiFiManager.h> //
static const char *TAG = "NET";

using namespace std;

WiFiClient client;
FileFetcher fileFetcher(client);

unsigned long lastTimeCore = 0;
unsigned long lastTimeSettings = 0;

unsigned long updateDelayCore = 1000;
unsigned long updateDelaySettings = 1000;

long screenTimeout = 10; // This can be overridden by server settings
long screenTimeoutSettings = 10;

void configModeCallback(WiFiManager *myWiFiManager)
{
    showLocalImage(PIC_NO_WIFI);
    String msg = "SSID: " + myWiFiManager->getConfigPortalSSID() + "  http://" + WiFi.softAPIP().toString();
    footbanner(msg);
    Serial.println(msg);
}

//----------------------------------------------
void connectWiFi()
{
    WiFiManager wm;
    std::vector<const char *> wmMenuItems = {"wifi", "exit"};

    //   wm.resetSettings(); // Uncomment to clear saved WiFi details (For testing only)
    wm.setAPCallback(configModeCallback);
    wm.setMenu(wmMenuItems);
    int res = wm.autoConnect(PRODUCT_NAME);
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
            showLocalImage(PIC_NO_WIFI);
        }
        lastTimeCore = millis();
    }
}