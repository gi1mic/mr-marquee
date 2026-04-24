# mr-marquee
Colour Marquees for MiSTer FPGA. 
Like tty2oled but using a colour screen and Wi-Fi to allow for remote connections instead of a USB port.

![alt text](https://github.com/gi1mic/mr-marquee/blob/main/img/n64.jpg?raw=true)
![alt text](https://github.com/gi1mic/mr-marquee/blob/main/img/ps1.jpg?raw=true)

## Introduction
The system consists of two parts. A server element for the MiSTer FPGA system and a Wi-Fi connected ESP32 based colour display.

Since we are using ESP32 Wi-Fi for communication the MiSTer FPGA will need to be accessible via the same network.

If you do not already have a MiSTer system, I highly recommend a Multisystem2 [from](https://multisystem.uk/products/mister-multisystem-2/).

### Operation on the MiSTer system
The mr-marquee service provides the following features:
1) it allows the mister system to be discovered on the network via the network name "mister.local"
2) it creates a web server that allows a remote system to read the current running core
3) It shares a settings.json file providing configuration information to the remote system
4) It shares a folder called banners containing JPG marquees.

### Operation on the ESP32

If there are no stored Wi-Fi credentials it creates a local Wi-Fi access point allowing the user to select an available network and provide a suitable password for connection.

The ESP32 application then uses mDNS to discover the IP address of mister FPGA system. It then queries the 
mr-marquee web server to get the current running corename. If no core is running it shows a simple banner called menu.jpg which is held on a local SPIFFS filesystem.

If a core name is received it uses that name to download a marquee image via HTTP and display it on the local colour screen.

The screen will go blank after a predetermined time if the mister.local webserver can’t be contacted i.e. the MiSTer FPGA unit has been turned off. It will turn back on again once a connection can be reestablished.

The ESP32 should appear on the local network as "mr_marquee.local" and the application includes a simple web-based file management system for updating the system files held on the SPIFFS filesystem of the ESP32. 

It supports upgrade the ESP32 firmware remotely via Wi-Fi using the standard Espressif ESP32 OTA Update process.

# Installation
## MiSTer Installation

Login to a MiSTer bash shell using SSH.
Download the file "install-mr-marquee.sh" to you MiSTer system using the command 
> wget https://raw.githubusercontent.com/gi1mic/mr-marquee/refs/heads/main/mister-fpga/mr-marquee/install-mr-marquee.sh

Them make it executable by typing the command
> chmod +x install-mr-marquee.sh

Then simply run the script using the command by typing
> ./install-mr-marquee.sh

The script will download and install the MiSTer components to a directory called /media/fat/mr-marquee.
It may take it a few minutes to install the necessary python files.
Once complete re-boot the MiSTer system to finish the install by typing
> reboot

You can check the server is running by pointing a web browser to
> http://mister.local:8090/
and viewing some of the available marquees


## ESP32-S3 3.16" Waveshare programming
Once you have the files installed on your MiSTer system you can program the ESP32-S3 3.16" screen directly by connecting it to an available USB port. Please make sure it is the ONLY ESP32 device connected to your MiSTer system as no checks are done!!

CD into /media/fat/mr-marquee/esptools. And run the script flash-mr-marquee.sh. After a few seconds the ESP32 will reboot and show a Wi-Fi icon and the connection details on its screen. 

At this point the ESP32 will have created its own local Wi-Fi access point called mr_marquee.

![alt text](https://github.com/gi1mic/mr-marquee/blob/main/img/no-wifi.jpg?raw=true)

Connect your phone or laptop to this Wi-Fi network (no password) and point a web browser at http://192.168.4.1 where you will see a simple menu to connect the ESP32 to your main network.

Once you do this the ESP32 will reboot again and this time connect to your house Wi-Fi. If all is OK it will show a MiSTer logo.

![alt text](https://github.com/gi1mic/mr-marquee/blob/main/img/mister.jpg?raw=true)

The ESP32 will revert to creating a local access point if it cannot register with your Wi-Fi.

## Manual installation on a Mister FPGA System
Files inside the mister-fpga folder should be copied to /media/fat on your mister system.

SSH into the mister system and run the following command to install the necessary Python dependencies.  
>pip install zeroconf --target /media/fat/mr-marquee

Then use nano to edit /media/fat/linux/user-startup.sh and add the following two lines. This will start the mr-marquee service on boot.

>#Startup mr-marquee
>[[ -e /media/fat/mr-marquee/mr-marquee-init ]] && /media/fat/mr-marquee/mr-marquee-init $1

# Thanks to:
This project started life as a customisation of the abandoned tty2tft project (https://github.com/ojaksch/MiSTer_tty2tft). It has since evolved into a very different application.

Many of the Marquees were converted from artwork designed for Pixelcade which I found while doing a web search. Unfortunately, I did not save the forum link at the time and I am not sure who shared or created the files. 

The FileFetcher code from Brian Lough is included as source files and not as a library. I had to amend the library to support ports other than port 80 and 443 and fix an issue with missing data at the end of the stream. 

# Things to do
1. Create more Marquees
2. Add support for ESP32 CYD boards to provide an example of how to add other devices
3. Add MJPEG playback (The code already supports local MJPEG playback, but I have not tested with streaming video)
4. Create a 3D printed enclosure for the screen

## Not planned
1. I do not intend to implement any clock or screensaver features. I only require the screen to blank when the MiSTer FPGA is not available.
2. Touchscreen support


# Building the ESP32 code using platformIO

Install Visual Studio Code and the platformIO plugin.

Create a new project using the [git repository](https://github.com/gi1mic/mr-marquee.git) .

PlatformIO will install the necessary libraries, and you should be able to build and flash the code in a few minutes.

The current Arduino_GFX library does not play well with platformIO and you will need to make the following changes to get it to work: 

### Arduino_GFX Library (src/databus/ES32RGBPanel.h) 
To avoid compilation errors of Arduino_GFX under PlatformIO you need to edit Arduino_ESP32RGBPanel.h and switch the defines on line 55 and 59 to:
>        #if (!defined(ESP_ARDUINO_VERSION_MAJOR)) || (ESP_ARDUINO_VERSION_MAJOR < 3)
>        //#if (!defined(ESP_ARDUINO_VERSION_MAJOR)) || (ESP_ARDUINO_VERSION_MAJOR >5)

### Arduino_GFX Library (src/databus/Arduino_ESP32SPI.h):
To avoid compilation errors of Arduino_GFX under PlatformIO you need to edit Arduino_ESP32SPI.h and comment out the following two includes:
>        //#include "esp32-hal-periman.h"
>        //#include "esp_private/periph_ctrl.h"

After that you should be able to build the code and the SPIFFS filesystem. There are scripts in the win-esptool folder to build a merged firmware file containing all the necessary bits and then flash that file to a connected ESP32 device.

You can uncomment sections in platformio.ini to enable direct OTA flashing and to enable realtime, single step debugging and of course you can change the debug level to serial port messages.

## Notes:

The current application is specifically designed for the ESP32-S3-LCD-3.16 screen from Waveshare. The MaTouch ESP32-S3 Parallel TFT 3.16“ ST7701S looks to be a very similar board but use it at your own risk.

The code should run on and ESP32 based display board with a few changes as long as the display controller is supported by the Arduino_GFX library. I will probably add support for Cheap Yellow Display board in due course as I happen to have a few already.

For real-time debugging use the [Zadig](https://zadig.akeo.ie/) to change the ESP32 USB drivers to: 
>            USB Interface 0 = WinUSB driver
>            USB Interface 2 = libusbK driver
