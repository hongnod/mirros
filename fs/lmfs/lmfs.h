#ifndef _lmfs_h
#define _lmfs_h

#include"lmfs_config.h"

#define SECTOR_PER_BLOCK (BLOCK_SIZE/SECTOR_PER_SIZE)
#define FILE_PER_BLOCK (BLOCK_SIZE/DIR_SIZE)
#define NODE_PER_SECTOR (SECTOR_PER_SIZE/64)
#define FILE_PER_SECTOR (SECTOR_PER_SIZE/DIR_SIZE)
#define DIR_SIZE (MAX_FILE_NAME+sizeof(FSU32))
#define NODE_INFO_SIZE (64)
#define FREE_PER_SECTOR (SECTOR_PER_SIZE/(sizeof(FSU32)))
#define BLOCK_PER_BLOCK (BLOCK_SIZE/sizeof(FSU32))

#define WRITE 1
#define READ 2
#define ADD 4
#define READ_WRITE 8


#define TIME 0

//define some attrib of file

//
#define O_FILE 0x1
#define DIR 0x2
#define DEV 0x4
#define BINARY 0x8
#define READ_ONLY 0x10		
#define HIDE 0X20
#define SYSTEM 0X40
#define UNUSED 0X80



//#define NODE FSU32
typedef FSU32 NODE,*PNODE;
#define SUCCESS 1
#define UNSUCCESS 0

#define TRUE 1
#define FALSE 0 

typedef struct _disk_drive		//�豸����������
{
	FSBOOL (*read_one_sector)(FSU32 sector,char *buffer);		//��һ��������
	FSBOOL (*write_one_sector)(FSU32 sector,char *buffer);		//дһ��������
	FSBOOL (*read_n_sector)(FSU32 sector,char *buffer,int n);		//��N��������
	FSBOOL (*write_n_sector)(FSU32 sector,char *buffer,int n);		//дN��������
	FSU32 sector_per_size;									//һ�������Ĵ�С
	FSU32 disk_total_sector;									//����������			
}DISK_DRIVE,*PDISK_DRIVE;
 
typedef struct _file
{
	FSU32 node_number;			//�ļ��Ľڵ��
	FSU32 block_alloc;			//�ļ������˶��ٿ�
	FSU32 file_size;			//�ļ��Ĵ�С
	FSU32 cur_rw_block;			//�ļ����ڶ��Ŀ�
	FSU32 cur_rw_in_buffer;			//�ļ����ڶ�ȡ��λ��
	FSU32 level0_block[7];			//�ļ���ǰ�߿�
	FSU32 level1_block;				//�ļ���һ����
	FSU32 level2_block;				//�ļ��Ķ�����
	FSU32 node_sector;				//�ļ��ڵ����ڵ�����
	FSU32 node_pos;					//�ļ��ڵ��������е�λ��
	FSU32 leave_size;				// �ļ���û�ж�ȡ�Ĵ�С��
	char *buffer;			//���л���33*512B��Ϊ�ļ�����Ļ�������
	char *file_rw_buffer;			//������
	char *level1_buffer;				//һ���黺��
	char *level2_buffer;				//�����黺��
	char *node_buffer;					//�ڵ㻺����
//	char *other_buffer;
	char open_mode;					//�򿪷�ʽ��
}LMFILE,*PLMFILE;

typedef struct _super_block				// ��¼���������Ҫ��Ϣ���Ǽ�����ز�������Ҫ������
{
	FSU32 jmp_code;							//��ת��
	FSU32 data_section_first_sector;		//�������ĵ�һ������
	FSU32 node_section_first_sector;		//�ڵ����ĵ�һ������
	FSU32 free_block_first_sector;		//���п�ĵ�һ������
	FSU32 super_block_first_sector;		//������ĵ�һ������
	FSU32 fs_fisrt_sector;
	FSU32 fs_end_sector;					//����Ϣ��д�볬����ʱ ��˳
	unsigned char fs_type;						//0X88;�ļ�ϵͳ��ʶ�š�
	char fs_name[7];						//�ļ�ϵͳ������
	char fs_version;						//�ļ�ϵͳ�İ汾
	char reserve;							//�ļ�ϵͳ�İ汾��				
	char media_type;						//�豸�Ľ������͡�
	char block_per_sector;					//�ļ�ϵͳ��һ������ռ����������
	FSU32 data_section_total_sector;			//������������������
	FSU32 node_section_total_sector;			//�ڵ���������������
	FSU32 free_section_total_sector;			//���п���ռ������������
	FSU32 device_total_sector;					//�豸��������������
	FSU32 device_sector_size;					//�豸ÿ�������Ĵ�С��
	FSU32 free_block_cur_sector;			//�������ĵ�ǰ����
	FSU32 free_block_cur_pos;				//���п��ڵ�ǰ�����е�λ�ã�ָ��ǰ���õķ���ţ����
	FSU32 free_block_leave;					//ÿ����һ������������ͼ�1��										//ȫ�ֱ���free_posʹ�á�
	FSU16 super_block_size;					//��������ռ�Ĵ�С��
	char data_section_first_block;		//1	//����������ſ�ʼ�㣬ǰ��Ķ������ڷ��������ݡ�
}SUPER_BLOCK,*PSUPER_BLOCK;

typedef struct _mbr_partion			//��¼��������������Ϣ��������Ϣ��
{
	unsigned char boot_ind;				//�����Ƿ����ö��
	unsigned char begin_head;			//�豸��ʼ��ͷ
	FSU16 begin_sector;					//������ʼ������
	unsigned char partion_type;			//�������͡�
	unsigned char end_head;
	FSU16 end_sector;					//��������������
	FSU32 begin_ab_sector;			//����������ʼ������
	FSU32 total_sector;				//�����ܹ�ռ�õ�������

}MBR_PARTION,*PMBR_PARTION;				//��Ӳ���豸û������������

typedef struct _node_info						//�ڵ���Ϣ��ռ64�ֽڡ�
{
	char used;								//�ڵ��Ƿ�ռ�á�
	char file_type;							//�ļ������͡�
	unsigned char file_attrib;				//�ļ�������
	unsigned char unused;					//����
	FSU32 file_amount;						//Ŀ¼�Ļ���Ŀ¼���ж��ٸ��ļ���
	FSU32 block_alloc;						//�ļ������˶��ٸ��顣
	FSU32 file_size;						//�ļ��Ĵ�С��
	FSU32 pos_in_sector;					//�ļ��ڵ�������������λ�á�
	FSU32 sector;							//�ڵ����ڵ�������
	FSU32 last_modify_time;					//�ļ����һ���޸ĵ�ʱ�䡣
	FSU32 level0_block[7];					//�ļ������7��0����
	FSU32 level1_block;						//�ļ������һ�������ڵĿ�š�
	FSU32 level2_block;						//�ļ����������ڵĿ��
}NODE_INFO,*PNODE_INFO;

typedef struct _catalog_info			//Ŀ¼����Ϣռ��64�ֽ�
{
	FSU32 node;							//�ļ����ڵĽڵ㡣
	char name[60];						//�ļ������֡�
}CATALOG_INFO,*PCATALOG_INFO;

//ȫ�ֱ������塣

unsigned char _write_buffer[SECTOR_PER_SIZE];			//����Ӧ�ó�����Ϊ�м�洢�ռ�ʹ�á�

unsigned char _super_block_buffer[SECTOR_PER_SIZE];			//�洢super_block_�����ݲ������������;��

unsigned char _free_block_buffer[SECTOR_PER_SIZE];			//���п�Ķ�ջ����

PSUPER_BLOCK sp;		//�������ָ�룬����ġ�

DISK_DRIVE sd;			//�豸��������

NODE_INFO cur_dir_node;			//��ǰ��Ŀ¼�Ľڵ㡣Ĭ���Ǹ�Ŀ¼��ָ��ǰĿ¼����λ���ǵ�ǰĿ¼��CD��������ı�����

NODE_INFO root_dir_node;		//ָ���Ŀ¼����Ϣ����䡣�ڵ���Ϣ

FSU32 *free_pos;				//ָ��free-block�ĵ�ǰλ�á�

///////////////////////////////////////

extern FSBOOL init_fmls(void);						//��ʼ���ļ�ϵͳ��

extern FSBOOL format_device(void);					//��ʽ���豸��

extern PLMFILE open_file(char *name,char mode);		//��һ���ļ���

extern NODE search_file(PLMFILE file,char *name,PNODE_INFO info,FSU32 *position,FSU32 *file_count);			//�����ļ���

extern NODE_INFO get_node_info(FSU32 node_number,char *buffer);		//��ýڵ���Ϣ��

extern void get_node_pos(int *x,int *y,FSU32 node_number);			//��ýڵ��λ�á�

extern char *lm_strtok(char *str,char ch);				//�����ַ����е�һ���ض��ַ���

extern NODE search_file_from_node(PLMFILE file,char *name,PNODE_INFO info,FSU32 *position,FSU32 *file_count);			//�ڽڵ���Ϣ�������ļ���

extern FSBOOL check_filename(char *name);			//����ļ����Ƿ�Ϸ���

extern NODE search_in_level0(PLMFILE file,char *name,PNODE_INFO info,FSU32 *position,FSU32 *file_count);		//��0������������

extern NODE search_in_level1(PLMFILE file,char *name,PNODE_INFO info,FSU32 *position,FSU32 *file_count);		//��һ������������

extern NODE search_in_level2(PLMFILE file,char *name,PNODE_INFO info,FSU32 *position,FSU32 *file_count);		//�ڶ�������������

extern void read_block(FSU32 block,char *buffer);		//��ȡһ���顣

extern PLMFILE open_file_write(char *name,char mode);		//һЩ��ʽ���ļ���

extern PLMFILE open_file_read(char *name,char mode);		//�Զ���ʽ���ļ���

extern char *lm_strrtok(char *str,char ch);					//�����ַ������ض��ַ������ĵ�λ�á�

extern char close_file(PLMFILE tmp_file);			//һ���Ǿ�����̬������ڴ档

extern PLMFILE create_tmp_file(void);				//������ʱ�ļ���

extern void get_file_info(PLMFILE tmp_file,PNODE_INFO node_tmp,FSU32 file_node,char mode);			//����ļ���Ϣ��

extern FSU32 get_first_sector(FSU32 block);			//���һ����ĵ�һ��������

extern void write_block(FSU32 block,char *buffer);		//дһ��������

extern PLMFILE create_file(char *name,char attrib);			//����һ���ļ���

extern NODE pop_free(void);			//���п��ջ

extern void update_free(void);		//���¿��п�

extern FSU32 alloc_new_block(PLMFILE file,PNODE_INFO ptr);		//����һ���¿顣

extern FSU32 get_last_file_pos(PLMFILE file,PNODE_INFO info);			//���Ŀ¼�����һ���ļ����ڵĿ顣

extern NODE lm_create_file(PLMFILE file,char *name,PNODE_INFO info,unsigned char attrib,FSU32 pos);			//�����ļ���ͨ�ú�����

extern FSBOOL delete_file(char *name);		//ɾ���ļ���

extern void push_free(FSU32 block);			//���п�ѹջ

extern void update_dir_info(PLMFILE tmp_file,PNODE_INFO dir_up,FSU32 block);			//����Ŀ¼��Ϣ��

extern FSU32 get_last_file_info(PLMFILE tmp_file,PNODE_INFO dir_up,PCATALOG_INFO last_file);		//���Ŀ¼�����һ���ļ�����Ϣ��

extern FSU32 get_file_block(PLMFILE tmp_file,FSU32 file_count,PNODE_INFO dir_up);			//����ļ����ڵĿ��

extern FSU32 block_in_table(PLMFILE tmp_file,FSU32 i,PNODE_INFO dir_up);			//�ڱ��в����ļ����ڵĿ顣

extern void release_file_block(PLMFILE tmp_file,PNODE_INFO dir_up);			//�ͷ��ļ�����Ŀ顣

extern void release_level0(PLMFILE tmp_file,PNODE_INFO dir_up);		//�ͷ�0����

extern void release_level1(PLMFILE tmp_file,PNODE_INFO dir_up);		//�ͷ�1����

extern void release_level2(PLMFILE tmp_file,PNODE_INFO dir_up);		//�ͷ�2����


extern FSU32 read_file(char *buffer,FSU32 size,PLMFILE file);			//��ȡ�ļ���

extern void write_super_block_data(void);			//���³���������ݡ�

extern void init_free_section(FSU32 start_sector,FSU32 free_block,FSU32 total_data_sector);		//��ʼ�����п��

extern void init_node_section(FSU32 start_sector,FSU32 node_block,FSU32 total_data_sector);		//��ʼ���ڵ��

extern FSBOOL sd_read_one_sector(FSU32 sector,char *buffer);			//�豸��ȡһ��������

extern FSBOOL sd_write_one_sector(FSU32 sector,char *buffer);			//�豸дһ��������

extern FSBOOL write_new_block(PLMFILE file);							//д�ļ���дһ���µĿ顣

extern FSU32 write_file(char *buffer,FSU32 size,PLMFILE file);			//д�ļ�SIZE���ֽڡ�

extern char read_new_block(PLMFILE file);				//��ȡһ���¿顣

#endif




