int main(int argc, char **argv)
{
	int ret;

	ret = execve("test", 0, 0);

	while (1);
}
