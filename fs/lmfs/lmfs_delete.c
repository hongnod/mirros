#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include"lmfs_config.h"
#include"lmfs.h"
#include"device_drive.h"

//void storage_filename(char *path,char *name);			//�˺����������Ǵ����ļ�����
														//����ֵʱ0��1��2
							//0��ʾ�ڸ�Ŀ¼�½���������1��ʾ�Ӹ�Ŀ¼��������2��ʾ�ڵ�ǰĿ¼�½���������



//�����Ĺ����Ƿ����ļ����ڵĽڵ㣬��һ��Ŀ¼�Ľڵ���Ϣ����dirup�С�
//NODE get_dir_node(PLMFILE file,char *name,PNODE_INFO info,FSU32 *position,FSU32 *file_count);


FSBOOL delete_file(char *name)
{
	NODE file_node=0;

	PLMFILE tmp_file;

	NODE_INFO dir_up; 

	FSU32 i,tmp;

	FSU32 block;

	FSU32 pos;

	CATALOG_INFO last_file;

	PCATALOG_INFO ptr;

	char tmp_name[60];

	char *token_pos;

	if(!check_filename(name))
	{
		return UNSUCCESS;
	}

	tmp_file=create_tmp_file();

	token_pos=lm_strrtok(name,'/');	//�洢�ļ����ļ�����
	strcpy(tmp_name,token_pos);

	if(*name == '/')			//�����ļ�����Ŀ¼�Ľڵ�š�
	{
		name++;

		dir_up=get_node_info(0,tmp_file->node_buffer);

		if(*(lm_strtok(name,'/')) == 0)
		{
			file_node=0;
		}
		else
		{
			token_pos--;
			*token_pos=0;
			file_node=search_file(tmp_file,name,&dir_up,&pos,&i);
			if(file_node == 0)
			{
				close_file(tmp_file);
				return UNSUCCESS;
			}

			dir_up=get_node_info(file_node,tmp_file->node_buffer);

			if(dir_up.file_type != DIR)
			{
				close_file(tmp_file);
				return UNSUCCESS;
			}
		}
	}
	else
	{
		dir_up=get_node_info(cur_dir_node.level0_block[0],tmp_file->node_buffer);
		file_node=cur_dir_node.level0_block[0];
	}
//�������ҵ����ļ����ڵ�Ŀ¼�����ļ�Ŀ¼�Ľڵ���Ϣ�洢��dirup�С�
	//Ѱ��Ŀ¼�����һ���ļ�����Ϣ��
	file_node=search_file(tmp_file,tmp_name,&dir_up,&pos,&i);
	if(file_node == 0)
	{
		close_file(tmp_file);
		return UNSUCCESS;
	}

	block=get_last_file_info(tmp_file,&dir_up,&last_file);			//���Ŀ¼���һ���ļ�����Ϣ�����������һ���顣

	if( i != dir_up.file_amount)
	{
		ptr=(PCATALOG_INFO)tmp_file->file_rw_buffer;
		ptr+=pos-1;
		ptr->node=last_file.node;
		strcpy(ptr->name,last_file.name);			//�ѵ�ǰ�ļ�����Ϣ�滻�����һ���ļ�����Ϣ��

		i=get_file_block(tmp_file,i,&dir_up);
		tmp=(pos*DIR_SIZE)/SECTOR_PER_SIZE;
		i=get_first_sector(i);			//�����ļ�����Ϣ
		write_sector(i+tmp,&(tmp_file->file_rw_buffer[tmp*SECTOR_PER_SIZE]));
	}


	update_dir_info(tmp_file,&dir_up,block);		//�����ļ�Ŀ¼��Ϣ��
	//�ͷ��ļ������ݿ�.

	dir_up=get_node_info(file_node,tmp_file->node_buffer);

	release_file_block(tmp_file,&dir_up);

	update_free();		//���¿��п�ͳ����顣

	close_file(tmp_file);

	return 1;
}

FSU32 get_last_file_info(PLMFILE tmp_file,PNODE_INFO dir_up,PCATALOG_INFO last_file)		//�ҵ�Ŀ¼�����һ���ļ�����Ϣ��
{																							//�������һ���ļ����ڵĿ顣
	FSU32 i;
	FSU32 pos;
	PCATALOG_INFO info;

	i=dir_up->block_alloc;

	pos=block_in_table(tmp_file,i,dir_up);		//�õ��ļ����ڵĿ顣

	i=dir_up->file_amount;
	
	i=(i%FILE_PER_BLOCK)-1;			//�ҵ��ļ��ڿ��е�λ�á�

	read_block(pos,tmp_file->level2_buffer);	//��ȡ�ÿ�

	info=(PCATALOG_INFO)(&tmp_file->level2_buffer[i*DIR_SIZE]);	//����ļ�Ŀ¼��Ϣ��

	last_file->node=info->node;

	strcpy(last_file->name,info->name);			//������һ���ļ�����Ϣ��

	return pos;
}

void update_dir_info(PLMFILE tmp_file,PNODE_INFO dir_up,FSU32 block)		// dir_up���ڵ�����������tmp_file->node_buffer�У���
{		//����Ŀ¼��Ϣ��
	PNODE_INFO ptr;
	ptr=(PNODE_INFO)tmp_file->node_buffer;
	ptr+=dir_up->pos_in_sector;
	ptr->file_amount--;
	if(((ptr->file_amount)%FILE_PER_BLOCK) == 0)			//�ѷ�������һ���鵯����
	{
		push_free(block);		//���Ҫɾ�����ļ����ڵĿ�ֻ��һ���ļ�����
		ptr->block_alloc--;
	}
	write_sector(dir_up->sector,tmp_file->node_buffer);	//����Ŀ¼�ڵ���Ϣ��
}


void push_free(FSU32 block)			//�黹һ�����п顣
{
	if(sp->free_block_cur_pos == 0)		//�����ǰ���п�������������
	{
		write_sector(sp->free_block_cur_sector,_free_block_buffer);	//���������Ϣ��
		sp->free_block_cur_sector--;
		sp->free_block_cur_pos=SECTOR_PER_SIZE/sizeof(FSU32);
		if((sp->free_block_cur_sector) < (sp->free_block_first_sector))		//������˱��ͷ�������ء�
		{
			return;
		}
		read_sector(sp->free_block_cur_sector,_free_block_buffer);		//��ȡ��һ��Ŀ��п��б�
		free_pos=(FSU32 *)_free_block_buffer;	//ָ��ָ����п��ͷ����
		free_pos+=FREE_PER_SECTOR;				//ָ�����ջ��β����
	}
	free_pos--;									//ѹջ��
	*free_pos=block;
	sp->free_block_cur_pos--;					//����������ݡ�
	sp->free_block_leave++;

}

FSU32 get_file_block(PLMFILE tmp_file,FSU32 file_count,PNODE_INFO dir_up)			//һ���ļ��������Ǹ����С�
{
	FSU32 i;			//�����ļ�����ŵõ��ļ����ڵĿ�š�
	FSU32 pos;

	i=(file_count/FILE_PER_BLOCK);		//  �ļ��ڷ���Ŀ��еġ�

	if(file_count%FILE_PER_BLOCK != 0)
	{
		i+=1;			//�ڱ��еڼ��飬Ҳ���ǵڼ�����������
	}
	pos=block_in_table(tmp_file,i,dir_up);		//�ҵ��ļ��Ŀ顣

	return pos;
}


FSU32 block_in_table(PLMFILE tmp_file,FSU32 i,PNODE_INFO dir_up)		//����ļ����ڵĿ�
{
	FSU32 pos;
	FSU32 *ptr;
	if(i <= 7)
	{
		pos=dir_up->level0_block[i-1];		//����ļ����ڵ����顣
	}
	else if(i <= 1031)
	{
		read_block(dir_up->level1_block,tmp_file->level1_buffer);
		ptr=(FSU32 *)tmp_file->level1_buffer;
		pos=ptr[i-7-1];
	}
	else
	{
		read_block(dir_up->level2_block,tmp_file->level2_buffer);
		ptr=(FSU32 *)tmp_file->level2_buffer;
		read_block(ptr[(i-1031-1)/FILE_PER_BLOCK],tmp_file->level1_buffer);
		ptr=(FSU32 *)tmp_file->level1_buffer;
		pos=ptr[(i-1032-1)%FILE_PER_BLOCK];
	}
	return pos;
}

void release_file_block(PLMFILE tmp_file,PNODE_INFO dir_up)			//�ͷ��ļ�����Ŀ顣
{

	release_level0(tmp_file,dir_up);			//�ͷ�0����

	if((dir_up->block_alloc) == 0)		
	{
		return;
	}

	release_level1(tmp_file,dir_up);			//�ͷ�һ�����е����п�

	if((dir_up->block_alloc) == 0)
	{
		return;
	}

	release_level2(tmp_file,dir_up);			//�ͷŶ������е����п顣
}

void release_level0(PLMFILE tmp_file,PNODE_INFO dir_up)			//�ͷ�0����
{
	FSU32 i;
	for(i=0;i<7;i++)
	{
		push_free(dir_up->level0_block[i]);
		(dir_up->block_alloc)--;
		if(dir_up->block_alloc == 0)			//������п��ͷ����ˡ����ء�
		{
			return;
		}
	}
}

void release_level1(PLMFILE tmp_file,PNODE_INFO dir_up)	//�ͷ�һ����
{
	FSU32 i;
	FSU32 *ptr;
	read_block(dir_up->level1_block,tmp_file->level1_buffer);
	ptr=(FSU32 *)tmp_file->level1_buffer;
	for(i=0;i<BLOCK_PER_BLOCK;i++)			//4096/4���顣
	{
		push_free(ptr[i]);
		(dir_up->block_alloc)--;
		if(dir_up->block_alloc == 0)
		{
			break;
		}
	}
	push_free(dir_up->level1_block);			//��һ����Ҳ�ͷš�
}

void release_level2(PLMFILE tmp_file,PNODE_INFO dir_up)		//�ͷŶ����顣
{
	FSU32 i,j;
	FSU32 *ptr;
	FSU32 *ptr1;
	read_block(dir_up->level2_block,tmp_file->level2_buffer);
	ptr1=(FSU32 *)tmp_file->level2_buffer;
	for(i=0;i<BLOCK_PER_BLOCK;i++)
	{
		read_block(ptr1[i],tmp_file->level1_buffer);

		ptr=(FSU32 *)tmp_file->level1_buffer;

		for(j=0;j<BLOCK_PER_BLOCK;j++)
		{
			push_free(ptr[i]);

			(dir_up->block_alloc)--;

			if(dir_up->block_alloc == 0)
			{
				break;
			}
		}

		push_free(ptr1[i]);		//�ͷŷ����һ���顣

		if(dir_up->block_alloc == 0)
		{
			break;
		}
	}

	push_free(dir_up->level2_block);			//�ͷŶ����顣
}














