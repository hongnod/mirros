#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include "ramdisk.h"
#include <string.h>

int __gen_ramdisk(int fd, char *buf, char *path)
{
	struct ramdisk_header *ramdisk_header;
	struct file_header *file_header;
	struct dirent *pdir;
	char temp[123] = {0};
	int count = 0;
	char *file_header_base = sizeof(struct ramdisk_header) + buf;
	char *file_data_base;
	int file;
	int read_size;
	DIR *dir;

	printf("size of unsigned int is %d\n", sizeof(unsigned int));
	dir = opendir(path);
	if (dir == NULL) {
		printf("Can not open dir %s\n", path);
		return 1;
	}

	while (pdir = readdir(dir)) {
		count ++;	
	}

	count -= 2;

	printf("There are %d file in directory %s\n", count, path);
	file_data_base = file_header_base + count * sizeof(struct file_header);

	seekdir(dir, 0);
	file_header = (struct file_header *)file_header_base;
	while (pdir = readdir(dir)) {
		/*
		 *pass . and .. directory
		 */
		if (!strcmp(pdir->d_name, "."))
			continue;
		if (!strcmp(pdir->d_name, ".."))
			continue;

		strcpy(temp, path);
		strcat(temp, "/");
		strcat(temp, pdir->d_name);
		printf("%s\n",temp);
		
		file = open(temp, O_RDONLY);
		if (file < 0) {
			printf("can not open file %s in directory %s\n",temp, path);
			continue;
		}
		
		strcpy(file_header->name, pdir->d_name);
		file_header->base = file_data_base - buf;
		printf("file base is %d\n", file_header->base);
		file_header->size = 0;

		do {
			read_size = read(file, file_data_base, 512);
			printf("read %d byte data\n", read_size);
			file_data_base += read_size;
			file_header->size += read_size;
		} while(read_size == 512);

		close(file);
		file_header ++;
	}

	ramdisk_header = (struct ramdisk_header *)buf;
	strcpy(ramdisk_header->name, "ramdisk");
	ramdisk_header->file_count = count;
	ramdisk_header->total_size = file_data_base - buf;

	printf("ramdisk size is %d\n", file_data_base - buf);
	write(fd, buf, file_data_base - buf);

	return 0;
}

int gen_ramdisk(char *dir, char *target)
{
	DIR *folder = NULL;
	char *buf = NULL;
	int ramdisk = 0;

	buf = malloc(1024*1024);
	if (buf == NULL) {
		printf("can not allocate memory\n");
		return 1;
	}

	ramdisk = open(target, O_RDWR | O_CREAT, S_IRWXU);
	if (ramdisk < 0) {
		printf("can not create ramdisk.img\n");
		goto release_mem; 
	}

	__gen_ramdisk(ramdisk, buf, dir);

close_ramdisk:
	close(ramdisk);

release_mem:
	free(buf);

	return 0;
}

int main(int argc, char **argv)
{
	if (!argv[1] || !argv[2]) {
		printf("please select a folder to generate ramdisk\n");
		return 1;
	}
	
	gen_ramdisk(argv[1], argv[2]);

	return 0;
}
