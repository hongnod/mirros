export ARCH 		:= arm
export CROSS_COMPILE 	:= arm-linux-
export CC 		:= $(CROSS_COMPILE)gcc
export LD 		:= $(CROSS_COMPILE)ld
export OBJ_COPY	:= $(CROSS_COMPILE)objcopy
export OBJ_DUMP 	:= $(CROSS_COMPILE)objdump
export INCLUDE_DIR := $(PWD)/include/os/*.h $(PWD)/include/asm/*.h
export CCFLAG :=-Wall -nostdlib -fno-builtin -g -I$(PWD)/include
export OUT := $(shell pwd)/out

export LDS := arch/arm/machine/lds/tq2440.lds

export LDFLAG := -T$(LDS) 
export LDPATH := -L/opt/FriendlyARM/toolschain/4.4.3/lib/gcc/arm-none-linux-gnueabi/4.4.3 -lgcc

export OUT_BOOT = $(OUT)/boot
export OUT_KERNEL = $(OUT)/kernel
export OUT_ARCH = $(OUT)/arch

obj-boot = $(OUT_BOOT)/boot.o
obj-arch = $(OUT_ARCH)/*.o
obj-kernel = $(OUT_KERNEL)/*.o

object = $(obj-boot) $(obj-arch) $(obj-kernel)

boot_elf := $(OUT)/boot.elf
boot_bin := $(OUT)/boot.bin
boot_dump :=$(OUT)/boot.s
ramdisk := ramdisk/ramdisk.img

$(boot_bin) : $(boot_elf)
	$(OBJ_COPY) -O binary $(boot_elf) $(boot_bin)
	$(OBJ_DUMP) $(boot_elf) -D > $(boot_dump)

$(boot_elf) : $(object) $(LDS)
	$(LD) $(LDFLAG) -o $(boot_elf) $(object) $(LDPATH)

$(object): subsystem

subsystem: mkdir
	@make all -C arch/arm/machine
	@cd mm/ && make
	@cd kernel/ && make
	@cd init/ && make

mkdir:
	@mkdir -p $(OUT_KERNEL)
	@mkdir -p $(OUT_ARCH)
	@mkdir -p $(OUT_BOOT)

.PHONY: clean run

clean:
	@rm -rf $(OUT)
	@echo "all build have been removed"

run: 
	@cd tools && ./sy.sh
