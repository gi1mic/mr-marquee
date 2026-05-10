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
#include <UrlEncode.h>


static const char *TAG = "NET";

using namespace std;

WiFiClient client;
FileFetcher fileFetcher(client);
WiFiManager wm;

unsigned long lastTimeCore = 0;
unsigned long lastTimeSettings = 0;

unsigned long updateDelayCore = 1000;
unsigned long updateDelaySettings = 1000;

long screenTimeout = 10; // This can be overridden by server settings
long screenTimeoutSettings = 10;

int tryGamenameThenCore = 0;
// char *serverOptions[] = {"mr-marquee", "remote script"};

//----------------------------------------------
void loadConfig()
{
    ESP_LOGD(TAG, "Loading config");
    // Read WiFi Credentials
    File filehandle;
#ifdef USE_INTERNAL_SPIFFS
    filehandle = SPIFFS.open("/config.json", FILE_READ);
#else
    filehandle = SD_MMC.open("/wifi.txt", FILE_READ);
#endif
    if (!filehandle)
    {
        footbanner("Could not open config.json");
        ESP_LOGI(TAG, "Could not open the file config.json");
        while (1)
            delay(0);
    }
    else
    {
        JsonDocument config;

        DeserializationError error = deserializeJson(config, filehandle);
        if (error)
        {
            ESP_LOGI(TAG, "Error while reading config.json: %s", error.c_str());
            delay(2000);
            return;
        }
        tryGamenameThenCore = config["remoteServer"].as<int>();
        ESP_LOGI(TAG, "tryGamenameThenCore: %d", tryGamenameThenCore);
    }
    filehandle.close();
}

//----------------------------------------------
void configModeCallback(WiFiManager *myWiFiManager)
{
    showLocalImage(PIC_NO_WIFI);
    String msg = "SSID: " + myWiFiManager->getConfigPortalSSID() + "  http://" + WiFi.softAPIP().toString();
    footbanner(msg);
    Serial.println(msg);
}

//-----------------------------------------------
void ConfigFile_Save_Variable(String VarName, String VarValue)
{
    ESP_LOGI(TAG, "Saving settings %s = %s", VarName, VarValue);

    File file = SPIFFS.open("/config.json", "r");
    JsonDocument doc;

    DeserializationError error = deserializeJson(doc, file);
    if (error)
        Serial.println(F("Failed to read file, using default configuration"));
    file.close();

    doc[VarName] = VarValue;

    file = SPIFFS.open("/config.json", "w");
    if (serializeJson(doc, file) == 0)
    {
        Serial.println(F("Failed to write to file"));
    }
    else
    {
        ESP_LOGI(TAG, "config.json saved - OK.");
    }
    file.close();
}

//----------------------------------------------
String getParam(String name)
{
    // read parameter from server, for customhmtl input
    String value;
    if (wm.server->hasArg(name))
    {
        value = wm.server->arg(name);
    }
    return value;
}

//----------------------------------------------
void saveParamCallback()
{
    Serial.println("[CALLBACK] saveParamCallback fired");
    Serial.println("PARAM customfieldid = " + getParam("T"));
    // if you get here you have connected to the WiFi
    ESP_LOGI(TAG, "Connected to WiFi");
    //        tryGamenameThenCore = (strncmp(custom_serverType_checkbox.getValue(), "T", 1) == 0);
    tryGamenameThenCore = (getParam("T") == "T" ? 1 : 0);
    if (tryGamenameThenCore)
    {
        ConfigFile_Save_Variable("remoteServer", "1");
    }
    else
    {
        ConfigFile_Save_Variable("remoteServer", "0");
    }
}

//----------------------------------------------
void connectWiFi()
{
    // char customHtml_checkbox[50] = "type=\"checkbox\" checked"; // Check Box
    char customHtml_checkbox[50] = "type=\"checkbox\"";
    std::vector<const char *> wmMenuItems = {"wifi", "exit"};
    WiFiManagerParameter custom_serverType_checkbox("Server", "Try Gamename then Core", "T", 2, customHtml_checkbox, WFM_LABEL_AFTER);

    wm.addParameter(&custom_serverType_checkbox);
    wm.setAPCallback(configModeCallback);
    wm.setMenu(wmMenuItems);
    wm.setSaveConfigCallback(saveParamCallback);
    wm.setAPCallback(configModeCallback); // set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode

    if (!wm.autoConnect(PRODUCT_NAME))
    {
        ESP_LOGI(TAG, "Failed to connect to WiFi");
        // ESP.restart();
    }
}

//----------------------------------------------
void checkButtonPressed()
{
#ifdef HUB75
    if (digitalRead(TRIGGER_PIN) == HIGH)
#else
    if (digitalRead(TRIGGER_PIN) == LOW)
#endif
    {
        wm.resetSettings(); // Uncomment to clear saved WiFi details (For testing only)
        Serial.println("Re-start to Wi-Fi config mode");
        delay(10000);
        ESP.restart();
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
    ArduinoOTA.setPassword(OTApassword);
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
        ESP_LOGI(TAG, "read settings");
        if (WiFi.status() == WL_CONNECTED)
        {
            HTTPClient http;

            http.useHTTP10(true);
            http.begin(MISTER_SETTINGS);

            int httpResponseCode = http.GET();
            //            ESP_LOGE("HTTP", "HTTP Response code: %d", httpResponseCode);

            if (httpResponseCode > 0)
            {
                JsonDocument settings;
                DeserializationError error = deserializeJson(settings, http.getString());
                if (error)
                {
                    ESP_LOGI(TAG, "Error %s", error);
                    http.end();
                    return;
                }

                videoPlay = settings["videoPlay"].as<bool>();
                //                ESP_LOGI("Net", "Play Video = %s", videoPlay ? "TRUE" : "FALSE");

                screenTimeoutSettings = settings["timeout"].as<long>();
                // ESP_LOGI("Net", "Timeout = %l", screenTimeoutSettings);
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
            http.useHTTP10(true);
            if (tryGamenameThenCore == 1)
                http.begin(MISTER_REMOTE_STATUS);
            else
                http.begin(MISTER_STATUS);

            int httpResponseCode = http.GET();
            ESP_LOGD(TAG, "HTTP Response code: %d", httpResponseCode);

            if (httpResponseCode > 0)
            {
                screenTimeout = screenTimeoutSettings;
                screenOn();
                if (tryGamenameThenCore == 1)
                {
                    ESP_LOGI(TAG, "Getting core name from remote server");
                    JsonDocument settings;
                    DeserializationError error = deserializeJson(settings, http.getStream());
                    if (error)
                    {
                        ESP_LOGI(TAG, "Error %s", error);
                        http.end();
                        return;
                    }

                    String payload = settings["core"];
                    String gameName = settings["gameName"].as<String>();
                    showPayload(payload, gameName);
                }
                else
                {
                    ESP_LOGI(TAG, "Getting core name from mr server");
                    String payload = http.getString();
                    currentGame = "";
                    showPayload(payload, "");
                }
            }

            http.end();
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

        lastTimeCore = millis();
    }
}
