#include "include/arm920t.h"

.macro set_mode, mode
msr cpsr_c, #\mode
.endm

	.text
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

	ldr r2, =current	@change page table.
	ldr r3, =next_run
	ldr r0, [r2]
	ldr r1, [r3]
	bl switch_task

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
