#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include"lmfs.h"
#include"lmfs_config.h"
#include"device_drive.h"


/*********************************************************************************************
文件在读和写之前必须先调用打开函数，的到文件的句柄号，
如果是新建了一个文件后也可以进行读写操作
新建一个文件也将返回文件的句柄。
此文件系统没有完全考虑断电的危险，做到的只是保证文件不会发生混乱，如果在写入的过程中发生了断电
会照成空间的浪费。
**********************************************************************************************/


PLMFILE open_file(char *name,char mode)	
{
	PLMFILE file;

	if(mode == WRITE)			//如果是些模式的打开。
	{
		file=open_file_write(name,mode);
	}
	else					//如果是读或者追加方式打开。
	{
		file=open_file_read(name,mode);
	}

	return file;
}

PLMFILE open_file_write(char *name,char mode)			//已写的方式打开文件的话。
{
	PLMFILE tmp_file;			//文件的句柄，分配了必要的缓冲区。
	NODE file_node;				//文件的节点号。
	//NODE_INFO node_tmp;			//文件的节点信息。
	FSU32 i=0;					//跟踪文件计数。
	FSU32 pos=0;					//跟踪在块中的位置。
	char tmp_name[MAX_FILE_NAME];
	char *token_pos;

	NODE_INFO dir_up;

	if(!check_filename(name))		//检查文件名是否合法。
	{
		return NULL;
	}

	tmp_file=create_tmp_file();		//分配文件指针空间。

	if(*name == '/')		//如果要从根目录下开始搜索。
	{
		token_pos=lm_strrtok(name,'/');	//查找/ 最后一次出现的地方。

		dir_up=get_node_info(0,tmp_file->node_buffer);			//记录上一级目录

		if(token_pos=(name+1))		//如果是要在根目录下创建文件的话。
		{
			name++;

			if((file_node=search_file(tmp_file,name,&dir_up,&pos,&i)) == 0)		//从根目录开始搜索要创建文件的目录。			//文件不存在。
			{
				//在根目录下当前目录下创建一个文件。
				//创建的那个文件实际上是返回它的节点。
				//file_node=create_file_x;
			}
		}
		else
		{
			strcpy(tmp_name,token_pos);		//存储文件的名字。

			token_pos--;

			*token_pos=0;		//处理文件的名字。

			file_node=search_file(tmp_file,name,&dir_up,&pos,&i);		//查找文件所在的目录。

			if(file_node == 0)
			{
				close_file(tmp_file);
				return NULL;			//没有此目录。
			}

			dir_up=get_node_info(file_node,tmp_file->node_buffer);	

			if(dir_up.file_type != DIR)			//如果得到的文件不是目录的话。
			{
				close_file(tmp_file);			//关闭文件，返回。
				return NULL;
			}

			if((file_node=search_file(tmp_file,tmp_name,&dir_up,&pos,&i)) == 0)		//如果文件不存在的话获得新创建文件的节点号
			{											//存在的话节点号就是寻找到的节点号。
				//file_node=create_file_x
				file_node=lm_create_file(tmp_file,tmp_name,&dir_up,READ_WRITE|O_FILE,pos);
			}
		}

	}
	else			//文件在当前目录进行查找。
	{
		dir_up=get_node_info(cur_dir_node.level0_block[0],tmp_file->node_buffer);;

		if((file_node=search_file(tmp_file,name,&dir_up,&pos,&i)) == 0)		//在当前目录上搜索
		{
			//file_node=create_file;
			file_node=lm_create_file(tmp_file,tmp_name,&dir_up,READ_WRITE|O_FILE,pos);	//如果文件不存在的话就新建一个文件。
		}
	}

	if(file_node == 0)			//创建或者搜索文件失败。
	{
		close_file(tmp_file);
		return NULL;
	}

	dir_up=get_node_info(file_node,tmp_file->node_buffer);			//获得文件的信息，并把文件节点所在的扇区信息存储在NODE_buffer中供后面使用。

	get_file_info(tmp_file,&dir_up,file_node,mode);

	return tmp_file;
}

PLMFILE open_file_read(char *name,char mode)			//以读或者追加的方式打开文件，。
{
	PLMFILE tmp_file;			//文件的句柄，分配了必要的缓冲区。
	NODE file_node;				//文件的节点号。
	NODE_INFO dir_up;
	FSU32 i=0;					//跟踪文件计数。
	FSU32 pos=0;					//跟踪在块中的位置。

	if(!check_filename(name))		//检查文件名是否合法。
	{
		return NULL;
	}

	tmp_file=create_tmp_file();			//建立临时文件。

	if(*name == '/')		//如果文件名为/开头，则从根目录找起
	{
		name++;
		dir_up=get_node_info(0,tmp_file->node_buffer);
		file_node=search_file(tmp_file,name,&dir_up,&pos,&i);	// 给定一个文件名查找它的节点号。
	}
	else		//否则得话从当前目录找起。
	{
		dir_up=get_node_info(cur_dir_node.level0_block[0],tmp_file->node_buffer);
		file_node=search_file(tmp_file,name,&dir_up,&pos,&i);
	}
	//搜索完毕后如果当前目录没有找到，说明已搜索到了目录的最后
	//i指向了最后一个文件的代号。
	//pos指向了在缓冲区中的位置，利用此可以直接进行新建的操作。的到节点号，在当前节点新建的速度就更快。
	if(file_node == 0)		//如果没有找到	
	{
		close_file(tmp_file);
		return NULL;
	}
	
	dir_up=get_node_info(file_node,tmp_file->node_buffer);	//找到的话提前文件的重要信息

	if(dir_up.file_type == DIR)
	{
		close_file(tmp_file);
		return NULL;
	}

	get_file_info(tmp_file,&dir_up,file_node,mode);

	read_new_block(tmp_file);			//读取文件的第一块的内容到缓冲区。	

	return tmp_file;
}

NODE search_file(PLMFILE file,char *name,PNODE_INFO info,FSU32 *position,FSU32 *file_count)		//search file		
{
	NODE_INFO tmp;			//用作节点信息的备份和更新。
	NODE node_tmp;			//查找节点号
	char *str=name;		
	char *token_pos;
	char name_tmp[MAX_FILE_NAME];		//用于文件名的缓存
			//file_count在删除文件的时候有用。
	tmp=*info;

	while(*str)				//此函数保证了在返回的时候，dir_up指向了要查找文件上一级目录的信息
	{
		token_pos=lm_strtok(str,'/');				//node_tmp为文件的节点号或者为0

		strncpy(name_tmp,str,token_pos-str);
		name_tmp[token_pos-str]=0;	//确保字符串以0结尾。

		node_tmp=search_file_from_node(file,name_tmp,&tmp,position,file_count);		//需返回找到的结果，0为未找到，非零为存在。

		if(node_tmp == 0)			//如果在当前目录没有找到。返回否则继续查找。
		{
			return 0;
		}

		if(*token_pos != 0)			//如果没有到路径名的最后，则更新文件上一级目录的信息。
		{
			str=token_pos+1;
			tmp=get_node_info(node_tmp,file->node_buffer);
			if(tmp.file_type == O_FILE)
			{
				return 0;
			}
			*info=tmp;	//把上一级目录信息更改为当前目录的信息。以便继续查找。
		}
		else			//指向下一个文件名或者目录名。
		{
			str=token_pos;			//此部不需要，直接break
		}
	}
	return node_tmp;			//返回查找到的文件的节点号。
}
/******************************************
当下面这个函数返回时pos和file——count存储
着文件在目录中的位置，可以留作以后计算的参数。
*******************************************/
NODE search_file_from_node(PLMFILE file,char *name,PNODE_INFO info,FSU32 *position,FSU32 *file_count)				//从节点信息中查找文件。
{
	NODE node_tmp=0;

	if(info->file_amount == 0)			//在零级存储块中搜索。
	{
		return 0;
	}
	*file_count=0;
	node_tmp=search_in_level0(file,name,info,position,file_count);

	if(node_tmp !=0) return node_tmp;		//如果查找到的话就返回。

	if(*file_count > info->file_amount) return 0;	//如果已经搜索了所有的文件。

	node_tmp=search_in_level1(file,name,info,position,file_count);	//在一级块中搜索。

	if(node_tmp !=0) return node_tmp;

	if(*file_count > info->file_amount) return 0;

	node_tmp=search_in_level2(file,name,info,position,file_count);			//如果找到了目录的最后位置，则相关信息存在于文件读缓冲区中。

	return node_tmp;
}

NODE search_in_level0(PLMFILE file,char *name,PNODE_INFO info,FSU32 *position,FSU32 *file_count)
{
	int j=0;
	int k=0;

	PCATALOG_INFO ptr;

	for(j=0;j<7;j++)		//在一级存储块中搜索。
	{
		read_block(info->level0_block[j],file->file_rw_buffer);

		*position=0;

		for(k=0;k<FILE_PER_BLOCK;k++)
		{
			ptr=(PCATALOG_INFO)(&file->file_rw_buffer[(*position)*DIR_SIZE]);

			(*file_count)++;	//查找的文件加1.		

			(*position)++;			//position返回始终为下一个空间。

			if(strcmp(ptr->name,name) == 0)			//比较文件的内容。
			{
				return ptr->node;
			}
			
			if((*file_count) == info->file_amount)		//如果已经查找了所有的文件，直接返回。
			{
				return 0;
			}
		}
	}
	return 0;
}

NODE search_in_level1(PLMFILE file,char *name,PNODE_INFO info,FSU32 *position,FSU32 *file_count)
{
	int j=0;
	int k=0;
	PCATALOG_INFO ptr;
	FSU32 *pos;

	read_block(info->level1_block,file->level1_buffer);		//读取一级块到一级缓冲区。

	pos=(FSU32 *)(file->level1_buffer);

	for(j=0;j<BLOCK_PER_BLOCK;j++)
	{
		read_block(pos[j],file->file_rw_buffer);

		*position=0;

		for(k=0;k<FILE_PER_BLOCK;k++)
		{
			ptr=(PCATALOG_INFO)(&file->file_rw_buffer[(*position)*DIR_SIZE]);

			(*file_count)++;

			(*position)++;

			if(strcmp(ptr->name,name) == 0)
			{
				return ptr->node;
			}

			if((*file_count) == info->file_amount)
			{
				return 0;
			}
		}
	}
	return 0;
}

NODE search_in_level2(PLMFILE file,char *name,PNODE_INFO info,FSU32 *position,FSU32 *file_count)
{
	
	int j=0;
	int k=0;
	int m=0;
	PCATALOG_INFO ptr;
	FSU32 *pos1;
	FSU32 *pos2;

	read_block(info->level2_block,file->level2_buffer);		//读取第二级指针

	pos2=(FSU32 *)file->level2_buffer;						// 指向二级指针的位置。

	for(j=0;j<BLOCK_PER_BLOCK;j++)				//读取一级指针的位置需要1024块。
	{
		read_block(pos2[j],file->level1_buffer);

		pos1=(FSU32 *)file->level1_buffer;					//指向一级指针的读取位置。

		for(k=0;k<BLOCK_PER_BLOCK;k++)			//再分别读取每个一级指针，1024块。
		{
			read_block(pos1[k],file->file_rw_buffer);		//读取一级指针中的文件内容。64个文件

			*position=0;

			for(m=0;m<FILE_PER_BLOCK;m++)				//依次比较文件。
			{
				ptr=(PCATALOG_INFO)(&file->file_rw_buffer[(*position)*DIR_SIZE]);

				(*file_count)++;

				(*position)++;

				if(strcmp(ptr->name,name) == 0)
				{
					return ptr->node;
				}

				if((*file_count) == info->file_amount)
				{
					return 0;
				}

			}
		}
	}
	return 0;
}

void read_block(FSU32 block,char *buffer)			//把一个快的内容读取到缓冲区。
{
	block=get_first_sector(block);
	read_nsector(block,buffer,SECTOR_PER_BLOCK);
}

void write_block(FSU32 block,char *buffer)			//写一个块的内容。
{
	block=get_first_sector(block);
	write_nsector(block,buffer,SECTOR_PER_BLOCK);
}

FSU32 get_first_sector(FSU32 block)		//获得某个数据块的第一个扇区。
{
	return ((block-sp->data_section_first_block) * SECTOR_PER_BLOCK + sp->data_section_first_sector);
}

NODE_INFO get_node_info(FSU32 node_number,char *buffer)		//获得一个节点的信息
{
	int x, y;
	NODE_INFO tmp;
	PNODE_INFO ptmp;
	get_node_pos(&x,&y,node_number);			//获得节点所在的扇区和位置。
	read_sector(x,buffer);
	ptmp=(PNODE_INFO)(&buffer[y*NODE_INFO_SIZE]);			//找到节点信息。
	tmp=*ptmp;
	return tmp;
}

void get_node_pos(int *x,int *y,FSU32 node_number)		//给出一个节点号算出它在节点表中的位置。
{
	*x=(node_number*64)/512+sp->node_section_first_sector;
	*y=node_number%NODE_PER_SECTOR;
}



char *lm_strtok(char *str,char ch)			//查找字符串中的特定字符。
{
	char *tmp=NULL;

	while(*str)
	{
		if(*str == ch)
		{
			tmp=str;
			return tmp;
		}
		str++;
	}
	return str;
}

char *lm_strrtok(char *str,char ch)		//找到文件名中特定字符最后出现的位置。；返回他的下一个指针。
{
	char *tmp;
	tmp=str+(strlen(str)-1);
	while(str<tmp)
	{
		if((*tmp) == ch)
		{
			break;
		}
		tmp--;
	}
	return (tmp+1);
}


FSBOOL check_filename(char *name)//文件名检查。		//需要改写确保在进入实际操作阶段就保证了文件名是合法的。
{
	
	char *pos;

	if(*name == 0)			//没有指定名字
	{
		return 0;
	}
	
	if(*name != '/')			//文件名不已/开头。
	{
		if(*(lm_strtok(name,'/')) == '/')	//如果文件中还有/的话不合法。
		{
			return 0;
		}
	}
	while(1)			//文件出现类似这样的文件名“//DIR//DIR/”
	{
		pos=lm_strtok(name,'/');

		if(*pos != 0)
		{
			if(pos-name > (MAX_FILE_NAME-1)) return 0;			//文件名大于规定的大小。

			name=pos+1;

			if((*name == '/') || (*name == 0))
			{
				return 0;
			}
		}
		else
		{
			break;
		}

	}

	return 1;
}

char close_file(PLMFILE tmp_file)		//关闭文件。		//还要加以修改。
{
	if(tmp_file != NULL)				//在写模式下把剩余的缓冲数据写入到文件中。
	{
		if(tmp_file->open_mode == WRITE)			//如果文件是以写方式打开的，把缓冲区中剩余的内容写入到文件中。
		{
			if(tmp_file->cur_rw_in_buffer >0)
			{
				write_new_block(tmp_file);
				write_sector(tmp_file->node_sector,tmp_file->node_buffer);
			}
		}
		free(tmp_file->buffer);			//释放文件占用的空间。
		free(tmp_file);
		return 1;
	}
	return 0;
}

PLMFILE create_tmp_file(void)		//创建一个文件缓冲。建立一个临时文件的缓冲。
{
	
	PLMFILE tmp_file;
	tmp_file=(PLMFILE)malloc(sizeof(LMFILE));		//分配文件指针空间。
	tmp_file->buffer=(char *)malloc(25*SECTOR_PER_SIZE);
	tmp_file->file_rw_buffer=tmp_file->buffer;		//为文件分配存储空间，缓冲空间。
	tmp_file->level1_buffer=(tmp_file->buffer)+BLOCK_SIZE;
	tmp_file->level2_buffer=(tmp_file->buffer)+BLOCK_SIZE+BLOCK_SIZE;
	tmp_file->node_buffer=(tmp_file->buffer)+BLOCK_SIZE+BLOCK_SIZE+BLOCK_SIZE;
	//tmp_file->other_buffer=(tmp_file->buffer)+BLOCK_SIZE+BLOCK_SIZE+BLOCK_SIZE+SECTOR_PER_SIZE;
	return tmp_file;
}

void get_file_info(PLMFILE tmp_file,PNODE_INFO node_tmp,FSU32 file_node,char mode)		//根据节点信息得到要使用文件的信息。
{
	int i;
	tmp_file->block_alloc=node_tmp->block_alloc;
	tmp_file->cur_rw_in_buffer=0;			//这些参数可能只在read函数中需要用到。
	tmp_file->cur_rw_block=0;		//序号

	if(mode != WRITE)
	{
		tmp_file->file_size=node_tmp->file_size;
		tmp_file->leave_size=tmp_file->file_size;
	}
	else
	{
		tmp_file->file_size=0;
	}

	for(i=0;i<7;i++)
	{
		tmp_file->level0_block[i]=node_tmp->level0_block[i];
	}
	tmp_file->level1_block=node_tmp->level1_block;
	tmp_file->level2_block=node_tmp->level2_block;

	tmp_file->node_number=file_node;			//这个可以去掉，因为文件的节点就是他分配的第一个块。
	tmp_file->open_mode=mode;
	tmp_file->node_sector=node_tmp->sector;
	tmp_file->node_pos=node_tmp->pos_in_sector;
}



