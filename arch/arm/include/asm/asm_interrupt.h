#ifndef _ASM_INTERRUPT_H
#define _ASM_INTERRUPT_H

#include <os/interrupt.h>

void arch_disable_irqs(void);
void arch_enable_irqs(void);
struct irq_des *arch_get_irq_description(int nr);
int arch_register_irq(int nr,int (*fn)(void *arg),void *arg);

#endif
