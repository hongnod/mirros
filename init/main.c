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

int printk_thread(void *arg)
{
	while(1){
		printk("%s\n",(char *)arg);
		sched();
	}

	return 0;
}

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
	arch_irq_init();
	sched_init();
	timer_tick_init();
	if(build_idle_task()){
		panic("can not build kernel task\n");
	}

	kthread_run(printk_thread,"hello world\n");
#if 0
	while(1){
		buf = (char *)kmalloc(640,GFP_KERNEL);
		if(!buf){
			printk("no more memory\n");
			break;
		}
		memset(buf,1,640);
		printk("buf = 0x%x\n",(unsigned int)buf);
	}
#endif
	printk("begin to loop");
	while(1){
		kernel_info("in idle\n");
		sched();
	}

	return 0;
}
