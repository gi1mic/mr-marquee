# This script only runs on Windows, and is used to merge the bootloader, partitions, and firmware
#  into a single binary for flashing with esptool. 
# It is a post-build script for platformio.ini and will not run under python directly. 
# It is run by platformio after the build process is complete.

import subprocess
import os
from os import path
#Import("env")

# access to global construction environment
print("Printing environment variables:")
#print("Current CLI targets", COMMAND_LINE_TARGETS)
#print("Current Build targets", BUILD_TARGETS)
#print(env)
print("Running post script")
current_directory = os.getcwd()
print(current_directory)

esptool = os.path.join(current_directory, 'win-esptool', 'esptool.exe')
command = [ '--chip', 'esp32-s3', 
        'merge-bin',
        '--output', 'mister-fpga\\mr-marquee\\esp_tools\\firmware-mr-marquee-ESP32-S3-HUB75.bin',
        '--flash-mode', 'dio', 
        '0x0000', '.pio\\build\\ESP32-S3-HUB75\\bootloader.bin',
        '0x8000', '.pio\\build\\ESP32-S3-HUB75\\partitions.bin',
        '0xe000', os.path.expandvars('%USERPROFILE%\\.platformio\\packages\\framework-arduinoespressif32\\tools\\partitions\\boot_app0.bin'),
        '0x10000','.pio\\build\\ESP32-S3-HUB75\\firmware.bin' ]
#print("Running command: " + esptool + " " + " ".join(command))

result = subprocess.run([esptool] + command, capture_output=True, text=True)
#print("Command executed with return code: " + str(result.returncode))
#if result.returncode != 0:
#    print("Error executing command: " + result.stderr)
print(result.stdout)
