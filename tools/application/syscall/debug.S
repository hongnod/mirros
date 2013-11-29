	.text

#include <syscall.h>
.global debug
debug:
	stmfd sp!, {r4, r7}
	ldr r7, = 0
	swi #0
	ldmfd sp!, {r4, r7}
	movs r0, r0
	bx lr


