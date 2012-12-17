#include "lmfs.h"
#include "stdio.h"

char *buffer="lemin like study .";

int main(void)
{
	PLMFILE output;
	init_fmls();
	create_file("lemin",O_FILE);
	output=open_file("lemin",WRITE);
	printf("%d\n",write_file(buffer,17,output));
	close_file(output);
	while(1);
	return 0;
}





