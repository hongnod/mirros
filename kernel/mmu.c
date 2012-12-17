#include <os/types.h>
#include <os/printk.h>
#include <os/mmu.h>
#include <os/string.h>
#include <os/sched.h>
#include <os/mm.h>
#include <os/sched.h>
#include <asm/asm_mmu.h>
#include <os/bound.h>

static inline void flush_mmu_tlb(void)
{
	arch_flush_mmu_tlb();
}

static inline unsigned long get_tlb_base(void)
{
	return arch_get_tlb_base();
}

static inline u32 build_tlb_des(unsigned long pa,u32 attr)
{
	return arch_build_tlb_des(pa,attr);
}

static inline u32 build_page_table_des(unsigned long pa,u32 attr)
{
	return arch_build_page_table_des(pa,attr);
}

static inline u32 get_tlb_attr(u32 flag)
{
	return arch_get_tlb_attr(flag);
}

static inline u32 get_page_table_attr(u32 flag)
{
	return arch_get_page_table_attr(flag);
}

void clear_tlb_entry(unsigned long va,size_t size)
{
	int i;
	unsigned long *tlb_base = (unsigned long *)get_tlb_base();

	if(va == 0)
		va = 1;
	/*
	 *for this type of loop, we can optimize it
	 *do it later we system is ok.
	 */
	for(i = ((va-1) >> 20); i < size; i++){
		*tlb_base = 0;
		tlb_base++;
	}

	flush_mmu_tlb();
}

/*
 *build_tlb_table_entry
 *this function will help to build first level tlb
 *descriptions when memory maped as section.
 */
int build_tlb_table_entry(unsigned long vstart,unsigned long pstart,
			size_t size,u32 flag)
{
	unsigned long *tlb_base;
	int offset;
	unsigned long ps = pstart;
	int i;
	u32 value;
	u32 attr = 0;

	if(is_aligin(vstart,SIZE_1M) &&
	   is_aligin(vstart,SIZE_1M) &&
	   is_aligin(size,SIZE_1M))
	{
		tlb_base = (unsigned long *)get_tlb_base();
		ps = pstart;
		offset = (vstart - 0) >> 20;
		tlb_base += offset;
		attr = get_tlb_attr(flag);

		for(i = 0; i < (size>>20); i++){
			value = build_tlb_des(ps,attr);
			*(unsigned long *)tlb_base = value;

			ps += SIZE_1M;
			tlb_base ++;
		}

		flush_mmu_tlb();

		return 0;
	}

	return 1;
}

int build_page_table_entry(unsigned long base,
			   unsigned long vstart,
			   size_t size,u32 flag)
{
	int i;
	unsigned long *addr_base = (unsigned long *)base;
	u32 value;
	u32 attr;
	unsigned long pa = va_to_pa(vstart);

	if(!is_aligin(size,SIZE_4K) || size > SIZE_1M){
		kernel_debug("build page_table_entry:size not aligin");
		return 1;
	}

	attr = get_page_table_attr(flag);
	for(i = 0; i < (size >> PAGE_SHIFT); i++){
		value = build_page_table_des(pa,attr);
		*addr_base = value;

		addr_base++;
		pa += SIZE_4K;
	}

	return 0;
}

