#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include"lmfs.h"
#include"lmfs_config.h"
#include"device_drive.h"




FSU32 read_file(char *buffer,FSU32 size,PLMFILE file)			//��ȡ�ļ���size���ֽڵ����ݡ�
{
	FSU32 n=0;
	if(file->open_mode !=READ) return 0;
	while(n < size)
	{
		n++;
		if(file->leave_size == 0)			//����ļ��������ļ��Ķ�����
		{
			break;
		}
		if(file->cur_rw_in_buffer == BLOCK_SIZE)
		{
			if(!read_new_block(file)) break;
		}
		*buffer++=file->file_rw_buffer[file->cur_rw_in_buffer++];
		file->leave_size--;
	}
	return (n-1);
}

char read_new_block(PLMFILE file)			//��ȡһ���¿飬�������ļ������Ϣ��
{
	FSU32 *ptr;
	FSU32 *ptr1;
	if(file->cur_rw_block == file->block_alloc)			//�Ѿ����������еĿ顣ֱ�ӷ��ء�
	{
		return 0;
	}
	file->cur_rw_block++;

	file->cur_rw_in_buffer=0;

	if(file->cur_rw_block < 8)
	{
		read_block(file->level0_block[file->cur_rw_block-1],file->file_rw_buffer);
	}
	else if(file->cur_rw_block < 1032)
	{
		ptr=(FSU32 *)file->level1_buffer;
		read_block(ptr[file->cur_rw_block-7-1],file->file_rw_buffer);
	}
	else
	{
		ptr1=(FSU32 *)file->level2_buffer;
		ptr=(FSU32 *)file->level1_buffer;
		if((file->cur_rw_block-1031)%1024 == 0)		//Ҫ��ȡһ���µĶ�����
		{
			read_block(ptr1[(file->cur_rw_block-1032)/1024],file->level1_buffer);
			read_block(ptr[0],file->file_rw_buffer);
		}
		else				//����Ļ��ڵ�ǰһ�����ж�ȡ��
		{
			read_block(ptr[(file->cur_rw_in_buffer-1032)%1024],file->file_rw_buffer);
		}
	}
	return 1;
}



