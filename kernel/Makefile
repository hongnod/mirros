obj-c  = bitops.o mutex.o string.o interrupt.o mmu.o  \
	 printk.o sched.o task.o elf.o io.o panic.o

out-dir := $(OUT_KERNEL)

all:$(obj-c)

$(obj-c):%.o:%.c $(INCLUDE_DIR)
	$(CC) $(CCFLAG) -c $< -o $(out-dir)/$@
