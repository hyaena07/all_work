#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <libgen.h>

#include "ComLib.h"
#include "jplib.h"

char	errmsg[MAX_ERRO_LEN + 1];

int main(int argc, char *argv[])
{
	int ret;
	int seq;
	char buf[1024];

	if(argc != 2)  {
		printf("--> %s seq\n", basename(argv[0]));
		exit(1);
	} else
		seq = atoi(argv[1]);

	ret = jpjn_read("./test_jnl", seq, buf);

	if(ret < 0)
		printf("[%d][%s]\n", ret, errmsg);
	else if(ret == 0)
		printf("---준비된 데이터 없음.\n");
	else
		printf("---[%.*s]\n", ret, buf);

	exit(0);
}

