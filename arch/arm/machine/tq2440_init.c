#include <os/init.h>
#include <os/mm.h>

void tq2440_parse_memory_region(struct platform_info *info)
{
	register_memory_region(0x30000000, 64*1024*1024, NORMAL_MEM,info);
	register_memory_region(0x48000000, 0x5b000020-0x48000000, IO_MEM,info);
}

PLATFORM_INFO_START(tq2440)
	.machine = 0x88,
	.arch = "arm920t",
	.board_name = "tq2440",
	.parse_memory_region = tq2440_parse_memory_region,
PLATFORM_INFO_END(tq2440)
