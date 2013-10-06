#ifndef _MM_H
#define _MM_H

#include <os/list.h>
#include <os/types.h>
#include <os/bound.h>
#include <os/mutex.h>
#include <os/mirros.h>

struct platform_info;

#define NORMAL_MEM		0
#define DMA_MEM			1
#define RES_MEM			2
#define IO_MEM			3
#define MM_ZONE_MASK		0x3
#define UNKNOW_MEM		4

typedef enum __mm_zone_t {
	MM_ZONE_NORMAL = NORMAL_MEM,
	MM_ZONE_DMA = DMA_MEM,
	MM_ZONE_RES = RES_MEM,
	MM_ZONE_IO = IO_MEM,
	MM_ZONE_UNKNOW,
} mm_zone_t;

/*
 *indicate which zone memory needed to be allocted.
 */
#define __GFP_KERNEL		0x00000001
#define __GFP_DMA		0x00000002
#define __GFP_RES		0x00000004
#define __GFP_IO		0x00000008

#define GFP_ZONE_ID_MASK	0x0000000f

/*
 *indicate the usage of the memory
 */
#define __GFP_PGT		0x00000010
#define __GFP_PAGE_HEADER	0x00000020
#define __GFP_SLAB		0x00000040
#define __GFP_SLAB_HEADER	0x00000080
#define __GFP_USER		0x00000100

#define GFP_KERNEL		(__GFP_KERNEL)
#define GFP_USER		(__GFP_KERNEL | __GFP_USER)
#define GFP_RES			(__GFP_RES)
#define GFP_PGT			(__GFP_KERNEL | __GFP_PGT)
#define GFP_DMA			(__GFP_DMA)
#define GFP_FULL		(__GFP_KERNEL | __GFP_FULL)
#define GFP_SLAB		(__GFP_KERNEL | __GFP_SLAB)
#define GFP_SLAB_HEADER		(__GFP_SLAB_HEADER | __GFP_KERNEL)

#define __GFP_MASK		(__GFP_PGT | __GFP_FULL |__GFP_SLICE | __GFP_SLAB_HEADER | __GFP_USER)

#define mm_debug(fmt, ...)	debug("[  mm:  ] ", fmt,##__VA_ARGS__)
#define mm_info(fmt, ...)	info("[  mm:  ] ", fmt,##__VA_ARGS__)
#define mm_error(fmt, ...)	error("[  mm:  ] ", fmt,##__VA_ARGS__)

struct memory_region {
	unsigned long start;
	u32 size;
	int attr;
};

/*page:represent 4k in physical memory
 * virtual_address:virtual address that page maped to 
 * flag:attribute of the page
 * free_base: if page is used as kernel allocation,the free base of page
 * free_size:remain size of this page
 * usage:usage of this page,if 0 page can release.
 */
struct page {
	unsigned long phy_address;
	u32 flag;
	/*
	 * if page used as a pgt then use pgt_list
	 * if page used as slab elment then use slab_list
	 */
	union {
		struct list_head plist;
		struct list_head pgt_list;
		struct list_head slab_list;
		struct list_head slab_header_list;
	};

	union {
		unsigned long free_base;
		u32 extra_size;
	};

	/*
	 *if flag PAGE_FULL the count used to indicate how many 
	 *continueous pages were alloctated,else if flag PAGE_SLAB
	 *then free_size indicate how many free size in this page.
	 */
	
	u32 free_size : 16;
	u32 count : 8;
	u32 usage : 8;
};

#define PAGE_SIZE		4096
#define PAGE_SHIFT		12

#define page_nr(size)		(baligin(size,PAGE_SIZE)>>PAGE_SHIFT)

void register_memory_region(unsigned long start,
			    u32 size,int attr,
			    struct platform_info *info);

void page_get(struct page *pg);

void free_pages(void *addr);

void page_put(struct page *pg);

void  *get_free_pages(int count,unsigned long flag);
	
static inline void *get_free_page(unsigned long flag)
{
	return get_free_pages(1, flag);
}

int page_state(int i);
/*
 *fix me why can not use (struct page *)(&page_table[i]);
 */
struct page *get_page(int i);
/*some macro to covert page and PA with each other*/
struct page *va_to_page(unsigned long va);
unsigned long page_id_to_va(int index);
int va_to_page_id(unsigned long va);
unsigned long pa_to_va(unsigned long pa);
int page_to_page_id(struct page *page);
u32 mm_free_page(unsigned long flag);
unsigned long page_to_va(struct page *page);
void copy_page_va(u32 target, u32 source);
void copy_page_pa(u32 target, u32 source);
void copy_page_ua(u32 taget, u32 source);
struct page *pa_to_page(unsigned long pa);
unsigned long va_to_pa(unsigned long va);
unsigned long page_to_va(struct page *page);
unsigned long page_to_pa(struct page *page);
void *get_free_page_aligin(unsigned long aligin, u32 flag);

#endif
