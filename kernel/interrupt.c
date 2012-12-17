#include <os/interrupt.h>
#include <os/types.h>
#include <os/printk.h>
#include <asm/asm_interrupt.h>
#include <os/errno.h>

void inline enable_irqs(void)
{
	arch_enable_irqs();
}

void inline disable_irqs(void)
{
	arch_disable_irqs();
}

static inline struct irq_des *get_irq_description(int nr)
{
	return arch_get_irq_description(nr);
}

int register_irq(int nr,int (*fn)(void *arg),void *arg)
{
	if( (nr > IRQ_NR-1) || (fn == NULL) )
		return -EINVAL;

	return arch_register_irq(nr,fn,arg);
}

void do_irq_handler(int nr)
{
	struct irq_des *des;
	int ret = 0;

	des = get_irq_description(nr);
	if(des){
		printk("get an unknow irq\n");
		return;
	}
	
	ret = des->fn(des->arg);
	if(ret){
		printk("something error in irq handler %d\n",nr);
	}
}
