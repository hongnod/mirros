	.text
#include "include/kconfig.h"
#include "include/arm920t.h"

.macro set_mode, mode
msr cpsr_c, #\mode
.endm

.global _vector_start	
_vector_start:		@exception vector
	ldr pc, _reset
	ldr pc, _undefined_instruction
	ldr pc, _software_interrupt
	ldr pc, _prefetch_abort
	ldr pc, _data_abort
	ldr pc, _not_used
	ldr pc, _irq
	ldr pc, _fiq

_reset:			.word _start
_undefined_instruction: .word undefined_instruction
_software_interrupt:    .word software_interrupt
_prefetch_abort:	.word prefetch_abort
_data_abort:		.word data_abort
_not_used:		.word not_used
_irq:			.word irq
_fiq:			.word fiq

.global _vector_end
_vector_end:

undefined_instruction:
	stmfd sp!,{r0-r3,lr}
	ldmfd sp!,{r0-r3,pc}^				@^ equle spsr->cpsr
	
	.align 5					@when swi interrput,cpu is in svc mode
software_interrupt:
	stmfd sp!, {r0-r12,lr}				@push the register to stack

	msr cpsr, #SYSTEM_MODE				@change to system mode to get the stack base of user space
	mov r0, sp
	msr cpsr, #SVC_MODE

	stmfd sp!, {r0}					@store the stack of user space.

	mrs r0, spsr					@store spsr
	stmfd sp!, {r0}			

	ldr r0, =KERNEL_BASE_ADDRESS 
	add r0, r0, r7, lsl #2
	ldr r1, [r0]
	mov lr, pc

	mov pc, r1					@goto syscall handler

	ldmfd sp!, {r0}
	msr spsr_cf, r0

	add sp, sp, #8

	ldmfd sp!, {r1-r12,pc}^				@return 

	.align 5
prefetch_abort:
	subs lr, lr, #4
	stmfd sp!, {r0-r1, lr}
	ldmfd sp!, {r0-r12,pc}^

	.align 5
data_abort:
	subs lr, lr, #8
	stmfd sp!, {r0-r12, lr}
	bl data_abort_handler
	ldmfd sp!, {r0-r12, pc}^
	
	.align 5
not_used:
	mov r0, r0

	.align 5
irq:
	sub lr, lr, #4
	push {lr}
	mrs lr, spsr					@store register in irq mode
	push {lr}

	set_mode SVC_MODE | NO_INT			@change to svc mode and store r0-r12 to stack
	sub sp, sp, #8
	push {r0-r12}
	
	set_mode SYSTEM_MODE | NO_INT		@change to system mode to get the stack base of user space and store to r1
	mov r1, sp
	set_mode IRQ_MODE | NO_INT			@change to irq mode and get lr and spsr
	pop {r2,r3}					@r1= user sp, r2=spsr,r3=lr
	
	set_mode  SVC_MODE | NO_INT			@push the three register to the stack of svc mode.
	push {r2}
	str r1,[sp, #56]
	str r3,[sp, #60]

	bl irq_handler

	set_mode IRQ_MODE | NO_INT
	bl os_exit_irq					@need copy the contex to irq stack from svc stack (process kernel stack)

	ldr r1, [sp, #56]				@restroe the sp point of the user space
	set_mode SYSTEM_MODE | NO_INT
	mov sp, r1				
	set_mode IRQ_MODE | NO_INT

	pop {r0}					@restore spsr
	msr spsr_csxf, r0				
	pop {r0-r12}					@restore r0-r12

	add sp ,sp, #4					@sp has benn restored,so add 4 to sp
	
	ldmfd sp!, {pc}^				@switch to another process or return to the current process

	.align 5
fiq:
	sub lr, lr, #4
	stmfd sp!, {lr}
	mrs r14, spsr					@store register in frq mode
	stmfd sp!, {r0-r4, r12, r14}

	ldr r12, =0				@get the address of fiq handler
	add r12, r12, r3, lsl #2
	msr cpsr_c, #0x1f				@got to the svc mode

	stmfd sp!, {r0-r4, lr}				@store the register of svc mode 

	mov lr, pc
	ldr pc, [r12]					@got to the fiq handler of each interrupt

	ldmfd sp!, {r0-r4, lr}				@restore the register of svc mode 

	msr cpsr_c, #0x52				@got to fiq mode
	ldmfd sp!, {r0-r4, r12, r14}
	msr spsr_csxf, r14
	ldmfd sp!, {pc}^				@return 
