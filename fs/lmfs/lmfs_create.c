#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include"lmfs_config.h"
#include"lmfs.h"
#include"device_drive.h"


/********************************
���Լ������Ѻ�����ӡ�ļ���дʧ��
��ԭ����Ϊ��ѡ���ܡ�
*********************************/


PLMFILE create_file(char *name,char attrib)		//��ָ����Ŀ¼�´���һ���ļ���
{
	PLMFILE tmp_file;		//��������Ŀ¼Ҳ��ʹ�ô˺������������������

	NODE_INFO dir_up;
	
	NODE file_node;

	char tmp_name[60];

	char *token_pos;

	FSU32 i=0;					//�����ļ�������
	FSU32 pos=0;				//�����ļ��ڻ������е�λ�á�

	if(!check_filename(name))		//����ļ����Ƿ���Ч��			
	{
		return NULL;
	}
	
	tmp_file=create_tmp_file();	

	//����Ŀ¼�Ƿ���ڣ�������һ����ͨ�ļ���
	if(*name == '/')			//���Ҫ�Ӹ�Ŀ¼�¿�ʼ������
	{
		name++;

		dir_up=get_node_info(0,tmp_file->node_buffer);	//��ø�Ŀ¼�ڵ���Ϣ�����Ѹ�Ŀ¼��Ϣ���ڵ����������buffer�У������ļ���ʱ���ʹ�á�

		if(*(lm_strtok(name,'/')) == 0)				//Ҫ�ڸ�Ŀ¼�´����ļ���
		{
			file_node=search_file(tmp_file,name,&dir_up,&pos,&i);

			if(file_node !=0)			//�ļ��Ѿ����ڡ�
			{
				close_file(tmp_file);
				return NULL;
			}
			//�ļ������ڣ�����һ���ļ�
			file_node=lm_create_file(tmp_file,name,&dir_up,attrib,pos);		//Ӧ�û�Ҫ����i��POS

		}
		else			//��������ڸ�Ŀ¼�½����Ļ������Ҵ�Ŀ¼�Ƿ���ڡ�
		{
			token_pos=lm_strrtok(name,'/');		
				
			strcpy(tmp_name,token_pos);		//�洢�ļ������֡�

			token_pos--;

			*token_pos=0;

			file_node=search_file(tmp_file,name,&dir_up,&pos,&i);	// �����ļ�����Ŀ¼�Ƿ���ڣ���

			if(file_node == 0)
			{
				//Ŀ¼�����ڡ�
				close_file(tmp_file);
				return NULL;
			}

			dir_up=get_node_info(file_node,tmp_file->node_buffer);		//��ȡ�ļ�����Ŀ¼�Ľڵ���Ϣ�����洢��file��дbuffer�С�	

			if(dir_up.file_type != DIR)			//�����һ��Ŀ¼����Ŀ¼��
			{
				close_file(tmp_file);
				return NULL;
			}
			
			file_node=search_file(tmp_file,tmp_name,&dir_up,&pos,&i);

			if(file_node !=0)
			{
				//���ļ��Ѿ�����
				close_file(tmp_file);
				return NULL;
			}

			file_node=lm_create_file(tmp_file,name,&dir_up,attrib,pos);
		}
	}
	else		//�ڵ�ǰĿ¼�´�����
	{
		dir_up=get_node_info(cur_dir_node.level0_block[0],tmp_file->node_buffer);
		
		file_node=search_file(tmp_file,name,&dir_up,&pos,&i);

		if(file_node !=0)
		{
			close_file(tmp_file);
			return NULL;
		}

		file_node=lm_create_file(tmp_file,name,&dir_up,attrib,pos);		//����i��pos���Ժܿ�֪��Ҫ�����ļ���Ϣ��Ҫд�����
	}
	
	if(file_node == 0)		//�����ļ�ʧ��
	{
		close_file(tmp_file);
		return NULL;
	}

//	dir_up=get_node_info(file_node,tmp_file->node_buffer);		//���֮ǰ�ļ��Ѿ������ɹ������Ľڵ���ϢҲ�Ѿ�д�룬��ȡ���Ľڵ���Ϣ��

	get_file_info(tmp_file,&dir_up,file_node,WRITE);				//��ȡҪ�����ļ�����Ϣ�����ء�

	return tmp_file;
}

NODE lm_create_file(PLMFILE file,char *name,PNODE_INFO info,unsigned char attrib,FSU32 pos)
{
	//�ڽڵ���ϢΪinfo��Ŀ¼�´���һ���ļ���
	//֮ǰһ��˳���Ļ���ǰĿ¼��������һ��������ݴ�����read-buffer�С�
	//pos��������buffer�е�λ�á�
	//��ʱtmpfile�еĻ���������������Ҫ�����ݣ���Ϊ�Ǵ�������һ�����Ļ������ܹ�����������ݡ���
	//Ҫ�����ļ��Ļ���Ҫ����һ����ʳ�ļ����ڽ���ʱ����ʳ�ļ�ɾ�����С�
	NODE file_node;

	FSU32 x=0,y=0;

	PNODE_INFO ptr;

	PCATALOG_INFO cptr;

	file_node=pop_free();			//����һ�����п飬���п�ĺž����ļ��Ľڵ�š���������
	//���µ����񽻸��Լ�							//�������ַ�����ȱ��������ļ��Ĵ�СΪ0����Ҳ��ռ��һ����Ŀռ䣬�ճ��˷ѡ�������Ҫ������
	if(file_node == 0)				//����Ƿ����ɹ���
	{
		return 0;
	}
//��ɸ��¡� ������д�ļ���Ϣ��Ŀ¼��Ŀ¼��Ϣ�Ѿ�������read����buffer��
//����pos��i�������Ҫд�뵽�ĸ����������Ҵ�ϵͳΪ��������������Ǹ����С�
//�����ļ�����Ŀ¼����Ϣ����Ҫ��д���ļ���Ϣ��Ŀ¼��
	//read_sector(info->sector,file->node_buffer);		//���¶�ȡĿ¼�ڵ���Ϣ�����壬�е���࣬�Ժ��ٽ�������԰Ѹ����ļ�����Ϣ���ڸ���Ŀ¼��Ϣ֮��

	ptr=(PNODE_INFO)(&file->node_buffer[NODE_INFO_SIZE*info->pos_in_sector]);			// ����ϼ��ļ��еýڵ���Ϣ��

	if(pos == FILE_PER_BLOCK)		//Ŀ¼��ǰ��¼��������Ҫ����һ���¿顣
	{
		//���·���Ŀ���д��Ŀ¼��Ϣ��xΪ�·��������Ŀ¼���ݿ顣�����ĵ�һ��λ��д����Ϣ��
		if((x=alloc_new_block(file,ptr)) == 0) return 0;		//���������¿��Ͻ�����ֱ��д���һ��λ�á�
		cptr=(PCATALOG_INFO)file->file_rw_buffer;		//����鲻�ɹ��Ļ�ֱ�ӷ��ء�
		cptr->node=file_node;
		strcpy(cptr->name,name);
		write_sector(get_first_sector(x),file->file_rw_buffer);		//����Ŀ¼��Ϣ��
	}
	else
	{
		//�����п���д��Ŀ¼��Ϣ��pos��¼��λ��
		x=get_last_file_pos(file,info);			//x��¼�����һ���ļ����ڵĿ顣
		cptr=(PCATALOG_INFO)file->file_rw_buffer;
		cptr+=pos;
		cptr->node=file_node;
		strcpy(cptr->name,name);
		x=get_first_sector(x);
		y=(pos*sizeof(FSU32))/SECTOR_PER_SIZE;
		write_sector(x+y,&(file->file_rw_buffer[y*SECTOR_PER_SIZE]));		//����Ŀ¼��Ϣ��
	}

	ptr->file_amount++;

	update_free();			//�ڴ˽��и��¿�������Ŀռ�	

	write_sector(info->sector,file->node_buffer);		//����Ŀ¼��Ϣ���ļ��Ƿ���ڵı�־��Ŀ¼���ļ����Ƿ񱻼�1
	// �˴������⡣��ǰ���д���ļ��ڵ���Ϣ�ɳ�ͻ��													//��������Ļ����ճɷ���Ŀ�Ķ�ʧ��
	
	get_node_pos(&x,&y,file_node);			

	read_sector(x,file->node_buffer);			//�˽ڵ��Ƿ�����ļ��Ľڵ㡣��ȡ�˽ڵ�������������Ϣ��
// ��д�ļ��Ľڵ���Ϣ����д�롣
	ptr=(PNODE_INFO)(&(file->node_buffer[y*NODE_INFO_SIZE]));
	ptr->block_alloc=1;
	ptr->file_amount=0;
	ptr->file_attrib=attrib;			//????�ж��Ƿ�����ͨ�ļ�����Ŀ¼����Ŀ¼�Ļ�Ϊ4096�����ļ��Ļ�Ϊ0
	ptr->file_size=0;
	ptr->file_type=attrib&0x0f;
	ptr->last_modify_time=TIME;			//ʱ��Ļ��Ժ���ʵ�֡������ϵͳ�ṩʱ�亯����
	ptr->level0_block[0]=file_node;
	ptr->used=1;
	ptr->unused=0;
	ptr->sector=x;
	ptr->pos_in_sector=y;
	write_sector(x,file->node_buffer);

	return file_node;
}

NODE pop_free(void)			//�ڽ��д˲���ʱ����Ҫ���źš�
{
	NODE file_node=0;

	file_node=*free_pos;

	if(!file_node)			//�������Ŀ�Ϊ0��˵��û�п��п��ˣ��豸û�п��пռ��ˡ�
	{
		return 0;			//Ҳ�Ͳ�������Ϣ�ĸ����ˡ�
	}

	*free_pos=0;
	free_pos++;
	sp->free_block_cur_pos++;
	sp->free_block_leave--;

	if(sp->free_block_cur_pos == FREE_PER_SECTOR)			// ��ǰ�����Ŀ��п���ȫ�����ꡣ��Ҫ��ȡ��һ������
	{
												//�����ݣ������������Ϣ��
		sp->free_block_cur_sector++;
		sp->free_block_cur_pos=0;
		read_sector(sp->free_block_cur_sector,_free_block_buffer);
		free_pos=(FSU32 *)_free_block_buffer;
	}
	return file_node;
}

FSU32 get_last_file_pos(PLMFILE file,PNODE_INFO info)	//���Ŀ¼�����һ���ļ����ڵĿ�š�
{
	FSU32 m;
	FSU32 *y;
	FSU32 block;
	m=info->block_alloc;
	if(m<8)			//����ļ�����Ŀ�С��8.
	{
		block=info->level0_block[m-1];
	}
	else if(m<1032)		//����ļ���һ�����С�
	{
		y=(FSU32 *)file->level1_buffer;
		block=y[m-7-1];
	}
	else			//�ڶ������С�
	{
		y=(FSU32 *)file->level1_buffer;
		block=y[(m-1031)%BLOCK_PER_BLOCK];
	}

	return block;
}
//���ļ����䲻�����������̫�����ʺϸ�Ŀ¼���䡣
FSU32 alloc_new_block(PLMFILE file,PNODE_INFO ptr)		//��Ŀ¼����һ���¿飬д��Ŀ¼�ڵ��С�
{
	FSU32 block;			//�ʺ����ݺ�Ŀ¼�ķ��䣬������������Ϣ��					//
	FSU32 m,tmp,n;
	FSU32 *bptr;

	m=ptr->block_alloc;			//�ڵ�����ݴ������ļ�write_buffer�У�һ�����������ݡ�

	if((block=pop_free()) == 0)		//������ļ����¿飬�����һ����ʱ��һ����ʹ�á�
	{
		return 0;
	}
	if(m<7)				//�������������ֻ����һ��������֣���֤�˻����������ݲ����б仯��
	{
		//���½ڵ���Ϣ������Ҫ���������Ŀ顣
		ptr->level0_block[m]=block;
		ptr->block_alloc++;
		//write_sector(info.sector,file->file_write_buffer);
					//���ŷ��ء�
	}
	else if(m=7)			// ����ѷ���Ŀ���Ϊ7������Ҫ����һ���顣��һ�η���
	{						//Ҳ˵��file->level1_buffer�е�����û���壬��������Ϊ���塣
		ptr->level1_block=block;
		//����Ҫ�����˿������ֱ�ӿ��Խ���д�룬λ�����ĵ�һ������
		if((block=pop_free()) == 0)			//��������Ƿ����Ŀ¼��¼�ļ���Ϣ�ġ�Ҫ���صĿ顣
		{
			return 0;
		}
		bptr=(FSU32 *)file->level1_buffer;
		*bptr=block;
		write_sector(get_first_sector(ptr->level1_block),file->level1_buffer);
	//	write_sector(info.sector,file->file_write_buffer);
	}
	else if(m<1031)			//��һ������д�롣����ĺš���ʱlevel1������һ�����е���Ϣ��
	{
		bptr=(FSU32 *)file->level1_buffer;
		bptr[m-7]=block;
		tmp=get_first_sector(file->level1_block);
		n=( (m-7+1)*(sizeof(FSU32)) /SECTOR_PER_SIZE);			//Ҫ��Ҫ��1��û����֤��
		tmp=tmp+n;			//������������⡣
		write_sector(tmp,&(file->level1_buffer[n*SECTOR_PER_SIZE]));
		ptr->block_alloc++;
		//write_sector(info.sector,file->file_write_buffer);
	}
	else if(m=1031)			//��ҪΪĿ¼�����һ��������
	{
		ptr->level2_block=block;			//�����飬˵��level2�Ļ���Һ����ʹ��
		if((tmp=pop_free()) == 0)			//�ٸ����������һ��һ���顣
		{
			return 0;
		}
		bptr=(FSU32 *)file->level2_buffer;
		*bptr=tmp;		//д����Ϣ��
		write_sector(get_first_sector(ptr->level2_block),file->level2_buffer);
		if((block=pop_free()) == 0)			//�ٸ����������һ���㼶�顣
		{
			return 0;
		}
		*bptr=block;
		write_sector(get_first_sector(tmp),file->level2_buffer);
		ptr->block_alloc++;
		//write_sector(info.sector,file->file_write_buffer);
	}
	else if(m<(7+1024+1024*1024))
	{
		//����Ѿ������˶����飬�Ҷ������е�һ����δ������Ҫ�ּ�������ˣ�һ���ǵ�ǰ�������е�һ����δ��
		//����һ����պ����ˣ�Ҫ����һ��һ�����һ���µ��㼶�顣
		//��������Ϣ�����ڶ��������У�һ������Ϣ������һ�������С�
		if( ((m-1031)%1024) !=0)	//һ����δ��,д�뵱ǰ��һ�����С�һ�����λ��Ҳ�ܼ���
		{
			bptr=(FSU32 *)file->level1_buffer;
			bptr[(m-1031)%BLOCK_PER_BLOCK]=block;		//ˢ�µ�ǰ�����ݡ�	//�����⡣
			bptr=(FSU32 *)file->level2_buffer;
			tmp=bptr[(m-1031)/BLOCK_PER_BLOCK];			//�ҵ���ǰ��һ����
			tmp=get_first_sector(tmp);			//��ǰ�ļ�һ����ĵ�һ��������
			n=( ((m-1031+1)%BLOCK_PER_BLOCK)*sizeof(FSU32) )/SECTOR_PER_SIZE;
			tmp+=n;
			//write_block(block,file->level1_buffer);			//��һ������д�룬�����㷨���Ӷȡ�
			write_sector(tmp,&(file->level1_buffer[n*SECTOR_PER_SIZE]));
			ptr->block_alloc++;
			//write_sector(info.sector,file->file_write_buffer);	
		}
		else			//����Ϊ������һ�������顣
		{
			bptr=(FSU32 *)file->level2_buffer;
			bptr[((m-1031)/BLOCK_PER_BLOCK)]=block;			//һ���µĶ����顣
			tmp=get_first_sector(ptr->level2_block);
			n=( ((m-1031+1)/BLOCK_PER_BLOCK)*sizeof(FSU32) )/SECTOR_PER_SIZE ;
			tmp=tmp+n;
			//write_block(info.level2_block,file->level2_buffer);
			write_sector(tmp,&(file->level2_buffer[n*SECTOR_PER_SIZE]));
			if((tmp=pop_free()) == 0)			//�ٷ���һ��һ���顣
			{
				return 0;
			}
			bptr=(FSU32 *)file->level1_buffer;
			*bptr=tmp;
			write_sector(get_first_sector(block),file->level1_buffer);		//��block��ĵ�һ��������һ��λ��д����Ϣ��
			block=tmp;
			ptr->block_alloc++;
			//write_sector(info.sector,file->file_write_buffer);
		}
	}
	else
	{
		return 0;				 //�������ļ�������С�������ٽ��з����ˡ�
	}
				// ���£�������䲻�ɹ��Ļ��Ͳ����¡�
	return block;
}

void update_free(void)				// ��������������1�����³����飬���¿��п�
{									//��Ȼ���п��Ѿ������ˣ�����û�и������֮ǰ���з���Ŀ�ʵ���ϻ�û�б�ע�ᡣ
	write_sector(sp->free_block_cur_sector,_free_block_buffer);
	write_sector(sp->super_block_first_sector,_super_block_buffer);
}





