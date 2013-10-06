#ifndef _SCHED_H
#define _SCHED_H

#include <os/task.h>

#define MAX_PRIO		64
#define PROCESS_DEFAULT_PRIO	20
#define KERNEL_THREAD_PRIO	5

extern struct task_struct *current;
extern struct task_struct *idle;
extern struct task_struct *next_run;
extern int in_interrupt;

void set_task_state(struct task_struct *task, state_t state);
state_t get_task_state(struct task_struct *task);
int add_new_task(struct task_struct *task);
void sched(void);
pid_t get_new_pid(struct task_struct *task);
int init_sched_struct(struct task_struct *task);
int suspend_task_timeout(struct task_struct *task, int timeout);
int wakeup_task(struct task_struct *task);

#define suspend_task(task)	suspend_task_timeout(task, -1);

#endif
