#ifndef _MUTEX_H
#define _MUTEX_H

#include <os/list.h>

struct mutex{
	int count;
	struct list_head wait;
};

#define DECLARE_MUTEX(name)	\
	struct mutex name = {	\
		.count = 0,	\
		.wait = {	\
			.pre = &wait,	\
			.next = &wait,	\
		}			\
	}

static void inline init_mutex(struct mutex *m)
{
	m->count = 0;
	m->wait.pre = &m->wait;
	m->wait.next = &m->wait;
}

void mutex_lock(struct mutex *m);
void mutex_unlock(struct mutex *m);

#endif
