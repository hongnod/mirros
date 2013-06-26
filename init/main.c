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

int main(void)
{
	/*
	 *this function can let us use printk before 
	 *kernel mmu page table build
	 */
	console_early_init();

	init_platform_info();
	mm_init();

	/*
	 *after kernel mmu talbe build,we need init the
	 *console again to use the uart0,or we can implement
	 *a more stronger printk system if needed.
	 */
	console_late_init();
	slab_init();
	trap_init();
	arch_init_exception_stack();
	arch_irq_init();
	sched_init();
	timer_tick_init();

	if(build_idle_task()){
		panic("can not build kernel task\n");
	}

	/*
	 *now we can enable irq
	 */
	enable_irqs();
	mount_ramdisk();

	init_task();

	for (;;) {
		printk("In Idle State\n");
		sched();
	}

	return 0;
}
