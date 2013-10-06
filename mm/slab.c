#include <os/slab.h>
#include <os/mm.h>
#include <os/types.h>
#include <os/list.h>
#include <os/bound.h>
#include <os/printk.h>
#include <os/string.h>

/* 
 * every allocted memory has a header named slab_attr
 * size: size of this allocated memory
 * plist: connect to the page it belongs to
 * slab_list: connect to the slab free list which
 * in his size
 */
struct slab_header {
	unsigned long base;
	int size;
	union {
		struct list_head plist;
		struct list_head slab_list;
		struct list_head header_list;
	};
};

/*
 * each memory allcoated by slab has a struct named 
 * slab_header,this data struct used to manage the memory 
 * which allocated out and which is free in slab
 * page_current: current page used to allocat slab header
 * table_list: connect the all page that used to allcated 
 * page header
 * free_header: when used kfree() to free a allocted memory
 * the header will free too. then when need a free header
 * to record allocated memory ,this list can help to
 * find free headers quickly
 * table_total_pages: how many page habe been used as slab header
 * page_current: the current page in table_list
 * header_in_page: the current free slab_header in page
 */
struct slab_header_table {
	struct page *table_page_current;
	struct list_head table_list;
	struct list_head free_header;
	int table_total_pages;
	int page_current;
	int header_in_page;
};

#define SLAB_HEADER_SIZE	sizeof(struct slab_header)
#define SLAB_MIN		16
#define SLAB_STEP		16
#define SLAB_NODE		((PAGE_SIZE-SLAB_HEADER_SIZE)/SLAB_MIN)
#define SLAB_MAX		(SLAB_NODE*SLAB_STEP)

/*
 * repersent a slab memory pool in system
 * current:page that used to allocate new memory just now.
 * list:memory cache list
 */
struct slab {
	struct mutex slab_mutex;
	struct page *slab_page_current;
	struct slab_header_table table;
	struct list_head list[SLAB_NODE];
};

static struct slab slab;
struct slab *pslab = &slab;

#define list_id(size)		((size>>4) -1)

#define FREE_HEADER_FROM_PAGE		0
#define FREE_HEADER_FROM_SLAB		1
static void __kfree_header_slab(struct page *pg, unsigned long addr);

static inline int get_new_alloc_size(int size, unsigned long flag)
{
	if ((flag & GFP_DMA) || (flag &GFP_RES))
		return baligin(size, PAGE_SIZE);

	return baligin(size, SLAB_STEP);
}

#define SLAB_TO_PAGE	0
#define PAGE_TO_SLAB	1
void inline header_list_switch(struct slab_header *header,
		               struct page *pg,
			       struct list_head *list,
			       int dir)
{
	if (dir) {
		list_del(&header->plist);
		list_add(list, &header->slab_list);
	} else {
		list_del(&header->slab_list);
		list_add(&pg->slab_list, &header->plist);
	}
}

static inline void add_slab_header_to_page(struct page *pg,
				struct slab_header *header)
{
	list_add(&pg->slab_list, &header->plist);
}

static inline void add_slab_header_to_list(struct slab_header *header)
{
	struct list_head *list;

	list = &pslab->list[list_id(header->size)];
	list_add(list, &header->slab_list);
}

static inline void init_slab_header(struct slab_header *header,
		                    int size, unsigned long base)
{
	header->base = base;
	header->size = size;
	init_list(&header->plist);
}

static inline struct slab_header *get_slab_header_from_page(struct page *pg)
{
	struct slab_header *header;

	header = (struct slab_header *)pg->free_base;
	pg->free_base = pg->free_base + SLAB_HEADER_SIZE;
	pg->free_size -= SLAB_HEADER_SIZE;
	page_get(pg);

	return header;
}

static struct slab_header *get_new_slab_header(void)
{
	struct slab_header_table *table = &pslab->table;
	struct list_head *free = &table->free_header;
	struct slab_header *header = NULL;
	struct page *pg = table->table_page_current;
	unsigned long base;

	/*
	 * find if there are headers in free list freed
	 * by other slab
	 */
	if (!is_list_empty(free)) {
		header = list_first_entry(free, struct slab_header, header_list);
		list_del(&header->header_list);
		return header;
	}

	/*
	 * find slab header in current slab header page
	 */
	if (table->header_in_page < (PAGE_SIZE/SLAB_HEADER_SIZE)) {
		if (pg != NULL) {
			table->header_in_page += 1;
			return get_slab_header_from_page(pg);
		}
	}

	/*
	 * get a new page to used as slab_header
	 */
	base = (unsigned long )get_free_page(GFP_SLAB_HEADER);
	if (!base) {
		mm_error("allocate page for slab_header failed\n");
		return NULL;
	}
	pg = va_to_page(base);

	/*
	 * at this time we needn't decrease the usage
	 * of the page this action will keep the slab
	 * header page not free
	 */
	pg->free_base = page_to_va(pg);
	pg->free_size = PAGE_SIZE;

	/*
	 * update slab_header_table information
	 */
	list_add(&table->table_list,&pg->slab_header_list);
	table->table_total_pages += 1;
	table->page_current = table->table_total_pages - 1; //base is 0
	table->header_in_page = 1;	//free point
	table->table_page_current = pg;

	header = get_slab_header_from_page(pg);

	return header;
}

/*
 * get a new slab mem from a new page.
 */
static struct slab_header *get_slab_from_page(struct page *pg, int size)
{
	struct slab_header *header;

	header = get_new_slab_header();
	if(header == NULL)
		return NULL;

	/*
	 * fix me : if size > pg->free_size;
	 */
	init_slab_header(header, size, pg->free_base);
	pg->free_size -= size;
	pg->free_base += size;
	add_slab_header_to_page(pg, header);
	page_get(pg);
	
	return header;
}


/*
 * system slab
 * when need to update the current free page of slab
 * we compare the size of this two pages,and biger ones
 * will be the current free page.
 */
static void update_slab_free_page(struct page *pg)
{
	struct page *current_pg = pslab->slab_page_current;
	struct page *new = NULL;
	struct page *old = NULL;
	struct slab_header *header = NULL;

	if (current_pg == NULL) {
		new = pg;
		old = NULL;
	} else if (pg->free_size > current_pg->free_size){
		new = pg;
		old = current_pg;
	} else {
		new = current_pg;
		old = pg;
	}

	pslab->slab_page_current = new;
	if (old != NULL && old->free_size > 16) {
		header = get_slab_from_page(old, old->free_size);
		add_slab_header_to_list(header);
	}
}

/*
 * find memory from slab cache list
 */
static void *get_slab_in_list(int size, unsigned long flag)
{
	struct list_head *list;
	struct slab_header *header;
	struct page *pg;
	
	if (!(flag & GFP_KERNEL))
		return NULL;

	list = &pslab->list[list_id(size)];
repet:
	if (is_list_empty(list))
		return NULL;

	/*
	 * get the slab_header to find the free memory
	 */
	header = list_first_entry(list,struct slab_header,slab_list);

	/*
	 * check whether the page has been released.
	 */
	if (!page_state(va_to_page_id(header->base))) {
		__kfree_header_slab(va_to_page(header->base), header->base);
		goto repet;
	}

	/*
	 * update the page information
	 * delete slab from cache list
	 * add slab to page list
	 */
	pg = va_to_page(header->base);
	header_list_switch(header, pg, list, SLAB_TO_PAGE);

	return (void *)(header->base);
}


static void *get_slab_from_page_free(int size, unsigned long flag)
{
	struct page *pg;
	struct slab_header *header;

	pg = pslab->slab_page_current;
	if (pg == NULL)
		return NULL;

	/*
	 * if the page has been released,we need a new page
	 */
	if (!page_state(page_to_page_id(pg))) {
		pslab->slab_page_current = NULL;
		return NULL;
	}

	if(size > pg->free_size)
		return NULL;

	header = get_new_slab_header();
	if (header == NULL)
		return NULL;

	init_slab_header(header, size, pg->free_base);
	pg->free_size -= size;
	pg->free_base += size;
	add_slab_header_to_page(pg, header);
	page_get(pg);

	return (void *)(header->base);
}

static void *get_kernel_slab(int size, unsigned long flag)
{
	int count;
	unsigned long base,endp;
	struct page *pg;
	int leave_size = 0;

	if (is_aligin(size, PAGE_SIZE))
		return get_free_pages(page_nr(size), flag);

	count = page_nr(size);
	leave_size = size - (count - 1) * PAGE_SIZE;
	base =(unsigned long ) get_free_pages(count, flag);
	if (!base) {
		mm_error("get memory faile at get_kernel_slab\n");
		return NULL;
	}

	endp = base + (count-1) * PAGE_SIZE;

	/*
	 * first we need modify the information of the full page;
	 * for the header page, we override the free_base scope
	 * to record how many slice has been allocated.
	 */
	if (count > 1) {
		pg = va_to_page(base);
		pg->count--;
		pg->extra_size = leave_size;
	}

	/*
	 * now we modify the last page information
	 */
	pg = va_to_page(endp);
	pg->flag |= __GFP_SLAB;
	//pg->free_size = PAGE_SIZE;

	/*
	 * the last page was used as SLAB memory,so we need
	 * sub the conter.
	 */
	pg->usage--;

	/*
	 *since the result is base so we ingrion the return value
	 */
	get_slab_from_page(pg, leave_size);

	/*
	 * update the slab's current free page;
	 */
	update_slab_free_page(pg);

	return (void *)base;
}

static void *get_new_slab(int size, unsigned long flag)
{
	if (flag == GFP_DMA || flag == GFP_RES)
		return get_free_pages(page_nr(size), flag);

	return get_kernel_slab(size, flag);
}

static void *__kmalloc(int size, unsigned long flag)
{
	int new_size = get_new_alloc_size(size, flag);
	void *ret = NULL;

	mutex_lock(&pslab->slab_mutex);

	if (flag == GFP_DMA ||
	    flag == GFP_RES ||
	    size >= PAGE_SIZE) {
		goto new_slab;
	}

	/*
	 * first we find slab in cache list
	 */
	ret = get_slab_in_list(new_size, flag);
	if (ret)
		goto exit;
	
	/*
	 * cache list does not cotain the memory we needed
	 * we get memory from current free page;
	 */
	ret = get_slab_from_page_free(new_size, flag);
	if (ret)
		goto exit;
	
	/*
	 * finally we need the allocater allocte a new slab
	 */
new_slab:
	ret = get_new_slab(new_size, flag);

exit:
	mutex_unlock(&pslab->slab_mutex);
	return ret;
}

void *kmalloc(int size, unsigned long flag)
{
	if (size <= 0)
		return NULL;

	if ((flag != GFP_KERNEL) &&
	    (flag != GFP_DMA) &&
	    (flag != GFP_RES))
		return NULL;

	return __kmalloc(size, flag);
}

void *kzalloc(int size,unsigned long flag)
{
	char *ret;

	ret = (char *)kmalloc(size, flag);
	memset(ret, 0, size);

	return (void *)ret;
}

static void __kfree_header_slab(struct page *pg, unsigned long addr)
{
	struct slab_header *header = (struct slab_header *)addr;
	struct list_head *free = &pslab->table.free_header;

	list_del(&header->slab_list);
	list_add(free, &header->header_list);
	page_put(pg);
}

static void __kfree_slice_slab(struct page *pg, unsigned long addr)
{
	struct list_head *head = &pg->slab_list;
	struct slab_header *header;
	struct list_head *list;
	int flag = 0;

	/*
	 * first we need to find the slab_header of
	 * this memory section.
	 */
	list_for_each(head,list) {
		header = container_of(list,struct slab_header,plist);
		if (header->base == addr) {
			flag = 1;
			break;
		}
	}

	if (flag) {
		list_del(&header->plist);
		add_slab_header_to_list(header);
		page_put(pg);
	} else {
		mm_error("need to notice users\n");
	}
}

static void __kfree_page_slab(struct page *pg, void *addr)
{
	struct page *last_page;
	int count = pg->count;
	
	last_page = pg + count;
	free_pages(pg);

	__kfree_slice_slab(last_page, page_to_va(last_page));
}

void kfree_kernel_mem(void *addr)
{
	struct page *pg;
	unsigned long flag;

	mutex_lock(&pslab->slab_mutex);

	pg = va_to_page((unsigned long)addr);
	flag = pg->flag;

	if (flag & __GFP_SLAB)
		__kfree_slice_slab(pg, (unsigned long)addr);
	else {

		if (!is_aligin((unsigned long)addr, PAGE_SIZE))
			return;

		__kfree_page_slab(pg, addr);
	}

	mutex_unlock(&pslab->slab_mutex);
}

void kfree_user_mem(void *addr)
{

}

void kfree(void *addr)
{
	if (addr != NULL) {
		if ((unsigned long)addr < SIZE_1G)
			kfree_kernel_mem(addr);
		else
			kfree_user_mem(addr);
	}
}

int slab_init(void)
{
	int i;

	mm_info("Slab allocter init\n");

	for (i = 0; i < SLAB_NODE; i++)
		init_list(&pslab->list[i]);

	init_mutex(&pslab->slab_mutex);
	
	pslab->table.table_page_current = NULL;
	init_list(&pslab->table.table_list);
	init_list(&pslab->table.free_header);
	pslab->table.table_total_pages = 0;
	pslab->table.page_current = -1;
	pslab->table.header_in_page = -1;

	return 0;
}

