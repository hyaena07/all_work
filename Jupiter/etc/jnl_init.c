#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include "ComLib.h"
#include "jplib.h"

char	errmsg[MAX_ERRO_LEN + 1];

int main(int argc, char *argv[])
{
	int ret;
	int opt;

	if(argc > 1)
		opt = atoi(argv[1]);
	else
		opt = 0;

	ret = jpjn_init_daily("./test_jnl");

	if(ret < 0)
		printf("%s\n", errmsg);

	exit(0);
}

