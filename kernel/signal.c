#include <os/types.h>
#include <os/sched.h>
#include <os/string.h>
#include <os/bitops.h>
#include <os/list.h>
#include <os/mm.h>
#include <os/slab.h>
#include <os/errno.h>
#include <os/printk.h>
#include <os/mmu.h>
#include <asm/asm_sched.h>
#include <os/elf.h>
#include <os/fs.h>
#include <os/mm.h>
#include <os/task.h>
#include <os/syscall.h>
#include <os/signal.h>

static void *signal_table[MAX_SIGNAL] = { 0 };

int task_kill_self(struct task_struct *task)
{
	struct task_struct *system_killer = pid_get_task(0);

	if (!system_killer)
		return -EINVAL;
	
	/*
	 * when a task try to kill himself, need to
	 * wake up system_killer process to help doing
	 * this
	 */
	kernel_debug("Kill self\n");
	set_task_state(task, PROCESS_STATE_IDLE);
	wakeup_task(system_killer);
	sched();
	return 0;
}

int task_kill_other(struct task_struct *killer, struct task_struct *task)
{
	return kill_task(task);
}

int signal_kill(struct signal *signal)
{
	struct task_struct *from;
	struct task_struct *to;
	int ret = 0;

	from = pid_get_task(signal->from);
	to = pid_get_task(signal->to);

	/*
	 * if from == to, it means task has stoped and 
	 * need to exit. if not it means one task need
	 * to kill another task, low prio can not kill
	 * the task whose prio is higher than him
	 */
	if ((signal->from) == (signal->to)) {
		ret = task_kill_self(from);
	}
	else {
		if (from->prio < to->prio)
			ret = task_kill_other(from, to);
		else {
			kernel_error("Can not kill task %d --> %d\n",
				     signal->from, signal->to);
			ret = -EINVAL;
		}
	}

	return ret;
}

int is_vaild_signal(struct signal *signal)
{
	if (signal->sig >= MAX_SIGNAL)
		return 0;

	if(!pid_get_task(signal->from))
		return 0;

	if (!pid_get_task(signal->to))
		return 0;

	return 1;
}

int __sys_signal(struct signal *signal)
{
	int (*handler)(struct signal *signal);

	if (is_vaild_signal(signal)) {
		handler = signal_table[signal->sig];
		if (handler)
			return handler(signal);
		else
			kernel_error("No signal handler to handle \
				      %d signal\n", signal->sig);
	}

	return -EINVAL;
}

int sys_signal(pid_t pid, signal_t sig, void *arg)
{
	struct signal signal;

	kernel_debug("sys signal pid is %d\n", pid);
	signal.from = get_task_pid(current);
	signal.to = pid;
	signal.sig = sig;
	signal.message = arg;

	return __sys_signal(&signal);
}
DEFINE_SYSCALL(signal, __NR_signal, (void *)sys_signal);

int register_signal_handler(signal_t sig, int (*handler)(struct signal *signal))
{
	if (!handler || (sig >= MAX_SIGNAL))
		return -EINVAL;
	
	kernel_debug("Register signal hanlder for SIGNAL: %d\n", sig);
	if (signal_table[sig])
		kernel_info("Override handler for %d\n", sig);
	signal_table[sig] = handler;

	return 0;
}

int init_signal_handler(void)
{
	register_signal_handler(PROCESS_SIGNAL_KILL, signal_kill);
}
