#include <os/types.h>
#include <os/sched.h>
#include <os/string.h>
#include <os/bitops.h>
#include <os/list.h>
#include <os/errno.h>
#include <os/mutex.h>
#include <os/mm.h>
#include <os/printk.h>
#include <asm/asm_interrupt.h>

struct sched_list{
	struct list_head list;
	u32 count;
};

#define PID_MAP_PAGE_NR			1
struct pid_map{
	int nr_free;
	int current;
	int nr;
	struct mutex pid_map_mutex;
	u32 *addr;
};
struct pid_map pid_map;

/*
 *used to find the running task, if the task with the related prio
 *is in TASK_STATE_RUNNING count means how many running task is in 
 *this prio
 */
static struct sched_list os_sched_table[MAX_PRIO];

/*
 *represent the current running task
 *and task will be schedule at now
 */
struct task_struct *current;
struct task_struct *kernel;
static struct task_struct *next_run;
struct mutex sched_mutex;

static int schedule_running = 0;
int in_interrupt = 0;

#define DECLARE_SCHED_LIST(name)		\
	static struct sched_list task_##name

#define init_sched_list(name)		\
	do{						\
		init_list(&task_##name.list);		\
		task_##name.count = 0;			\
	}while(0)


#define sched_list_add_task(name,task)		\
	do{					\
		list_add(&task_##name.list,&task->name);  \
		task_##name.count++;		\
	}while(0)

#define sched_list_del_task(name,task)	\
	do{						\
		list_del(&task->name);	\
		task_##name.count--;		\
	}while(0)

#define sched_list_for_each(head,list)	\
	list_for_each((&task_##head.list),list)

/*
 *used to trace the all task in the system with
 *different state
 */
DECLARE_SCHED_LIST(system);
DECLARE_SCHED_LIST(idle);
DECLARE_SCHED_LIST(sleep);

static void inline prio_list_add_task(struct task_struct *task)
{
	list_add(&os_sched_table[task->prio].list,&task->prio_running);
	os_sched_table[task->prio].count++;
}

static void inline prio_list_add_task_tail(struct task_struct *task)
{
	list_add_tail(&os_sched_table[task->prio].list,&task->prio_running);
	os_sched_table[task->prio].count++;
}

static void inline prio_list_del_task(struct task_struct *task)
{
	list_del(&task->prio_running);
	os_sched_table[task->prio].count--;
}

void inline set_task_state(struct task_struct *task,state_t state)
{
	task->state = state;
}

state_t inline get_task_state(struct task_struct *task)
{
	return task->state;
}

static int inline sched_table_empty(int i)
{
	return !(os_sched_table[i].count);
}

static int get_running_slice(int prio)
{
	return ((prio*MAX_RUNNING_SLICE)/MAX_PRIO); 
}

static int init_pid_allocater(void)
{
	init_mutex(&pid_map.pid_map_mutex);
	pid_map.nr = (PID_MAP_PAGE_NR*PAGE_SIZE)/sizeof(void *);
	pid_map.nr_free = pid_map.nr;
	pid_map.current = 0;
	pid_map.addr = get_free_page(GFP_KERNEL);
	if(pid_map.addr == NULL){
		return -ENOMEM;
	}

	memset((char *)pid_map.addr,0,PID_MAP_PAGE_NR*PAGE_SIZE);

	return 0;	
}

pid_t get_new_pid(struct task_struct *task)
{
	int i;
	struct pid_map *map = &pid_map;
	u32 *base = map->addr;

	mutex_lock(&map->pid_map_mutex);

	if(map->nr_free == 0)
		goto out;

	for(i = map->current; i < map->nr; i++){
		if(base[i] == 0){
			map->nr_free--;
			map->current++;
			base[i] = (u32)task;
			mutex_unlock(&map->pid_map_mutex);

			return i;
		}

		if(i == map->nr-1){
			i = 0;
		}
	}

out:
	mutex_unlock(&map->pid_map_mutex);

	return 0;
}

static int get_waitting_slice(int prio)
{
	return (((MAX_PRIO-prio)*MAX_RUNNING_SLICE)/MAX_PRIO);
}

int add_new_task(struct task_struct *task)
{
	if(!task)
		return 1;

	/*
	 *fix me:using mutex may take some issue
	 *will fix it later
	 */
	mutex_lock(&sched_mutex);

	sched_list_add_task(system,task);
	set_task_state(task,TASK_STATE_PREPARE);
	prio_list_add_task_tail(task);

	mutex_unlock(&sched_mutex);

	return 0;
}

static void find_next_run_task(void)
{
	int i;
	struct list_head *task_head;

	for(i=0;i<MAX_PRIO;i++){
		if(os_sched_table[i].count)
			break;
	}

	task_head = &os_sched_table[i].list;
	next_run = list_first_entry(task_head,struct task_struct,prio_running);
}

static void prepare_to_switch(void)
{
	current->wait_time = get_waitting_slice(current->prio);	//
	current->run_time = 0;
	sched_list_add_task(sleep,current);			//sleep for a second,then other task have time to run.
	set_task_state(current,TASK_STATE_SLEEP);

	next_run->run_count++;
	next_run->run_time = get_running_slice(next_run->prio);
	next_run->wait_time = 0;
	prio_list_del_task(next_run);
	set_task_state(next_run,TASK_STATE_RUNNING);

}

static void inline switch_task_sw(void)
{
	arch_switch_task_sw();
}

static void inline switch_task_hw(void)
{
	arch_switch_task_hw();
}

void sched(void)
{
	if(in_interrupt){
		kernel_debug("Do not call sched in interrupt\n");
		return;
	}
	if(!schedule_running){
		kernel_debug("Schedule has not benn init\n");
		return;
	}

	disable_irqs();

	find_next_run_task();
	if(next_run == current){
		goto re_run;
	}
	prepare_to_switch();
	switch_task_sw();

re_run:
	enable_irqs();
}

int os_tick_handler(void *arg)
{
	struct list_head *list;
	struct task_struct *task;

	/*
	 * look up for each task in sleep list,if task is waitting
	 * for his time to run again,the sub his wait_time,otherwise 
	 * means that the task needed other action to resume him
	 */
	sched_list_for_each(sleep,list){
		task = list_entry(list,struct task_struct,sleep);
		if(task->wait_time)
			task->wait_time--;

		if(task->wait_time == 0){
			sched_list_del_task(sleep,task);
			prio_list_add_task(task);
		}
	}
	
	/*
	 * for the current task, first decrease his run_timem,if his 
	 * run time is over, then find another task to run,but now we 
	 * are in irq mode,so we need to use a different way to switch 
	 * task
	 */
	current->run_time--;
	if(current->run_time == 0){
		find_next_run_task();
		if(next_run == current)
			goto exit;
		prepare_to_switch();
	}

exit:
	current->run_time = get_running_slice(current->prio);

	return 0;
}

int sched_init(void)
{
	int i;

	kernel_debug("**** init schedule ****\n");

	for(i=0;i<MAX_PRIO;i++){
		init_list(&os_sched_table[i].list);
		os_sched_table[i].count = 0;
	}

	init_sched_list(sleep);
	init_sched_list(idle);
	init_sched_list(system);

	init_mutex(&sched_mutex);

	init_pid_allocater();

	current = NULL;
	next_run = NULL;
	schedule_running = 1;

	return 0;
}
