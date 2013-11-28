#ifndef _SYSCALL_H
#define _SYSCALL_H

#define SYSCALL_NR	1024

struct syscall {
	int nr;
	unsigned long *addr;
};

#define DEFINE_SYSCALL(name, n, syscall_addr)	\
static struct syscall __syscall_##name __attribute__((section(".__syscall"))) = {	\
	.nr = n,	\
	.addr = syscall_addr	\
}

#define SYSCALL_BASE			0
#define SYSCALL_FORK_NR			(SYSCALL_BASE + 1)
#define SYSCALL_EXECVE_NR		(SYSCALL_BASE + 2)

#endif
