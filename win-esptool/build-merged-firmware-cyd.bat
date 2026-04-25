REM Build a single firmware image for direct flashing to the ESP32
REM
esptool.exe --chip esp32 merge-bin --output ../mister-fpga/mr-marquee/esp_tools/firmware-mr-marquee-cyd.bin --flash-mode dio ^
0x0000   ..\.pio\build\cyd\bootloader.bin ^
0x8000   ..\.pio\build\cyd\partitions.bin ^
0xe000  %USERPROFILE%\.platformio\packages\framework-arduinoespressif32\tools\partitions\boot_app0.bin ^
0x10000  ..\.pio\build\cyd\firmware.bin 
rem 0xc90000 ..\.pio\build\cyd\spiffs.bin