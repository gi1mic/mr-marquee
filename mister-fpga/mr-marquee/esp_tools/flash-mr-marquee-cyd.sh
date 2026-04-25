#!/bin/bash

esptool --baud 115200  --chip esp32 \
 --before default-reset --after hard-reset write-flash -z \
 --flash-mode dio --flash-freq 40m --flash-size detect \
0x0000 firmware-mr-marquee-cyd.bin



exit

esptool --baud 115200 --port /dev/ttyACM0 --chip esp32-s3 --before default-reset --after hard-reset write-flash -z --flash-mode dio --flash-freq 40m --flash-size detect 0x0000 firmware-mr-marquee.bin


# Build SPIFFS filesystem
if grep -q your-ssid data/config.json || grep -q your-password data/config.json; then
	echo "Please update you Wi-Fi SSID and PASSWORD in data/config.json"
fi

echo "Build SPIFFS filesystem"
mkspiffs -c data -b 4096 -p 256 -s 0x360000 mr_marquee_spiffs.bin

echo "Flashing SPIFFS filesystem"
#esptool --chip esp32-s3 --port /dev/ttyACM0 --baud 460800 write_flash 0x290000 mr_marquee_spiffs.bin

#echo "Flashing bootloader"
#esptool --chip esp32-s3 --port /dev/ttyACM0 --baud 460800 write_flash 0x1000 mr_marquee_bootloader.bin

echo "Flashing ESP32 firmware"
esptool --chip esp32-s3 --port /dev/ttyACM0 --baud 460800 --before default-reset --after hard-reset   write_flash 0x1000 mr_marquee_bootloader.bin 0x8000 mr_marquee_firmware.bin


exit

# Discover ip address of mr_marquee.local
IP=$(../discover.py  2> /dev/null mr_marquee.local | grep -o "([^)]*)" | tr -d "(')")


# Flash SPIFFS OTA
#espota.py -d -i $IP -f firmware.bin -p 3232 -a ota_password

# Flash device OTA
#./espota.py -d -i $IP -f firmware.bin -p 3232 -a ota_password
