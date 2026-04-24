.\esptool.exe --baud 115200  --chip esp32-s3 ^
 --before default-reset --after hard-reset write-flash -z ^
 --flash-mode dio --flash-freq 40m --flash-size detect ^
0x0000 firmware-mr-marquee.bin