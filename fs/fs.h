#ifndef _FS_H
#define _FS_H

#include <os/types.h>

#define SECTOR_SIZE		512

#define WRITE			1
#define	READ			2
#define ADD			4
#define RW			8

#define O_FILE			0x1
#define DIR			0x2
#define DEV			0x4
#define BINARY			0x8
#define READ_ONLY		0x10		
#define HIDE			0X20
#define SYSTEM			0X40
#define UNUSED			0X80

struct file{
	u32 node_number;
	u32 block_alloc;
	u32 file_size;
	u32 cur_rw_block;
	u32 cur_rw_in_buffer;
	u32 level0_block[7];
	u32 level1_block;
	u32 level2_block;
	u32 node_sector;
	u32 node_sector;
	u32 leave_size;

	char *buffer;
	char *file_rw_buffer;
	char *level1_buffer;
	char *level2_buffer;
	char *node_buffer;

	char open_mode;
};

struct _node{
	char name[256];
	char file_type;
	unsigned char file_attrib;
	unsigned char unused;
	u32 file_amount;
	u32 block_alloc;
	u32 file_size;
	u32 pos_in_sector;
	u32 sector;
	u32 last_modify_time;
	u32 level0_block[7];
	u32 level1_block;
	u32 level2_block;
};

/*
 *node information of this file a node need 512 bytes
 *in partition
 */
struct node{
	union{
		struct _node file_node;
		char buf[512];
	}
};

/*
 *usually the first sector of the partition will
 *store the superblock of this partition
 */
struct super_block{
	u32 free_block_first_sector;
	u32 super_block_sector;
	u32 partition_first_sector;
	u32 fs_end_sector;
	unsigned char fs_type;
	char fs_name[7];
	char fs_version;
	char media_type;
	char block_per_sector;
	u32 free_section_total_sector;
	u32 partition_total_sector;
	u32 partition_sector_size;
	u32 free_block_cur_sector;
	u32 free_block_leave;
	u32 data_sector_first_block;

	/*
	 *super block need tell the system some information 
	 *of the root dir
	 */
	struct node root;
};

struct lba_partition_entry{
	char state;
	unsigned char start_chs[3];
	char fs_id;				/*for lmfs is 0x88*/
	unsigned char end_chs[3];
	u32 base_sector;			/*base sectors of this partition; */
	u32 total_sectors;			/*size of partition in sectors*/
}

#define SECTOR_PER_BLOCK	(BLOCK_SIZE/SECTOR_PER_SIZE)
#define FILE_PER_BLOCK		(BLOCK_SIZE/DIR_SIZE)
#define NODE_PER_SECTOR		(SECTOR_PER_SIZE/sizeof(struct node_info))
#define FILE_PER_SECTOR		(SECTOR_PER_SIZE/DIR_SIZE)
#define DIR_SIZE		(MAX_FILE_NAME+sizeof(u32))
#define NODE_INFO_SIZE		(sizeof(struct node_info))
#define FREE_PER_SECTOR		(SECTOR_PER_SIZE/(sizeof(u32)))
#define BLOCK_PER_BLOCK		(BLOCK_SIZE/sizeof(u32))

#endif
