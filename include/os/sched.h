#ifndef _SCHED_H
#define _SCHED_H

#include <os/list.h>
#include <os/types.h>
#include <os/mutex.h>
#include <asm/asm_sched.h>
#include <asm/config.h>

#define KERNEL_STACK_SIZE	ARCH_KERNEL_STACK_SIZE

#define MAX_PRIO		64
#define MAX_PID			0XFFFF
#define TASK_DEFAULT_PRIO	20
#define MAX_RUNNING_SLICE	500

typedef enum _task_state{
	TASK_STATE_PREPARE,
	TASK_STATE_RUNNING,
	TASK_STATE_SLEEP,
	TASK_STATE_IDLE,
	TASK_STATE_UNKNOWN
}state_t;

#define PROCESS_TYPE_KERNEL	0x00000001
#define PROCESS_TYPE_USER	0x00000002

#define TASK_MAP_STACK	0
#define TASK_MAP_ELF	1
#define TASK_MAP_HEAP	2
#define TASK_MAP_MASK	0x0f

#define KERNEL_THREAD_PRIO	5
/*
 *elf_size: the size of all elf section. at first we will 
 *allocate elf_size to load all section of elf file;
 *stack_size: max 4m, when fork only allocate 16k
 */
struct mm_struct{
	int elf_size;		/*stack size + bin file size*/
	int stack_size;		/*MAX 4M 16k when allocateed*/
	struct list_head *stack_curr;
	struct list_head *elf_curr;	/*help to map memory if needed*/
	struct list_head stack_list;
	struct list_head elf_list;
};

struct sched_struct{
	int run_time;
	int wait_time;
	int time_out;
	u32 run_count;

	int prio;
	int pre_prio;

	struct list_head prio_running;
	struct list_head system;
	struct list_head sleep;
	//struct list_head prepare;
	struct list_head idle;

	/*for mutex*/
	struct list_head wait;

};

/*
 * task_struct:represent a process in the kernel
 * stack_base:the base address of stack of one task
 * regs:backup the process's register value when switch
 * pid:id of the task
 * uid:the task belongs to which user
 * state:current task state of the task
 * exit_state:the exit state of the task;
 * prio_running:use to communicate to the sched array for find next run task
 * task:used to connect all the task in system
 * sleep:used to connect all the sleeping task in system
 * running:used to connect all the running task in system
 * idle:used to connect all the idle task in system
 * wait:used for mutex,if a task is waitting a mutex,this list_head will insert to the mutex's waitting list
 */
struct task_struct{
	void *stack_base;		/*kernel stack of task*/
	pt_regs regs;
	char name[16];

	u32 pid;
	u32 uid;
	
	/*
	 *record the memory allocate to the process.
	 */
	struct mm_struct mm_struct;

	state_t state,exit_state;
	u32 signal,exit_code;

	struct elf_file *bin;

	u32 flag;

	struct task_struct *parent;
	struct list_head child;
	struct list_head p;

	struct sched_struct sched;

	/* mutex for this task_struct
	 * when need to modify the data of this task_struct
	 * we must get this mutex
	 */
	struct mutex mutex;

	void *message;
};

extern struct task_struct *current;
extern struct task_struct *idle;
extern struct task_struct *next_run;

void set_task_state(struct task_struct *task,state_t state);
state_t get_task_state(struct task_struct *task);
int add_new_task(struct task_struct *task);
void sched(void);
pid_t get_new_pid(struct task_struct *task);
int init_sched_struct(struct task_struct *task);

#endif
