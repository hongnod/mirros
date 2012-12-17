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
		if(file->cur_rw_in_buffer == BLOCK_SIZE)			//���дһ���¿鲻�ɹ��Ļ�����0
		{
			if(!write_new_block(file))
			{
				return n-1;
			}
			file->cur_rw_in_buffer=0;
			write_sector(file->node_sector,file->node_buffer);		//���½ڵ���Ϣ��
		}
		file->file_rw_buffer[file->cur_rw_in_buffer++]=*buffer++;
		file->file_size++;
	}		
	//д��֮����½ڵ���Ϣ��
	return n;
}

FSBOOL write_new_block(PLMFILE file)
{
	PNODE_INFO ptr;
	FSU32 block;

	ptr=(PNODE_INFO)(&file->node_buffer[file->node_pos*NODE_INFO_SIZE]);
	if(file->block_alloc == 1)			//����ļ�������һ���飬˵���ǵ�һ��д��
	{
		write_block(file->level0_block[0],file->file_rw_buffer);
		ptr->file_size+=file->cur_rw_in_buffer;			//�����ļ��Ĵ�С
	}
	else		//���ǵ�һ��д�룬�ͷ���һ���¿顣
	{
		if( (block=alloc_new_block(file,ptr)) == 0)			//�����ʱ���·���Ŀ���Ϣ��
		{
			return 0;
		}
		write_block(block,file->file_rw_buffer);
		ptr->file_size+=file->cur_rw_in_buffer;
		file->block_alloc+=1;			//�����ļ�����Ŀ顣
	}
	return 1;
}




