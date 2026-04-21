# mr-marquee
Colour Marquees for MiSTer FPGA. 
Like tty2oled but using a colour screen and Wi-Fi to allow for remote connections instead of a USB port.

** This is a work in progress..... **

## Introduction
The system consists of two parts. A server element for the MiSTer FPGA system and a Wi-Fi connected ESP32 based colour display.

Since we are using ESP32 WiFi for communication the MiSTer FPGA will need to be accessible via the same network.

## Manual installation on a Mister FPGA System
Files inside the mister-fpga folder should be copied to /media/fat on your mister system.

SSH into the mister system and run the following command to install the nexessary Python dependencies.  
>pip install zeroconf --target /media/fat/mr-marquee

Then use nano to edit /media/fat/linux/user-startup.sh and add the following two lines. This will start the mr-marquee service on boot.

>#Startup mr-marquee
>[[ -e /media/fat/mr-marquee/mr-marquee-init ]] && /media/fat/mr-marquee/mr-marquee-init $1

### What the code does
The mr-marquee service provides the following features:
1) it allows the mister system to be discovered on the network via the network name "mister.local"
2) it creates a web server that allows a remote system to read the current running core
3) It shares a settings.json file providing configuration information to the remote system
4) It shares a folder called banners containing JPG marquees.

## Installation on ESP32 hardware

The current application is specifically designed for the ESP32-S3-LCD-3.16 screen from Waveshare. The MaTouch ESP32-S3 Parallel TFT 3.16“ ST7701S looks to be a very similar board but use it at your own risk.

The code should run on and ESP32 based display board with a few changes as long as the display controller is supported by the Arduino_GFX library. I will probably add support for Cheap Yellow Display board in due course as I happen to have a few already.

## Operation

The ESP32 application uses mDNS to discover the IP address of mister FPGA system. It then queries the 
mr-marquee web server to get the current running corename. If no core is running it shows a simple banner called menu.jpg which is held on a local SPIFFS filesystem.

If a core name is received it uses that name to download a marquee image via HTTP and display it on the local colour screen.

The screen will go blank after a predetermined time if the mister.local webserver can’t be contacted i.e. the MiSTer FPGA unit has been turned off. It will turn back on again once a connection can be reestablished.

The ESP32 should appear on the local network as "mr_marquee.local" and the application includes a simple web-based file management system for updating the system files held on the SPIFFS filesystem of the ESP32. 

It is possible to upgrade the ESP32 application remotely via Wi-Fi using the standard Espressif ESP32 OTA Update process.

## Thanks to:
This project started life as a customisation of the abandoned tty2tft project (https://github.com/ojaksch/MiSTer_tty2tft). It has since evolved into a very different application.

Many of the Marquees were converted from artwork designed for Pixelcade which I found while doing a web search. Unfortunately I did not save the forum link at the time and I am not sure who shared or created the files. 

The FileFetcher code from Brian Lough is included as source files and not as a library. I had to amend the library to support ports other than port 80 and 443 and fix an issue with missing data at the end of the stream. 

## Things to do
1. Create an installer for the Mister FPGA that will install the code and optionally Flash the ESP32 display
2. Create a binary release of the ESP32 firmware for use with the above installer
3. Create more Marquees
4. Add support for ESP32 CYD boards to provide an example of how to add other devices
5. Add MJPEG playback (The code already supports local MJPEG playback, but I have not tested with streaming video)

## Not planned
1. I do not intend to implement any clock or screensaver features. I only require the screen to blank when the MiSTer FPGA is not available.
2. Touchscreen support


## Compiling the ESP32 Code

You will need to install VisualCode and the PlatformIO extension.

Then simply create a new project from github.

PlatformIO will install the necessary libraries, and you should be able to build and flash the code in a few minutes.

Note the following libraries need editing after they have been downloaded.

### JPEDEC Library: (src/JPEGDC.h)
 Increase the buffer sizes by editting JPEGDEC.h and change the values of
> JPEG_FILE_BUF_SIZE and MAX_BUFFERED_PIXELS to 4096

### Arduino_GFX Library (src/databus/ES32RGBPanel.h) 
To avoid compilation errors of Arduino_GFX under PlatformIO you need to edit Arduino_ESP32RGBPanel.h and switch the defines on line 55 and 59 to:
>        #if (!defined(ESP_ARDUINO_VERSION_MAJOR)) || (ESP_ARDUINO_VERSION_MAJOR < 3)
>        //#if (!defined(ESP_ARDUINO_VERSION_MAJOR)) || (ESP_ARDUINO_VERSION_MAJOR >5)

### Arduino_GFX Library (src/databus/Arduino_ESP32SPI.h):
To avoid compilation errors of Arduino_GFX under PlatformIO you need to edit Arduino_ESP32SPI.h and comment out the following two includes:
>              //#include "esp32-hal-periman.h"
>              //#include "esp_private/periph_ctrl.h"










