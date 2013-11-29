	.text

#include <syscall.h>

.global fork
fork:
	stmfd sp!, {r4, r7}
	ldr r7, = SYSCALL_FORK_NR
	swi #0
	ldmfd sp!, {r4, r7}
	movs r0, r0
	bx lr
