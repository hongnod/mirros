ARCH 		:= arm
CROSS_COMPILE 	:= arm-linux-
CC 		:= $(CROSS_COMPILE)gcc
LD 		:= $(CROSS_COMPILE)ld
OBJ_COPY	:= $(CROSS_COMPILE)objcopy
OBJ_DUMP 	:= $(CROSS_COMPILE)objdump
AR	 	:= $(CROSS_COMPILE)ar
INCLUDE_DIR := $(PWD)/include/*.h
CCFLAG :=-Wall -nostdlib -fno-builtin -I$(PWD)/include
OUT := $(shell pwd)/out

LDS := app.lds

LDFLAG := -T$(LDS)
LDPATH := -L/opt/FriendlyARM/toolschain/4.4.3/lib/gcc/arm-none-linux-gnueabi/4.4.3 -lgcc

all: init test
	@ echo "finish build application"

init:
	@ $(CC) $(CCFLAG) -c init.c -o init.o
	@ $(LD) $(LDFLAG) -o init ./lib/main.o init.o -L./lib -lsyscall
	@ $(OBJ_DUMP) -D init > ../../out/init.s
	@ cp init ../../ramdisk/
	@ rm init.o init
test:
	@ $(CC) $(CCFLAG) -c test.c -o test.o
	@ $(LD) $(LDFLAG) -o test ./lib/main.o test.o -L./lib -lsyscall
	@ $(OBJ_DUMP) -D test > ../../out/test.s
	@ cp test ../../ramdisk/
	@ rm test.o test

main:
	@ $(CC) $(CCFLAG) -c main.S -o lib/main.o

libsyscall:
	@ $(CC) $(CCFLAG) -c syscall/debug.S
	@ $(CC) $(CCFLAG) -c syscall/execve.S
	@ $(CC) $(CCFLAG) -c syscall/fork.S
	@ $(CC) $(CCFLAG) -c syscall/exit.S
	@ rm lib/libsyscall.a
	@ $(AR) -rc lib/libsyscall.a debug.o execve.o fork.o exit.o
	@ rm *.o
