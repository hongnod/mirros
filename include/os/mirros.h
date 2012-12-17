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

#endif
