#ifndef _ASM_SCHED_H
#define _ASM_SCHED_H

#include <os/types.h>

typedef struct _pt_regs{
	u32 r0;
	u32 r1;
	u32 r2;
	u32 r3;
	u32 r4;
	u32 r5;
	u32 r6;
	u32 r7;
	u32 r8;
	u32 r9;
	u32 r10;
	u32 r11;
	u32 r12;
	u32 sp;
	u32 lr;
	u32 pc;
	u32 cpsr;
	/*u32 spsr*/		
}pt_regs;

void arch_switch_task_sw(void);
void arch_switch_task_hw(void);

#endif
