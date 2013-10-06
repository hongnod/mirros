#include <os/type.h>
#include <os/mm.h>
#include <os/slab.h>
#include <os/errno.h>
#include <os/printk.h>
#include <os/list.h>
#include <os/string.h>
#include <os/sched.h>
#include <os/fs.h>
#include <os/elf.h>

pid_t sys_fork(pt_regs regs, u32 sp)
{
	u32 flag = 0;

	flag |= PROCESS_TYPE_USER;

	return do_fork(NULL, &regs, sp, flag);
}

pid_t sys_exec(pt_regs regs)
{
	return do_exec()
}
