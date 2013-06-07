#include <os/types.h>
#include <os/string.h>
#include <os/mutex.h>
#include <os/printk.h>
#include <os/ramdisk.h>

extern unsigned long __ramdisk_start;

unsigned long mount_ramdisk(void)
{

	return __ramdisk_start;
}


int ramdisk_read(struct file *file, char *buf, int size, u32 offset)
{
	return 0;
}

int ramdisk_seek(struct file *file, u32 offset)
{
	return 0;
}
