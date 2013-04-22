/*
*basic opreation for arm920 MMU
*/
#include "include/arm920t.h"

.macro set_mode, mode
msr cpsr_c, #\mode
.endm

	.text

.global get_cpu_id
get_cpu_id:
	mrc p15,0,r0,c0,c0,0
	mov pc, lr
			
.global get_cache_id
get_cache_id:
	mrc p15,0,r0,c0,c0,1
	mov pc, lr 
			
.global disable_mmu
disable_mmu:
	mrc p15,0,r0,c1,c0,0
	bic r0,r0,#0x01
	mcr p15,0,r0,c1,c0,0
	mov pc, lr
			
.global enable_mmu
enable_mmu:
	mrc p15,0,r0,c1,c0,0
	orr r0,r0,#1
	mcr p15,0,r0,c1,c0,0
	mov pc,lr
	mov pc, lr

.global disable_align
disable_align:
	mrc p15,0,r0,c1,c0,0
	bic r0,r0,#0x02
	mcr p15,0,r0,c1,c0,0
	mov pc, lr
			
.global enable_align
enable_align:
	mrc p15,0,r0,c1,c0,0
	orr r0,r0,#0x02
	mcr p15,0,r0,c1,c0,0
	mov pc, lr
			
.global disable_dcache
disable_cache:
	mrc p15,0,r0,c1,c0,0
	bic r0,r0,#0x04
	mcr p15,0,r0,c1,c0,0
	mov pc, lr
			
.global enable_dcache
enable_cache:
	mrc p15,0,r0,c1,c0,0
	orr r0,r0,#0x04
	mcr p15,0,r0,c1,c0,0
	mov pc, lr
			
			
.global disable_wbuffer
disable_buffer:
	mrc p15,0,r0,c1,c0,0
	bic r0,r0,#0x08
	mcr p15,0,r0,c1,c0,0
	mov pc, lr
			
.global enable_wbuffer
enable_wbuffer:
	mrc p15,0,r0,c1,c0,0
	orr r0,r0,#0x08
	mcr p15,0,r0,c1,c0,0
	mov pc, lr
			
.global select_little_endian
select_little_endian:
	mrc p15,0,r0,c1,c0,0
	bic r0,r0,#80
	mcr p15,0,r0,c1,c0,0
	mov pc, lr
			
.global select_big_endian
select_big_endian:
	mrc p15,0,r0,c1,c0,0
	orr r0,r0,#0x80
	mcr p15,0,r0,c1,c0,0
	mov pc, lr
			
.global disable_icache
disable_icache:
	mrc p15,0,r0,c1,c0,0
	bic r0,r0,#(1<<12)
	mcr p15,0,r0,c1,c0,0
	mov pc, lr
			
.global enable_icache
enable_icache:
	mrc p15,0,r0,c1,c0,0
	orr r0,r0,#(1<<12)
	mcr p15,0,r0,c1,c0,0
	mov pc, lr
						
.global set_fast_bus_mode
set_fast_bus_mode:
	mrc p15,0,r0,c1,c0,0
	bic r0,r0,#((1<<30)|(1<<31))
	mcr p15,0,r0,c1,c0,0
	mov pc, lr


.global set_async_bus_mode
set_async_bus_mode:
	mrc p15,0,r0,c1,c0,0
	orr r0,r0,#((1<<30)|(1<<31))
	mcr p15,0,r0,c1,c0,0
	mov pc, lr
  			
.global set_tlb_base
set_tlb_base:
	mcr p15,0,r0,c2,c0,0
	mov pc, lr
			
.global set_domain
set_domain:
	mcr p15,0,r0,c3,c0,0
	mov pc, lr
/*
.global invalidate_all_tlb
invalidate_all_tlb:
	push {r0}
	mov r0,#0
	mcr p15,0,r0,c8,c7,0
	nop
	nop
	pop {r0}
	mov pc, lr
	
.global invalidate_id_cache
invalidate_id_cache:
	push {r0}
	mcr p15,0,r0,c7,c7,0
	pop {r0}
	mov pc, lr
*/

.global invalidate_icache
invalidate_icache:
	mcr p15,0,r0,c7,c5,0
	mov pc, lr

.global invalidate_icache_mva
invalidate_icache_mva:
	mcr p15,0,r0,c7,c5,1
	mov pc, lr

.global prefetch_icache_mva
prefetch_icache_mva:
	mcr p15,0,r0,c7,c13,1
	mov pc, lr

.global invalidate_dcache
invalidate_dcache:
	mcr p15,0,r0,c7,c6,0
	mov pc, lr

.global invalidate_dcache_mva
invalidate_dcache_mva:
	mcr p15,0,r0,c7,c6,1
	mov pc, lr

.global clean_dcache_mva
clean_dcache_mva:
	mcr p15,0,r0,c7,c10,1
	mov pc, lr

.global clean_invalidate_dcache_mva
clean_invalidate_dcache_mva:
	mcr p15,0,r0,c7,c14,1
	mov pc, lr

.global clean_dcache_index
clean_dcache_index:
	mcr p15,0,r0,c7,c10,2
	mov pc, lr

.global clean_invalidate_dcache_index
clean_invalidate_dcache_index:
	mcr p15,0,r0,c7,c14,2
	mov pc, lr

.global wait_for_interrupt
wait_for_interrupt:
	mcr p15,0,r0,c7,c0,4
	mov pc, lr
   			 
.global clean_wb
clean_wb:
	mov r0,#0
	mcr p15,0,r0,c7,c10,4
	mov pc, lr

.global invalidate_tlb
invalidate_tlb:
	mcr p15,0,r0,c8,c7,0
	mov pc, lr

.global invalidate_itlb
invalidate_itlb:
	mcr p15,0,r0,c8,c5,0
	mov pc, lr

.global invalidate_itlb_mva
invalidate_itlb_mva:
	mcr p15,0,r0,c8,c5,1
	mov pc, lr

.global invalidate_dtlb
invalidate_dtlb:
	mcr p15,0,r0,c8,c6,0
	mov pc, lr

.global invalidate_dtlb_mva
invalidate_dtlb_mva:
	mcr p15,0,r0,c8,c6,1
	mov pc, lr

.global set_dcache_lockdown_base
set_dcache_lockdown_base:
	mcr p15,0,r0,c9,c0,0
	mov pc, lr


.global set_icache_lockdown_base
set_icache_lockdown_base:
	mcr p15,0,r0,c9,c0,1
	mov pc, lr

.global set_dtlb_lockdown
set_dtlb_lockdown:
	mcr p15,0,r0,c10,c0,0
	mov pc, lr

.global set_itlb_lockdown
set_itlb_lockdown:
	mcr p15,0,r0,c10,c0,1
	mov pc, lr

.global set_process_id
set_process_id:
	mcr p15,0,r0,c13,c0,0
	mov pc, lr
	
/*
 *code for schedule
 */
.global arch_switch_task_sw
arch_switch_task_sw:
	push {lr}		@frist we must restore the process context.
	sub sp, sp, #12		@sp
	push {r0-r12}		@r0-r12
	mrs r0, cpsr
	push {r0}		@cpsr or spsr

	mrs r1, cpsr
	msr cpsr, #SYSTEM_MODE | NO_INT
	mov r0, sp
	msr cpsr, r1
	str r0, [sp, #64]	@sp of user space

	mrs r0, spsr
	str r0, [sp, #56]	@spsr
	str lr, [sp, #60]	@lr

	ldr r0, =current
	ldr r1, =next_run

	ldr r2, [r0]		@store sp to current process.
	str sp, [r2]

	ldr r2, [r1]
	str r2, [r0]		@current = next_run

	ldr sp, [r2]		@load sp of next_run.

	ldr r0, [sp, #56]	@user sp
	msr cpsr_c, #SYSTEM_MODE | NO_INT
	mov sp, r0
	msr cpsr_c, #SVC_MODE | NO_INT

	pop {r0}		@cpsr
	msr cpsr, r0

	ldr r0, [sp, #52]	@spsr
	msr spsr, r0

	ldr r0, [sp, #56]	@lr
	mov lr, r0

	pop {r0-r12}		@r0-r12

	add sp, sp, #12		@lr
	
	ldmfd sp!, {pc}

.global arch_switch_task_hw
arch_switch_task_hw:
	mov pc, lr

/* r0 = &current
 * r1 = &next_run
 * r2 = current
 * r3 = next_run
 * we are in irq mode
 */
.global os_exit_irq
os_exit_irq:
	cmp r2, r3
	beq out

	set_mode SVC_MODE | NO_INT
	str sp, [r2]			@save current stack to current process
	set_mode IRQ_MODE | NO_INT

	mov r0, r2
	mov r1, r3
	push {lr}
	bl switch_task
	pop {lr}

	ldr r0, = current
	ldr r1, = next_run
	ldr r2, [r0]
	ldr r3, [r1]
	
	str r3, [r0]			@current = next_run	
	ldr r0, [r3]			@r0 = stack of next task
	mov pc, lr
out:
	set_mode SVC_MODE | NO_INT
	mov r0, sp
	set_mode IRQ_MODE | NO_INT
	mov pc,lr
