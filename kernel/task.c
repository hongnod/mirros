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

static pid_t init_task_struct(struct task_struct *task,u32 flag)
{
	struct task_struct *parent;

	if(flag & PROCESS_TYPE_KERNEL){
		parent = kernel;
	}
	else{
		parent = current;
	}

	task->pid = get_new_pid(task);
	task->uid = parent->uid;
	task->stack_base = NULL;
	//strncpy(task->name,name,15);
	task->bin = NULL;
	task->prio = parent->prio;
	task->pre_prio = parent->pre_prio;
	task->stack_base = NULL;
	task->flag = flag;

	task->mm_struct.elf_size = 0;
	task->mm_struct.stack_size = 0;
	task->mm_struct.stack_curr = NULL;
	task->mm_struct.elf_curr = NULL;
	init_list(&task->mm_struct.stack_list);
	init_list(&task->mm_struct.elf_list);

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

#define KERNEL_STACK_SIZE	4096
static int inline release_kernel_stack(struct task_struct *task)
{
	if(task->stack_base){
		free_pages(task->stack_base);
	}

	return 0;
}

static int release_task_memory(struct task_struct *task)
{

	release_task_pages(task);
	release_task_page_table(task);
	release_kernel_stack(task);

	return 0;
}

static int alloc_page_table(struct task_struct *old,struct task_struct *new)
{
	struct mm_struct *tmp = &new->mm_struct;
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
	if(task->stack_base == NULL){
		return -ENOMEM;
	}

	return 0;
}

static int alloc_memory_for_task(struct task_struct *task)
{

	int ret = 0;

	if(task->flag & PROCESS_TYPE_KERNEL){
		ret = alloc_kernel_stack(task);
		if(ret){
			kernel_error("allocate kernel stack failed\n");
		}

		return ret;
	}

	ret = alloc_page_table(current,task);
	if(ret){
		kernel_error("allcate page_table failed\n");
		return ret;
	}

	ret = alloc_memory_and_map(task);
	if(ret){
		kernel_error("no enough memory for task\n");
		return ret;
	}

	return 0;
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

static int copy_process(struct task_struct *old,struct task_struct *new)
{
	copy_process_memory(old,new);

	return 0;
}

static void set_up_process(pt_regs regs,struct task_struct *task)
{
	if(task->flag & PROCESS_TYPE_KERNEL)
		regs.sp = (u32)task->stack_base;

	task->regs = regs;
	if(task->flag &PROCESS_TYPE_USER){
		/*
		 *if task is user fork task,r0 need tob set as the return value.
		 */
		task->regs.r0 = 0;
	}
}

pid_t do_fork(pt_regs regs,u32 sp,u32 flag)
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
	
	ret = alloc_memory_for_task(new);
	if(ret){
		kernel_error("allocate memory for task failed\n");
		goto exit;
	}

	if(flag & PROCESS_TYPE_KERNEL)
		goto setup_and_add_task;

	copy_process(current,new);

setup_and_add_task:
	set_up_process(regs,new);
	add_new_task(new);

	return pid;

exit:
	kfree(new);

	return -1;
}

pid_t sys_fork(pt_regs regs,u32 sp)
{
	u32 flag = 0;

	flag |= PROCESS_TYPE_USER;

	return do_fork(regs,sp,flag);
}

static void inline init_pt_regs(pt_regs *regs,int (*fn)(void *arg),void *arg)
{
	regs->r0 = (u32)arg;
	regs->r1 = 1;
	regs->r2 = 2;
	regs->r3 = 3;
	regs->r4 = 4;
	regs->r5 = 5;
	regs->r6 = 6;
	regs->r7 = 7;
	regs->r8 = 8;
	regs->r9 = 9;
	regs->r10 = 10;
	regs->r11 = 11;
	regs->r12 = 12;
	regs->sp = 0;
	regs->lr = (u32)fn;
	regs->pc = 0;
	regs->cpsr = 0;
}

int kthread_run(int (*fn)(void *arg),void *arg)
{
	u32 flag = 0;
	pt_regs regs;
	pid_t pid = 0;

	flag |= PROCESS_TYPE_KERNEL;	
	init_pt_regs(&regs,fn,arg);

	pid = do_fork(regs,0,flag);

	if(!pid)
		return -ENOMEM;
		
	return 0;
}
