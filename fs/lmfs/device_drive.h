#ifndef _device_drive_h
#define _device_drive_h

//	根据具体情况进行修改。
extern FSBOOL sd_read_n_sector(FSU32 sector,char *buffer,int n);


extern FSBOOL sd_write_n_sector(FSU32 sector,char *buffer,int n);
		
extern FSBOOL sd_read_one_sector(FSU32 sector,char *buffer);			//设备读取一个扇区。

extern FSBOOL sd_write_one_sector(FSU32 sector,char *buffer);			//设备写一个扇区。

extern void get_device_drive(void);	

#define read_sector (sd.read_one_sector)			//读取一个扇区。
#define write_sector (sd.write_one_sector)			//写一个扇区。

#define read_nsector (sd.read_n_sector)			//读取n个扇区。
#define write_nsector (sd.write_n_sector)			//写N个扇区。


#endif

