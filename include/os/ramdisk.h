#ifndef _RAM_DISK_H
#define _RAM_DISK_H

#define FILE_NAME_SIZE 32

#include <os/types.h>

struct ramdisk_header {
	char name[16];
	s32 total_size;
	s32 file_count;	
	u32 unused;
	void *data;
};

struct file_header {
	char name[FILE_NAME_SIZE];
	u32 base;
	u32 size;
};

struct file {
	u32 base;
	u32 size;
	u32 curr;
};


int ramdisk_read(struct file *file, char *buf, int size, u32 offset);
int ramdisk_seek(struct file *file, u32 offset);

#define fs_read ramdisk_read
#define fs_seek ramdisk_seek

#endif
