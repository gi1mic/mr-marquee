#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>
#include <ESP32Time.h>
#include "SPI.h"
#include <time.h>
#include <JPEGDEC.h>
#include "FS.h"
#include <JPEGDEC.h>
#include "display.h"
#include "filesystem.h"
#include "network.h"
#include <WebServer.h>
#include <ESPFMfGK.h>
#include "include.h"
#include <String.h>

#define ADC_PIN 4
static const char *TAG = "MAIN";


//----------------------------------------------
void setup(void)
{
  Serial.begin(BAUDRATE);
  int cnt = 100;
  while (!Serial || (cnt-- > 0)) // Wait for Serial to initialize
  {
    delay(50);
  }

  pinMode(TFT_BL, OUTPUT);

  randomSeed(analogRead(ADC_PIN)); // Init Random Generator with empty Port Analog value

  Serial.setDebugOutput(true);
  //  Serial.flush(); // Wait for empty Send Buffer
  Serial.println();
  Serial.println();
  Serial.println();
  Serial.println(PRODUCT_NAME);
  Serial.printf("Build version: %s\n", BUILD_VERSION);
  Serial.setTimeout(500); // Set max. Serial "Waiting Time", default = 1000ms

  // show what levels are supported
  ESP_LOGE(TAG, "Error Reporting On");
  ESP_LOGW(TAG, "Warning Reporting On");
  ESP_LOGI(TAG, "Information Reporting On");
  ESP_LOGD(TAG, "Debug Reporting On");
  ESP_LOGV(TAG, "Verbose Reporting On");

  tftInit();

#ifdef USE_INTERNAL_SPIFFS
  if (!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED))
  {
    footbanner("Error while initialising SPIFFS. Halting.");
    return;
  }
#else
  ESP_LOGI("SD", "Mounting SDcard");
  if (!SD_MMC.setPins(SD_CLK, SD_CMD, SD_D0))
  {
    Serial.println("SD MMC pin change failed!");
    return;
  }

  if (!SD_MMC.begin("/sdcard", true))
  {
    Serial.println("Card Mount Failed");
    writetextcentered("Error while initialising MMCSD. Halting.", 20, u8g2_font_luBS10_tf, 0, BLUE, false, "clear");
    return;
  }

  ESP_LOGI("SD", "SD_MMC Card Type: %d\n", SD_MMC.cardType());
  ESP_LOGI("SD", "SD_MMC Card Size: %lluMB\n", SD_MMC.cardSize() / (1024 * 1024));
#endif

//  loadWiFiConfig();
  connectWiFi();
  startOTA();
  startMdns();
  startFileManager();

  showLocalImage(PIC_MENU);
  writetext("Ver: " BUILD_VERSION, false, 600, 300, TFT_FONT_SMALL, 3, WHITE, false, "");
}

// ================ MAIN LOOP ===================
void loop(void)
{
  processCore();
  readSettings();
  filemgr.handleClient();
  ArduinoOTA.handle();
}
// =========== End of main routines =============
