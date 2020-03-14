#!/bin/bash

hex=`cat main.map | grep 'PROVIDE (__noinit_end, .)' | awk '{print $1}' | sed 's/0x0080//g' | tr "[:lower:]" "[:upper:]" `
echo '-------------------'
echo 'BSS usage: ' `echo 'ibase=16;'$hex | bc ` bytes
