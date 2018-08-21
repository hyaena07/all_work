#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "ComLib.h"

/****************************************************************************
 * Prototype : pid_t SavePID(const void *)									*
 *																			*
 * ���� : ���� PID�� ���Ͽ� �����Ѵ�.										*
 *		  (���� : ���� �̸��� ������ ��� �����.)						*
 *																			*
 * �μ� : const void *fpath													*
 *			PID ���ϸ� (��� ���� ����)										*
 *																			*
 * ���� : PID  - ����														*
 *		  (-1) - ����														*
 *																			*
 * �ۼ� : 2000.11.30.														*
 * ���� : 2001.03.19.														*
 *			�����ϸ� '0'�� �����ϴ� ���� PID�� �����ϵ��� ����				*
 ****************************************************************************/
pid_t SavePID(const void *fpath)
{
	int		fd, pid_len;
	char	pid_str[10];
	pid_t	cur_pid;

	cur_pid = getpid();
	pid_len = sprintf(pid_str, "%d", cur_pid);

	/* ���� �̸��� ������ �����ϴ��� �����ϰ� �����.					*/
	if((fd = creat(fpath, 0600)) < 0)  {
		snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] creat():%s", __FILE__, __LINE__, strerror(errno));
#ifdef _LIB_DEBUG
		perror("[SavePID]creat()");
#endif

		return(-1);
	}

	if((write(fd, pid_str, pid_len)) != pid_len)  {
		snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] write():%s", __FILE__, __LINE__, strerror(errno));
#ifdef _LIB_DEBUG
		perror("[SavePID]write()");
#endif

		close(fd);
		unlink(fpath);
		return(-2);
	}

	close(fd);
	return(cur_pid);
}

/****************************************************************************
 * Prototype : void DelePID(const void *)									*
 *																			*
 * ���� : ���� PID ������ �����Ѵ�.											*
 *																			*
 * �μ� : const void *fpath													*
 *			PID ���ϸ� (��� ���� ����)										*
 *																			*
 * ���� : ( 0) - ����														*
 *		  (-1) - ����														*
 *																			*
 * �ۼ� : 2000.12.01.														*
 * ���� :																	*
 ****************************************************************************/
void DelePID(const void *fpath)
{
	/* �� �Լ��� ���μ����� �����ϱ� ������ ȣ��ǹǷ� ���� ó���� ����
	   �ʾƵ� ������ �� ����.  												*/
	unlink(fpath);
	return;
}

/****************************************************************************
 * Prototype : pid_t ReadPID(const void *)									*
 *																			*
 * ���� : ����� PID ���Ϸκ��� PID�� �����´�.								*
 *																			*
 * �μ� : const void *fpath													*
 *			PID ���ϸ� (��� ���� ����)										*
 *																			*
 * ���� : PID  - ����														*
 *		  (-1) - ����														*
 *																			*
 * �ۼ� : 2000.12.01.														*
 * ���� :																	*
 ****************************************************************************/
pid_t ReadPID(const void *fpath)
{
	int		fd, cur_pid;
	char	pid_str[10];

	if((fd = open(fpath, O_RDONLY)) < 0)  {
		snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] open():%s", __FILE__, __LINE__, strerror(errno));
#ifdef _LIB_DEBUG
		perror("[ReadPID]open()");
#endif

		return(-1);
	}

	bzero(pid_str, 10);
	if(read(fd, pid_str, 9) < 0)  {
		snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] read():%s", __FILE__, __LINE__, strerror(errno));
#ifdef _LIB_DEBUG
		perror("[ReadPID]read()");
#endif

		close(fd);
		return(-2);
	}

	close(fd);

	/* ���� atoi���� ������ �߻��ϸ� ��¼��? (����~~) 						*/
	return((pid_t)atoi(pid_str));
}

/****************************************************************************
 * Prototype : pid_t RunProg(const char *, const char *)					*
 *																			*
 * ���� : ���α׷��� �����ϰ� pid�� ��ȯ�Ѵ�.								*
 *																			*
 * �μ� : const char *pname													*
 *			���α׷��� (��ΰ� ���Ե��� ���� ��� .profile�� PATH�� �˻�)	*
 *			const char *args												*
 *			���ڿ� ������ �μ�												*
 *																			*
 * ���� : PID  - ����														*
 *		  (-1) - ����														*
 *																			*
 * �ۼ� : 2010.02.04.														*
 * ���� : 2017.07.16.														*
 *			fork() �� ��� �ñ׷� ���� �����ϵ��� ����					*
 ****************************************************************************/
pid_t RunProg(const char *pname, const char *args)
{
	pid_t		pid;
	sigset_t	pend, old_mask;

	pid = fork();
	switch(pid)  {
		case	0	:
			// ��� �ñ׳��� ���� ������ �� �����Ѵ�.
			sigfillset(&pend);
			sigprocmask(SIG_UNBLOCK, &pend, &old_mask);
			// ������� 2017.07.26. �߰�

			if(execlp(pname, pname, args, (char *)NULL) < 0)	{
				snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] execlp():%s", __FILE__, __LINE__, strerror(errno));
#ifdef _LIB_DEBUG
				perror("[RunProg]execlp()");
#endif
				return(-2);
			}
			break;
		case	-1	:
			snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] fork():%s", __FILE__, __LINE__, strerror(errno));
#ifdef _LIB_DEBUG
			perror("[RunProg]fork()");
#endif
			return(-1);
		default		:
			return(pid);
	}

	return(0);
}

/****************************************************************************
 * Prototype : pid_t RunProg_wait(const char *, const char *)				*
 *																			*
 * ���� : ���α׷��� �����ϰ� ���ᰪ�� ��ȯ�Ѵ�.							*
 *																			*
 * �μ� : const char *pname													*
 *			���α׷��� (��ΰ� ���Ե��� ���� ��� .profile�� PATH�� �˻�)	*
 *			const char *args												*
 *			���ڿ� ������ �μ�												*
 *																			*
 * ���� : ���ᰪ - ����														*
 *		  < 0 - ����														*
 *																			*
 * �ۼ� : 2017.10.20.														*
 * ���� : 																	*
 ****************************************************************************/
pid_t RunProg_wait(const char *pname, const char *args)
{
	int			wait_status;
	pid_t		pid;
	sigset_t	pend, old_mask;

	pid = fork();
	switch(pid)  {
		case	0	:
			// ��� �ñ׳��� ���� ������ �� �����Ѵ�.
			sigfillset(&pend);
			sigprocmask(SIG_UNBLOCK, &pend, &old_mask);
			// ������� 2017.07.26. �߰�

			if(execlp(pname, pname, args, (char *)NULL) < 0)	{
				snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] execlp():%s", __FILE__, __LINE__, strerror(errno));
#ifdef _LIB_DEBUG
				perror("[RunProg]execlp()");
#endif
				return(-2);
			}
			break;
		case	-1	:
			snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] fork():%s", __FILE__, __LINE__, strerror(errno));
#ifdef _LIB_DEBUG
			perror("[RunProg]fork()");
#endif
			return(-1);
		default		:
			waitpid(pid, &wait_status, 0);		// ������ ������ ���
			return(WEXITSTATUS(wait_status));
	}

	return(0);
}

/****************************************************************************
 * Prototype : void toBackground(void)										*
 *																			*
 * ���� : ���α׷��� ��׶���� ��ȯ�ϱ�.									*
 *																			*
 * �μ� : 																	*
 *																			*
 * ���� : 																	*
 *																			*
 * �ۼ� : 2010.02.04.														*
 * ���� :																	*
 ****************************************************************************/
void toBackground(void)
{
	if(fork() > 0)
		exit(1);

//	setpgrp();
//	close(0);
//	close(1);
//	close(2);
//	setsid();

//	if(fork() > 0)
//		exit(1);
}
