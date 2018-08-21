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
 * 설명 : 현재 PID를 파일에 저장한다.										*
 *		  (주의 : 같은 이름이 존재할 경우 덮어쓴다.)						*
 *																			*
 * 인수 : const void *fpath													*
 *			PID 파일명 (경로 포함 가능)										*
 *																			*
 * 리턴 : PID  - 성공														*
 *		  (-1) - 실패														*
 *																			*
 * 작성 : 2000.11.30.														*
 * 수정 : 2001.03.19.														*
 *			성공하면 '0'을 리턴하던 것을 PID를 리턴하도록 수정				*
 ****************************************************************************/
pid_t SavePID(const void *fpath)
{
	int		fd, pid_len;
	char	pid_str[10];
	pid_t	cur_pid;

	cur_pid = getpid();
	pid_len = sprintf(pid_str, "%d", cur_pid);

	/* 같은 이름의 파일이 존재하더라도 무시하고 덮어쓴다.					*/
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
 * 설명 : 현재 PID 파일을 삭제한다.											*
 *																			*
 * 인수 : const void *fpath													*
 *			PID 파일명 (경로 포함 가능)										*
 *																			*
 * 리턴 : ( 0) - 성공														*
 *		  (-1) - 실패														*
 *																			*
 * 작성 : 2000.12.01.														*
 * 수정 :																	*
 ****************************************************************************/
void DelePID(const void *fpath)
{
	/* 이 함수는 프로세스가 종료하기 직전에 호출되므로 에러 처리를 하지
	   않아도 괜찮을 것 같다.  												*/
	unlink(fpath);
	return;
}

/****************************************************************************
 * Prototype : pid_t ReadPID(const void *)									*
 *																			*
 * 설명 : 저장된 PID 파일로부터 PID를 가져온다.								*
 *																			*
 * 인수 : const void *fpath													*
 *			PID 파일명 (경로 포함 가능)										*
 *																			*
 * 리턴 : PID  - 성공														*
 *		  (-1) - 실패														*
 *																			*
 * 작성 : 2000.12.01.														*
 * 수정 :																	*
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

	/* 만일 atoi에서 에러가 발생하면 어쩌지? (설마~~) 						*/
	return((pid_t)atoi(pid_str));
}

/****************************************************************************
 * Prototype : pid_t RunProg(const char *, const char *)					*
 *																			*
 * 설명 : 프로그램을 실행하고 pid를 반환한다.								*
 *																			*
 * 인수 : const char *pname													*
 *			프로그램명 (경로가 포함되지 않을 경우 .profile의 PATH로 검색)	*
 *			const char *args												*
 *			문자열 형태의 인수												*
 *																			*
 * 리턴 : PID  - 성공														*
 *		  (-1) - 실패														*
 *																			*
 * 작성 : 2010.02.04.														*
 * 수정 : 2017.07.16.														*
 *			fork() 후 모든 시그럴 블럭을 해제하도록 수정					*
 ****************************************************************************/
pid_t RunProg(const char *pname, const char *args)
{
	pid_t		pid;
	sigset_t	pend, old_mask;

	pid = fork();
	switch(pid)  {
		case	0	:
			// 모든 시그널의 블럭을 해제한 후 실행한다.
			sigfillset(&pend);
			sigprocmask(SIG_UNBLOCK, &pend, &old_mask);
			// 여기까지 2017.07.26. 추가

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
 * 설명 : 프로그램을 실행하고 종료값을 반환한다.							*
 *																			*
 * 인수 : const char *pname													*
 *			프로그램명 (경로가 포함되지 않을 경우 .profile의 PATH로 검색)	*
 *			const char *args												*
 *			문자열 형태의 인수												*
 *																			*
 * 리턴 : 종료값 - 성공														*
 *		  < 0 - 실패														*
 *																			*
 * 작성 : 2017.10.20.														*
 * 수정 : 																	*
 ****************************************************************************/
pid_t RunProg_wait(const char *pname, const char *args)
{
	int			wait_status;
	pid_t		pid;
	sigset_t	pend, old_mask;

	pid = fork();
	switch(pid)  {
		case	0	:
			// 모든 시그널의 블럭을 해제한 후 실행한다.
			sigfillset(&pend);
			sigprocmask(SIG_UNBLOCK, &pend, &old_mask);
			// 여기까지 2017.07.26. 추가

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
			waitpid(pid, &wait_status, 0);		// 종료할 때까지 대기
			return(WEXITSTATUS(wait_status));
	}

	return(0);
}

/****************************************************************************
 * Prototype : void toBackground(void)										*
 *																			*
 * 설명 : 프로그램을 백그라운드로 전환하기.									*
 *																			*
 * 인수 : 																	*
 *																			*
 * 리턴 : 																	*
 *																			*
 * 작성 : 2010.02.04.														*
 * 수정 :																	*
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
