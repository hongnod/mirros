	.text
/*
 * r0 is the stack of SVC mode arg of the syscall, 
 * r7 is the syscall nr, we need to create
 * the stack frame for the syscall. in arm ACPS
 * fp is r11, ip is r12 the prototyep of this function
 * void *_syscall_handler(pt_regs *regs)
 */
.global syscall_table_start
syscall_table_start:	.word __syscall_table_start

.global syscall_table_end
syscall_table_end: .word __syscall_table_end

.global _syscall_handler
_syscall_handler:
	push {fp, lr}
	add fp, sp, #4
	sub sp, sp, #16

	str r4, [fp, #-8]

	ldr r3, [r4, #12]	@ arg3 the third arg for function
	str r3, [sp]

	mov r0, r4		@ arg0
	ldr r1, [r0]		@ arg1
	ldr r2, [r0, #4]	@ arg2
	ldr r3, [r0, #8]	@ arg3

	ldr r5, =syscall_table_base
	ldr r5, [r5]
	ldr r7, [r4, #28]	@ load r7 to get the syscall number
	add r5, r5, r7, lsl #2
	ldr r5, [r5]

	mov lr, pc
	mov pc, r5		@ goto syscall handler

	ldr r4, [fp, #-8]	@ will return here, return value is r0
	str r0, [r4]		@ store return valude to process context

	sub sp, fp, #4

	pop {fp, lr}
	bx lr
