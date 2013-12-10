	.text

.global __start
__start:
	bl main
	mov r0, r0
	b exit
	nop
