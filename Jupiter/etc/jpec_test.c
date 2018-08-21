#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "ComLib.h"
#include "jplib.h"

char	errmsg[MAX_ERRO_LEN + 1];

int main(int argc, char *argv[])
{
	int		rc;
	struct	jpec_data jd;

	memcpy(jd.jpec_name, "jpec_subp", 9);
	jd.jpec_i_len = 10;
	memcpy(jd.jpec_i_buf, "0123456789", 10);

	rc = jpec_call(&jd, 10);
	if(rc < 0)  {
		printf("jpec_tset : jpec_call() fail.[%d][%s]\n", rc, errmsg);
		exit(0);
	} else  {
		printf("jpec_test : [%d][%.*s]\n", rc, jd.jpec_o_len, jd.jpec_o_buf);
	}

	exit(0);
}
