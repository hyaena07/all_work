#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "ComLib.h"
#include "jplib.h"

char	errmsg[MAX_ERRO_LEN + 1];

int main(int argc, char *argv[])
{
	int	rc, jpec_handle;
	struct jpec_data ecdt;

	jpec_handle = jpec_open(atoi(argv[1]));
	if(jpec_handle < 0)  {
		printf("jpec_open() fail. [%d]\n", jpec_handle);
	}

	rc = jpec_recv(jpec_handle, (char *)&ecdt);
	if(rc < 0)  {
		printf("jpec_subp : jpec_recv() fail.[%d]\n", rc);
		jpec_close(jpec_handle);
		exit(0);
	} else  {
		printf("jpec_subp : [%d][%.*s]\n", rc, ecdt.jpec_i_len, ecdt.jpec_i_buf);
	}

//sleep(10 + 1);
	ecdt.jpec_o_len = 10;
	memcpy(ecdt.jpec_o_buf, "9876543210", 10);

	rc = jpec_send(jpec_handle, (char *)&ecdt);
	if(rc < 0)  {
		printf("jpec_subp : jpec_send() fail.[%d]\n", rc);
		jpec_close(jpec_handle);
		exit(0);
	}

	jpec_close(jpec_handle);

	exit(1);
}
