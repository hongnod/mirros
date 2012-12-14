	.text
#include "include/kconfig.h"
#include "include/arm920t.h"

.macro set_mode, mode
msr cpsr_c, #\mode
.endm

.macro va_to_pa, reg, reg_tmp
#ifdef PAGE_OFFSET
	sub \reg, \reg, #PAGE_OFFSET
#else
	ldr \reg_tmp, page_offset
	sub \reg, \reg, \reg_tmp
#endif
.endm

.global _start	
_start:		@interrupt vector
	b reset

.global bss_start
bss_start:		.word _bss_start

.global start
start:			.word _start

.global bss_end
bss_end:		.word _bss_end

.global _platform_start
_platform_start:	.word __platform_start

/* these three var will be used in kernel
 * we need to count their value
 */
.global page_offset
page_offset:		.word 0

.global kernel_start
kernel_start:		.word 0

.global kernel_phy_start
kernel_phy_start:	.word 0

	.align 5	
reset:
	@mmu off, icache disable, d cache disable
	set_mode SVC_MODE | NO_INT
	mrc p15, 0, r0, c0, c0

	@whether the load address are kernel_load_address-ALIGIN
	adr r0, _start
	mov r2, r0
	sub r2, r2,#KERNEL_LOAD_ADDRESS_ALIGIN
	adr r1, kernel_phy_start
	str r2, [r1]
	ldr r1, =KERNEL_LOAD_ADDRESS_MASK
	and r0, r0, r1
	cmp r0, #KERNEL_LOAD_ADDRESS_ALIGIN
	bne boot_error

	adr r1, page_offset	@store page_offset size 
	adr r0, _start
	ldr r2, start
	mov r3, r2
	sub r3, r3, #KERNEL_LOAD_ADDRESS_ALIGIN
	adr r4, kernel_start
	str r3, [r4]
	sub r0, r2, r0
	str r0, [r1]

	bl look_up_proc		@r1:return value, r0:arch information
	cmp r0, #0
	bne boot_error

	bl low_level_init

clear_bss:
	ldr r2, bss_start
	va_to_pa r2, r3
1:
	mov r0, #0			@clear the bss section
	ldr r1, bss_end
	va_to_pa r1, r3
2:	
	cmp r2, r1
	strcc r0, [r2], #4
	bcc 2b

init_memory_banks:
	bl init_mm_bank

	bl init_kernel_stack

init_mmu:
	mrc p15,0,r0,c1,c0,0		@disable d cache
	bic r0, r0, #0x04
	mcr p15, 0, r0, c1, c0, 0

	mrc p15, 0, r0, c1, c0, 0	@diable i cache
	bic r0, r0, #(1<<12)
	mcr p15, 0, r0, c1, c0, 0

	mov r1, #0			@clean invalidate dcache index
3:
	mov r2, #0
4:
	mov r3, r1, lsl #26
	mov r4, r2, lsl #5
	orr r0, r3, r4
	mcr p15, 0, r0, c7, c14, 2

	add r2, r2, #1
	cmp r2, #8
	bls 4b

	add r1, r1, #1
	cmp r1, #64
	bls 3b
	
	mcr p15, 0, r0, c7, c5, 0	@invalidate id cache

	mrc p15, 0, r0, c1, c0, 0	@disable mmu
	bic r0, r0, #0x01
	mcr p15, 0, r0, c1, c0, 0

	mcr p15, 0, r0, c8, c7, 0	@invalidate tlb

	ldr r5, bss_end
	sub r5, r5, #KERNEL_BASE_ADDRESS
	add r5, r5, #0x100000
	sub r5, r5, #1
	ldr r6, =0xfffff
	and r2, r5, r6			@how many memory we need remap when booting
	lsr r2, r2, #20

	ldr r0, =KERNEL_BASE_ADDRESS		@c0000000--->30000000
	mov r1, r0
	va_to_pa r1, r7
	ldr r3, =(MST_WB_AP_SRW_UN | DOMAIN0)
	bl set_mmu_table
	
	ldr r0, = KERNEL_BASE_ADDRESS		@30000000--->30000000
	va_to_pa r0, r7
	mov r1, r0
	ldr r3, =(MST_WB_AP_SRW_UN | DOMAIN0)
	bl set_mmu_table

	/*temp for uart0 debug*/
	ldr r0, = 0x48000000			@48000000--->48000000
	ldr r1, = 0x48000000
	mov r2, #0x200
	ldr r3, =(MST_NCNB_AP_SRW_URW | DOMAIN0)
	bl set_mmu_table

	ldr r0, =KERNEL_BASE_ADDRESS
	va_to_pa r0, r1
	mcr p15, 0, r0, c2, c0, 0

	ldr r0, domain_data		@set mmu domain
	mcr p15, 0, r0, c3, c0, 0

	mov r0, #0			@set process id
	mcr p15, 0, r0, c13, c0, 0

	mrc p15, 0, r0, c1, c0, 0	@enable align fault
	orr r0, r0, #2
	mcr p15, 0, r0, c1, c0, 0

	mrc p15, 0, r0, c1, c0, 0	@enable mmu
	orr r0, r0, #1
	mcr p15, 0, r0, c1, c0, 0

	mrc p15, 0, r0, c1, c0, 0	@enable i cache
	orr r0, r0, #(1<<12)
	mcr p15, 0, r0, c1, c0, 0

	mrc p15, 0, r0, c1, c0, 0	@enable d cache
	orr r0, r0, #4
	mcr p15, 0, r0, c1, c0, 0	

	ldr pc, _start_kernel
	.align 5

_start_kernel:	.word main
domain_data:	.word (0x55555550 | 0<<2 | 1<<0)

boot_error:
	mov r0, #0
	b boot_error

	.align 5

set_mmu_table:				@r0=vaddr r1=paddr r2=size_m r3=mem_attr
	ldr r4, =KERNEL_BASE_ADDRESS
	va_to_pa r4, r7
	mov r5, #0

	lsr r0, r0, #20
	lsl r0, r0, #2			@4b for a 1M page
	add r4, r4, r0

6:
	mov r6, r1
	lsr r6, r6, #20			@fill the page table
	add r6, r6, r5
	lsl r6, r6, #20
	orr r6, r6, r3
	
	str r6, [r4], #4		@write phy address to the page table

	add r5, r5, #1
	cmp r5, r2
	bls 6b	
	mov pc, lr

.global look_up_proc
look_up_proc:
	ldr r1, _platform_start
	va_to_pa r1, r2
	mov r0, #0
	ldr r2, [r1]

	cmp r2, #0x88
	str r2, [r1]
	beq 7f
	mov r0, #1
7:
	mov pc, lr

.global init_stack
init_kernel_stack:
	ldr r0, =KERNEL_BASE_ADDRESS			@stack_base
	add r0, r0, #KERNEL_LOAD_ADDRESS_ALIGIN
	mov sp, r0

	mov pc, lr
