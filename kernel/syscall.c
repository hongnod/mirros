#include <os/types.h>
#include <os/mm.h>
#include <os/slab.h>
#include <os/errno.h>
#include <os/printk.h>
#include <os/list.h>
#include <os/string.h>
#include <os/sched.h>
#include <os/fs.h>
#include <os/elf.h>
#include <os/syscall.h>
#include <os/panic.h>

unsigned long *syscall_table_base = NULL;
extern unsigned long syscall_table_start;
extern unsigned long syscall_table_end;

/*
pid_t sys_fork(pt_regs *regs)
{
	u32 flag = 0;

	flag |= PROCESS_TYPE_USER;

	return do_fork(NULL, &regs, sp, flag);
}

pid_t sys_exec(pt_regs *regs)
{
	return do_exec()
}

int sys_open(pt_regs *regs, char *name, int flags)
{
	return NULL;
}
*/

int sys_debug(pt_regs *regs, int a, int b, int c, int d)
{
	printk("syscall debug %d %d %d %d\n", a, b, c, d);

	return a + b + c + d;
}

DEFINE_SYSCALL(debug, 0, sys_debug);

int install_syscall(int nr, unsigned long *addr)
{
	unsigned long *tmp;
	int error = -EINVAL;

	if (nr > (SYSCALL_NR - 1) || (nr <0) || (!addr)) {
		kernel_error("invaild argument for install syscall\n");
		return -EINVAL;
	}

	tmp = syscall_table_base + nr;
	if (!(*tmp)) {
		kernel_info("install %d syscall addr is0x%x\n", nr, (u32)addr);
		*tmp = (unsigned long)addr;
		kernel_info("addr base is 0x%x\n", (u32)*tmp);
		error = 0;
	}

	return error;
}

int syscall_init(void)
{
	int pages = page_nr(SYSCALL_NR * sizeof(unsigned long));
	int error;
	struct syscall *table = (struct syscall *)syscall_table_start;
	int nr;
	int i;

	syscall_table_base = get_free_pages(pages, GFP_KERNEL);
	kernel_debug("syscall table base is 0x%x\n", (u32)syscall_table_base);
	if (!syscall_table_base) {
		panic("can not allocate memory for syscall table\n");
	}
	memset((char *)syscall_table_base, 0, pages << PAGE_SHIFT);

	/*
	 * we are in interrupt disabled state, so do not need
	 * to dsiable irqs, install all defined syscall.
	 */
	nr = (syscall_table_end - syscall_table_start) / sizeof(struct syscall);
	for (i = 0; i < nr; i++) {
		error = install_syscall(table->nr, table->addr);
		if (error) {
			kernel_error("syscall %d has been registered\n", table->nr);
		}
		table++;
	}

	return 0;
}

