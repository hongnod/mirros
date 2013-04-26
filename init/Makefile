obj-c = init.o main.o init_task.o

out-dir := $(OUT_KERNEL)

all:$(obj-c)

$(obj-c):%.o:%.c $(INCLUDE_DIR)
	$(CC) $(CCFLAG) -c $< -o $(out-dir)/$@
