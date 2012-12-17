#include"lmfs_config.H"
#include"device_drive.h"
#include"lmfs.h"

char fs[10*512*2*1024];
//调试时候用的，fs为基地址。

//设备驱动函数。实现

FSBOOL sd_read_one_sector(FSU32 sector,char *buffer)
{
	char *base;
	int i;
	base=fs+(sector*SECTOR_PER_SIZE);		//找到扇区的起始地址。
	for(i=0;i<512;i++)
	{
		buffer[i]=base[i];
	}
	return 1;
}

FSBOOL sd_write_one_sector(FSU32 sector,char *buffer)			
{
	char *base;
	int i;
	base=fs+(sector*SECTOR_PER_SIZE);		//找到扇区的起始地址。
	for(i=0;i<512;i++)
	{
		base[i]=buffer[i];
	}
	return 1;
}

FSBOOL sd_read_n_sector(FSU32 sector,char *buffer,int n)
{
	char *base;
	int i;
	base=fs+(sector*SECTOR_PER_SIZE);		//找到扇区的起始地址。
	for(i=0;i<SECTOR_PER_SIZE*n;i++)
	{
		buffer[i]=base[i];
	}
	return 1;
}

FSBOOL sd_write_n_sector(FSU32 sector,char *buffer,int n)		
{
	char *base;
	int i;
	base=fs+(sector*SECTOR_PER_SIZE);		//找到扇区的起始地址。
	for(i=0;i<SECTOR_PER_SIZE*n;i++)
	{
		base[i]=buffer[i];
	}
	return 1;
}

void get_device_drive(void)		//注册sd卡的驱动程序。
{
	sd.read_one_sector=sd_read_one_sector;
	sd.write_one_sector=sd_write_one_sector;
	sd.read_n_sector=sd_read_n_sector;
	sd.write_n_sector=sd_write_n_sector;
	sd.sector_per_size=SECTOR_PER_SIZE;
	sd.disk_total_sector=DISK_TOTAL_SECTOR;
}

