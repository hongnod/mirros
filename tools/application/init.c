void delay(int a)
{
	while (a--);
}

int main(int argc, char **argv)
{
	int pid;

	pid = fork();
	if (pid == 0) {
		while (1) {
			debug(0, 0, 0, 0);
			delay(500000);
		}
	} else {
		while (1) {
			debug(1, 1, 1, 1);
			delay(50000);
		}
	}

	return 0;
}
