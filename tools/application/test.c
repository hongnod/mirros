void delay(int a)
{
	while (a--);
}

int main(int argc, char **argv)
{
	int b;

	while (1) {
		b = debug(1, 2, 3, 4);
		debug(b, 0, 0, 0);
		delay(500000);
	}

	return 0;
}
