#ifndef _TASK_H	
#define _TASK_H

#include <os/list.h>
#include <os/types.h>
#include <os/mutex.h>
#include <asm/asm_sched.h>
#include <asm/config.h>

#define KERNEL_STACK_SIZE	ARCH_KERNEL_STACK_SIZE

#define PROCESS_NAME_SIZE	16

#define PROCESS_TYPE_KERNEL	0x00000001
#define PROCESS_TYPE_USER	0x00000002

#define PROCESS_MAP_STACK	0
#define PROCESS_MAP_ELF		1
#define PROCESS_MAP_HEAP	2
#define PROCESS_MAP_MASK	0x0f

typedef enum _task_state {
	PROCESS_STATE_PREPARE,
	PROCESS_STATE_RUNNING,
	PROCESS_STATE_SLEEP,
	PROCESS_STATE_IDLE,
	PROCESS_STATE_STOP,
	PROCESS_STATE_UNKNOWN
} state_t;

/*
 * elf_size:the size of all elf section. at first we will
 * allocate elf_size to load all section of elf file
 * stack_size:max 4m, when fork only allocate 16k
 * elf_curr:current page table entry in list
 * stack_cur:current page table entry in list
 * stack_list:each page table of stack table will be mounted
 * on this list
 * elf_list:each page table of elf image will be mounted
 * on this list
 * elf_stack_list:memory allocated for task stack
 * elf_image_list:memory allocated for task image
 */
struct mm_struct {
	int elf_size;
	int stack_size;
	struct list_head *stack_curr;
	struct list_head *elf_curr;
	struct list_head stack_list;
	struct list_head elf_list;
	struct list_head elf_stack_list;
	struct list_head elf_image_list;
};

/*
 * task_struct:represent a process in the kernel
 * stack_base:the base address of stack of one task
 * regs:backup the process's register value when switch
 * pid:id of the task
 * uid:the task belongs to which user
 * state:current task state of the task
 * exit_state:the exit state of the task;
 * prio_running:use to communicate to the sched array
 * for find next run task
 * task:used to connect all the task in system
 * sleep:used to connect all the sleeping task in system
 * running:used to connect all the running task in system
 * idle:used to connect all the idle task in system
 * wait:used for mutex,if a task is waitting a mutex,
 * this list_head will insert to the mutex's waitting list
 */
struct task_struct {
	void *stack_base;
	void *stack_origin;
	char name[16];

	u32 pid;
	u32 uid;

	struct mm_struct mm_struct;

	state_t state,exit_state;
	u32 signal,exit_code;

	u32 flag;

	struct task_struct *parent;
	struct list_head child;
	struct list_head p;

	/*
	 * below member are used to schedule.
	 * run_time: each task have his run slice
	 * wait_time: if a task was wait for a event, this
	 *	     member can set to the wait time
	 * time_out: wether this task wait for someting time out
	 * run_count: how many time has this task runed.
	 * prio: the prio of this task
	 * pre_prio: previous prio of this task
	 * prio_running: if the task is in running state, this list is
	 * attach to the prio prepare list.
	 * wiat : when task was wait for a mutex or other things, this
	 * list is attach to them
	 */
	int run_time;
	int wait_time;
	int time_out;
	u32 run_count;
	int prio;
	int pre_prio;
	struct list_head prio_running;
	struct list_head system;
	struct list_head sleep;
	struct list_head idle;
	struct list_head wait;

	/* mutex for this task_struct
	 * when need to modify the data of this task_struct
	 * we must get this mutex TBD
	 */
	struct mutex mutex;

	void *message;
};

int kthread_run(char *name, int (*fn)(void *arg), void *arg);
pid_t sys_fork(pt_regs regs, u32 sp);
int kernel_exec(char *filename);

#endif
