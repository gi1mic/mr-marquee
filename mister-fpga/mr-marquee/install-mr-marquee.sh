#!/bin/bash

APP_ROOT="/media/fat/mr-marquee"
INIT_FILE="/media/fat/linux/user-startup.sh"
DOWNLOAD="https://github.com/gi1mic/mr-marquee/archive/refs/heads/main.zip"


mkdir -p $APP_ROOT
cd $APP_ROOT

#--------------------------
# Update user-startup script
echo Updating MiSTer Init Script
if ! grep -q mr-marquee "$INIT_FILE"; then
cat <<EOF >> "$INIT_FILE"
	
# Startup mr-marquee
[[ -e /media/fat/mr-marquee/mr-marquee.py ]] && /media/fat/mr-marquee/mr-marquee-init \$1
EOF
else
 echo "MiSter init script already modified"
fi

#----------------------
# Install Python import modules
if [ ! -d "$APP_ROOT/zeroconf" ]; then
	echo Installing Python import modules
	pip install zeroconf -t /media/fat/mr-marquee/
else
	echo "Python import modules alread installed"
fi

#----------------------
# Downloading files
# wget https://github.com/gi1mic/mr-marquee/archive/refs/heads/main.zip
echo "Downloading files - this may take a few minutes..."
wget_output=$(wget -q "$DOWNLOAD")
if [ $? -ne 0 ]; then
	echo "Download failed"
	exit 1
fi

#----------------------
# Unzipping and installing files
echo "Istalling files.."
unzip main.zip
mv mr-marquee-main/mister-fpga/mr-marquee/* .

#----------------------
# Cleanup
echo "Cleaning up"
rm -r mr-marquee-main
rm main.zip

exit
