#ifndef _TASK_H	
#define _TASK_H

#include <asm/asm_sched.h>

int kthread_run(char *name,int (*fn)(void *arg), void *arg);
pid_t sys_fork(pt_regs regs,u32 sp);

#endif
