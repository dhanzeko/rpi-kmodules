#!/bin/sh

export ARCH=arm 
export CCPREFIX=arm-rpi-linux-gnueabihf-
export KSRC=/home/dhanzeko/work/rpi/linux-kernel
export PATH=/opt/x-tools/arm-rpi-linux-gnueabihf/bin/:$PATH

ARC=Arm CROSS_COMPILE=${CCPREFIX} make $1
