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

#define MAX_ARGV	16
#define MAX_ENVP	16
static char *init_argv[MAX_ARGV + 1] = {NULL};
static char *init_envp[MAX_ENVP + 1] = {NULL};

static int init_mm_struct(struct task_struct *task, u32 user_sp)
{
	struct mm_struct *mm = &task->mm_struct;
	struct mm_struct *mmp;
	
	/*
	 * init mm_struct, kernel task do not need this
	 */
	if (task->flag & PROCESS_TYPE_USER) {
		mmp = &task->parent->mm_struct;
		mm->elf_size = mmp->elf_size;
		mm->stack_size = PROCESS_USER_STACK_BASE - user_sp;
		mm->stack_curr = NULL;
		mm->elf_curr = NULL;
		init_list(&mm->stack_list);
		init_list(&mm->elf_list);
		init_list(&mm->elf_stack_list);
		init_list(&mm->elf_image_list);
	}
	
	return 0;
}

static pid_t init_task_struct(struct task_struct *task, u32 flag)
{
	struct task_struct *parent;

	/*
	 * if thread is a kernel thread, his parent is idle
	 */
	if (flag & PROCESS_TYPE_KERNEL)
		parent = idle;
	else
		parent = current;

	/*
	 * get a new pid
	 */
	task->pid = get_new_pid(task);
	if (task->pid == 0)
		return -EINVAL;

	task->uid = parent->uid;
	task->stack_base = NULL;
	task->stack_base = NULL;
	strncpy(task->name, parent->name, 15);
	task->flag = flag;
	task->state = 0;

	/*
	 * add task to the child list of his parent.
	 */
	task->parent = parent;
	init_list(&task->p);
	init_list(&task->child);
	init_mutex(&task->mutex);

	mutex_lock(&parent->mutex);
	list_add(&parent->child, &task->p);
	mutex_unlock(&parent->mutex);
	
	return task->pid;
}

static void _release_task_memory(struct list_head *head)
{
	struct list_head *list;
	struct page *page;
	void *addr;

	list_for_each(head, list) {
		page = list_entry(list, struct page, plist);
		list_del(list);
		addr = (void *)page_to_va(page);
		free_pages(addr);
	}
}

static int release_task_page_table(struct task_struct *task)
{
	struct mm_struct *tmp = &task->mm_struct;

	/*
	 * release task's page table
	 */
	_release_task_memory(&tmp->stack_list);
	_release_task_memory(&tmp->elf_list);

	return 0;
}

static int inline release_task_pages(struct task_struct *task)
{
	struct mm_struct *ms = &task->mm_struct;

	/*
	 * release task memeory which allocated
	 * for task image.
	 */
	_release_task_memory(&ms->elf_stack_list);
	_release_task_memory(&ms->elf_image_list);

	return 0;
}

static int inline release_kernel_stack(struct task_struct *task)
{
	if (task->stack_base)
		free_pages(task->stack_base);

	task->stack_base = NULL;
	task->stack_origin = NULL;
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
	int i, s, e;
	void *addr;
	struct page *page;

	s = (PROCESS_STACK_SIZE + SIZE_1M - 1) >> 20;
	e = (PROCESS_IMAGE_SIZE + SIZE_1M - 1) >> 20;

	/*
	 * at first,only allocate 16k stack(need 4k pagetable
	 * for future) and ro data bss,section. now the max
	 * size for elf image is 1M, so we also need 4k page
	 * table for elf image. allocate page table for user
	 * stack, one page can map 1M memory. first stack and
	 * elf we only need one page for each.
	 */
	for (i=0; i < s ; i++) {
		addr = get_free_page(GFP_PGT);
		kernel_debug("allcoate statck page_table 0x%x\n", (u32)addr);
		if (addr == NULL) {
			goto error;
		}
		page = va_to_page((unsigned long)addr);
		page->free_base += (SIZE_4K - sizeof(u32));
		list_add_tail(&tmp->stack_list, &page->pgt_list);
	}
	tmp->stack_curr = list_next(&tmp->stack_list);

	/*
	 * allocate page table for elf file memory
	 */
	for (i=0; i < e; i ++) {
		addr = get_free_page(GFP_PGT);
		kernel_debug("allcoate elf page_table 0x%x\n", (u32)addr);
		if (addr == NULL) {
			goto error;
		}
		page = va_to_page((unsigned long)addr);
		list_add_tail(&tmp->elf_list, &page->pgt_list);
	}
	tmp->elf_curr = list_next(&tmp->elf_list);

	return 0;

error:
	release_task_page_table(new);

	return -ENOMEM;
}


static int task_map_memory(void *addr, struct task_struct *task, int flag)
{
	struct mm_struct *ms = &task->mm_struct;
	struct list_head *list;
	struct page *page;
	int flag_new = flag & PROCESS_MAP_MASK;

	switch (flag_new) {
		case PROCESS_MAP_STACK:
			list = ms->stack_curr;
			break;

		case PROCESS_MAP_ELF:
			list = ms->elf_curr;
			break;

		default:
			return -EINVAL;
	}

repeat:
	if (list == NULL) {
		/*
		 * page table has been used over,need more,TBD
		 */
		return -ENOMEM;
	}

	page = list_entry(list, struct page, pgt_list);
	if (page->free_size == 0) {
		list = list_next(list);
		goto repeat;
	}

	/*
	 * we get the base address of the page table to map
	 * the new address default 4k a timei(4b for 4k).
	 * fill page table, flag argument TBD
	 */
	build_page_table_entry((unsigned long)page->free_base,
			       (unsigned long)addr, PAGE_SIZE, 0);

	if (flag_new == PROCESS_MAP_ELF)
		page->free_base += sizeof(unsigned long);
	else
		page->free_base -= sizeof(unsigned long);

	page->free_size -= sizeof(unsigned long);

	return 0;
}

static int alloc_memory_and_map(struct task_struct *task)
{
	struct mm_struct *ms = &task->mm_struct;
	int i, s, e;
	void *addr;
	struct page *page;

	s = page_nr(PROCESS_STACK_SIZE);
	e = page_nr(PROCESS_IMAGE_SIZE);

	/*
	 * if there are no enough memory for task
	 */
	if (mm_free_page(MM_ZONE_NORMAL) < (s + e)) {
		kernel_error("bug: no enough memory\n");
		return -ENOMEM;
	}

	for (i = 0; i < s; i++) {
		addr = get_free_page(GFP_KERNEL);
		if (addr == NULL)
			goto out;

		task_map_memory(addr, task, PROCESS_MAP_STACK);
		page = va_to_page((unsigned long)addr);
		list_add_tail(&ms->elf_stack_list, &page->plist);
	}

	for (i = 0; i < e; i ++) {
		addr = get_free_page(GFP_KERNEL);
		if (addr == NULL)
			goto out;

		task_map_memory(addr, task, PROCESS_MAP_ELF);
		page = va_to_page((unsigned long)addr);
		list_add_tail(&ms->elf_image_list, &page->plist);
	}

	return 0;

out:
	release_task_memory(task);

	return -ENOMEM;
}

static int alloc_kernel_stack(struct task_struct *task)
{
	if(task->stack_base)
		return -EINVAL;

	task->stack_base = get_free_pages(KERNEL_STACK_SIZE >> PAGE_SHIFT,
					  GFP_KERNEL);
	task->stack_origin = task->stack_base;
	kernel_debug("task %s stack is 0x%x\n",
		     task->name, (u32)task->stack_base);

	if (task->stack_base == NULL)
		return -ENOMEM;

	return 0;
}

static int alloc_memory_for_task(struct task_struct *task)
{

	int ret = 0;

	ret = alloc_kernel_stack(task);
	if (ret) {
		kernel_error("allocate kernel stack failed\n");
		return ret;
	}

	if (task->flag & PROCESS_TYPE_USER) {
		ret = alloc_page_table(task);
		if (ret) {
			kernel_error("allcate page_table failed\n");
			goto error;
		}

		ret = alloc_memory_and_map(task);
		if (ret) {
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
	int i = 0, s, e;
	struct list_head *list_old = &old_mm->elf_stack_list;
	struct list_head *list_new = &new_mm->elf_stack_list;
	struct page *page_old;
	struct page *page_new;
	u32 old_base; u32 new_base;

	/*
	 * one page for argv, so need to be included in elf_size
	 * frist need to flush cache to phsyic memory
	 */
	s = page_nr(new_mm->stack_size);
	e = page_nr(new_mm->elf_size + PAGE_SIZE);

	flush_cache();

	/*
	 * copy stack
	 */
	for (i = 0; i < s; i ++) {
		list_old = list_next(list_old);
		list_new = list_next(list_new);
		page_old = list_entry(list_old, struct page, pgt_list);
		page_new = list_entry(list_new, struct page, pgt_list);
		old_base = (u32)page_to_va(page_old);
		new_base = (u32)page_to_va(page_new);
		copy_page_va(new_base, old_base);
	}

	/*
	 * copy elf memory to new task
	 */
	list_old = &old_mm->elf_image_list;
	list_new = &new_mm->elf_image_list;
	for (i = 0; i < e; i++) {
		list_old = list_next(list_old);
		list_new = list_next(list_new);
		page_old = list_entry(list_old, struct page, pgt_list);
		page_new = list_entry(list_new, struct page, pgt_list);
		old_base = (u32)page_to_va(page_old);
		new_base = (u32)page_to_va(page_new);
		copy_page_va(new_base, old_base);
	}

	return 0;
}

static int copy_process(struct task_struct *new)
{
	struct task_struct *old = new->parent;

	if (old)
		copy_process_memory(old, new);

	return 0;
}

static int inline set_up_task_stack(struct task_struct *task, pt_regs *regs)
{
	return arch_set_up_task_stack(task, regs);
}

/*
 * this function used to do some work when
 * switch task such as exchange the page
 * table.
 */
int switch_task(struct task_struct *cur,
		struct task_struct *next)
{
	struct list_head *list;
	struct list_head *head;
	u32 stack_base = PROCESS_USER_STACK_BASE;
	u32 task_base = PROCESS_USER_BASE;
	unsigned long pa;
	struct page *page;

	if (!cur || !next)
		return -EINVAL;

	/*
	 * frist we need flush all cache for the current
	 * process, this cacue the two bugs which spend
	 * me two weeks to debug.
	 */
	flush_cache();

	/*
	 * if the task is a kernel task, we do not
	 * need to load his page table. else if next
	 * run is a userspace task, we load his page
	 * table.
	 */
	if (next->flag & PROCESS_TYPE_KERNEL)
		return 0;

	/*
	 * load the page table for stack, because stack
	 * is grow downward, sub 4m for one page.
	 */
	kernel_debug("switch task next is %s\n", next->name);
	head = &next->mm_struct.stack_list;
	list_for_each (head, list) {
		page = list_entry(list, struct page, pgt_list);
		pa = page_to_pa(page);
		build_tlb_table_entry(stack_base - SIZE_NM(4), pa,
				      SIZE_NM(4), TLB_ATTR_USER_MEMORY);
		stack_base -= SIZE_NM(4);
	}

	/*
	 * load elf image memory page table
	 */
	head = &next->mm_struct.elf_list;
	list_for_each (head, list) {
		page = list_entry(list, struct page, pgt_list);
		pa = page_to_pa(page);
		build_tlb_table_entry(task_base, pa, SIZE_NM(4), TLB_ATTR_USER_MEMORY);
		task_base += SIZE_NM(4);
	}

	return 0;
}

static int set_up_task_argv(struct task_struct *task,
			    char **argv)
{
	char *argv_base = (char *)(PROCESS_USER_BASE + MAX_ARGV * sizeof(void *));
	int i, length;
	struct list_head *list = &task->mm_struct.elf_image_list;
	unsigned long load_base;
	struct page *page;
	u32 *table_base;

	/*
	 * copy the argument to the process memory space
	 * the argv is stored at the first page of the
	 * task memory space. argv_base is user space base
	 * address and load base is kernel space address.
	 */
	list = list_next(list);
	page = list_entry(list, struct page, plist);
	load_base = page_to_va(page);
	table_base = (u32 *)load_base;
	load_base = load_base + ((unsigned long)argv_base - PROCESS_USER_BASE);
	for (i = 0; i < MAX_ARGV; i++) {
		if (argv[i] == NULL)
			break;
		length = strlen(argv[i]);
		strcpy((char *)load_base, argv[i]);
		*table_base = (unsigned long)argv_base;
		argv_base += length;
		load_base += length;
		table_base++;
		argv_base = (char *)baligin((u32)argv_base, 4);
		load_base = baligin(load_base, 4);
	}

	kernel_debug("exit set up argv\n");

	return 0;
}

static int inline set_task_return_value(pt_regs *reg,
		struct task_struct *task)
{
	return arch_set_task_return_value(reg, task);
}

struct task_struct *fork_new_task(char *name, u32 user_sp, u32 flag)
{
	pid_t pid;
	struct task_struct *new;
	int ret = 0;

	new = kmalloc(sizeof(struct task_struct), GFP_KERNEL);
	if (new == NULL) {
		kernel_error("can not allcate memory for new task\n");
		return NULL;
	}

	pid = init_task_struct(new, flag);
	if (pid == 0) {
		kernel_error("invaild pid \n");
		goto exit;
	}

	/*
	 * before init mm_struct and sched_struct.
	 * the parent process must be ensure.
	 */
	init_mm_struct(new, user_sp);
	init_sched_struct(new);

	if (name)
		strncpy(new->name, name, 15);
	
	/*
	 * allocate page table and memory for task
	 */
	ret = alloc_memory_for_task(new);
	if (ret) {
		kernel_error("allocate memory for task failed\n");
		goto exit;
	}

exit:
	if (ret) {
		kfree(new);
		new = NULL;
	}

	return new;
}

static pid_t do_fork(char *name, pt_regs *regs, u32 user_sp, u32 flag)
{
	struct task_struct *new;

	/*
	 * get a new task_struct instance
	 */
	new = fork_new_task(name, user_sp, flag);
	if (!new) {
		kernel_error("fork new task failed\n");
		return -ENOMEM;
	}

	/*
	 * if the task is a kernel task, or the task is
	 * forked by a kernel task,we do not need to copy
	 * process to it
	 */
	if (flag & PROCESS_TYPE_USER)
		copy_process(new);

	if (flag & PROCESS_FLAG_FORK)
		set_task_return_value(regs, new);

	set_up_task_stack(new, regs);
	set_task_state(new, PROCESS_STATE_PREPARE);

	return 0;
}

static void inline init_pt_regs(pt_regs *regs, void *fn, void *arg)
{
	/*
	 * set up the stack of the task before his
	 * first running
	 */
	arch_init_pt_regs(regs, fn, arg);
}

int kthread_run(char *name, int (*fn)(void *arg), void *arg)
{
	u32 flag = 0;
	pt_regs regs;

	flag |= PROCESS_TYPE_KERNEL;	
	init_pt_regs(&regs, (void *)fn, arg);

	kernel_debug("ready to fork a new task %s\n", name);
	if (do_fork(name, &regs, 0, flag)) {
		kernel_error("create kernel thread failed\n");
		return -ENOMEM;
	}

	return 0;
}

/*
 * this function is used to kill a task
 * and recycle its resource
 */
void release_task(struct task_struct *task)
{
	if (task) {
		release_task_memory(task);
		kfree(task);
	}
}

int load_elf_section(struct elf_section *section,
		     struct file *file,
		     struct mm_struct *mm)
{
	struct list_head *list = &mm->elf_image_list;
	struct page *page = NULL;
	int i, j, k;
	char *base_addr;
	unsigned long base;

	/*
	 * the first 4k memory for task is 0x00001000
	 * so we can calculate the location of the section
	 * in the memeory
	 */
	if ((section->load_addr < 0x00001000) ||
	    (section->size == 0))
		return -EINVAL;

	/*
	 * find the memeory location in the list
	 */
	base = section->load_addr - PROCESS_USER_BASE;
	i = base / SIZE_4K;
	j = base % SIZE_4K;
	kernel_debug("load elf image page %d offset %d\n", i, j);
	for (k = 0; k <= i; k++) {
		list = list_next(list);
		if (list == NULL)
			return -ENOMEM;
	}

	/*
	 * if the section is bss, we clear the all memory
	 * to 0, else we load the data to the image.
	 */
	k = section->size;
	i = 0;
	mm->elf_size += k;
	do {
		page = list_entry(list, struct page, plist);
		base_addr = (char *)page->free_base;
		base_addr = base_addr + j;
		if (strncmp(section->name, ".bss", 4)) {
			kernel_debug("load image: %s 0x%x %d\n",
				     section->name, base_addr, i);
			fs_seek(file, section->offset);
			i = fs_read(file, base_addr, SIZE_4K - j);
			if (i < 0)
				return -EIO;
		} else {
			memset(base_addr, 0, k >= SIZE_4K ? SIZE_4K -j : k);
			break;
		}

		k = k - (SIZE_4K - j);
		j = 0;
		list = list_next(list);

	} while (k > 0);
	
	return 0;
}

int load_elf_image(struct task_struct *task,
		   struct file *file,
		   struct elf_file *elf)
{
	struct mm_struct *mm = &task->mm_struct;
	struct elf_section *section = elf->head;
	int ret = 0;

	if (!task || !file || !elf)
		return -ENOENT;

	/*
	 * load each section to memory
	 */
	for ( ; section != NULL; section = section->next) {
		ret = load_elf_section(section, file, mm);
		if (ret)
			return ret;
	}

	return 0;
}

int do_exec(char *name,
	    char **argv,
	    char **envp,
	    pt_regs *regs)
{
	struct task_struct *new;
	struct file *file;
	int err = 0;
	struct elf_file *elf = NULL;

	if (current->flag & PROCESS_TYPE_KERNEL) {
		/*
		 * need to fork a new task, before exec
		 * but no call sys_fork to fork a new task
		 * becase kernel process has not allocat page
		 * table and mm_struct.
		 */
		new = fork_new_task(name, PROCESS_USER_STACK_BASE, PROCESS_TYPE_USER);
		if (!new) {
			kernel_debug("can not fork new process when exec\n");
			return -ENOMEM;
		}
	} else
		new = current;

	file = fs_open(name);
	if (!file) {
		kernel_error("no such file %s\n", name);
		err = -ENOENT;
		goto err_open_file;
	}

	elf = get_elf_info(file);
	if (!elf) {
		err = -EINVAL;
		goto exit;
	}

	/*
	 * load the elf file to memory
	 */
	err = load_elf_image(new, file, elf);
	if (err) {
		kernel_error("failed to load elf file to memory\n");
		goto release_elf_file;
	}

	init_pt_regs(regs, NULL, (void *)argv);
	set_up_task_argv(new, argv);
	set_up_task_stack(new, regs);
	set_task_state(new, PROCESS_STATE_PREPARE);

release_elf_file:
	release_elf_file(elf);
exit:
	fs_close(file);

err_open_file:
	if(err && (current->flag & PROCESS_TYPE_KERNEL)) {
		release_task(new);
	}

	return err;
}

int kernel_exec(char *filename)
{
	pt_regs regs;

	memset((char *)&regs, 0, sizeof(pt_regs));
	init_argv[0] = filename;
	init_argv[1] = NULL;
	return do_exec(filename, init_argv, init_envp, &regs);
}

pid_t sys_fork(void)
{
	u32 flag = 0;
	pt_regs *regs = get_pt_regs();

	flag |= PROCESS_TYPE_USER | PROCESS_FLAG_FORK;

	return do_fork(NULL, regs, regs->sp, flag);
}
DEFINE_SYSCALL(fork, SYSCALL_FORK_NR, sys_fork);

pid_t sys_exec(pt_regs *regs)
{
	return 0;
}
DEFINE_SYSCALL(exec, SYSCALL_EXEC_NR, sys_exec);

int build_idle_task(void)
{
	idle = kmalloc(sizeof(struct task_struct), GFP_KERNEL);
	if (idle == NULL)
		return -ENOMEM;

	idle->pid = get_new_pid(idle);
	if (idle->pid != 0) {
		kfree(idle);
		return -EFAULT;
	}
	idle->uid = 0;

	strncpy(idle->name, "idle", PROCESS_NAME_SIZE - 1);
	idle->flag = 0 | PROCESS_TYPE_KERNEL;

	idle->parent = NULL;
	init_list(&idle->child);
	init_list(&idle->p);

	init_mutex(&idle->mutex);
	init_sched_struct(idle);
	idle->state = PROCESS_STATE_RUNNING;

	/*
	 * add task,because kernel already have memory,we
	 * do not allocate memory for him
	 */
	current = idle;
	next_run = idle;

	return 0;
}
