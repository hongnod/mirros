	.text

/*
 *_sys_fork(void)
 */
_sys_fork:
	push {fp,lr}
	add fp, sp, #4
	sub sp, sp, #56

	ldr r0, [fp, #8]		@sp
	str r0, [fp, #-8]
	str r0, [fp, #-24]
	
	ldr r0, [fp, #4]		@cpsr
	str r0, [fp, #-12]	
	mov r0, #0			@pc
	str r0, [fp, #-16]
	ldr r0, [fp, #64]		@lr
	str r0, [fp, #-20]
	add r0, fp, #28			@r4-r12
	mov r10, sp
	ldm r0, {r1-r9}
	stm r10, {r1-r9}
	sub r4, fp, #12			@r0-r3
	ldm r4, {r0-r3}

	bl sys_fork

	sub sp, fp, #4
	pop {fp,lr}
	mov pc, lr			@return
