#include <os/interrupt.h>
#include <os/types.h>
#include <os/printk.h>
#include <asm/asm_interrupt.h>
#include <os/errno.h>

extern int in_interrupt;

void inline enable_irqs(void)
{
	arch_enable_irqs();
}

void inline disable_irqs(void)
{
	arch_disable_irqs();
}

void inline enter_critical(unsigned long *val)
{
	arch_enter_critical(val);
}

void inline exit_critical(unsigned long *val)
{
	arch_exit_critical(val);
}

static struct irq_des *get_irq_description(int nr)
{
	return arch_get_irq_description(nr);
}

int register_irq(int nr,int (*fn)(void *arg),void *arg)
{
	int err = 0;
	unsigned long flags;

	if( (nr > IRQ_NR-1) || (fn == NULL) )
		return -EINVAL;

	enter_critical(&flags);
	err = arch_register_irq(nr,fn,arg);
	exit_critical(&flags);

	return err;
}

int do_irq_handler(int nr)
{
	struct irq_des *des;
	int ret;

	in_interrupt = 1;

	des = get_irq_description(nr);
	if(!des){
		return -EINVAL;
	}
	
	ret = des->fn(des->arg);

	in_interrupt = 0;

	return ret;
}
