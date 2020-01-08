#!/bin/bash

start=`date +%s`

# get current dir so we can go back there when done
Back_To_Dir=$PWD
Port="None"

if [ -e "/dev/ttyUSB0" ];
    then
        Port=/dev/ttyUSB0
fi

if [ -e "/dev/ttyUSB1" ];
    then
        Port=/dev/ttyUSB1
fi

if [ "$Port" = "None" ];
    then
        echo "Unable to find port"
        exit
fi

echo ""
echo "Port set to: $Port"

if [ "$1" = "--upload" ];
    then
        echo 
        echo "ONLY upload firmware"

        # change dir to micropyhton
        cd ~/micropython/ports/esp8266/

        if [ "$2" = "-pro" ];
            then
            echo "   D1 mini PRO v1.0"
            esptool.py --port $Port --baud 460800 write_flash -fm dio -fs 4MB -ff 40m 0x0000000 build-GENERIC/firmware.bin
        else
            echo "   D1 mini"
            esptool.py --port $Port --baud 460800 write_flash --flash_size=detect 0 build-GENERIC/firmware.bin
        fi

        echo "start serial"
        ~/piusb0.sh
        exit
fi


if [ "$1" = "-e" ];
    then
        echo 
        echo "Erassing flash before uploading firmware"
        esptool.py -p $Port erase_flash
        echo "   Done"
        exit
fi

if [ "$1" = "-f" ];
    then
        echo 
        echo "Uploading $2 to /lib"
        echo "   Creating $2.mpy"
        ~/micropython/mpy-cross/mpy-cross $2.py
        echo "   Uploading $2.mpy"
        ampy -p $Port put $2.mpy '/lib/'$2'.mpy'
        echo "   Starting Serial"
        ~/piusb0.sh
        exit
fi




# if [ "$1" = "--power" ];
#     then
#         echo "Removing old Dobby modules"
#         rm -v -Rf ~/micropython/ports/esp32/modules/dobby

#         echo "Copying Power modules"
#         cp -v -R "$(dirname "$(realpath "$0")")"/../bin/Special/Power/modules ~/micropython/ports/esp32/

if [ "$1" = "--nocopy" ];
    then
        echo "NOT copying modules"
    else
        # copy modules
        echo "Copying modules"
        cp -v -r "$(dirname "$(realpath "$0")")"/../os/Shared/*.py ~/micropython/ports/esp8266/modules
        # copy modules
        echo "Copying esp8266 modules"
        cp -v -r "$(dirname "$(realpath "$0")")"/../os/esp8266/*.py ~/micropython/ports/esp8266/modules
fi


# change dir to micropyhton
cd ~/micropython/ports/esp8266/

# clean after last make
echo 
echo make clean
make clean | grep error

# make 
echo 
echo Making
make | grep 'error'


# Check if make went ok aka we have the file below
FILE="build-GENERIC/firmware-combined.bin"
if test -f "$FILE";
    then
        echo "OK"
    else
        echo
        echo "Make failed"
        exit
fi



# Upload firmware
echo 
echo "Uploading firmware:"


if [ "$1" = "-pro" ];
    then
    echo "   D1 mini PRO v1.0"
    esptool.py --port $Port --baud 460800 write_flash -fm dio -fs 4MB -ff 40m 0x0000000 $FILE
else
    echo "   D1 mini"
    esptool.py --port $Port --baud 460800 write_flash --flash_size=detect 0 $FILE
fi


# back to the dir we came from
cd $Back_To_Dir

end=`date +%s`
runtime=$((end-start))

echo "Upload time: "$runtime"s"

# Open serial connection
echo start serial
picocom -b 115200 $Port