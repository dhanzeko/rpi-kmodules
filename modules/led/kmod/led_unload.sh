#!/bin/sh

sudo rmmod led.ko && sudo rm -f /dev/led[0,1,2]
