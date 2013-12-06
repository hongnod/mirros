	.text

#include <syscall.h>

.global exit
exit:
	stmfd sp!, {r4, r7}
	ldr r7, = SYSCALL_EXIT_NR
	swi #0
	ldmfd sp!, {r4, r7}
	movs r0, r0
	bx lr

