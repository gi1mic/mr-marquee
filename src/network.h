#ifndef NETWORK_H
#define NETWORK_H

#include <Arduino.h>
#include "include.h"
#include <WiFi.h>
#include "display.h"
#include <FileFetcher.h>
#include <ArduinoOTA.h>

#ifdef USE_INTERNAL_SPIFFS
#include <SPIFFS.h>
#else
#include <SD_MMC.h>
#endif


#define MISTER_HOSTNAME             "mister.local"
#define MISTER_PORT                 "8090"
#define MISTER_WEBSERVER            "http://" MISTER_HOSTNAME ":" MISTER_PORT
#define MISTER_BANNER_SERVER        MISTER_WEBSERVER "/"
#define MISTER_SETTINGS             MISTER_WEBSERVER "/settings"
#define MISTER_CURRENT_STATUS       MISTER_WEBSERVER "/corename"

extern String WSSID;
extern String WPASS;
extern FileFetcher fileFetcher;

void loadWiFiConfig();
void connectWiFi();
void processCore();
void readSettings();
void startOTA();
void startMdns();
void startFileManager();


#endif // NETWORK_H
