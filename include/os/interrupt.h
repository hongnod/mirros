#ifndef _INTERRUPT_H
#define _INTERRUPT_H

#include <asm/config.h>

struct irq_des {
	int (*fn)(void *arg);
	void *arg;
};

#define IRQ_NR		ARCH_IRQ_NR
#define IRQ_TIMER_TICK	ARCH_IRQ_TIMER_TICK

int do_irq_handler(int nr);
void disable_irqs(void);
void enable_irqs(void);
void enter_critical(unsigned long *val);
void exit_critical(unsigned long *val);
int register_irq(int nr, int (*fn)(void *arg), void *arg);

#endif
