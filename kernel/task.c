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

static int init_mm_struct(struct task_struct *task)
{
	struct mm_struct *mm = &task->mm_struct;
	
	/*
	 * init mm_struct
	 */
	mm->elf_size = 0;
	mm->stack_size = 0;
	mm->stack_curr = NULL;
	mm->elf_curr = NULL;
	init_list(&mm->stack_list);
	init_list(&mm->elf_list);
	
	return 0;
}

static pid_t init_task_struct(struct task_struct *task,u32 flag)
{
	struct task_struct *parent;

	/*
	 * if thread is a kernel thread, his parent is idle
	 */
	if(flag & PROCESS_TYPE_KERNEL){
		parent = idle;
	}
	else{
		parent = current;
	}

	/*
	 * get a new pid
	 */
	task->pid = get_new_pid(task);{
		if(task->pid == 0){
			return -EINVAL;
		}
	}
	task->uid = parent->uid;
	task->stack_base = NULL;
	task->bin = NULL;
	strncpy(task->name,parent->name,15);
	task->flag = flag;

	/*
	 *add task to the child list of his parent.
	 */
	task->parent = parent;
	init_list(&task->p);
	init_list(&task->child);
	init_mutex(&task->mutex);

	mutex_lock(&parent->mutex);
	list_add(&parent->child,&task->p);
	mutex_unlock(&parent->mutex);
	
	/*
	 * before init mm_struct and sched_struct.
	 * the parent process must be ensure.
	 */
	init_mm_struct(task);
	init_sched_struct(task);
	
	return task->pid;
}

static inline void _release_task_page_table(struct list_head *head)
{
	struct list_head *list;
	struct page *page;

	list_for_each(head,list){
		page = list_entry(list,struct page,pgt_list);
		list_del(list);
		free_pages((void *)page_to_va(page));
	}
}

static int release_task_page_table(struct task_struct *task)
{
	struct mm_struct *tmp = &task->mm_struct;

	/*
	 *release task's page table
	 */
	_release_task_page_table(&tmp->stack_list);
	_release_task_page_table(&tmp->elf_list);

	return 0;
}

static int _release_task_memory(struct list_head *head,int size)
{
	struct list_head *list;
	struct page *page;
	int i = 0;
	int sum = 0;
	u32 *addr;
	unsigned long val;

	/*
	 *a page table contains 1024 page,
	 *one page takes 4b in page_table;
	 *free_pages --->va
	 */
	list_for_each(head,list){
		page = list_entry(list,struct page,pgt_list);
		addr = (u32 *)page_to_va(page);
		for(i = 0; i < PAGE_SIZE>>2;i++){
			val = *addr;
			free_pages((void *)pa_to_va(val));
			addr++;
			sum += PAGE_SIZE;
			if(sum == size)
				return 0;
		}
	}

	return 1;
}

static int inline release_task_pages(struct task_struct *task)
{
	struct mm_struct *ms = &task->mm_struct;

	_release_task_memory(&ms->stack_list,ms->stack_size);
	_release_task_memory(&ms->elf_list,ms->elf_size);

	return 0;
}

static int inline release_kernel_stack(struct task_struct *task)
{
	if(task->stack_base){
		free_pages(task->stack_base);
	}

	task->stack_base = NULL;

	return 0;
}

static int release_task_memory(struct task_struct *task)
{

	release_task_pages(task);
	release_task_page_table(task);
	release_kernel_stack(task);

	return 0;
}

static int alloc_page_table(struct task_struct *new)
{
	struct mm_struct *tmp = &new->mm_struct;
	struct task_struct *old = new->parent;
	int i;
	void *addr;
	struct page *page;

	/*
	 *at first,only allocate 16k stack(need 1pagetable for future)
	 *and ro data bss,section
	 *according to following var we can find the location
	 *table when we map a new memory.
	 */
	tmp->elf_size = old->mm_struct.elf_size;
	tmp->stack_size = old->mm_struct.stack_size;

	for(i=0; i < tmp->stack_size ; i += PAGE_MAP_SIZE){
		kernel_debug("allcoate statck page_table %d\n",i);
		addr = get_free_page(GFP_PGT);
		if(addr == NULL){
			goto error;
		}
		page = va_to_page((unsigned long)addr);
		list_add_tail(&tmp->stack_list,&page->pgt_list);
	}
	tmp->stack_curr = list_next(&tmp->stack_list);

	for(i=0; i < tmp->elf_size ; i += PAGE_MAP_SIZE){
		kernel_debug("allcoate elf page_table %d\n",i);
		addr = get_free_page(GFP_PGT);
		if(addr == NULL){
			goto error;
		}
		page = va_to_page((unsigned long)addr);
		list_add_tail(&tmp->elf_list,&page->pgt_list);	
	}
	tmp->elf_curr = list_next(&tmp->elf_list);

	return 0;

error:
	release_task_page_table(new);

	return 1;
}


static int task_map_memory(void *addr,struct task_struct *task,int flag)
{
	struct mm_struct *ms = &task->mm_struct;
	struct list_head *list;
	struct page *page;

	switch(flag & TASK_MAP_MASK){
		case TASK_MAP_STACK:
			list = ms->stack_curr;
			break;

		case TASK_MAP_ELF:
			list = ms->elf_curr;
			break;

		default:
			break;
	}

repeat:
	if(list == NULL){
		/*
		 *page table has been used over,need more,TBD
		 */
		return -ENOMEM;
	}

	page = list_entry(list,struct page,pgt_list);
	if(page->free_size == 0){
		list = list_next(list);
		goto repeat;
	}

	/*
	 *we get the base address of the page table to map the new address
	 *default 4k a timei(4b for 4k).
	 */
	/*
	 *fill page table, flag argument TBD
	 */
	build_page_table_entry((unsigned long)page->free_base,(unsigned long)addr,PAGE_SIZE,0);

	page->free_base = page->free_base + sizeof(unsigned long);
	page->free_size = page->free_size - sizeof(unsigned long);

	return 0;
}

static int alloc_memory_and_map(struct task_struct *task)
{
	struct mm_struct *ms = &task->mm_struct;
	int i;
	void *addr;

	/*
	 *if there are no enough memory for task
	 */
	if(mm_free_page(MM_ZONE_NORMAL) < page_nr(ms->stack_size + ms->elf_size)){
		kernel_error("bug: no enough memory\n");
		return -ENOMEM;
	}

	for(i=0;i < ms->stack_size; i++){
		addr = get_free_page(GFP_KERNEL);
		if(addr == NULL){
			goto out;
		}
		task_map_memory(addr,task,TASK_MAP_STACK);
	}

	for(i=0; i < ms->elf_size; i++){
		addr = get_free_page(GFP_KERNEL);
		if(addr == NULL){
			goto out;
		}
		task_map_memory(addr,task,TASK_MAP_ELF);
	}

out:
	release_task_memory(task);

	return -ENOMEM;
}

static int alloc_kernel_stack(struct task_struct *task)
{
	if(task->stack_base)
		return -EINVAL;

	task->stack_base = get_free_pages(KERNEL_STACK_SIZE>>PAGE_SHIFT,GFP_KERNEL);
	kernel_debug("task %s stack is 0x%x\n",task->name,(u32)task->stack_base);
	if(task->stack_base == NULL){
		return -ENOMEM;
	}

	return 0;
}

static int alloc_memory_for_task(struct task_struct *task)
{

	int ret = 0;

	ret = alloc_kernel_stack(task);
	if(ret){
		kernel_error("allocate kernel stack failed\n");
		return ret;
	}

	if(task->flag & PROCESS_TYPE_USER){
		ret = alloc_page_table(task);
		if(ret){
			kernel_error("allcate page_table failed\n");
			goto error;
		}

		ret = alloc_memory_and_map(task);
		if(ret){
			kernel_error("no enough memory for task\n");
			goto error;
		}
	}

	return ret;

error:
	release_kernel_stack(task);

	return -ENOMEM;
}

static int copy_process_memory(struct task_struct *old,struct task_struct *new)
{
	struct mm_struct *old_mm = &old->mm_struct;
	struct mm_struct *new_mm = &new->mm_struct;
	int i = 0;
	struct list_head *list_old = &old_mm->stack_list;
	struct list_head *list_new = &new_mm->stack_list;
	struct page *page_old;
	struct page *page_new;
	u32 *old_base; u32 *new_base;

	if(old_mm->elf_size != new_mm->elf_size || 
			old_mm->stack_size != new_mm->stack_size){
		kernel_error("different size with father process\n");
		return -EFAULT;
	}

	/*
	 *copy stack
	 */
	for(i = 0; i < old_mm->stack_size; i++){
		if(i % (PAGE_SIZE/sizeof(u32)) == 0){
			list_old = list_next(list_old);
			list_new = list_next(list_new);
			page_old = list_entry(list_old,struct page,pgt_list);
			page_new = list_entry(list_new,struct page,pgt_list);
			old_base = (u32 *)page_to_va(page_old);
			new_base = (u32 *)page_to_va(page_new);
		}
		copy_page_pa(*new_base,*old_base);

		old_base++;
		new_base++;
	}

	/*
	 *copy elf memory to new task
	 */
	list_old = &old_mm->elf_list;
	list_new = &new_mm->elf_list;
	for(i = 0; i < old_mm->elf_size; i++){
		if(i % (PAGE_SIZE/sizeof(u32)) == 0){
			list_old = list_next(list_old);
			list_new = list_next(list_new);
			page_old = list_entry(list_old,struct page,pgt_list);
			page_new = list_entry(list_new,struct page,pgt_list);
			old_base = (u32 *)page_to_va(page_old);
			new_base = (u32 *)page_to_va(page_new);
		}
		copy_page_pa(*new_base,*old_base);

		old_base++;
		new_base++;	
	}

	return 0;
}

static int copy_process(struct task_struct *new)
{
	struct task_struct *old = new->parent;

	copy_process_memory(old,new);

	return 0;
}

static int inline set_up_process(pt_regs *regs,
		struct task_struct *task)
{
	return arch_set_up_process(regs,task);
}

static int inline set_task_return_value(pt_regs *reg,
		struct task_struct *task)
{
	return arch_set_task_return_value(reg,task);
}

static pid_t do_fork(char *name,pt_regs regs,u32 sp,u32 flag)
{
	pid_t pid;
	struct task_struct *new;
	int ret = 0;

	new = kmalloc(sizeof(struct task_struct),GFP_KERNEL);
	if(new == NULL){
		kernel_error("can not allcate memory for new task\n");
		return -ENOMEM;
	}

	pid = init_task_struct(new,flag);
	if(pid == 0){
		kernel_error("invaild pid \n");
		goto exit;
	}
	if(name){
		strncpy(new->name,name,15);
	}
	
	/*
	 * allocate page table and memory for task
	 */
	ret = alloc_memory_for_task(new);
	if(ret){
		kernel_error("allocate memory for task failed\n");
		goto exit;
	}

	/*
	 * if the task is a kernel task, we do not need to copy
	 * process to it
	 */
	if(flag & PROCESS_TYPE_KERNEL)
		goto setup_and_add_task;

	copy_process(new);
	set_task_return_value(&regs,new);

setup_and_add_task:
	set_up_process(&regs,new);
	set_task_state(new, TASK_STATE_PREPARE);
	add_new_task(new);

	return pid;

exit:
	kfree(new);

	return ret;
}

pid_t sys_fork(pt_regs regs,u32 sp)
{
	u32 flag = 0;

	flag |= PROCESS_TYPE_USER;

	return do_fork(NULL,regs,sp,flag);
}

static void inline init_pt_regs(pt_regs *regs,int (*fn)(void *arg),void *arg)
{
	/*
	 * set up the stack of the task before his
	 * first running
	 */
	arch_init_pt_regs(regs,fn,arg);
}

int kthread_run(char *name,int (*fn)(void *arg),void *arg)
{
	u32 flag = 0;
	pt_regs regs;

	flag |= PROCESS_TYPE_KERNEL;	
	init_pt_regs(&regs,fn,arg);

	kernel_debug("ready to fork a new task %s\n",name);
	if(!do_fork(name,regs,0,flag)){
		kernel_error("create kernel thread failed\n");
		return -ENOMEM;
	}

	return 0;
}

int build_idle_task(void)
{
	idle = kmalloc(sizeof(struct task_struct),GFP_KERNEL);
	if(idle == NULL){
		return -ENOMEM;
	}

	idle->pid = get_new_pid(idle);
	if(idle->pid != 0){
		kfree(idle);
		return -EFATAL;
	}
	idle->uid = 0;

	strncpy(idle->name,"idle",15);
	idle->bin = NULL;
	idle->flag = 0 | PROCESS_TYPE_KERNEL;

	idle->parent = NULL;
	init_list(&idle->child);
	init_list(&idle->p);

	init_mutex(&idle->mutex);
	init_sched_struct(idle);
	/*
	 * add task,because kernel already have memory,we
	 * do not allocate memory for him
	 */

	current = idle;
	next_run = idle;

	/*
	 * after add the ilde task, the kernel is runing 
	 * and the irqs is enable.
	 */
	set_task_state(idle, TASK_STATE_RUNNING);
	add_new_task(idle);

	return 0;
}
