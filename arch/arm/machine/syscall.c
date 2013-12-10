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
#include <os/mm.h>
#include <os/task.h>
#include <os/syscall.h>
#include <os/signal.h>

int sys_execve(char __user *filename,
	       char __user **argv,
	       char __user **envp)
{
	int ret = 0;
	pt_regs *regs = get_pt_regs();

	ret = do_exec(filename, argv, envp, regs);
	if (!ret)
		ret = regs->r0;

	return ret;
}
DEFINE_SYSCALL(execve, SYSCALL_EXECVE_NR, (void *)sys_execve);

pid_t sys_fork(void)
{
	u32 flag = 0;
	pt_regs *regs = get_pt_regs();

	flag |= PROCESS_TYPE_USER | PROCESS_FLAG_FORK;

	return do_fork(NULL, regs, regs->sp, flag);
}
DEFINE_SYSCALL(fork, SYSCALL_FORK_NR, (void *)sys_fork);

void sys_exit(int ret)
{
	struct task_struct *task = current;

	kernel_debug("task %s exit with code %d\n", task->name, ret);
	sys_signal(get_task_pid(current), PROCESS_SIGNAL_KILL, NULL);
}
DEFINE_SYSCALL(exit, SYSCALL_EXIT_NR, (void *)sys_exit);


