#include <os/types.h>
#include <os/init.h>
#include <os/mm.h>
#include <os/slab.h>
#include <os/bound.h>
#include <os/mirros.h>

struct platform_info *platform_info = NULL;

int init_platform_info(void)
{
	extern unsigned long _platform_start;

	platform_info = (struct platform_info *)_platform_start;

	return 0;
}

inline struct platform_info *get_platform(void)
{
	return platform_info;
}

