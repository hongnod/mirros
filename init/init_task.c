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
#include <os/mutex.h>

DECLARE_MUTEX(printk_mutex);

int printk_thread(void *arg)
{
	for (;;) {
		mutex_lock(&printk_mutex);
		printk("hello world process %d\n", (int)arg);
		mutex_unlock(&printk_mutex);
	}
}

int init_task(void)
{
	kernel_exec("test");
	while (1);
	kthread_run("printk1", printk_thread, (void *)1);
	kthread_run("printk2", printk_thread, (void *)2);
	kthread_run("printk3", printk_thread, (void *)3);
	kthread_run("printk4", printk_thread, (void *)4);
	kthread_run("printk5", printk_thread, (void *)5);
	kthread_run("printk6", printk_thread, (void *)6);
	kthread_run("printk7", printk_thread, (void *)7);
	kthread_run("printk8", printk_thread, (void *)8);
	kthread_run("printk9", printk_thread, (void *)9);
	kthread_run("printk10", printk_thread, (void *)10);

	return 0;
}
