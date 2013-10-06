#ifndef _MIRROS_H
#define _MIRROS_H

#include <config/config.h>

#ifdef HOST_TEST
	#define MM_SECTION_ALIGIN	4
#else 
	#define MM_SECTION_ALIGIN	(SIZE_1M)
#endif

#ifndef HOST_TEST
	#define KERNEL_BASE_ADDRESS 	0x80000000
#endif

#define PROCESS_USER_STACK_BASE		0x80000000
#define PROCESS_USER_EXEC_BASE		0x00401000
/*
 * Ww resever 4k for other useage, such as task
 * argv.
 */
#define PROCESS_USER_BASE		0X00400000

#endif
