#ifndef _lmfs_config_h
#define _lmfs_config_h

typedef unsigned int FSU32;
typedef char FSBOOL;
typedef unsigned short FSU16;
typedef unsigned char FSU8;
typedef int FSS32;
typedef short FSS16;
typedef char FSS8;

#define LMFS_AUTO_FORMAT (1)			//�������û�и�ʽ���Ƿ��Զ���ʽ�����̡�

#define BLOCK_SIZE (4096)			//������Ĵ�С��������SECTOR_PER_SIZE��ż����

#define OS_SUPPORT (0)			//�Ƿ�֧�ֲ���ϵͳ��	//�Ժ�ʵ�֡�

#define SECTOR_PER_SIZE (512)	    // ������С��

#define DISK_TOTAL_SECTOR (20*1024)

#define PARTION_FIRST_SECTOR (0)			//�ļ�ϵͳ��ʼ������

#define MAX_FILE_NAME (60)		//�ļ�������󳤶ȡ�����sizeof��FSU32�����뱻512����

#endif



