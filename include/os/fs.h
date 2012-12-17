#ifndef _FS_H
#define _FS_H

struct file{
	int fd;
};

int fs_read(struct file *file,char *buf,u32 size,unsigned long offset)
{
	return 0;
}

int fs_seek(struct file *file,unsigned long offset)
{
	return 0;
}

#endif
