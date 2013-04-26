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
struct task_struct *idle;
struct task_struct *next_run;
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


#define sched_list_add_task(name,sched)		\
	do{					\
		list_add(&task_##name.list,&sched->name);  \
		task_##name.count++;		\
	}while(0)

#define sched_list_add_task_tail(name,sched)		\
	do{					\
		list_add_tail(&task_##name.list,&sched->name);  \
		task_##name.count++;		\
	}while(0)

#define sched_list_del_task(name,sched)	\
	do{						\
		list_del(&sched->name);	\
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

static void inline prio_list_add_task(struct sched_struct *sched)
{
	list_add(&os_sched_table[sched->prio].list,&sched->prio_running);
	os_sched_table[sched->prio].count++;
}

static void prio_list_add_task_tail(struct sched_struct *sched)
{
	list_add_tail(&os_sched_table[sched->prio].list,&sched->prio_running);
	os_sched_table[sched->prio].count++;
}

static void inline prio_list_del_task(struct sched_struct *sched)
{
	list_del(&sched->prio_running);
	os_sched_table[sched->prio].count--;
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

static void get_task_run_time(struct sched_struct *sched)
{
	sched->run_time = 1;
}

int init_sched_struct(struct task_struct *task)
{
	struct sched_struct *sched = &task->sched;

	if( (task->parent) && (task->parent != idle)){
		sched->prio = task->parent->sched.prio;
		sched->pre_prio = task->parent->sched.pre_prio;
	}
	else if ( (task->parent) && (task->parent == idle) ){
		/*kernel thread*/
		sched->prio = KERNEL_THREAD_PRIO;
		sched->pre_prio = KERNEL_THREAD_PRIO;
	}
	else {
		/*
		 * idle process
		 */
		sched->prio = MAX_PRIO - 1;
		sched->pre_prio = sched->prio;
	}

	/*
	 * this value will be set to a default value
	 * later
	 */
	get_task_run_time(sched);
	sched->wait_time = -1;
	sched->time_out = 0;
	sched->run_count = 0;
	
	init_list(&sched->prio_running);
	init_list(&sched->system);
	init_list(&sched->sleep);
	init_list(&sched->idle);
	init_list(&sched->wait);

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

int add_new_task(struct task_struct *task)
{
	struct sched_struct *sched = &task->sched;
	unsigned long flags;
	
	if(!task)
		return 1;

	/*
	 * when add new task to system, we need disable irqs
	 * first.
	 */
	enter_critical(&flags);

	sched_list_add_task(system,sched);
	prio_list_add_task_tail(sched);

	exit_critical(&flags);

	return 0;
}

static void find_next_run_task(void)
{
	int i;
	struct list_head *task_head;
	struct sched_struct *sched;

	/*
	 *idle process is always on preparing state
	 */
	for(i=0; i < MAX_PRIO - 1; i++){
		if(os_sched_table[i].count){
			break;
		}
	}

	task_head = &os_sched_table[i].list;
	sched = list_first_entry(task_head,struct sched_struct,prio_running);
	next_run = list_entry(sched,struct task_struct,sched);
}

static void prepare_to_switch(void)
{
	struct sched_struct *current_s = &current->sched;
	struct sched_struct *next_s = &next_run->sched;
	state_t state = get_task_state(current);

	/*
	 * frist we check that whether the state of the task
	 * has been set to a correct state.if no we will take
	 * next actions, we add current task to the tail of 
	 * the list.
	 */
	if (state == TASK_STATE_RUNNING) {
		set_task_state(current,TASK_STATE_PREPARE);
		prio_list_add_task_tail(current_s);
	}
	else if (state == TASK_STATE_SLEEP) {
		/*
		 *this task is suspended by himself
		 */
		sched_list_add_task(sleep, current_s);
	}
	current_s->run_time = 0;

	/*
	 * notic:the running task must be choose from the prio
	 * list and can not be select from sleep list directly.
	 * so a running task must following the below state 
	 * sequence to run agin:
	 * 1)sleep->prepare->running
	 * 2)idle->prepare->running
	 *
	 * if next_run wait_time < 0, seems he is timeout
	 */
	next_s->run_count++;
	get_task_run_time(next_s);

	if (next_s->wait_time == 0) {
		next_s->time_out = 1;
	}
	next_s->wait_time = -1;

	/*
	 *delete from prepare list and ready to run.
	 */
	prio_list_del_task(next_s);
	set_task_state(next_run,TASK_STATE_RUNNING);
}

static void inline switch_task_hw(void)
{
	arch_switch_task_hw();
}

void sched(void)
{
	unsigned long flags; 

	if(in_interrupt){
		kernel_debug("Do not call sched in interrupt\n");
		return;
	}
	if(!schedule_running){
		kernel_debug("Schedule has not benn init\n");
		return;
	}

	enter_critical(&flags);

	find_next_run_task();
	if(next_run == current){
		goto re_run;
	}
	prepare_to_switch();

	/*
	 * in below function, the task will be change to next_run.
	 * and the lr or the return address will be set to re_run
	 * which eque to enable_irq. and this function is a arch 
	 * specfic function.
	 */
	arch_switch_task_sw();

re_run:
	exit_critical(&flags);
}

int os_tick_handler(void *arg)
{
	struct list_head *list;
	struct sched_struct *sched;
	struct sched_struct *sched_current = &current->sched;
	
	/*
	 * look up for each task in sleep list,if task is waitting
	 * for his time to run again,the sub his wait_time,otherwise 
	 * means that the task needed other action to resume him
	 */
	sched_list_for_each(sleep,list){
		sched = list_entry(list,struct sched_struct,sleep);
		if(sched->wait_time > 0){
			sched->wait_time--;
		}

		/*
		 * task has timeout and need to wake up
		 */
		if(sched->wait_time == 0){
			sched_list_del_task(sleep,sched);
			set_task_state(container_of(sched, struct task_struct, sched),
					TASK_STATE_PREPARE);
			prio_list_add_task_tail(sched);
		}
	}

	/*
	 * for the current task, first decrease his run_timem,if his 
	 * run time is over, then find another task to run,but now we 
	 * are in irq mode,so we need to use a different way to switch 
	 * task
	 */
	sched_current->run_time--;
	if(sched_current->run_time == 0){

		find_next_run_task();

		if(next_run == current){
			get_task_run_time(sched_current);
		}
		else{
			prepare_to_switch();
		}
	}

	return 0;
}

int switch_task(struct task_struct *cur,
		struct task_struct *next)
{
	return 0;
}

int suspend_task_timeout(struct task_struct *task, int timeout)
{
	struct sched_struct *sched_s;
	state_t state;
	unsigned long flags;

	if (!task || !timeout) {
		return -EINVAL;
	}
	sched_s = &task->sched;

	enter_critical(&flags);

	/*
	 * get the state of the task, if task state is idle
	 * then return;
	 */
	state = get_task_state(task);
	if (state == TASK_STATE_RUNNING) {
		set_task_state(task, TASK_STATE_SLEEP);
	}
	else {
		/*
		 * only running task can directly go to sleep state
		 */
		exit_critical(&flags);
		return -EINVAL;
	}

	/*
	 * set task timeout, and remove it from current list
	 * then add it to sleep list.
	 */
	sched_s->wait_time = timeout;
	sched_s->time_out = 0;

	exit_critical(&flags);

	sched();

	return sched_s->time_out;
}

int wakeup_task(struct task_struct *task)
{
	struct sched_struct *sched_s;
	unsigned long flags;

	if (get_task_state(task) != TASK_STATE_SLEEP)
		return -EINVAL;

	enter_critical(&flags);

	sched_s = &task->sched;
	sched_list_del_task(sleep, sched_s);
	set_task_state(task, TASK_STATE_PREPARE);
	prio_list_add_task(sched_s);

	exit_critical(&flags);

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
