int main(int argc, char **argv)
{
	int ret;

	ret = execve("test", 0, 0);
	if (ret) {
		debug(2, 2, 3, 4);
	}

	while (1);
}
