# mr-marquee
Colour Marquees for Mister FPGA. 
Like tty2oled but using Wi-Fi and a colour screen.

This is a work in progress.....

The system consists of two parts.

The files inside the mister-fpga folder should be copied to /media/fat on your mister system.

Then run the command "pip install zeroconf --target /media/fat/mr-marquee"

After that the following two lines should be added to /media/fat/linux/user-startup.sh

#Startup mr-marquee
[[ -e /media/fat/mr-marquee/mr-marquee-init ]] && /media/fat/mr-marquee/mr-marquee-init $1

This will start the mr-marquee service on boot.

The mr-marquee service provides the following features:
1) it allows the mister system to be discovered on the network via the network name "mister.local"
2) it creates a web server that allows a remote system to read the current running core
3) It shares a settings.json file providing configuration information to the remote system
4) It shares a folder called banners containing JPG marquees.


The second part is an application for an ESP32-S3. Currently is is designed for a 3.16" screen from Waveshare that costs around £20. But it would be easy enough to adapt the core to run on other ESP32 based boards.

The application uses mDNS to discover the IP address of mister FPGA system. It then queries the 
web server to get the current running corename. If no core is running it shows a simple banner called menu.jpg held on a local SPIFFS filesystem.

If a core name is received it uses that name to download a marquee image via HTTP and display it on the local colour screen.

The screen will go blank after a predetermined time if the mister.local webserver can’t be contacted i.e. the mister fpga unit has been turned off. It will turn back on again once a connection is reestablished.

The application includes a simple web-based file management system for updating some of the system files held on the SPIFFS filesystem on the ESP32. The ESP32 should appear on the local network as "mr_marquee.local".

It is possible to upgrade the ESP32 application remotely via Wi-Fi (OTA Update).







