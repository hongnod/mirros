#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include"lmfs_config.h"
#include"lmfs.h"
#include"device_drive.h"

//void storage_filename(char *path,char *name);			//此函数的作用是处理文件名。
														//返回值时0，1，2
							//0表示在根目录下进行搜索。1表示从根目录下搜索。2表示在当前目录下进行搜索。



//函数的功能是返回文件存在的节点，上一级目录的节点信息存在dirup中。
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

	token_pos=lm_strrtok(name,'/');	//存储文件的文件名。
	strcpy(tmp_name,token_pos);

	if(*name == '/')			//查找文件所在目录的节点号。
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
//现在已找到了文件所在的目录，且文件目录的节点信息存储在dirup中。
	//寻找目录中最后一个文件的信息。
	file_node=search_file(tmp_file,tmp_name,&dir_up,&pos,&i);
	if(file_node == 0)
	{
		close_file(tmp_file);
		return UNSUCCESS;
	}

	block=get_last_file_info(tmp_file,&dir_up,&last_file);			//获得目录最后一个文件的信息，并返回最后一个块。

	if( i != dir_up.file_amount)
	{
		ptr=(PCATALOG_INFO)tmp_file->file_rw_buffer;
		ptr+=pos-1;
		ptr->node=last_file.node;
		strcpy(ptr->name,last_file.name);			//把当前文件的信息替换成最后一个文件的信息。

		i=get_file_block(tmp_file,i,&dir_up);
		tmp=(pos*DIR_SIZE)/SECTOR_PER_SIZE;
		i=get_first_sector(i);			//更新文件块信息
		write_sector(i+tmp,&(tmp_file->file_rw_buffer[tmp*SECTOR_PER_SIZE]));
	}


	update_dir_info(tmp_file,&dir_up,block);		//更新文件目录信息。
	//释放文件的数据块.

	dir_up=get_node_info(file_node,tmp_file->node_buffer);

	release_file_block(tmp_file,&dir_up);

	update_free();		//更新空闲块和超级块。

	close_file(tmp_file);

	return 1;
}

FSU32 get_last_file_info(PLMFILE tmp_file,PNODE_INFO dir_up,PCATALOG_INFO last_file)		//找到目录中最后一个文件的信息。
{																							//返回最后一个文件所在的块。
	FSU32 i;
	FSU32 pos;
	PCATALOG_INFO info;

	i=dir_up->block_alloc;

	pos=block_in_table(tmp_file,i,dir_up);		//得到文件所在的块。

	i=dir_up->file_amount;
	
	i=(i%FILE_PER_BLOCK)-1;			//找到文件在块中的位置。

	read_block(pos,tmp_file->level2_buffer);	//读取该块

	info=(PCATALOG_INFO)(&tmp_file->level2_buffer[i*DIR_SIZE]);	//获得文件目录信息。

	last_file->node=info->node;

	strcpy(last_file->name,info->name);			//获得最后一个文件的信息。

	return pos;
}

void update_dir_info(PLMFILE tmp_file,PNODE_INFO dir_up,FSU32 block)		// dir_up所在的扇区数据在tmp_file->node_buffer中，。
{		//更新目录信息。
	PNODE_INFO ptr;
	ptr=(PNODE_INFO)tmp_file->node_buffer;
	ptr+=dir_up->pos_in_sector;
	ptr->file_amount--;
	if(((ptr->file_amount)%FILE_PER_BLOCK) == 0)			//把分配的最后一个块弹出。
	{
		push_free(block);		//如果要删除的文件所在的块只有一个文件，。
		ptr->block_alloc--;
	}
	write_sector(dir_up->sector,tmp_file->node_buffer);	//更新目录节点信息。
}


void push_free(FSU32 block)			//归还一个空闲块。
{
	if(sp->free_block_cur_pos == 0)		//如果当前空闲块扇区中已满。
	{
		write_sector(sp->free_block_cur_sector,_free_block_buffer);	//更新相关信息。
		sp->free_block_cur_sector--;
		sp->free_block_cur_pos=SECTOR_PER_SIZE/sizeof(FSU32);
		if((sp->free_block_cur_sector) < (sp->free_block_first_sector))		//如果到了表的头部，返回。
		{
			return;
		}
		read_sector(sp->free_block_cur_sector,_free_block_buffer);		//读取上一块的空闲块列表。
		free_pos=(FSU32 *)_free_block_buffer;	//指针指向空闲块的头部。
		free_pos+=FREE_PER_SECTOR;				//指向空闲栈的尾部。
	}
	free_pos--;									//压栈。
	*free_pos=block;
	sp->free_block_cur_pos--;					//更新相关数据。
	sp->free_block_leave++;

}

FSU32 get_file_block(PLMFILE tmp_file,FSU32 file_count,PNODE_INFO dir_up)			//一个文件存在于那个块中。
{
	FSU32 i;			//根据文件的序号得到文件所在的块号。
	FSU32 pos;

	i=(file_count/FILE_PER_BLOCK);		//  文件在分配的块中的。

	if(file_count%FILE_PER_BLOCK != 0)
	{
		i+=1;			//在表中第几块，也就是第几个扇区。。
	}
	pos=block_in_table(tmp_file,i,dir_up);		//找到文件的块。

	return pos;
}


FSU32 block_in_table(PLMFILE tmp_file,FSU32 i,PNODE_INFO dir_up)		//获得文件所在的块
{
	FSU32 pos;
	FSU32 *ptr;
	if(i <= 7)
	{
		pos=dir_up->level0_block[i-1];		//获得文件所在的区块。
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

void release_file_block(PLMFILE tmp_file,PNODE_INFO dir_up)			//释放文件分配的块。
{

	release_level0(tmp_file,dir_up);			//释放0级块

	if((dir_up->block_alloc) == 0)		
	{
		return;
	}

	release_level1(tmp_file,dir_up);			//释放一级块中的所有块

	if((dir_up->block_alloc) == 0)
	{
		return;
	}

	release_level2(tmp_file,dir_up);			//释放二级块中的所有块。
}

void release_level0(PLMFILE tmp_file,PNODE_INFO dir_up)			//释放0级块
{
	FSU32 i;
	for(i=0;i<7;i++)
	{
		push_free(dir_up->level0_block[i]);
		(dir_up->block_alloc)--;
		if(dir_up->block_alloc == 0)			//如果所有块释放完了。返回。
		{
			return;
		}
	}
}

void release_level1(PLMFILE tmp_file,PNODE_INFO dir_up)	//释放一级块
{
	FSU32 i;
	FSU32 *ptr;
	read_block(dir_up->level1_block,tmp_file->level1_buffer);
	ptr=(FSU32 *)tmp_file->level1_buffer;
	for(i=0;i<BLOCK_PER_BLOCK;i++)			//4096/4个块。
	{
		push_free(ptr[i]);
		(dir_up->block_alloc)--;
		if(dir_up->block_alloc == 0)
		{
			break;
		}
	}
	push_free(dir_up->level1_block);			//把一级块也释放。
}

void release_level2(PLMFILE tmp_file,PNODE_INFO dir_up)		//释放二级块。
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

		push_free(ptr1[i]);		//释放分配的一级块。

		if(dir_up->block_alloc == 0)
		{
			break;
		}
	}

	push_free(dir_up->level2_block);			//释放二级块。
}














