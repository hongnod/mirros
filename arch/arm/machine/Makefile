obj-asm := arm920t_s.o s3c2440_s.o  syscall.o entry.o

obj-c := arm920t_c.o s3c2440_c.o tq2440_init.o

obj-b := boot.o

INCLUDE_DIR += ./include/*.h

out_dir     := $(OUT_ARCH)

all:$(obj-c) $(obj-asm) $(obj-b)

$(obj-c):%.o:%.c $(INCLUDE_DIR)
	$(CC) $(CCFLAG) -c $< -o $(OUT_ARCH)/$@

$(obj-asm):%.o:%.S $(INCLUDE_DIR)
	$(CC) $(CCFLAG) -c $< -o $(OUT_ARCH)/$@

$(obj-b): boot.S $(INCLUDE_DIR)
	$(CC) $(CCFLAG) -c boot.S -o $(OUT_BOOT)/boot.o
