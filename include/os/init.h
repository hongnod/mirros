#ifndef _INIT_H
#define _INIT_H

#define INIT_PATH_SIZE		128

#define CMDLINE_TAG		"xxoo"
#define ARCH_NAME_SIZE		8
#define BOARD_NAME_SIZE		16

#define MEM_MAX_REGION		16

#include <os/mm.h>

typedef int (*init_call)(void);

struct cmdline {
	unsigned long tag;
	unsigned long head;
	char arg[256];
};

struct platform_info {
	unsigned long machine;
	char arch[ARCH_NAME_SIZE];
	char board_name[BOARD_NAME_SIZE];

	struct memory_region region[MEM_MAX_REGION];
	void (*parse_memory_region)(struct platform_info *info);
	int region_nr;
};

struct platform_info *get_platform(void);

#define PLATFORM_INFO_START(name)	\
	static struct platform_info name __attribute__((section(".platform_info"))) = {

#define PLATFORM_INFO_END(name)		\
	};
/*
#define arch_init(fn)	\
	static init_call arch_init_##fn __attribute__((section(".arch_init"))) = fn

#define platform_init(fn)	\
	static init_call platform_init_##fn __attribute__((section(".platform_init"))) = fn
*/
#endif
