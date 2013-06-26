#!/bin/sh

arm-linux-gcc -c test.c -o test.o
arm-linux-ld -Tapp.lds -o test test.o
cp test ../../ramdisk/
