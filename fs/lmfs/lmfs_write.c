#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include"lmfs.h"
#include"lmfs_config.h"
#include"device_drive.h"




FSU32 write_file(char *buffer,FSU32 size,PLMFILE file)
{
	FSU32 n=0;
	if(file->open_mode != WRITE) return 0;
	while(n < size)
	{
		n++;
		if(file->cur_rw_in_buffer == BLOCK_SIZE)			//如果写一个新块不成功的话返回0
		{
			if(!write_new_block(file))
			{
				return n-1;
			}
			file->cur_rw_in_buffer=0;
			write_sector(file->node_sector,file->node_buffer);		//更新节点信息。
		}
		file->file_rw_buffer[file->cur_rw_in_buffer++]=*buffer++;
		file->file_size++;
	}		
	//写完之后更新节点信息。
	return n;
}

FSBOOL write_new_block(PLMFILE file)
{
	PNODE_INFO ptr;
	FSU32 block;

	ptr=(PNODE_INFO)(&file->node_buffer[file->node_pos*NODE_INFO_SIZE]);
	if(file->block_alloc == 1)			//如果文件分配了一个块，说明是第一次写入
	{
		write_block(file->level0_block[0],file->file_rw_buffer);
		ptr->file_size+=file->cur_rw_in_buffer;			//增加文件的大小
	}
	else		//不是第一次写入，就分配一个新块。
	{
		if( (block=alloc_new_block(file,ptr)) == 0)			//分配块时更新分配的块信息。
		{
			return 0;
		}
		write_block(block,file->file_rw_buffer);
		ptr->file_size+=file->cur_rw_in_buffer;
		file->block_alloc+=1;			//增加文件分配的块。
	}
	return 1;
}




