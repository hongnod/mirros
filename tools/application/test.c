void delay(int a)
{
	while (a--);
}

int main(int argc, char **argv)
{
	int pid = 0;
	int i = 0;
#if 0
	while (1) {
		debug(0, 0, 0, 0);
		delay(5000000);
	}
#else
	pid = fork();
	if (pid == 0) {
		while (1) {
			debug(0, 0, 0, 0);
			delay(500000);
		}
	} else {
		for (i = 0; i < 5; i ++) {
			debug(1, 1, 1, 1);
			delay(50000);
		}
	}
#endif
	return 0;
}
