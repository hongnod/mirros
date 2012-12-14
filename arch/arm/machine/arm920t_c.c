#include <os/init.h>
#include <os/types.h>
#include <os/printk.h>
#include <asm/asm_mmu.h>
#include <os/mirros.h>
#include <os/mm.h>
#include <os/mmu.h>
#include "include/arm920t.h"
#include "include/platform.h"
#include <os/interrupt.h>
#include <os/errno.h>
#include <os/string.h>
#include <os/io.h>

/*
 *code for libgcc.a
 */
int raise(int signum)
{
	printk("raise: Signal %d catched \n",signum);

	return 0;
}

/*
 *code for arm920t mmu
 */
unsigned long arch_get_tlb_base(void)
{
	return KERNEL_BASE_ADDRESS;	
}

u32 arch_build_tlb_des(unsigned long pa,u32 attr)
{
	u32 ret_val;

	/*
	 *note if you use if(attr & 0x03 == 0x01) there is syntax error
	 *you need add a (attr & 0x03), remember it.
	 */
	if( (attr & 0x03) == 0X01)
		ret_val = (pa & 0xfffffc00) | attr;
	else
		ret_val = (pa & 0xfff00000) | attr;

	return ret_val;
}

u32 arch_build_page_table_des(unsigned long pa,u32 attr)
{
	return ((pa & 0xfffff000) | attr);
}

u32 arch_get_tlb_attr(u32 flag)
{
	u32 attr = 0;

	switch(flag & TLB_ATTR_MEMORY_MASK){
		case TLB_ATTR_KERNEL_MEMORY:
			attr |= MST_WB_AP_SRW_UN;
			break;

		case TLB_ATTR_DMA_MEMORY:
		case TLB_ATTR_IO_MEMORY:
			attr |= MST_NCNB_AP_SRW_UN;
			break;

		case TLB_ATTR_USER_MEMORY:
			attr |= CURDE_PAGE_TABLE;
			break;

		default:
			attr |= MST_WB_AP_SRW_UN;
			break;
	}

	return attr;
}

u32 arch_get_page_table_attr(u32 flag)
{
	return PGT_DES_DEFAULT;
}

static void arch_remap_vector(void)
{
	asm(
		"push {r0}\n\t"
		"mrc p15, 0, r0, c1, c0, 0\n\t"
		"orr r0, #1<<13\n\t"
		"mcr p15, 0, r0, c1, c0, 0\n\t"
		"pop {r0}\n\t"
	);
}

static void invalidate_id_cache(void)
{
	asm(
		"push {r0}\n\t"
		"mcr p15,0,r0,c7,c7,0\n\t"
		"pop {r0}\n\t"
	);

}

static void invalidate_all_tlb(void)
{
	asm(
		"push {r0}\n\t"
		"mov r0,#0\n\t"
		"mcr p15,0,r0,c8,c7,0\n\t"
		"nop\n\t"
		"nop\n\t"
		"pop {r0}\n\t"
	);
}

void arch_flush_mmu_tlb(void)
{
	invalidate_id_cache();
	invalidate_all_tlb();
}

/*
 *code for irq and interrupt for arm920t
 */
static void *irq_table_base = NULL;
void arch_disable_irqs(void)
{
	asm(
		"push {r1}\n\t"
		"mrs r1,cpsr\n\t"
		"orr r1, r1, #0xc0\n\t"
		"msr cpsr_c, r1\n\t"
		"pop {r1}\n\t"
	);	
}

void arch_enable_irqs(void)
{
	asm(
		"push {r1}\n\t"
		"mrs r1,cpsr\n\t"
		"bic r1, r1, #0xc0\n\t"
		"msr cpsr_c, r1\n\t"
		"pop {r1}\n\t"
	);	
}

void irq_handler(void)
{
	int nr;

	/*
	 *first get irq number, then clean irq pending
	 */
	nr = platform_get_irq_id();
	platform_irq_clean_pending();

	do_irq_handler(nr);
}

struct irq_des *arch_get_irq_description(int nr)
{
	struct irq_des *base = (struct irq_des*)irq_table_base;
	struct irq_des *ret = NULL;

	if(nr < 0 || nr > (IRQ_NR-1)){
		kernel_error("irq number is not correct\n");
		return NULL;
	}

	ret = base + nr;
	if(ret->fn == NULL){
		kernel_error("handler of irq %d has not been register\n");
		return NULL;
	}

	return ret;
}

int arch_register_irq(int nr,int (*fn)(void *arg),void *arg)
{
	/*
	 *register irq has ensure that fn != null and nr is 
	 *vaild
	 */
	struct irq_des *des = arch_get_irq_description(nr);

	if( (des !=NULL) && (des->fn == NULL) ){
		des->fn = fn;
		des->arg = arg;

		return 0;
	}

	platform_enable_irq(nr);

	return 1;
}

int arch_irq_init(void)
{
	int irq_table_size = IRQ_NR*sizeof(struct irq_des);
	int nr;

	nr = page_nr(irq_table_size);
	irq_table_base = get_free_pages(nr,GFP_KERNEL);
	if(irq_table_base == NULL){
		kernel_error("can not allocate size for irq_table\n");
		return -ENOMEM;
	}

	memset(irq_table_base,0,nr<<PAGE_SHIFT);

	platform_irq_init();

	return 0;
}

int trap_init(void)
{
	extern char _vector_start[];
	extern char _vector_end[];
	void *vector_page;

	/*
	 * first we get a page load the vector code,notice:
	 * since the vector must remaped at 0xffff0000, we also 
	 * need map 0xfff00000 to the page section address which
	 * we get.
	 */
	vector_page = get_free_page_aligin(0xffff0000,GFP_KERNEL);
	kernel_debug("vector address is 0x%x\n",(u32)vector_page);
	if(vector_page == NULL){
		kernel_error("faild get page for trap\n");
		return -ENOMEM;
	}

	/*
	 * copy the vector code to the page we get. then map 0xfff00000
	 * to va_to_pa(vector_page) & 0xfff00000. at the last chage the
	 * value of bit13 of cp register c1 to indicate that we need remap
	 * the vector.
	 */
	memcpy(vector_page,(void *)_vector_start,_vector_end - _vector_start);
	kernel_debug("physic address of vector is 0x%x\n",(u32)va_to_pa(vector_page));
	build_tlb_table_entry(0xfff00000,va_to_pa((unsigned long)vector_page)&0xfff00000,
			SIZE_1M,TLB_ATTR_KERNEL_MEMORY);	

	arch_remap_vector();

	return 0;
}
