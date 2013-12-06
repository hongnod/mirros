#ifndef _SIGNAL_H_
#define _SIGNAL_H_

typedef enum _signal_t {
	PROCESS_SIGNAL_KILL,
	PROCESS_SIGNAL_WAIT,
	PROCESS_SIGNAL_SUSPEND,
	PROCESS_SIGNAL_RESUME,
	MAX_SIGNAL
}signal_t;

struct signal {
	pid_t from;
	pid_t to;
	signal_t sig;
	void *message;
};

int sys_signal(pid_t pid, signal_t sig, void *arg);

#endif
