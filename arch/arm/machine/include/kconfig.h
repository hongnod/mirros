#ifndef _KCONFIG_H
#define _KCONFIG_H

#include <os/mirros.h>
/*
 * we give kernel 24K to store the important information
 * such mmu page table and kernel stack.
 * so the kernel load address must be 0x18000 aligin.
 */
#define KERNEL_LOAD_ADDRESS_MASK	0x000fffff
#define KERNEL_LOAD_ADDRESS_ALIGIN	0x06000

/* 
 *MMU table size
 */
#define PAGE_TABLE_SIZE			(4096*4)

/*
 *kernel stack size is 4k
 */
#define COMMAND_LINE_SIZE		\
	(KERNEL_LOAD_ADDRESS_ALIGIN-PAGE_TABLE_SIZE-KERNEL_STACK_SIZE)

#endif
