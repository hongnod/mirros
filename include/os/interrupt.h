#ifndef _INTERRUPT_H
#define _INTERRUPT_H

#include <asm/config.h>

struct irq_des{
	int (*fn)(void *arg);
	void *arg;
};

#define IRQ_NR		ARCH_IRQ_NR
#define IRQ_TIMER_TICK	ARCH_IRQ_TIMER_TICK

void do_irq_handler(int nr);
void disable_irqs(void);
void enable_irqs(void);
int register_irq(int nr,int (*fn)(void *arg),void *arg);

#endif
