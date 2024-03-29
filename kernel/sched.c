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
#include <os/task.h>
#include <os/syscall.h>

struct sched_list {
	struct list_head list;
	u32 count;
};

#define PID_MAP_PAGE_NR			1
struct pid_map {
	int nr_free;
	int current;
	int nr;
	struct mutex pid_map_mutex;
	u32 *addr;
};
struct pid_map pid_map;

/*
 * used to find the running task, if the task with
 * the related prio is in PROCESS_STATE_RUNNING
 * count means how many running task is in this prio
 */
static struct sched_list os_sched_table[MAX_PRIO];

/*
 * represent the current running task
 * and task will be schedule at now
 */
struct task_struct *current;
struct task_struct *idle;
struct task_struct *next_run;

static int schedule_running = 0;
int in_interrupt = 0;

#define DECLARE_SCHED_LIST(name)		\
	static struct sched_list task_##name

#define init_sched_list(name)		\
	do {						\
		init_list(&task_##name.list);		\
		task_##name.count = 0;			\
	} while(0)


#define sched_list_add_task(name, task)		\
	do {					\
		list_add(&task_##name.list, &task->name);  \
		task_##name.count++;		\
	} while(0)

#define sched_list_add_task_tail(name,task)		\
	do {					\
		list_add_tail(&task_##name.list, &task->name);  \
		task_##name.count++;		\
	} while(0)

#define sched_list_del_task(name, task)	\
	do {						\
		list_del(&task->name);	\
		task_##name.count--;		\
	} while(0)

#define sched_list_for_each(head, list)	\
	list_for_each((&task_##head.list), list)

/*
 * used to trace the all task in the system with
 * different state
 */
DECLARE_SCHED_LIST(system);
DECLARE_SCHED_LIST(idle);
DECLARE_SCHED_LIST(sleep);

static void inline prio_list_add_task(struct task_struct *task)
{
	list_add(&os_sched_table[task->prio].list,
		 &task->prio_running);
	os_sched_table[task->prio].count++;
}

static void prio_list_add_task_tail(struct task_struct *task)
{
	list_add_tail(&os_sched_table[task->prio].list,
		      &task->prio_running);
	os_sched_table[task->prio].count++;
}

static void inline prio_list_del_task(struct task_struct *task)
{
	list_del(&task->prio_running);
	os_sched_table[task->prio].count--;
}

/*
 * this function is used to remove a stoped task from system
 * if the state of the task is not IDLE, we can not remove it
 */
int sched_remove_task(struct task_struct *task)
{
	pid_t pid = get_task_pid(task);
	unsigned long flags;

	if (get_task_state(task) != PROCESS_STATE_IDLE)
		return -EINVAL;

	/* we also can nor remove init and idle task */
	if ((pid == 1) || (pid == 0))
		return -EINVAL;

	enter_critical(&flags);
	sched_list_del_task(idle, task);
	sched_list_del_task(system, task);
	exit_critical(&flags);

	release_pid(task);

	return 0;
}

typedef enum _state_change_t {
	STATE_NONE_TO_PREPARE,
	STATE_NONE_TO_SLEEP,
	STATE_PREPARE_TO_RUNNING,
	STATE_PREPARE_TO_SLEEP,
	STATE_PREPARE_TO_IDLE,
	STATE_RUNNING_TO_SLEEP,
	STATE_RUNNING_TO_PREPARE,
	STATE_RUNNING_TO_IDLE,
	STATE_SLEEP_TO_PREPARE,
	STATE_SLEEP_TO_IDLE,
	STATE_SLEEP_TO_RUNNING,
	STATE_PREPARE_TO_UNKNOWN,
	STATE_NONE_TO_NONE,
} state_change_t;

/* old, new, value */
#define CHANGE_TABLE_SIZE	(14 * 3)
static char change_table[CHANGE_TABLE_SIZE] = {
	PROCESS_STATE_UNKNOWN, PROCESS_STATE_PREPARE, STATE_NONE_TO_PREPARE,
	PROCESS_STATE_UNKNOWN, PROCESS_STATE_SLEEP, STATE_NONE_TO_SLEEP,
	PROCESS_STATE_PREPARE, PROCESS_STATE_RUNNING, STATE_PREPARE_TO_RUNNING,
	PROCESS_STATE_PREPARE, PROCESS_STATE_SLEEP, STATE_PREPARE_TO_SLEEP,
	PROCESS_STATE_PREPARE, PROCESS_STATE_IDLE, STATE_PREPARE_TO_IDLE,
	PROCESS_STATE_RUNNING, PROCESS_STATE_SLEEP, STATE_RUNNING_TO_SLEEP,
	PROCESS_STATE_RUNNING, PROCESS_STATE_PREPARE, STATE_RUNNING_TO_PREPARE,
	PROCESS_STATE_RUNNING, PROCESS_STATE_IDLE, STATE_RUNNING_TO_IDLE,
	PROCESS_STATE_SLEEP, PROCESS_STATE_PREPARE, STATE_SLEEP_TO_PREPARE,
	PROCESS_STATE_SLEEP, PROCESS_STATE_IDLE, STATE_SLEEP_TO_IDLE,
	PROCESS_STATE_SLEEP, PROCESS_STATE_RUNNING, STATE_SLEEP_TO_RUNNING,
	PROCESS_STATE_PREPARE, PROCESS_STATE_UNKNOWN, STATE_PREPARE_TO_UNKNOWN
};

static state_change_t get_state_change(state_t old, state_t new)
{
	int i = 0;

	for (i = 0; i < CHANGE_TABLE_SIZE ; i += 3) {
		if ((old == change_table[i]) && 
		    (new == change_table[i + 1])) {
			return change_table[i + 2];
		}
	}

	return STATE_NONE_TO_NONE;
}

void set_task_state(struct task_struct *task, state_t state)
{
	unsigned long flags;
	state_t old = get_task_state(task);
	state_t new = state;
	state_change_t change = STATE_NONE_TO_NONE;

	//kernel_debug("task %s state old %d new %d\n", task->name, old, new);
	change = get_state_change(old, new);
	if (change == STATE_NONE_TO_NONE) {
		return;
	}

	enter_critical(&flags);
	task->state = new;
	switch (change) {
		case STATE_NONE_TO_PREPARE:
			sched_list_add_task_tail(system, task);
			prio_list_add_task_tail(task);
			break;
		case STATE_NONE_TO_SLEEP:
			sched_list_add_task_tail(system, task);
			sched_list_add_task(sleep, task);
			break;
		case STATE_PREPARE_TO_RUNNING:
			prio_list_del_task(task);
			break;
		case STATE_PREPARE_TO_IDLE:
			prio_list_del_task(task);
			sched_list_add_task_tail(idle, task);
			break;
		case STATE_PREPARE_TO_SLEEP:
			prio_list_del_task(task);
			sched_list_add_task_tail(sleep, task);
			break;
		case STATE_RUNNING_TO_SLEEP:
			sched_list_add_task_tail(sleep, task);
			break;
		case STATE_RUNNING_TO_PREPARE:
			prio_list_add_task_tail(task);
			break;
		case STATE_RUNNING_TO_IDLE:
			sched_list_add_task_tail(idle, task);
			break;
		case STATE_SLEEP_TO_PREPARE:
			sched_list_del_task(sleep, task);
			prio_list_add_task_tail(task);
			break;
		case STATE_SLEEP_TO_IDLE:
			sched_list_del_task(sleep, task);
			sched_list_add_task_tail(idle, task);
			break;
		case STATE_SLEEP_TO_RUNNING:
			sched_list_del_task(sleep, task);
			break;
		case STATE_PREPARE_TO_UNKNOWN:
			prio_list_del_task(task);
			break;
		default:
			break;
	}
	exit_critical(&flags);
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
	pid_map.nr = (PID_MAP_PAGE_NR * PAGE_SIZE) / sizeof(void *);
	pid_map.nr_free = pid_map.nr;
	pid_map.current = 0;
	pid_map.addr = get_free_pages(PID_MAP_PAGE_NR, GFP_KERNEL);
	if (pid_map.addr == NULL) {
		return -ENOMEM;
	}

	memset((char *)pid_map.addr, 0, PID_MAP_PAGE_NR * PAGE_SIZE);

	return 0;	
}

static void get_task_run_time(struct task_struct *task)
{
	task->run_time = 1;
}

int init_sched_struct(struct task_struct *task)
{
	if ((task->flag & PROCESS_TYPE_KERNEL) && (task->pid > 0)) {
		/* kernel thread except idle */
		task->prio = KERNEL_THREAD_PRIO;
		task->pre_prio = KERNEL_THREAD_PRIO;
	}
	else if ((task->flag & PROCESS_TYPE_USER) && (task->pid == 1)) {
		/* init process */
		task->prio = PROCESS_DEFAULT_PRIO;
		task->pre_prio = PROCESS_DEFAULT_PRIO;
	}
	else if ((task->flag & PROCESS_TYPE_USER) && (task->pid > 1)) {
		/* other process */
		task->prio = task->parent->prio;
		task->pre_prio = task->prio;
	}
	else {
		/* idle process, we set idle task as a high prio at
		 * boot stage, when idle is over and init task is ready
		 * to run we will set its prio to MAX_PRIO -1
		 */
		task->prio = KERNEL_THREAD_PRIO -1;
		task->pre_prio = task->prio;
	}

	/*  this value will be set to a default value later */
	get_task_run_time(task);
	task->wait_time = -1;
	task->time_out = 0;
	task->run_count = 0;
	task->state = PROCESS_STATE_UNKNOWN;
	task->exit_state = PROCESS_STATE_UNKNOWN;
	
	init_list(&task->prio_running);
	init_list(&task->system);
	init_list(&task->sleep);
	init_list(&task->idle);

	init_list(&task->mutex_get);
	init_list(&task->wait);

	return 0;
}

int release_pid(struct task_struct *task)
{
	struct pid_map *map = &pid_map;
	u32 *base = (u32 *)map->addr;

	mutex_lock(&map->pid_map_mutex);
	base[task->pid] = 0;
	mutex_unlock(&map->pid_map_mutex);

	return 0;
}

pid_t get_new_pid(struct task_struct *task)
{
	int i;
	struct pid_map *map = &pid_map;
	u32 *base =(u32 *)map->addr;

	mutex_lock(&map->pid_map_mutex);

	if (map->nr_free == 0)
		goto out;

	for (i = map->current; i < map->nr; i++) {
		if (base[i] == 0) {
			map->nr_free--;
			map->current++;
			base[i] = (u32)task;

			mutex_unlock(&map->pid_map_mutex);
			return i;
		}
	}

out:
	mutex_unlock(&map->pid_map_mutex);

	return (-1);
}

pid_t get_task_pid(struct task_struct *task)
{
	return task->pid;
}

pid_t sys_getpid(void)
{
	return get_task_pid(current);
}
DEFINE_SYSCALL(get_pid, __NR_getpid, (void *)sys_getpid);

struct task_struct *pid_get_task(pid_t pid)
{
	u32 *addr;
	u32 flags;
	
	if (pid < pid_map.nr) {
		enter_critical(&flags);
		addr = pid_map.addr;
		exit_critical(&flags);
		return (struct task_struct *)(*(addr + pid));
	}

	return NULL;
}

int set_task_prio(struct task_struct *task, int prio)
{
	state_t state = get_task_state(task);

	if (prio >= MAX_PRIO)
		return -EINVAL;

	switch (state) {
		/*
		 * if the task is in running or sleep state, we
		 * can directory set it prio to the right one.
		 */
		case PROCESS_STATE_RUNNING:
		case PROCESS_STATE_SLEEP:
			task->pre_prio = task->prio;
			task->prio = prio;
			break;

		/*
		 * if the task is in prepare state, we first set its
		 * task to UNKNOWN state, then change its prio to new one.
		 */
		case PROCESS_STATE_PREPARE:
			set_task_state(task, PROCESS_STATE_UNKNOWN);
			task->pre_prio = task->prio;
			task->prio = prio;
			set_task_state(task, PROCESS_STATE_PREPARE);
			break;

		default:
			kernel_error("Unknown state when set prio");
			break;
	}

	return 0;
}

static struct task_struct *find_next_run_task(void)
{
	int i;
	struct list_head *task_head;
	struct task_struct *task;

	/* idle process is always on preparing state */
	for (i = 0; i < MAX_PRIO - 1; i++) {
		if (os_sched_table[i].count) {
			break;
		}
	}
	
	if ((MAX_PRIO - 1) == i) {
		return current;
	}

	task_head = &os_sched_table[i].list;
	task = list_first_entry(task_head,
			struct task_struct,
			prio_running);

	return task;
}

static int prepare_to_switch(struct task_struct *next)
{
	state_t state = get_task_state(current);

	if (current == next) {
		next = current;
		goto re_run;
	}

	/*
	 * frist we check that whether the state of the task
	 * has been set to a correct state.if no we will take
	 * next actions, we add current task to the tail of 
	 * the list.
	 */
	if (state == PROCESS_STATE_RUNNING) {
		set_task_state(current, PROCESS_STATE_PREPARE);
	}

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
	if (next->wait_time == 0)
		next->time_out = 1;

	next->wait_time = -1;
	set_task_state(next, PROCESS_STATE_RUNNING);

re_run:
	/* delete from prepare list and ready to run. */
	next->run_count++;
	get_task_run_time(next);

	return 0;
}

void sched(void)
{
	unsigned long flags; 

	if (in_interrupt) {
		kernel_debug("Do not call sched in interrupt\n");
		return;
	}

	if (!schedule_running) {
		kernel_debug("Schedule has not benn init\n");
		return;
	}

	enter_critical(&flags);

	next_run = find_next_run_task();
	prepare_to_switch(next_run);

	/*
	 * in below function, the task will be change to next_run.
	 * and the lr or the return address will be set to re_run
	 * which eque to enable_irq. and this function is a arch 
	 * specfic function.
	 */
	arch_switch_task_sw();

	exit_critical(&flags);
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
	sched_list_for_each(sleep, list) {
		task = list_entry(list, struct task_struct, sleep);
		if (task->wait_time > 0) {
			task->wait_time--;
		}

		/* task has timeout and need to wake up */
		if (task->wait_time == 0) {
			set_task_state(task, PROCESS_STATE_PREPARE);
		}
	}

	/*
	 * for the current task, first decrease his run_timem,if his 
	 * run time is over, then find another task to run,but now we 
	 * are in irq mode,so we need to use a different way to switch 
	 * task
	 */
	current->run_time--;
	if (current->run_time == 0) {

		next_run = find_next_run_task();
		prepare_to_switch(next_run);
	}

	return 0;
}

int suspend_task_timeout(struct task_struct *task, int timeout)
{
	state_t state;
	unsigned long flags;

	if (!task || !timeout) {
		return -EINVAL;
	}

	enter_critical(&flags);
	/*
	 * get the state of the task, if task state is idle
	 * then return;
	 */
	state = get_task_state(task);
	if (state != PROCESS_STATE_IDLE) {
		task->wait_time = timeout;
		task->time_out = 0;
		set_task_state(task, PROCESS_STATE_SLEEP);
	}
	else {
		/*  idle task can not go to sleep state */
		exit_critical(&flags);
		return -EINVAL;
	}
	exit_critical(&flags);

	sched();
	return task->time_out;
}

int wakeup_task(struct task_struct *task)
{
	if (get_task_state(task) != PROCESS_STATE_SLEEP)
		return -EINVAL;

	kernel_debug("wake up task %s\n", task->name);
	get_task_run_time(task);
	set_task_state(task, PROCESS_STATE_PREPARE);

	return 0;
}

/* system killer task is used to kill idle process */
int system_killer(void *arg)
{
	struct list_head *list;
	struct task_struct *task;

	for (; ;) {
		kernel_debug("Enter in system killer process\n");
		sched_list_for_each(idle, list) {
			task = list_entry(list, struct task_struct, idle);
			kernel_debug("kill task %s\n", task->name);
			kill_task(task);
		}

		suspend_task(current);
	}
}

int sched_init(void)
{
	int i;

	for (i = 0; i< MAX_PRIO; i++){
		init_list(&os_sched_table[i].list);
		os_sched_table[i].count = 0;
	}

	init_sched_list(sleep);
	init_sched_list(idle);
	init_sched_list(system);

	init_pid_allocater();

	current = NULL;
	next_run = NULL;
	schedule_running = 1;

	return 0;
}
