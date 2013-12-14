#include <os/types.h>
#include <os/mm.h>
#include <os/init.h>
#include <os/string.h>
#include <os/mirros.h>
#include <os/slab.h>
#include <os/printk.h>
#include <os/panic.h>
#include <os/task.h>
#include <os/sched.h>
#include <os/interrupt.h>

extern int mm_init(void);
extern int init_platform_info(void);
extern int slab_init(void);
extern int sched_init(void);
extern int console_early_init(void);
extern int console_late_init(void);
extern int arch_irq_init(void);
extern int trap_init(void);
extern int timer_tick_init(void);
extern int build_idle_task(void);
extern int arch_init_exception_stack(void);
extern int init_task();
extern unsigned long mount_ramdisk(void);
extern int syscall_init(void);
extern int init_signal_handler(void);
extern int system_killer(void *arg);

int main(void)
{
	/*
	 * this function can let us use printk before 
	 * kernel mmu page table build
	 */
	disable_irqs();
	console_early_init();
	init_platform_info();
	mm_init();

	/*
	 * after kernel mmu talbe build,we need init the
	 * console again to use the uart0,or we can implement
	 * a more stronger printk system if needed.
	 */
	console_late_init();
	slab_init();
	trap_init();
	arch_init_exception_stack();
	arch_irq_init();
	syscall_init();
	init_signal_handler();
	timer_tick_init();
	sched_init();
	mount_ramdisk();

	build_idle_task();
	/* now we can enable irq */
	enable_irqs();
	kthread_run("system_killer", system_killer, NULL);
	init_task();
	set_task_prio(idle, MAX_PRIO - 1);
	for (;;) {
		kernel_debug("In Idle State\n");
		sched();
	}

	return 0;
}
