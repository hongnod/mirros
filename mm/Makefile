obj-c = mm.o slab.o

out-dir = $(OUT_KERNEL)

all:$(obj-c)

$(obj-c):%.o:%.c $(INCLUDE_DIR)
	$(CC) $(CCFLAG) -c $< -o $(out-dir)/$@
