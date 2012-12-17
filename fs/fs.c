#include <os/types.h>
#include <os/mm.h>
#include <os/printk.h>
#include <os/list.h>

struct lmfs{
	struct disk_driver *disk;
	struct super_block *super_block;
	char *free_block_buf;
	struct node *root_node;
	struct mutex fs_mutex;
};

static struct lmfs *lmfs;

static inline int read_sectors(u32 start,char *buf,int count)
{
	return lmfs->disk->read_sectors(start,buf,count);
}

static inline int write_sectors(u32 start,char *buf,int count)
{
	return lmfs->disk->write_sectors(start,buf,count);
}

static inline int read_one_sector(u32 start,char *buf)
{
	return lmfs->disk->read_sectors(start,buf,1);
}

static inline int write_one_sector(u32 start,char *buf)
{
	return lmfs->disk->write_sectors(start,buf,1);
}

static struct lmfs *alloc_and_init_lmfs(void)
{
	int err = 0;

	lmfs = (struct lmfs *)kmalloc(sizeof(struct lmfs),GFP_KERNEL);
	if(lmfs == NULL){
		kernel_error("can not alloct lmfs\n");
		return -ENOMEM;
	}

	lmfs->disk_dirver = get_root_disk();
	if(lmfs->disk_driver == NULL){
		err = -ENODEV;
		kenrel_error("no device on this system\n");
		goto no_root_dev;
	}
	
	lmfs->super_block = kmalloc(SECTOR_SIZE,GFP_KERNEL);
	if(lmfs->super_block ==NULL){
		err = -ENOMEM;
		kernel_error("can not allocate memory for super_block\n");
		goto super_block_fail;
	}

	lmfs->free_block_buf = kmalloc(SECTOR_SIZE,GFP_KERNEL);
	if(lmfs->free_block_buf == NULL){
		err = -ENOMEM;
		kernel_error("can not allocate memory for free_block_buf\n");
		goto free_block_fail;
	}

free_block_fail:
	kfree(lmfs->super_block);

super_block_fail:
	release_disk(lmfs->disk_driver);

no_root_dev:
	kfree(lmfs);

	return err;
}

int init_fs(void)
{
	char *tmp;
	int err = 0;
	struct lba_partition_entry *pentry;

	disk = get_root_disk();
	if(!disk){
		kernel_error("can not get root disk\n");
		retutn -ENODEV;
	}

	tmp = kmalloc(SECTOR_SIZE,GFP_KERNEL);
	if(!tmp)
		return -ENOMEM;

	super_block = (struct super_block *)kmalloc(512,GFP_KERNEL);
	if(!super_block)
	{
		kfree(tmp);
		return -ENOMEM;
	}

	/*
	 * read the first sector of the disk to get 
	 * the partition information
	 */
	read_one_sectors(0,tmp);
	pentry = &tmp[446];
	
	/*
	 * get the super block informatition
	 * these data will used in furture
	 */
	read_one_sector(pentry->base_sector,super_block);
	if(super_block->fs_id != 0x88){
		kernel_error("partiton is not lmfs patition, can not reconlize\n");
		err = -ENODEV;
		goto out;
	}

	/*get infotmation of the root node*/
	init_mutex(&fs_mutex);
	root_node = &super_block->root;

	/*
	 *we need known how many free block in this partition
	 *and this buffer can help us find the free block quickly
	 */
	free_block_buf = (char *)kmalloc(SECTOR_SIZE,GFP_KERNEL);
	if(!free_block_buf){
		kernel_error("no mem for free_block_buf\n");
		err = -ENOMEM;
		goto out;
	}

	kfree(tmp);

	return 0;

out:
	kfree(tmp);
	kfree(super_block);
	super_block = NULL;

	return err;
}
