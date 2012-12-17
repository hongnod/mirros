#ifndef _lmfs_config_h
#define _lmfs_config_h

typedef unsigned int FSU32;
typedef char FSBOOL;
typedef unsigned short FSU16;
typedef unsigned char FSU8;
typedef int FSS32;
typedef short FSS16;
typedef char FSS8;

#define LMFS_AUTO_FORMAT (1)			//如果磁盘没有格式化是否自动格式化磁盘。

#define BLOCK_SIZE (4096)			//决定块的大小。必须是SECTOR_PER_SIZE的偶数倍

#define OS_SUPPORT (0)			//是否支持操作系统。	//以后实现。

#define SECTOR_PER_SIZE (512)	    // 扇区大小。

#define DISK_TOTAL_SECTOR (20*1024)

#define PARTION_FIRST_SECTOR (0)			//文件系统起始扇区。

#define MAX_FILE_NAME (60)		//文件名的最大长度。加上sizeof（FSU32）必须被512整除

#endif



