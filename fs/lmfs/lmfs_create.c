#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include"lmfs_config.h"
#include"lmfs.h"
#include"device_drive.h"


/********************************
可以加入提醒函数打印文件读写失败
的原因。作为可选功能。
*********************************/


PLMFILE create_file(char *name,char attrib)		//在指定的目录下创建一个文件。
{
	PLMFILE tmp_file;		//包括创建目录也是使用此函数。根据属性来辨别。

	NODE_INFO dir_up;
	
	NODE file_node;

	char tmp_name[60];

	char *token_pos;

	FSU32 i=0;					//跟踪文件计数。
	FSU32 pos=0;				//跟踪文件在缓冲区中的位置。

	if(!check_filename(name))		//检查文件名是否有效。			
	{
		return NULL;
	}
	
	tmp_file=create_tmp_file();	

	//检查此目录是否存在，或者是一个普通文件。
	if(*name == '/')			//如果要从根目录下开始搜索。
	{
		name++;

		dir_up=get_node_info(0,tmp_file->node_buffer);	//获得根目录节点信息，并把根目录信息所在的扇区存放于buffer中，创建文件的时候会使用。

		if(*(lm_strtok(name,'/')) == 0)				//要在根目录下创建文件。
		{
			file_node=search_file(tmp_file,name,&dir_up,&pos,&i);

			if(file_node !=0)			//文件已经存在。
			{
				close_file(tmp_file);
				return NULL;
			}
			//文件不存在；创建一个文件
			file_node=lm_create_file(tmp_file,name,&dir_up,attrib,pos);		//应该还要加上i和POS

		}
		else			//如果不是在根目录下建立的话，查找此目录是否存在。
		{
			token_pos=lm_strrtok(name,'/');		
				
			strcpy(tmp_name,token_pos);		//存储文件的名字。

			token_pos--;

			*token_pos=0;

			file_node=search_file(tmp_file,name,&dir_up,&pos,&i);	// 查找文件所在目录是否存在，。

			if(file_node == 0)
			{
				//目录不存在。
				close_file(tmp_file);
				return NULL;
			}

			dir_up=get_node_info(file_node,tmp_file->node_buffer);		//获取文件所在目录的节点信息。并存储在file的写buffer中。	

			if(dir_up.file_type != DIR)			//如果上一级目录不是目录。
			{
				close_file(tmp_file);
				return NULL;
			}
			
			file_node=search_file(tmp_file,tmp_name,&dir_up,&pos,&i);

			if(file_node !=0)
			{
				//此文件已经存在
				close_file(tmp_file);
				return NULL;
			}

			file_node=lm_create_file(tmp_file,name,&dir_up,attrib,pos);
		}
	}
	else		//在当前目录下创建。
	{
		dir_up=get_node_info(cur_dir_node.level0_block[0],tmp_file->node_buffer);
		
		file_node=search_file(tmp_file,name,&dir_up,&pos,&i);

		if(file_node !=0)
		{
			close_file(tmp_file);
			return NULL;
		}

		file_node=lm_create_file(tmp_file,name,&dir_up,attrib,pos);		//根据i和pos可以很快知道要创建文件信息将要写入哪里。
	}
	
	if(file_node == 0)		//创建文件失败
	{
		close_file(tmp_file);
		return NULL;
	}

//	dir_up=get_node_info(file_node,tmp_file->node_buffer);		//如果之前文件已经创建成功，它的节点信息也已经写入，获取它的节点信息。

	get_file_info(tmp_file,&dir_up,file_node,WRITE);				//获取要操作文件的信息，返回。

	return tmp_file;
}

NODE lm_create_file(PLMFILE file,char *name,PNODE_INFO info,unsigned char attrib,FSU32 pos)
{
	//在节点信息为info的目录下创建一个文件。
	//之前一切顺利的话当前目录非配的最后一个快的内容存在于read-buffer中。
	//pos就是他在buffer中的位置。
	//此时tmpfile中的缓冲区都存在了重要的数据，因为是创建所以一二级的缓冲区能够用来存放数据。】
	//要操作文件的话需要创建一个零食文件，在结束时把零食文件删除就行。
	NODE file_node;

	FSU32 x=0,y=0;

	PNODE_INFO ptr;

	PCATALOG_INFO cptr;

	file_node=pop_free();			//弹出一个空闲块，空闲块的号就是文件的节点号。包括跟新
	//跟新的任务交给自己							//采用这种方法的缺点是如果文件的大小为0，它也会占有一个快的空间，照成浪费。但不需要搜索。
	if(file_node == 0)				//检查是否分配成功。
	{
		return 0;
	}
//完成更新。 接着填写文件信息至目录，目录信息已经存在于read——buffer中
//根据pos和i可以算出要写入到哪个扇区。并且此系统为存在于最后分配的那个快中。
//更新文件所在目录的信息。主要是写入文件信息至目录。
	//read_sector(info->sector,file->node_buffer);		//重新读取目录节点信息到缓冲，有点多余，以后再解决。可以把更新文件的信息吊在更新目录信息之后。

	ptr=(PNODE_INFO)(&file->node_buffer[NODE_INFO_SIZE*info->pos_in_sector]);			// 获得上级文件夹得节点信息。

	if(pos == FILE_PER_BLOCK)		//目录当前记录已满，需要分配一个新块。
	{
		//在新分配的块上写入目录信息。x为新分配过来的目录数据块。在它的第一个位置写入信息。
		if((x=alloc_new_block(file,ptr)) == 0) return 0;		//由于是在新块上建立的直接写入第一个位置。
		cptr=(PCATALOG_INFO)file->file_rw_buffer;		//分配块不成功的话直接返回。
		cptr->node=file_node;
		strcpy(cptr->name,name);
		write_sector(get_first_sector(x),file->file_rw_buffer);		//跟新目录信息。
	}
	else
	{
		//在已有块上写入目录信息。pos记录了位置
		x=get_last_file_pos(file,info);			//x记录了最后一个文件所在的块。
		cptr=(PCATALOG_INFO)file->file_rw_buffer;
		cptr+=pos;
		cptr->node=file_node;
		strcpy(cptr->name,name);
		x=get_first_sector(x);
		y=(pos*sizeof(FSU32))/SECTOR_PER_SIZE;
		write_sector(x+y,&(file->file_rw_buffer[y*SECTOR_PER_SIZE]));		//更新目录信息。
	}

	ptr->file_amount++;

	update_free();			//在此进行更新空闲区块的空间	

	write_sector(info->sector,file->node_buffer);		//跟新目录信息，文件是否存在的标志是目录中文件数是否被加1
	// 此处有问题。与前面的写入文件节点信息由冲突。													//发生掉电的话会照成分配的块的丢失。
	
	get_node_pos(&x,&y,file_node);			

	read_sector(x,file->node_buffer);			//此节点是分配给文件的节点。读取此节点所在扇区的信息。
// 填写文件的节点信息。并写入。
	ptr=(PNODE_INFO)(&(file->node_buffer[y*NODE_INFO_SIZE]));
	ptr->block_alloc=1;
	ptr->file_amount=0;
	ptr->file_attrib=attrib;			//????判读是否是普通文件还是目录，是目录的话为4096，是文件的话为0
	ptr->file_size=0;
	ptr->file_type=attrib&0x0f;
	ptr->last_modify_time=TIME;			//时间的话以后再实现。需操作系统提供时间函数。
	ptr->level0_block[0]=file_node;
	ptr->used=1;
	ptr->unused=0;
	ptr->sector=x;
	ptr->pos_in_sector=y;
	write_sector(x,file->node_buffer);

	return file_node;
}

NODE pop_free(void)			//在进行此操作时，需要发信号。
{
	NODE file_node=0;

	file_node=*free_pos;

	if(!file_node)			//如果分配的块为0；说明没有空闲块了，设备没有空闲空间了。
	{
		return 0;			//也就不进行信息的更新了。
	}

	*free_pos=0;
	free_pos++;
	sp->free_block_cur_pos++;
	sp->free_block_leave--;

	if(sp->free_block_cur_pos == FREE_PER_SECTOR)			// 当前扇区的空闲块已全部用完。就要读取下一个扇区
	{
												//的内容，并更新相关信息。
		sp->free_block_cur_sector++;
		sp->free_block_cur_pos=0;
		read_sector(sp->free_block_cur_sector,_free_block_buffer);
		free_pos=(FSU32 *)_free_block_buffer;
	}
	return file_node;
}

FSU32 get_last_file_pos(PLMFILE file,PNODE_INFO info)	//获得目录中最后一个文件所在的块号。
{
	FSU32 m;
	FSU32 *y;
	FSU32 block;
	m=info->block_alloc;
	if(m<8)			//如果文件分配的块小于8.
	{
		block=info->level0_block[m-1];
	}
	else if(m<1032)		//如果文件在一级块中。
	{
		y=(FSU32 *)file->level1_buffer;
		block=y[m-7-1];
	}
	else			//在二级块中。
	{
		y=(FSU32 *)file->level1_buffer;
		block=y[(m-1031)%BLOCK_PER_BLOCK];
	}

	return block;
}
//给文件分配不能用这个函数太慢。适合给目录分配。
FSU32 alloc_new_block(PLMFILE file,PNODE_INFO ptr)		//给目录分配一个新块，写入目录节点中。
{
	FSU32 block;			//适合数据和目录的分配，不更新其他信息。					//
	FSU32 m,tmp,n;
	FSU32 *bptr;

	m=ptr->block_alloc;			//节点的数据存在于文件write_buffer中，一个扇区的数据。

	if((block=pop_free()) == 0)		//分配给文件的新块，如果是一级块时做一级块使用。
	{
		return 0;
	}
	if(m<7)				//分情况啦。而且只能有一种情况出现，保证了缓冲区的数据不会有变化。
	{
		//更新节点信息。不需要更新其他的块。
		ptr->level0_block[m]=block;
		ptr->block_alloc++;
		//write_sector(info.sector,file->file_write_buffer);
					//接着返回。
	}
	else if(m=7)			// 如果已分配的块数为7；则需要分配一级块。第一次分配
	{						//也说明file->level1_buffer中的数据没意义，可以来作为缓冲。
		ptr->level1_block=block;
		//不需要读出此块的内容直接可以进行写入，位于它的第一块区域。
		if((block=pop_free()) == 0)			//这个快则是分配给目录记录文件信息的。要返回的块。
		{
			return 0;
		}
		bptr=(FSU32 *)file->level1_buffer;
		*bptr=block;
		write_sector(get_first_sector(ptr->level1_block),file->level1_buffer);
	//	write_sector(info.sector,file->file_write_buffer);
	}
	else if(m<1031)			//在一级块中写入。分配的号。此时level1保存着一级块中的信息。
	{
		bptr=(FSU32 *)file->level1_buffer;
		bptr[m-7]=block;
		tmp=get_first_sector(file->level1_block);
		n=( (m-7+1)*(sizeof(FSU32)) /SECTOR_PER_SIZE);			//要不要加1还没有验证。
		tmp=tmp+n;			//这个可能有问题。
		write_sector(tmp,&(file->level1_buffer[n*SECTOR_PER_SIZE]));
		ptr->block_alloc++;
		//write_sector(info.sector,file->file_write_buffer);
	}
	else if(m=1031)			//需要为目录分配第一个二级块
	{
		ptr->level2_block=block;			//二级块，说明level2的缓冲液可以使用
		if((tmp=pop_free()) == 0)			//再给二级块分配一个一级块。
		{
			return 0;
		}
		bptr=(FSU32 *)file->level2_buffer;
		*bptr=tmp;		//写入信息。
		write_sector(get_first_sector(ptr->level2_block),file->level2_buffer);
		if((block=pop_free()) == 0)			//再给二级块分配一个零级块。
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
		//如果已经分配了二级块，且二级块中的一级块未满。又要分几种情况了，一种是当前二级块中的一级块未满
		//二是一级块刚好满了，要分配一个一级块和一个新的零级块。
		//二级块信息存在于二级缓冲中，一级块信息存在于一级缓存中。
		if( ((m-1031)%1024) !=0)	//一级块未满,写入当前的一级块中。一级块的位置也能计算
		{
			bptr=(FSU32 *)file->level1_buffer;
			bptr[(m-1031)%BLOCK_PER_BLOCK]=block;		//刷新当前块数据。	//有问题。
			bptr=(FSU32 *)file->level2_buffer;
			tmp=bptr[(m-1031)/BLOCK_PER_BLOCK];			//找到当前的一级块
			tmp=get_first_sector(tmp);			//当前文件一级快的第一个扇区。
			n=( ((m-1031+1)%BLOCK_PER_BLOCK)*sizeof(FSU32) )/SECTOR_PER_SIZE;
			tmp+=n;
			//write_block(block,file->level1_buffer);			//把一块数据写入，减轻算法复杂度。
			write_sector(tmp,&(file->level1_buffer[n*SECTOR_PER_SIZE]));
			ptr->block_alloc++;
			//write_sector(info.sector,file->file_write_buffer);	
		}
		else			//还需为他分配一个二级块。
		{
			bptr=(FSU32 *)file->level2_buffer;
			bptr[((m-1031)/BLOCK_PER_BLOCK)]=block;			//一个新的二级块。
			tmp=get_first_sector(ptr->level2_block);
			n=( ((m-1031+1)/BLOCK_PER_BLOCK)*sizeof(FSU32) )/SECTOR_PER_SIZE ;
			tmp=tmp+n;
			//write_block(info.level2_block,file->level2_buffer);
			write_sector(tmp,&(file->level2_buffer[n*SECTOR_PER_SIZE]));
			if((tmp=pop_free()) == 0)			//再分配一个一级块。
			{
				return 0;
			}
			bptr=(FSU32 *)file->level1_buffer;
			*bptr=tmp;
			write_sector(get_first_sector(block),file->level1_buffer);		//在block块的第一个扇区第一个位置写入信息。
			block=tmp;
			ptr->block_alloc++;
			//write_sector(info.sector,file->file_write_buffer);
		}
	}
	else
	{
		return 0;				 //超过了文件的最大大小，不能再进行分配了。
	}
				// 更新，如果分配不成功的话就不更新。
	return block;
}

void update_free(void)				// 包括两个工作（1）更新超级块，更新空闲块
{									//虽然空闲块已经分配了，但在没有更新这个之前所有分配的块实际上还没有被注册。
	write_sector(sp->free_block_cur_sector,_free_block_buffer);
	write_sector(sp->super_block_first_sector,_super_block_buffer);
}





