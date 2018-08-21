#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>

#include "ComLib.h"
#include "jplib.h"

char	errmsg[MAX_ERRO_LEN + 1];

int main(int argc, char *argv[])
{
	int ret;

	if(argc != 2)  {
		printf("--> %s journal_data\n", basename(argv[0]));
		exit(1);
	}

	ret = jpjn_write("./test_jnl", argv[1], strlen(argv[1]));

	if(ret < 0)
		printf("[%d][%s]\n", ret, errmsg);
	else
		printf("---[%d]\n", ret);

	exit(0);
}

