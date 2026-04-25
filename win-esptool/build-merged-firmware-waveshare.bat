REM Build a single firmware image for direct flashing to the ESP32
REM
esptool.exe --chip esp32-s3 merge-bin --output ../mister-fpga/mr-marquee/esp_tools/firmware-mr-marquee-waveshare.bin --flash-mode dio ^
0x0000   ..\.pio\build\ESP32-S3-LCD-3-16\bootloader.bin ^
0x8000   ..\.pio\build\ESP32-S3-LCD-3-16\partitions.bin ^
0xe000  %USERPROFILE%\.platformio\packages\framework-arduinoespressif32\tools\partitions\boot_app0.bin ^
0x10000  ..\.pio\build\ESP32-S3-LCD-3-16\firmware.bin 
rem 0xc90000 ..\.pio\build\ESP32-S3-LCD-3-16\spiffs.bin