#!/bin/sh

arm-linux-gcc -c test.c -o test.o
arm-linux-gcc -c syscall.S -o syscall.o
arm-linux-gcc -c main.S -o main.o
arm-linux-ld -Tapp.lds -o test main.o test.o syscall.o
arm-linux-objdump -D test > ../../out/test.s
cp test ../../ramdisk/
rm *.o test
