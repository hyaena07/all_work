#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <libgen.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "ComLib.h"
#include "jplib.h"

/****************************************************************************
 * Prototype : int jpsq_init(char *, int, int)								*
 *																			*
 * ���� : ������ ������ �ʱ�ȭ�Ѵ�.											*
 *		  ���϶��� Ȯ������ �ʱ� ������ �����ϰ� ����Ͽ�����.				*
 *																			*
 * �μ� : char *sq_path														*
 *			������ ������ ��ü ���											*
 *		  int sq_val														*
 *			������ �ʱⰪ													*
 *		  int opt															*
 *			JP_CREAT - ������ ������ �����Ѵ�.								*
 *																			*
 * ���� : ( < 0 ) ����														*
 *																			*
 * �ۼ� : 2011.12.28.														*
 * ���� :																	*
 ****************************************************************************/
int jpsq_init(char *sq_path, int sq_val, int opt)
{
	int		i, fd;
	int		flags;
	char	sq_str[80 + 1];
	char	sTemp[1024];

	bzero(sq_str, sizeof(sq_str));
	snprintf(sq_str, sizeof(sq_str) - 13, "Sequence %s : ", basename(sq_path));

	// ���� ���� �ɼǿ� ���� open()�� flags �μ��� �����Ѵ�.
	if(opt & JP_CREAT)
		flags = O_RDWR | O_CREAT;
	else
		flags = O_RDWR;

	// ������ ���� ����
	if((fd = open(sq_path, flags, 0600)) < 0)  {
		snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] open():%s", __FILE__, __LINE__, strerror(errno));
#ifdef _LIB_DEBUG
		perror("[jpsq_init]open()");
#endif

		return(-1);
	}

	// ���� ���� �ɼ��� ���� ��� �������� ������ �����Ѵ�.
	if(!(opt & JP_CREAT))  {
		bzero(sTemp, sizeof(sTemp));

		if(read(fd, sTemp, 80) < 0)  {
			snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] read():%s", __FILE__, __LINE__, strerror(errno));
#ifdef _LIB_DEBUG
			perror("[jpsq_init]read()");
#endif

			close(fd);
			return(-2);
		}

		if(memcmp(sTemp, sq_str, strlen(sq_str)) != 0)  {
			snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] �������� Ȯ�� ����. [%s]", __FILE__, __LINE__, sTemp);
#ifdef _LIB_DEBUG
			printf("[%s:%d] �������� Ȯ�� ����. [%s]\n", __FILE__, __LINE__, sTemp);
#endif

			close(fd);
			return(-3);
		}
	}

	// �������� ����
	sprintf(sq_str + strlen(sq_str), "%12d", sq_val);
	lseek(fd, 0, SEEK_SET);
		if(write(fd, sq_str, strlen(sq_str)) != strlen(sq_str))  {
		snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] write():%s", __FILE__, __LINE__, strerror(errno));
#ifdef _LIB_DEBUG
		perror("[jpsq_init]write()");
#endif
		close(fd);
		return(-4);
	}

	close(fd);
	return(0);
}

/****************************************************************************
 * Prototype : int jpsq_read(char *)										*
 *																			*
 * ���� : ������ ���� �б�� ���� 1 ����.									*
 *																			*
 * �μ� : char *sq_path														*
 *			������ ������ ��ü ���											*
 *		  int opt															*
 *			JP_NOWAIT - ������ ������̸� ���� �����Ѵ�.					*
 *																			*
 * ���� : (0 < ) ����. �� �ܴ� ��������.									*
 *																			*
 * �ۼ� : 2011.12.29.														*
 * ���� :																	*
 ****************************************************************************/
int jpsq_read(char *sq_path, int opt)
{
	int		i, fd, seq;
	char	sq_str[80 + 1];
	char	*seq_ptr = (char *)NULL;
	char	sTemp[1024];

	bzero(sq_str, sizeof(sq_str));
	snprintf(sq_str, sizeof(sq_str) - 13, "Sequence %s : ", basename(sq_path));

	if((fd = open(sq_path, O_RDWR)) < 0)  {
		snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] open():%s", __FILE__, __LINE__, strerror(errno));
#ifdef _LIB_DEBUG
		perror("[jpsq_open]open()");
#endif

		return(-1);
	}

	// ���϶��� �����Ѵ�. JP_NOWAIT �ɼ��� ��� ���� �����ϸ� �ٷ� ����.
	// ����� close() �� �� �ڵ����� �ǹǷ� ���� �� �ʿ����.
	while(1)  {
		if(lock_reg(fd, F_SETLK,  F_WRLCK, 0, SEEK_SET, 1) < 0)  {
			if(opt & JP_NOWAIT)  {
				snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] ������ ���� �̹� �����.", __FILE__, __LINE__);
#ifdef _LIB_DEBUG
				printf("[%s:%d] ������ ���� �̹� �����.\n", __FILE__, __LINE__);
#endif

				close(fd);
				return(-2);
			} else  {
				usleep(100000);
			}
		} else  {
			break;
		}
	}

	if(read(fd, sTemp, 80) < 0)  {
		snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] read():%s", __FILE__, __LINE__, strerror(errno));
#ifdef _LIB_DEBUG
		perror("[jpsq_read]read()");
#endif

		close(fd);
		return(-3);
	}
printf("--[%s]\n", sTemp);

	// �������� ����
	if(memcmp(sTemp, sq_str, strlen(sq_str)) != 0)  {
		snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] �������� Ȯ�� ����. [%s]", __FILE__, __LINE__, sTemp);
#ifdef _LIB_DEBUG
		printf("[%s:%d] �������� Ȯ�� ����. [%s]\n", __FILE__, __LINE__, sTemp);
#endif

		close(fd);
		return(-4);
	}

	// ���������� ���� ������ ���ϱ�
	for(i = 0; sTemp[i] != (char)0; i++)  {
		if(sTemp[i] == ':')  {
			seq_ptr = &sTemp[i + 2];
			break;
		}
	}

	if(seq_ptr == (char *)NULL)  {
		snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] ������ ���� ���� ����. [%s]", __FILE__, __LINE__, sTemp);
#ifdef _LIB_DEBUG
		printf("[%s:%d] ������ ���� ���� ����. [%s]\n", __FILE__, __LINE__, sTemp);
#endif

		close(fd);
		return(-5);
	}

	// ���� �������� ���ϱ�
	seq = atol(seq_ptr);

	// ������ �������� ����
	sprintf(sq_str + strlen(sq_str), "%12d", seq + 1);
printf("--[%s]\n", sTemp);
	lseek(fd, 0, SEEK_SET);
	if(write(fd, sq_str, strlen(sq_str)) != strlen(sq_str))  {
		snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] write():%s", __FILE__, __LINE__, strerror(errno));
#ifdef _LIB_DEBUG
		perror("[jpsq_read]write()");
#endif

		close(fd);
		return(-6);
	}

	close(fd);

	return(seq);
}
