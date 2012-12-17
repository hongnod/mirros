#ifndef _device_drive_h
#define _device_drive_h

//	���ݾ�����������޸ġ�
extern FSBOOL sd_read_n_sector(FSU32 sector,char *buffer,int n);


extern FSBOOL sd_write_n_sector(FSU32 sector,char *buffer,int n);
		
extern FSBOOL sd_read_one_sector(FSU32 sector,char *buffer);			//�豸��ȡһ��������

extern FSBOOL sd_write_one_sector(FSU32 sector,char *buffer);			//�豸дһ��������

extern void get_device_drive(void);	

#define read_sector (sd.read_one_sector)			//��ȡһ��������
#define write_sector (sd.write_one_sector)			//дһ��������

#define read_nsector (sd.read_n_sector)			//��ȡn��������
#define write_nsector (sd.write_n_sector)			//дN��������


#endif

