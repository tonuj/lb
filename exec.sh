#!/bin/sh

cd $1

/opt/local/bin/avrdude -p m32 -P usb -c avrispmkII -B 2 -U flash:w:main.hex:i

