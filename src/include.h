#ifndef INCLUDE_H
#define INCLUDE_H

#include <Arduino.h>
#include "esp_log.h"
#include "include.h"

#define USE_SPIFFS // else use the SDCARD

#ifdef USE_SPIFFS
#define FORMAT_SPIFFS_IF_FAILED true
#include "SPIFFS.h"
#else
#include "SD_MMC.h"
// SD card
#define CLK 1
#define CMD 2
#define D0 42
#define BOARD_MAX_SPI_FREQUENCY SD_MMC_FREQ_HIGHSPEED
#endif

#define OTApassword "password" // <- Match this password with the one in platform.ini
#define BAUDRATE 115200 // // 115200 for MiSTer ttyUSBx
#define PRODUCT_NAME "mr_marquee"
#define BUILD_VERSION "230421"
#define PIC_MENU "menu"
#define PIC_ERROR "no-image"
#define PIC_NOWIFI "no-wifi"
#define PIC_DOWNLOADING "ota-update"


#define NAK "ttynack;"
#define ACK "ttyack;"


#endif // INCLUDE_H