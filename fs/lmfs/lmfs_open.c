#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include"lmfs.h"
#include"lmfs_config.h"
#include"device_drive.h"


/*********************************************************************************************
�ļ��ڶ���д֮ǰ�����ȵ��ô򿪺������ĵ��ļ��ľ���ţ�
������½���һ���ļ���Ҳ���Խ��ж�д����
�½�һ���ļ�Ҳ�������ļ��ľ����
���ļ�ϵͳû����ȫ���Ƕϵ��Σ�գ�������ֻ�Ǳ�֤�ļ����ᷢ�����ң������д��Ĺ����з����˶ϵ�
���ճɿռ���˷ѡ�
**********************************************************************************************/


PLMFILE open_file(char *name,char mode)	
{
	PLMFILE file;

	if(mode == WRITE)			//�����Щģʽ�Ĵ򿪡�
	{
		file=open_file_write(name,mode);
	}
	else					//����Ƕ�����׷�ӷ�ʽ�򿪡�
	{
		file=open_file_read(name,mode);
	}

	return file;
}

PLMFILE open_file_write(char *name,char mode)			//��д�ķ�ʽ���ļ��Ļ���
{
	PLMFILE tmp_file;			//�ļ��ľ���������˱�Ҫ�Ļ�������
	NODE file_node;				//�ļ��Ľڵ�š�
	//NODE_INFO node_tmp;			//�ļ��Ľڵ���Ϣ��
	FSU32 i=0;					//�����ļ�������
	FSU32 pos=0;					//�����ڿ��е�λ�á�
	char tmp_name[MAX_FILE_NAME];
	char *token_pos;

	NODE_INFO dir_up;

	if(!check_filename(name))		//����ļ����Ƿ�Ϸ���
	{
		return NULL;
	}

	tmp_file=create_tmp_file();		//�����ļ�ָ��ռ䡣

	if(*name == '/')		//���Ҫ�Ӹ�Ŀ¼�¿�ʼ������
	{
		token_pos=lm_strrtok(name,'/');	//����/ ���һ�γ��ֵĵط���

		dir_up=get_node_info(0,tmp_file->node_buffer);			//��¼��һ��Ŀ¼

		if(token_pos=(name+1))		//�����Ҫ�ڸ�Ŀ¼�´����ļ��Ļ���
		{
			name++;

			if((file_node=search_file(tmp_file,name,&dir_up,&pos,&i)) == 0)		//�Ӹ�Ŀ¼��ʼ����Ҫ�����ļ���Ŀ¼��			//�ļ������ڡ�
			{
				//�ڸ�Ŀ¼�µ�ǰĿ¼�´���һ���ļ���
				//�������Ǹ��ļ�ʵ�����Ƿ������Ľڵ㡣
				//file_node=create_file_x;
			}
		}
		else
		{
			strcpy(tmp_name,token_pos);		//�洢�ļ������֡�

			token_pos--;

			*token_pos=0;		//�����ļ������֡�

			file_node=search_file(tmp_file,name,&dir_up,&pos,&i);		//�����ļ����ڵ�Ŀ¼��

			if(file_node == 0)
			{
				close_file(tmp_file);
				return NULL;			//û�д�Ŀ¼��
			}

			dir_up=get_node_info(file_node,tmp_file->node_buffer);	

			if(dir_up.file_type != DIR)			//����õ����ļ�����Ŀ¼�Ļ���
			{
				close_file(tmp_file);			//�ر��ļ������ء�
				return NULL;
			}

			if((file_node=search_file(tmp_file,tmp_name,&dir_up,&pos,&i)) == 0)		//����ļ������ڵĻ�����´����ļ��Ľڵ��
			{											//���ڵĻ��ڵ�ž���Ѱ�ҵ��Ľڵ�š�
				//file_node=create_file_x
				file_node=lm_create_file(tmp_file,tmp_name,&dir_up,READ_WRITE|O_FILE,pos);
			}
		}

	}
	else			//�ļ��ڵ�ǰĿ¼���в��ҡ�
	{
		dir_up=get_node_info(cur_dir_node.level0_block[0],tmp_file->node_buffer);;

		if((file_node=search_file(tmp_file,name,&dir_up,&pos,&i)) == 0)		//�ڵ�ǰĿ¼������
		{
			//file_node=create_file;
			file_node=lm_create_file(tmp_file,tmp_name,&dir_up,READ_WRITE|O_FILE,pos);	//����ļ������ڵĻ����½�һ���ļ���
		}
	}

	if(file_node == 0)			//�������������ļ�ʧ�ܡ�
	{
		close_file(tmp_file);
		return NULL;
	}

	dir_up=get_node_info(file_node,tmp_file->node_buffer);			//����ļ�����Ϣ�������ļ��ڵ����ڵ�������Ϣ�洢��NODE_buffer�й�����ʹ�á�

	get_file_info(tmp_file,&dir_up,file_node,mode);

	return tmp_file;
}

PLMFILE open_file_read(char *name,char mode)			//�Զ�����׷�ӵķ�ʽ���ļ�����
{
	PLMFILE tmp_file;			//�ļ��ľ���������˱�Ҫ�Ļ�������
	NODE file_node;				//�ļ��Ľڵ�š�
	NODE_INFO dir_up;
	FSU32 i=0;					//�����ļ�������
	FSU32 pos=0;					//�����ڿ��е�λ�á�

	if(!check_filename(name))		//����ļ����Ƿ�Ϸ���
	{
		return NULL;
	}

	tmp_file=create_tmp_file();			//������ʱ�ļ���

	if(*name == '/')		//����ļ���Ϊ/��ͷ����Ӹ�Ŀ¼����
	{
		name++;
		dir_up=get_node_info(0,tmp_file->node_buffer);
		file_node=search_file(tmp_file,name,&dir_up,&pos,&i);	// ����һ���ļ����������Ľڵ�š�
	}
	else		//����û��ӵ�ǰĿ¼����
	{
		dir_up=get_node_info(cur_dir_node.level0_block[0],tmp_file->node_buffer);
		file_node=search_file(tmp_file,name,&dir_up,&pos,&i);
	}
	//������Ϻ������ǰĿ¼û���ҵ���˵������������Ŀ¼�����
	//iָ�������һ���ļ��Ĵ��š�
	//posָ�����ڻ������е�λ�ã����ô˿���ֱ�ӽ����½��Ĳ������ĵ��ڵ�ţ��ڵ�ǰ�ڵ��½����ٶȾ͸��졣
	if(file_node == 0)		//���û���ҵ�	
	{
		close_file(tmp_file);
		return NULL;
	}
	
	dir_up=get_node_info(file_node,tmp_file->node_buffer);	//�ҵ��Ļ���ǰ�ļ�����Ҫ��Ϣ

	if(dir_up.file_type == DIR)
	{
		close_file(tmp_file);
		return NULL;
	}

	get_file_info(tmp_file,&dir_up,file_node,mode);

	read_new_block(tmp_file);			//��ȡ�ļ��ĵ�һ������ݵ���������	

	return tmp_file;
}

NODE search_file(PLMFILE file,char *name,PNODE_INFO info,FSU32 *position,FSU32 *file_count)		//search file		
{
	NODE_INFO tmp;			//�����ڵ���Ϣ�ı��ݺ͸��¡�
	NODE node_tmp;			//���ҽڵ��
	char *str=name;		
	char *token_pos;
	char name_tmp[MAX_FILE_NAME];		//�����ļ����Ļ���
			//file_count��ɾ���ļ���ʱ�����á�
	tmp=*info;

	while(*str)				//�˺�����֤���ڷ��ص�ʱ��dir_upָ����Ҫ�����ļ���һ��Ŀ¼����Ϣ
	{
		token_pos=lm_strtok(str,'/');				//node_tmpΪ�ļ��Ľڵ�Ż���Ϊ0

		strncpy(name_tmp,str,token_pos-str);
		name_tmp[token_pos-str]=0;	//ȷ���ַ�����0��β��

		node_tmp=search_file_from_node(file,name_tmp,&tmp,position,file_count);		//�践���ҵ��Ľ����0Ϊδ�ҵ�������Ϊ���ڡ�

		if(node_tmp == 0)			//����ڵ�ǰĿ¼û���ҵ������ط���������ҡ�
		{
			return 0;
		}

		if(*token_pos != 0)			//���û�е�·���������������ļ���һ��Ŀ¼����Ϣ��
		{
			str=token_pos+1;
			tmp=get_node_info(node_tmp,file->node_buffer);
			if(tmp.file_type == O_FILE)
			{
				return 0;
			}
			*info=tmp;	//����һ��Ŀ¼��Ϣ����Ϊ��ǰĿ¼����Ϣ���Ա�������ҡ�
		}
		else			//ָ����һ���ļ�������Ŀ¼����
		{
			str=token_pos;			//�˲�����Ҫ��ֱ��break
		}
	}
	return node_tmp;			//���ز��ҵ����ļ��Ľڵ�š�
}
/******************************************
�����������������ʱpos��file����count�洢
���ļ���Ŀ¼�е�λ�ã����������Ժ����Ĳ�����
*******************************************/
NODE search_file_from_node(PLMFILE file,char *name,PNODE_INFO info,FSU32 *position,FSU32 *file_count)				//�ӽڵ���Ϣ�в����ļ���
{
	NODE node_tmp=0;

	if(info->file_amount == 0)			//���㼶�洢����������
	{
		return 0;
	}
	*file_count=0;
	node_tmp=search_in_level0(file,name,info,position,file_count);

	if(node_tmp !=0) return node_tmp;		//������ҵ��Ļ��ͷ��ء�

	if(*file_count > info->file_amount) return 0;	//����Ѿ����������е��ļ���

	node_tmp=search_in_level1(file,name,info,position,file_count);	//��һ������������

	if(node_tmp !=0) return node_tmp;

	if(*file_count > info->file_amount) return 0;

	node_tmp=search_in_level2(file,name,info,position,file_count);			//����ҵ���Ŀ¼�����λ�ã��������Ϣ�������ļ����������С�

	return node_tmp;
}

NODE search_in_level0(PLMFILE file,char *name,PNODE_INFO info,FSU32 *position,FSU32 *file_count)
{
	int j=0;
	int k=0;

	PCATALOG_INFO ptr;

	for(j=0;j<7;j++)		//��һ���洢����������
	{
		read_block(info->level0_block[j],file->file_rw_buffer);

		*position=0;

		for(k=0;k<FILE_PER_BLOCK;k++)
		{
			ptr=(PCATALOG_INFO)(&file->file_rw_buffer[(*position)*DIR_SIZE]);

			(*file_count)++;	//���ҵ��ļ���1.		

			(*position)++;			//position����ʼ��Ϊ��һ���ռ䡣

			if(strcmp(ptr->name,name) == 0)			//�Ƚ��ļ������ݡ�
			{
				return ptr->node;
			}
			
			if((*file_count) == info->file_amount)		//����Ѿ����������е��ļ���ֱ�ӷ��ء�
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

	read_block(info->level1_block,file->level1_buffer);		//��ȡһ���鵽һ����������

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

	read_block(info->level2_block,file->level2_buffer);		//��ȡ�ڶ���ָ��

	pos2=(FSU32 *)file->level2_buffer;						// ָ�����ָ���λ�á�

	for(j=0;j<BLOCK_PER_BLOCK;j++)				//��ȡһ��ָ���λ����Ҫ1024�顣
	{
		read_block(pos2[j],file->level1_buffer);

		pos1=(FSU32 *)file->level1_buffer;					//ָ��һ��ָ��Ķ�ȡλ�á�

		for(k=0;k<BLOCK_PER_BLOCK;k++)			//�ٷֱ��ȡÿ��һ��ָ�룬1024�顣
		{
			read_block(pos1[k],file->file_rw_buffer);		//��ȡһ��ָ���е��ļ����ݡ�64���ļ�

			*position=0;

			for(m=0;m<FILE_PER_BLOCK;m++)				//���αȽ��ļ���
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

void read_block(FSU32 block,char *buffer)			//��һ��������ݶ�ȡ����������
{
	block=get_first_sector(block);
	read_nsector(block,buffer,SECTOR_PER_BLOCK);
}

void write_block(FSU32 block,char *buffer)			//дһ��������ݡ�
{
	block=get_first_sector(block);
	write_nsector(block,buffer,SECTOR_PER_BLOCK);
}

FSU32 get_first_sector(FSU32 block)		//���ĳ�����ݿ�ĵ�һ��������
{
	return ((block-sp->data_section_first_block) * SECTOR_PER_BLOCK + sp->data_section_first_sector);
}

NODE_INFO get_node_info(FSU32 node_number,char *buffer)		//���һ���ڵ����Ϣ
{
	int x, y;
	NODE_INFO tmp;
	PNODE_INFO ptmp;
	get_node_pos(&x,&y,node_number);			//��ýڵ����ڵ�������λ�á�
	read_sector(x,buffer);
	ptmp=(PNODE_INFO)(&buffer[y*NODE_INFO_SIZE]);			//�ҵ��ڵ���Ϣ��
	tmp=*ptmp;
	return tmp;
}

void get_node_pos(int *x,int *y,FSU32 node_number)		//����һ���ڵ��������ڽڵ���е�λ�á�
{
	*x=(node_number*64)/512+sp->node_section_first_sector;
	*y=node_number%NODE_PER_SECTOR;
}



char *lm_strtok(char *str,char ch)			//�����ַ����е��ض��ַ���
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

char *lm_strrtok(char *str,char ch)		//�ҵ��ļ������ض��ַ������ֵ�λ�á�������������һ��ָ�롣
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


FSBOOL check_filename(char *name)//�ļ�����顣		//��Ҫ��дȷ���ڽ���ʵ�ʲ����׶ξͱ�֤���ļ����ǺϷ��ġ�
{
	
	char *pos;

	if(*name == 0)			//û��ָ������
	{
		return 0;
	}
	
	if(*name != '/')			//�ļ�������/��ͷ��
	{
		if(*(lm_strtok(name,'/')) == '/')	//����ļ��л���/�Ļ����Ϸ���
		{
			return 0;
		}
	}
	while(1)			//�ļ����������������ļ�����//DIR//DIR/��
	{
		pos=lm_strtok(name,'/');

		if(*pos != 0)
		{
			if(pos-name > (MAX_FILE_NAME-1)) return 0;			//�ļ������ڹ涨�Ĵ�С��

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

char close_file(PLMFILE tmp_file)		//�ر��ļ���		//��Ҫ�����޸ġ�
{
	if(tmp_file != NULL)				//��дģʽ�°�ʣ��Ļ�������д�뵽�ļ��С�
	{
		if(tmp_file->open_mode == WRITE)			//����ļ�����д��ʽ�򿪵ģ��ѻ�������ʣ�������д�뵽�ļ��С�
		{
			if(tmp_file->cur_rw_in_buffer >0)
			{
				write_new_block(tmp_file);
				write_sector(tmp_file->node_sector,tmp_file->node_buffer);
			}
		}
		free(tmp_file->buffer);			//�ͷ��ļ�ռ�õĿռ䡣
		free(tmp_file);
		return 1;
	}
	return 0;
}

PLMFILE create_tmp_file(void)		//����һ���ļ����塣����һ����ʱ�ļ��Ļ��塣
{
	
	PLMFILE tmp_file;
	tmp_file=(PLMFILE)malloc(sizeof(LMFILE));		//�����ļ�ָ��ռ䡣
	tmp_file->buffer=(char *)malloc(25*SECTOR_PER_SIZE);
	tmp_file->file_rw_buffer=tmp_file->buffer;		//Ϊ�ļ�����洢�ռ䣬����ռ䡣
	tmp_file->level1_buffer=(tmp_file->buffer)+BLOCK_SIZE;
	tmp_file->level2_buffer=(tmp_file->buffer)+BLOCK_SIZE+BLOCK_SIZE;
	tmp_file->node_buffer=(tmp_file->buffer)+BLOCK_SIZE+BLOCK_SIZE+BLOCK_SIZE;
	//tmp_file->other_buffer=(tmp_file->buffer)+BLOCK_SIZE+BLOCK_SIZE+BLOCK_SIZE+SECTOR_PER_SIZE;
	return tmp_file;
}

void get_file_info(PLMFILE tmp_file,PNODE_INFO node_tmp,FSU32 file_node,char mode)		//���ݽڵ���Ϣ�õ�Ҫʹ���ļ�����Ϣ��
{
	int i;
	tmp_file->block_alloc=node_tmp->block_alloc;
	tmp_file->cur_rw_in_buffer=0;			//��Щ��������ֻ��read��������Ҫ�õ���
	tmp_file->cur_rw_block=0;		//���

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

	tmp_file->node_number=file_node;			//�������ȥ������Ϊ�ļ��Ľڵ����������ĵ�һ���顣
	tmp_file->open_mode=mode;
	tmp_file->node_sector=node_tmp->sector;
	tmp_file->node_pos=node_tmp->pos_in_sector;
}



