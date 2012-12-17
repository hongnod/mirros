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

typedef struct _disk_drive		//设备的驱动程序
{
	FSBOOL (*read_one_sector)(FSU32 sector,char *buffer);		//读一个扇区；
	FSBOOL (*write_one_sector)(FSU32 sector,char *buffer);		//写一个扇区；
	FSBOOL (*read_n_sector)(FSU32 sector,char *buffer,int n);		//读N个扇区；
	FSBOOL (*write_n_sector)(FSU32 sector,char *buffer,int n);		//写N个扇区；
	FSU32 sector_per_size;									//一个扇区的大小
	FSU32 disk_total_sector;									//总扇区数。			
}DISK_DRIVE,*PDISK_DRIVE;
 
typedef struct _file
{
	FSU32 node_number;			//文件的节点号
	FSU32 block_alloc;			//文件分配了多少块
	FSU32 file_size;			//文件的大小
	FSU32 cur_rw_block;			//文件现在读的块
	FSU32 cur_rw_in_buffer;			//文件现在读取的位置
	FSU32 level0_block[7];			//文件的前七块
	FSU32 level1_block;				//文件的一级块
	FSU32 level2_block;				//文件的二级块
	FSU32 node_sector;				//文件节点所在的扇区
	FSU32 node_pos;					//文件节点在扇区中的位置
	FSU32 leave_size;				// 文件还没有读取的大小。
	char *buffer;			//所有缓冲33*512B，为文件分配的缓冲区。
	char *file_rw_buffer;			//读缓冲
	char *level1_buffer;				//一级块缓冲
	char *level2_buffer;				//二级块缓冲
	char *node_buffer;					//节点缓冲区
//	char *other_buffer;
	char open_mode;					//打开方式。
}LMFILE,*PLMFILE;

typedef struct _super_block				// 记录超级快的重要信息，是计算相关参数的重要参数。
{
	FSU32 jmp_code;							//跳转码
	FSU32 data_section_first_sector;		//数据区的第一个扇区
	FSU32 node_section_first_sector;		//节点区的第一个扇区
	FSU32 free_block_first_sector;		//空闲块的第一个扇区
	FSU32 super_block_first_sector;		//超级快的第一个扇区
	FSU32 fs_fisrt_sector;
	FSU32 fs_end_sector;					//此信息在写入超级快时 按顺
	unsigned char fs_type;						//0X88;文件系统标识号。
	char fs_name[7];						//文件系统的名称
	char fs_version;						//文件系统的版本
	char reserve;							//文件系统的版本。				
	char media_type;						//设备的介质类型。
	char block_per_sector;					//文件系统中一个块所占的扇区数。
	FSU32 data_section_total_sector;			//数据区的总扇区数。
	FSU32 node_section_total_sector;			//节点区的总扇区数。
	FSU32 free_section_total_sector;			//空闲块所占的总扇区数。
	FSU32 device_total_sector;					//设备的总扇区数，。
	FSU32 device_sector_size;					//设备每个扇区的大小。
	FSU32 free_block_cur_sector;			//空闲区的当前扇区
	FSU32 free_block_cur_pos;				//空闲块在当前扇区中的位置，指向当前可用的分配号，配合
	FSU32 free_block_leave;					//每分配一个块这个变量就减1；										//全局变量free_pos使用。
	FSU16 super_block_size;					//超级块所占的大小。
	char data_section_first_block;		//1	//从数据区块才开始算，前面的都不属于分区的内容。
}SUPER_BLOCK,*PSUPER_BLOCK;

typedef struct _mbr_partion			//记录着启动扇区的信息，分区信息。
{
	unsigned char boot_ind;				//分区是否可以枚举
	unsigned char begin_head;			//设备起始磁头
	FSU16 begin_sector;					//分区起始扇区。
	unsigned char partion_type;			//分区类型。
	unsigned char end_head;
	FSU16 end_sector;					//分区结束扇区。
	FSU32 begin_ab_sector;			//分区绝对起始扇区。
	FSU32 total_sector;				//分区总共占用的扇区。

}MBR_PARTION,*PMBR_PARTION;				//非硬盘设备没有启动分区。

typedef struct _node_info						//节点信息。占64字节。
{
	char used;								//节点是否被占用。
	char file_type;							//文件的类型。
	unsigned char file_attrib;				//文件的属性
	unsigned char unused;					//保留
	FSU32 file_amount;						//目录的话，目录中有多少个文件。
	FSU32 block_alloc;						//文件分配了多少个块。
	FSU32 file_size;						//文件的大小。
	FSU32 pos_in_sector;					//文件节点在所在扇区的位置。
	FSU32 sector;							//节点所在的扇区。
	FSU32 last_modify_time;					//文件最后一次修改的时间。
	FSU32 level0_block[7];					//文件分配的7个0级块
	FSU32 level1_block;						//文件分配的一级块所在的块号。
	FSU32 level2_block;						//文件二级块所在的块号
}NODE_INFO,*PNODE_INFO;

typedef struct _catalog_info			//目录项信息占据64字节
{
	FSU32 node;							//文件所在的节点。
	char name[60];						//文件的名字。
}CATALOG_INFO,*PCATALOG_INFO;

//全局变量定义。

unsigned char _write_buffer[SECTOR_PER_SIZE];			//留给应用程序作为中间存储空间使用。

unsigned char _super_block_buffer[SECTOR_PER_SIZE];			//存储super_block_的内容不能用作别的用途。

unsigned char _free_block_buffer[SECTOR_PER_SIZE];			//空闲块的堆栈区。

PSUPER_BLOCK sp;		//超几块的指针，不变的。

DISK_DRIVE sd;			//设备驱动程序

NODE_INFO cur_dir_node;			//当前的目录的节点。默认是根目录。指向当前目录，复位后是当前目录。CD操作将会改变它。

NODE_INFO root_dir_node;		//指向根目录的信息不会变。节点信息

FSU32 *free_pos;				//指向free-block的当前位置。

///////////////////////////////////////

extern FSBOOL init_fmls(void);						//初始化文件系统。

extern FSBOOL format_device(void);					//格式化设备。

extern PLMFILE open_file(char *name,char mode);		//打开一个文件。

extern NODE search_file(PLMFILE file,char *name,PNODE_INFO info,FSU32 *position,FSU32 *file_count);			//搜索文件。

extern NODE_INFO get_node_info(FSU32 node_number,char *buffer);		//获得节点信息。

extern void get_node_pos(int *x,int *y,FSU32 node_number);			//获得节点的位置。

extern char *lm_strtok(char *str,char ch);				//查找字符串中的一个特定字符。

extern NODE search_file_from_node(PLMFILE file,char *name,PNODE_INFO info,FSU32 *position,FSU32 *file_count);			//在节点信息中搜索文件。

extern FSBOOL check_filename(char *name);			//检测文件名是否合法。

extern NODE search_in_level0(PLMFILE file,char *name,PNODE_INFO info,FSU32 *position,FSU32 *file_count);		//在0级块中搜索。

extern NODE search_in_level1(PLMFILE file,char *name,PNODE_INFO info,FSU32 *position,FSU32 *file_count);		//在一级块中搜索。

extern NODE search_in_level2(PLMFILE file,char *name,PNODE_INFO info,FSU32 *position,FSU32 *file_count);		//在二级块中搜索。

extern void read_block(FSU32 block,char *buffer);		//读取一个块。

extern PLMFILE open_file_write(char *name,char mode);		//一些方式打开文件。

extern PLMFILE open_file_read(char *name,char mode);		//以读方式打开文件。

extern char *lm_strrtok(char *str,char ch);					//查找字符串中特定字符最后初夏的位置。

extern char close_file(PLMFILE tmp_file);			//一定是经过动态分配的内存。

extern PLMFILE create_tmp_file(void);				//创建临时文件。

extern void get_file_info(PLMFILE tmp_file,PNODE_INFO node_tmp,FSU32 file_node,char mode);			//获得文件信息。

extern FSU32 get_first_sector(FSU32 block);			//获得一个块的第一个扇区。

extern void write_block(FSU32 block,char *buffer);		//写一个扇区。

extern PLMFILE create_file(char *name,char attrib);			//创建一个文件。

extern NODE pop_free(void);			//空闲块出栈

extern void update_free(void);		//更新空闲块

extern FSU32 alloc_new_block(PLMFILE file,PNODE_INFO ptr);		//分配一个新块。

extern FSU32 get_last_file_pos(PLMFILE file,PNODE_INFO info);			//获得目录中最后一个文件所在的块。

extern NODE lm_create_file(PLMFILE file,char *name,PNODE_INFO info,unsigned char attrib,FSU32 pos);			//创建文件的通用函数。

extern FSBOOL delete_file(char *name);		//删除文件。

extern void push_free(FSU32 block);			//空闲块压栈

extern void update_dir_info(PLMFILE tmp_file,PNODE_INFO dir_up,FSU32 block);			//更新目录信息。

extern FSU32 get_last_file_info(PLMFILE tmp_file,PNODE_INFO dir_up,PCATALOG_INFO last_file);		//获得目录中最后一个文件的信息。

extern FSU32 get_file_block(PLMFILE tmp_file,FSU32 file_count,PNODE_INFO dir_up);			//获得文件所在的块号

extern FSU32 block_in_table(PLMFILE tmp_file,FSU32 i,PNODE_INFO dir_up);			//在表中查找文件所在的块。

extern void release_file_block(PLMFILE tmp_file,PNODE_INFO dir_up);			//释放文件分配的块。

extern void release_level0(PLMFILE tmp_file,PNODE_INFO dir_up);		//释放0级块

extern void release_level1(PLMFILE tmp_file,PNODE_INFO dir_up);		//释放1级块

extern void release_level2(PLMFILE tmp_file,PNODE_INFO dir_up);		//释放2级块


extern FSU32 read_file(char *buffer,FSU32 size,PLMFILE file);			//读取文件。

extern void write_super_block_data(void);			//更新超级块的内容。

extern void init_free_section(FSU32 start_sector,FSU32 free_block,FSU32 total_data_sector);		//初始化空闲块表。

extern void init_node_section(FSU32 start_sector,FSU32 node_block,FSU32 total_data_sector);		//初始化节点表。

extern FSBOOL sd_read_one_sector(FSU32 sector,char *buffer);			//设备读取一个扇区。

extern FSBOOL sd_write_one_sector(FSU32 sector,char *buffer);			//设备写一个扇区。

extern FSBOOL write_new_block(PLMFILE file);							//写文件。写一个新的块。

extern FSU32 write_file(char *buffer,FSU32 size,PLMFILE file);			//写文件SIZE个字节。

extern char read_new_block(PLMFILE file);				//读取一个新块。

#endif




