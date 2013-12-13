ARCH 		:= arm
CROSS_COMPILE 	:= arm-linux-
CC 		:= $(CROSS_COMPILE)gcc
LD 		:= $(CROSS_COMPILE)ld
OBJ_COPY	:= $(CROSS_COMPILE)objcopy
OBJ_DUMP 	:= $(CROSS_COMPILE)objdump
AR	 	:= $(CROSS_COMPILE)ar
STRIP	 	:= $(CROSS_COMPILE)strip
INCLUDE_DIR := $(PWD)/include/*.h
CCFLAG :=-Wall -nostdlib -fno-builtin -I$(PWD)/include
OUT := $(shell pwd)/out

LDS := ./lds/app.lds

LDFLAG := -T$(LDS) -nostdlib
#LDPATH := -L/opt/FriendlyARM/toolschain/4.4.3/lib/gcc/arm-none-linux-gnueabi/4.4.3 -lgcc
LDPATH := -ldietlibc -lm -lgcc -L./lib

all: init test
	@ echo "finish build application"

init: init.c
	@ $(CC) $(CCFLAG) -o init ./lib/_start.o ./lib/exit.o ./lib/lib.o $< $(LDFLAG) $(LDPATH)
	@ cp init ../../ramdisk/
	@ $(STRIP) -s ../../ramdisk/init

test: test.c
	@ $(CC) $(CCFLAG) -o test ./lib/_start.o debug.o ./lib/exit.o ./lib/lib.o $< $(LDFLAG) $(LDPATH)
	@ cp test ../../ramdisk/
	@ $(STRIP) -s ../../ramdisk/test

.PHONY: clean
clean:
	@ rm init test
