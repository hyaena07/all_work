#include <time.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <dirent.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "ComLib.h"

/****************************************************************************
 * Prototype : int LogRotate(const char *, int , int)						*
 *																			*
 * ���� : �α� ȭ���� ������ ũ�⺸�� ũ�� ��� �α׸� �����, ��� �α�	*
 *		  ȭ���� ���� ������ �� �ִ�.										*
 *																			*
 * �μ� : const char *fpath													*
 *			log ȭ�ϸ� (��� ���� ����)										*
 *		  int maxsize														*
 *			log file�� �ִ� ũ�� (������ KByte)								*
 *		  int nofl															*
 *			log file�� ����� ����											*
 *																			*
 * ���� : ( 0) - ����														*
 *		  (-1) - ����														*
 *																			*
 * �ۼ� : 2000.12.01.														*
 * ���� : 2000.12.29.														*
 *			�α� ȭ���� ó�� �ۼ��� ��� ���� ������ �߻��ϴµ�				*
 *			�̶� ���ϰ��� '0'�� �����־�� �Ѵ�.							*
 ****************************************************************************/
int LogRotate(const char *fpath, int maxsize, int nofl)
{
	int			fd, i;
	char		source[256], target[256];
	struct stat	filestat;

	/* �α�ȭ���� �ִ� ���� ������ 0���� �۰ų� ������ ����					*/
	if(maxsize <= 0)  {
		snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] �α�ȭ���� �ִ� ���� ������ 0���� �۰ų� ����", __FILE__, __LINE__);
#ifdef _LIB_DEBUG
		perror("[LogRotate]�α�ȭ���� �ִ� ���� ������ 0���� �۰ų� ����");
#endif

		return(-1);
	}

	if((fd = open(fpath, O_RDONLY)) <= 0)  {
		return(0);
	}

	if(fstat(fd, (struct stat *)&filestat) != 0)  {
		snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] fstat():%s", __FILE__, __LINE__, strerror(errno));
#ifdef _LIB_DEBUG
		perror("[LogRotate]fstat()");
#endif

		close(fd);
		return(-2);
	}
	close(fd);

	/* ȭ�� ���̰� �ִ� ���̺��� ũ�� ��� ȭ���� �����.					*/
	if(filestat.st_size > maxsize * 1024)  {
		/* ��� ȭ�� ���� 0�̸� ��� ȭ���� ������ �ʴ´�.					*/
		if(nofl <= 0)  {
			unlink(fpath);
		} else  {
			sprintf(target, "%s.%d", fpath, nofl);
			unlink(target);
			for(i = nofl; i > 1; i--)  {
				sprintf(target, "%s.%d", fpath, i);
				sprintf(source, "%s.%d", fpath, i - 1);
				rename(source, target);
			}
			sprintf(target, "%s.%d", fpath, i);
			rename(fpath, target);
		}
	}

	return(0);
}

/****************************************************************************
 * Prototype : int WriteLog(const void *, int, int,							*
 *							 const void *, va_list)							*
 *																			*
 * ���� : ������ ȭ�Ͽ� �α� �޼����� �����Ѵ�.								*
 *		  (���� : ���� �̸��� ������ ��� ������ �߰��Ѵ�.)					*
 *																			*
 * �μ� : const void *fpath													*
 *			log ȭ�ϸ� (��� ���� ����)										*
 *		  int maxsize														*
 *			log file�� �ִ� ũ�� (������ KByte)								*
 *			�� ���� '0'�̸� LogRotate�� ���� �ʴ´�.						*
 *		  int nofl															*
 *			log file�� ����� ����											*
 *			�� ���� '0'�̸� ��� ȭ���� ������ �ʴ´�.						*
 *		  const void *fmt													*
 *			log message�̰ų� printf������ ���� �����̴�.					*
 *		  va_list args														*
 *			�ڿ� �� argment��... (���� ���� ����)							*
 *																			*
 * ���� : ( 0) - ����														*
 *		  (-1) - ����														*
 *																			*
 * �ۼ� : 2000.12.01.														*
 * ���� : 2001.03.12.														*
 *			�������� �޼����� �ϳ��� String���θ� �޾Ҵµ� printf			*
 *			�������� �ް� ���ƴµ� ��ũ�� ���Ǹ� �� �� ��� ������.		*
 *		  2001.03.13.														*
 *			�Լ� ��ũ�θ� ���� �� ��� �Ʒ��� ���� �����ϰ� ������		*
 *			�Լ��� ����� ����ϱ�� �ߴ�.									*
 *																			*
 *			��)																*
 *			int ACC_LOG(char *fmt, ...)										*
 *			{																*
 *				int ret;													*
 *				va_list args;												*
 *																			*
 *				va_start(args, fmt);										*
 *				ret = WriteLog(FILE_ACC_LOG, MAX_LOGSIZE, \					*
 *							NUM_OF_LOG, fmt, args);							*
 *				va_end(args);												*
 *				return(ret);												*
 *			}																*
 ****************************************************************************/
int WriteLog(const void *fpath, int maxsize, int nofl, const void *fmt, va_list args)
{
	char	msg[1024];
	FILE	*fp;

	vsprintf(msg, fmt, args);

	/* �α� ȭ���� �ִ� ���̰� 0���� �����Ǹ� LogRotate�� ���� �ʴ´�.		*/
	if(maxsize > 0)  {
		if(LogRotate(fpath, maxsize, nofl) != 0)  {
			snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] LogRotate():%s", __FILE__, __LINE__, strerror(errno));
#ifdef _LIB_DEBUG
			printf("[WriteLog]LogRotate() fail.\n");
#endif

			return(-1);
		}
	}

	if((fp = fopen(fpath, "a")) == NULL)  {
		snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] fopen():%s", __FILE__, __LINE__, strerror(errno));
#ifdef _LIB_DEBUG
		perror("[WriteLog]fopen()");
#endif

		return(-2);
	}

	if(fseek(fp, SEEK_END, 0) == -1)  {
		snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] fseek():%s", __FILE__, __LINE__, strerror(errno));
#ifdef _LIB_DEBUG
		perror("[WriteLog]fseek()");
#endif

		fclose(fp);
		return(-3);
	}

	fprintf(fp, "[%s] %s\n", getTime_s(), msg);

	fclose(fp);

	return(0);
}

/****************************************************************************
 * Prototype : int WriteDailyLog(const char *, const char *,				*
 *								  const char *, ...)						*
 *																			*
 * ���� : ���ں� �α� �޼����� �����Ѵ�.									*
 *																			*
 * �μ� : const char *fpath													*
 *			log ȭ�� ��� - ������ ���ڰ� '/'�̸� ���丮 �̸��̵�����		*
 *			'/'�� �ƴϸ� ȭ�� �̸� �տ� �ٴ� ���λ簡 �ȴ�.					*
 *		  const char *extension												*
 *			log ȭ�ϸ��� Ȯ����												*
 *		  const char *fmt													*
 *			log message�̰ų� printf������ ���� �����̴�.					*
 *		  ...																*
 *			�ڿ� �� argment��... (���� ���� ����)							*
 *																			*
 * ���� : ( 0) - ����														*
 *		  (-1) - ����														*
 *																			*
 * �ۼ� : 2001.08.14.														*
 * ���� : 2001.08.31.														*
 *			fpath�� �������� '/'�� ������ �߰��ϴ� ��ƾ�� �����ߴµ�,		*
 *			�� ������ ȭ�� �̸��� �տ� ���λ縦 ÷���� �� �ְ� �ϱ� ����	*
 *			���� ���� �ִ�.													*
 *		  2001.11.15.														*
 *			��û�ϰԵ� va_end()�� ���Ծ �߰��ߴ�.						*
 *		  2003.06.17.														*
 *			log�� �ۼ��� ���丮�� �����ϴ��� �˻��ϰ�, ���� ��������		*
 *			�ʴ´ٸ� ���丮�� �����.										*
 ****************************************************************************/
int WriteDailyLog(const char *fpath, const char *extension, const char *fmt, ...)
{
	int		i, ret;
	char	tmp[255], dir_path[255];
	va_list	args;
	DIR		*dp;

	va_start(args, fmt);

	bzero(tmp, sizeof(tmp));
	strncpy(tmp, fpath, strlen(fpath));

	/* 2003.06.07 �߰� - ���丮�� �������� ���� ��� ���丮�� �����.	*/
	for(i = sizeof(tmp) - 1; tmp[i] != '/'; i--);
	bzero(dir_path, sizeof(dir_path));
	strncpy(dir_path, tmp, i + 1);

	if((dp = opendir(dir_path)) == (DIR *)NULL)  {
		if(mkdir(dir_path, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP) != 0)  {
			snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] mkdir():%s", __FILE__, __LINE__, strerror(errno));
#ifdef _LIB_DEBUG
			perror("[WriteDailyLog]mkdir()");
#endif

			return(-1);
		}
	} else
		closedir(dp);
	/* 2003.06.07 �߰� �� 													*/

	/* �α� ȭ���� ��θ� �����Ҷ� �������� '/'�� ������ �߰��Ѵ�.			*/
	/* 2001.08.31 ���� ����
	if(tmp[strlen(tmp) - 1] != '/')
		strcat(tmp, "/");
	2001.08.31 ���� ��														*/

	/* ������ ��ο� ����, Ȯ���ڸ� �̿��� ������ ȭ�� ��θ� �ۼ��Ѵ�.		*/
	sprintf(tmp, "%s%.8s.%s", tmp, getDateTime_14(), extension);

	ret = WriteLog(tmp, 0, 0, fmt, args);
	va_end(args);

	return(ret);
}
