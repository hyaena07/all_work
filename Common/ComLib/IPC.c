#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "ComLib.h"

/*********************** 16 bit CRC TABLE for TOKEN ***************************/
static unsigned short s_crctab[256] = {
    0x0000,  0x1021,  0x2042,  0x3063,  0x4084,  0x50a5,  0x60c6,  0x70e7,
    0xef1f,  0xff3e,  0xcf5d,  0xdf7c,  0xaf9b,  0xbfba,  0x8fd9,  0x9ff8,
    0x9339,  0x8318,  0xb37b,  0xa35a,  0xd3bd,  0xc39c,  0xf3ff,  0xe3de,
    0xb5ea,  0xa5cb,  0x95a8,  0x8589,  0xf56e,  0xe54f,  0xd52c,  0xc50d,
    0x5844,  0x4865,  0x7806,  0x6827,  0x18c0,  0x08e1,  0x3882,  0x28a3,
    0x48c4,  0x58e5,  0x6886,  0x78a7,  0x0840,  0x1861,  0x2802,  0x3823,
    0xd94c,  0xc96d,  0xf90e,  0xe92f,  0x99c8,  0x89e9,  0xb98a,  0xa9ab,
    0x2462,  0x3443,  0x0420,  0x1401,  0x64e6,  0x74c7,  0x44a4,  0x5485,
    0xcb7d,  0xdb5c,  0xeb3f,  0xfb1e,  0x8bf9,  0x9bd8,  0xabbb,  0xbb9a,
    0x3653,  0x2672,  0x1611,  0x0630,  0x76d7,  0x66f6,  0x5695,  0x46b4,
    0xc9cc,  0xd9ed,  0xe98e,  0xf9af,  0x8948,  0x9969,  0xa90a,  0xb92b,
    0xa7db,  0xb7fa,  0x8799,  0x97b8,  0xe75f,  0xf77e,  0xc71d,  0xd73c,
    0xdbfd,  0xcbdc,  0xfbbf,  0xeb9e,  0x9b79,  0x8b58,  0xbb3b,  0xab1a,
    0x4a75,  0x5a54,  0x6a37,  0x7a16,  0x0af1,  0x1ad0,  0x2ab3,  0x3a92,
    0x26d3,  0x36f2,  0x0691,  0x16b0,  0x6657,  0x7676,  0x4615,  0x5634,
    0xa56a,  0xb54b,  0x8528,  0x9509,  0xe5ee,  0xf5cf,  0xc5ac,  0xd58d,
    0xedae,  0xfd8f,  0xcdec,  0xddcd,  0xad2a,  0xbd0b,  0x8d68,  0x9d49,
    0xff9f,  0xefbe,  0xdfdd,  0xcffc,  0xbf1b,  0xaf3a,  0x9f59,  0x8f78,
    0x1080,  0x00a1,  0x30c2,  0x20e3,  0x5004,  0x4025,  0x7046,  0x6067,
    0x9188,  0x81a9,  0xb1ca,  0xa1eb,  0xd10c,  0xc12d,  0xf14e,  0xe16f,
    0x7e97,  0x6eb6,  0x5ed5,  0x4ef4,  0x3e13,  0x2e32,  0x1e51,  0x0e70,
    0x7c26,  0x6c07,  0x5c64,  0x4c45,  0x3ca2,  0x2c83,  0x1ce0,  0x0cc1,
    0x1231,  0x0210,  0x3273,  0x2252,  0x52b5,  0x4294,  0x72f7,  0x62d6,
    0xfd2e,  0xed0f,  0xdd6c,  0xcd4d,  0xbdaa,  0xad8b,  0x9de8,  0x8dc9,
    0x6e17,  0x7e36,  0x4e55,  0x5e74,  0x2e93,  0x3eb2,  0x0ed1,  0x1ef0,
    0x8108,  0x9129,  0xa14a,  0xb16b,  0xc18c,  0xd1ad,  0xe1ce,  0xf1ef,
    0x02b1,  0x1290,  0x22f3,  0x32d2,  0x4235,  0x5214,  0x6277,  0x7256,
    0x83b9,  0x9398,  0xa3fb,  0xb3da,  0xc33d,  0xd31c,  0xe37f,  0xf35e,
    0xb75b,  0xa77a,  0x9719,  0x8738,  0xf7df,  0xe7fe,  0xd79d,  0xc7bc,
    0x6ca6,  0x7c87,  0x4ce4,  0x5cc5,  0x2c22,  0x3c03,  0x0c60,  0x1c41,
    0x5af5,  0x4ad4,  0x7ab7,  0x6a96,  0x1a71,  0x0a50,  0x3a33,  0x2a12,
    0x34e2,  0x24c3,  0x14a0,  0x0481,  0x7466,  0x6447,  0x5424,  0x4405
};

#define CRC(data, accum) ((accum>>8)^s_crctab[(accum^(data&0x00ff))&0x00ff])


/****************************************************************************
 * Prototype : int GetToken(char *, int)									*
 *																			*
 * 설명 : 입력된 IPC NAME 및 TYPE에 의하여 유일한 IPC-ID를 산출한다.		*
 *																			*
 * 인수 : char *ipc_name													*
 *			IPC 이름														*
 *		  int ipc_type														*
 *			 IPC 종류														*
 *																			*
 * 리턴 : (!= -1) IPC Key													*
 *		  (== -1) 실패														*
 *																			*
 * 작성 : 2017.08.31. (RMS 소스에서 차용 혹은 도용)							*
 * 수정 :																	*
 ****************************************************************************/
 int GetToken(char *ipc_name, char ipc_type)
{
	unsigned char    w_cc;
	int  w_crc,  w_token;
	register int ii;

	switch(ipc_type)  {
		case 'Q': break;				/* message queue */
		case 'M': break;				/* shared memory */
		case 'S': break;				/* semaphore     */
		default:
			snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] IPC type error.[%c]", __FILE__, __LINE__, ipc_type);
#ifdef _LIB_DEBUG
			printf("[%s:%d] IPC type error.[%c]", __FILE__, __LINE__, ipc_type);
#endif
			return (-1);
	}

	w_crc = 0;
	for (ii = 0; ii < strlen(ipc_name); ii++)  {
		w_cc  = ipc_name[ii];
		w_crc = CRC(w_cc, w_crc);
	}

	w_token = (ipc_type << 16) | w_crc;
	return (w_token);
}

/****************************************************************************
 * Prototype : int CreatMsq(char *, int flag)								*
 *																			*
 * 설명 : Message Queue를 만들고 식별을 위한 id를 돌려준다.					*
 *																			*
 * 인수 : char *ipc_name													*
 *			IPC 이름														*
 *		  int flag															*
 *			 Message Queue 생성에 필요한 flag들								*
 *																			*
 * 리턴 : (!= -1) Message Queue의 식별자 (id)								*
 *		  (== -1) 실패														*
 *																			*
 * 작성 : 2001.08.03. (삼성서울병원 프로젝트 중)							*
 * 수정 : 2017.08.31. GetToken() 사용하도록 수정							*
 ****************************************************************************/
int CreatMsq(char *ipc_name, int flag)
{
	int		msq_id;
	key_t	msq_key;

	if((msq_key = GetToken(ipc_name, 'Q')) < 0)  {
		return(-1);
	}

	if((msq_id = msgget(msq_key, flag)) < 0)  {
		snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] msgget():%s", __FILE__, __LINE__, strerror(errno));
#ifdef _LIB_DEBUG
		perror("[CreatMsq]msgget");
#endif

		return(-2);
	}

	return(msq_id);
}

int CreatMsq_(char *fpath, char proj, int flag)
{
	int		msq_id;
	key_t	msq_key;

	if((msq_key = ftok(fpath, proj)) == -1)  {
		snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] ftok():%s", __FILE__, __LINE__, strerror(errno));
#ifdef _LIB_DEBUG
		perror("[CreatMsq]ftok");
#endif

		return(-1);
	}

	if((msq_id = msgget(msq_key, flag)) < 0)  {
		snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] msgget():%s", __FILE__, __LINE__, strerror(errno));
#ifdef _LIB_DEBUG
		perror("[CreatMsq]msgget");
#endif

		return(-2);
	}

	return(msq_id);
}

/****************************************************************************
 * Prototype : int ClearMsq(int, long)										*
 *																			*
 * 설명 : Message Queue에 있는 모든 Message를 제거한다. (이런 무식한 방법	*
 *		  말고 더 산뜻한 방법을 찾아보자.)									*
 *																			*
 * 인수 : int msq_id														*
 *			Message Queue 식별자 (id)										*
 *		  long msg_type														*
 *			제거하고자하는 message type. 모든 message를 제거할 경우 '0'을	*
 *			사용한다.														*
 *																			*
 * 리턴 : (!= -1) 제거한 Message의 수										*
 *		  (== -1) 실패														*
 *																			*
 * 작성 : 2001.08.03. (삼성서울병원 프로젝트 중)							*
 * 수정 :																	*
 ****************************************************************************/
int ClearMsq(int msq_id, long msg_type)
{
	int		i = 0;
	char	buf[21];

	while(1)  {
		if(msgrcv(msq_id, buf, sizeof(long) + 1, msg_type, IPC_NOWAIT | MSG_NOERROR) < 0)  {
			if(errno == ENOMSG)
				return(i);
			else  {
				snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] msgrcv():%s", __FILE__, __LINE__, strerror(errno));
#ifdef _LIB_DEBUG
				perror("[ClearMsq]msgrcv");
#endif

				return(-1);
			}
		}

		i++;
	}
}

/****************************************************************************
 * Prototype : int CreatShm(char *, int size, int flag)						*
 *																			*
 * 설명 : Shared memory를 만들고 식별을 위한 id를 돌려준다.					*
 *																			*
 * 인수 : char *ipc_name													*
 *			IPC 이름														*
 *		  int size															*
 *			기억장소 조각(segment)의 크기									*
 *		  int flag															*
 *			Shared memory 생성에 필요한 flag들								*
 *																			*
 * 리턴 : (!= -1) Shared memory의 식별자 (id)								*
 *		  (== -1) 실패														*
 *																			*
 * 작성 : 2001.12.19.														*
 * 수정 : 2017.08.31. GetToken() 사용하도록 수정							*
 ****************************************************************************/
int CreatShm(char *ipc_name, int size, int flag)
{
	int		shm_id;
	key_t	shm_key;

	if((shm_key = GetToken(ipc_name, 'M')) < 0)  {
		return(-1);
	}

	if((shm_id = shmget(shm_key, size, flag)) < 0)  {
		snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] shmget():%s", __FILE__, __LINE__, strerror(errno));
#ifdef _LIB_DEBUG
		perror("[CreatShm]shmget");
#endif

		return(-2);
	}

	return(shm_id);
}

int CreatShm_(char *fpath, char proj, int size, int flag)
{
	int		shm_id;
	key_t	shm_key;

	if((shm_key = ftok(fpath, proj)) == -1)  {
		snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] ftok():%s", __FILE__, __LINE__, strerror(errno));
#ifdef _LIB_DEBUG
		perror("[CreatShm]ftok");
#endif

		return(-1);
	}

	if((shm_id = shmget(shm_key, size, flag)) < 0)  {
		snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] shmget():%s", __FILE__, __LINE__, strerror(errno));
#ifdef _LIB_DEBUG
		perror("[CreatShm]shmget");
#endif

		return(-2);
	}

	return(shm_id);
}

/****************************************************************************
 * Prototype : int Signal(int, void *)										*
 *																			*
 * 설명 : signal handler를 설정한다.										*
 *		  기존 함수 signal()을 대체하는 것으로 한 번만 설정하면 handler가	*
 *		  계속 유지되는 장점이 있다.										*
 *																			*
 * 인수 : int signo															*
 *			signal number													*
 *		  void *func														*
 *			handler function address										*
 *																			*
 * 리턴 : ( 0) - 성공														*
 *		  (-1) - 실패														*
 *																			*
 * 작성 : 2001.08.31.														*
 * 수정 :																	*
 ****************************************************************************/
int Signal(int signo, void *func)
{
	struct sigaction	act, oact;

	act.sa_handler = (void *)func;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;

	if(signo == SIGALRM)  {
#ifdef SA_INTERRUPT
		/* Sun OS */
		act.sa_flags &= ~ SA_INTERRUPT;
#endif
	} else  {
#ifdef SA_RESTART
		/* SVR4, 4.3+BSD */
		act.sa_flags |= SA_RESTART;
#endif
	}

	return(sigaction(signo, &act, &oact));
}

/****************************************************************************
 * Prototype : int SendFIFO(char *, char *, int, int, int)					*
 *																			*
 * 설명 : FIFO를 이용해 프로세스에 데이터 전송								*
 *																			*
 * 인수 : char *fifo_path													*
 *			FIFO 파일 경로													*
 *		  char *sbuf														*
 *			전송할 버퍼														*
 *		  int slen															*
 *			전송할 길이														*
 *		  int mode															*
 *			FF_BLOCK : 블로킹모드											*
 *		  int wsec															*
 *			블로킹모드가 아닐 경우 timeout 초 값							*
 *																			*
 * 리턴 : (== n) - 성공														*
 *		  (!= n) - 실패														*
 *		  ( < 0) - 에러 발생												*
 *																			*
 * 작성 : 2001.08.31.														*
 * 수정 :																	*
 ****************************************************************************/
int SendFIFO(char *fifo_path, char *sbuf, int slen, int mode, int wsec)
{
	int				fd, ret;
	fd_set			fds;
	struct timeval	tv, *ptr_tv;

	ret = mkfifo(fifo_path, S_IRWXU);
	if(ret < 0 && errno != EEXIST)  {
		snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] mkfifo():%s", __FILE__, __LINE__, strerror(errno));
#ifdef _LIB_DEBUG
		perror("[SendFIFO]mkfifo");
#endif

		return(-1);
	}

	if((fd = open(fifo_path, O_APPEND | O_NDELAY)) < 0)  {
		snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] open():%s", __FILE__, __LINE__, strerror(errno));
#ifdef _LIB_DEBUG
		perror("[SendFIFO]open");
#endif

		return(-2);
	}

	FD_ZERO(&fds);
	FD_SET(fd, &fds);

	if(mode & FF_BLOCK)  {
		ptr_tv		 = (struct timeval *)NULL;
	} else  {
		tv.tv_sec	= wsec;
		tv.tv_usec	= 0;
		ptr_tv		= &tv;
	}

	if((ret = select(fd + 1, NULL, &fds, NULL, ptr_tv)) < 0)  {
		snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] select():%s", __FILE__, __LINE__, strerror(errno));
#ifdef _LIB_DEBUG
		perror("[SendFIFO]select");
#endif
		close(fd);
		return(-3);
	}

	if(ret == 0)  {
		close(fd);
		return(0);
	}

	ret = write(fd, sbuf, slen);
	if(ret < 0 || ret != slen)  {
		snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] write():%s", __FILE__, __LINE__, strerror(errno));
#ifdef _LIB_DEBUG
		perror("[SendFIFO]write");
#endif

		close(fd);
		return(-4);
	}

	close(fd);
	return(slen);
}

/****************************************************************************
 * Prototype : int SemOpr(char *, int, int, int)							*
 *																			*
 * 설명 : Semaphore Lock or unLock											*
 *		  다음과 같이 정의하여 사용하면 편하겠지?							*
 *																			*
 *		#define	JPS_NAM		"JPCOMD"										*
 *		#define	JPS_MAX		250												*
 *																			*
 *		#define	JPS_LOCK(semno)		SemOpr(JPS_NAM, JPS_MAX, semno, 1)		*
 *		#define	JPS_UNLOCK(semno)	SemOpr(JPS_NAM, JPS_MAX, semno, 2)		*
 *		#define	JPS_CHECK(semno)	SemOpr(JPS_NAM, JPS_MAX, semno, 3)		*
 *																			*
 * 인수 : char *prjnm														*
 *			프로젝트명														*
 *		  int nsems															*
 *			Semaphore 집합의 수  (커널 매개변수 SEMMSL 보다 크면 안되겠지?) *
 *		  int semno															*
 *			Semaphore 집합의 번호 ('0'부터 시작하며 SEMMSL 보다 작아야함)	*
 *		  int flag															*
 *			1 : Lock														*
 *			2 : unLock														*
 *			3 : Check														*
 *																			*
 * 리턴 : ( < 0) - 에러 발생												*
 *																			*
 * 작성 : 2017.09.22.														*
 * 수정 :																	*
 ****************************************************************************/
int SemOpr(char *prjnm, int nsems, int semno, int flag)
{
	int		rc = 0;
	int		sem_id;
	key_t	sem_key;

	static struct sembuf op_lock[2] = {
	   0, 0, 0,								// wait for sem#0 to become 0
	   0, 1, SEM_UNDO						// then increment sem#0 by 1
	};

	static struct sembuf op_unlock[1] = {
	   0, -1, (IPC_NOWAIT|SEM_UNDO)			// decrement sem#0 by 1 (sets it to 0)
	};

	union semun {
		int val;                    // SETVAL을 위한값
		struct semid_ds *buf;       // IPC_STAT, IPC_SET을 위한 버퍼
		unsigned short int *array;  // GETALL, SETALL을 위한 배열
		struct seminfo *__buf;      // IPC_INFO을 위한 버퍼
	} semarg;

	if(semno >= nsems)  {
		snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] 세마포어 집합의 수를 초과하여 지정. MAX:%d", __FILE__, __LINE__, nsems);
#ifdef _LIB_DEBUG
		printf("[%s:%d] 세마포어 집합의 수를 초과하여 지정. MAX:%d", __FILE__, __LINE__, nsems);
#endif
		return(-2);
	}

	if((sem_key = GetToken(prjnm, 'S')) < 0)  {
		return(-3);
	}

	sem_id = semget(sem_key, nsems, IPC_CREAT|0600);
	if(sem_id < 0)  {
		snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] semget():%s", __FILE__, __LINE__, strerror(errno));
#ifdef _LIB_DEBUG
		perror("[SemOpr]semget");
#endif
		return (-4);
	}

	op_lock[0].sem_num = op_lock[1].sem_num = semno;
	op_unlock[0].sem_num = semno;

	if(flag == 1)	{			// Lock
		if(semop(sem_id, &op_lock[0], 2) < 0) {
			snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] semop():%s", __FILE__, __LINE__, strerror(errno));
#ifdef _LIB_DEBUG
			perror("[SemOpr]semop");
#endif
			return(-5);
		}
	} else if(flag == 2)  {		// unLock
		if(semop(sem_id, &op_unlock[0], 1) < 0)  {
			snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] semop():%s", __FILE__, __LINE__, strerror(errno));
#ifdef _LIB_DEBUG
			perror("[SemOpr]semop");
#endif
			return(-6);
		}
	} else if(flag == 3)  {		// Check
		if((rc = semctl(sem_id, semno, GETVAL, semarg)) < 0)  {
			snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] semctl():%s", __FILE__, __LINE__, strerror(errno));
#ifdef _LIB_DEBUG
			perror("[SemOpr]semctl");
#endif
			return(-7);
		}
	} else  {
		snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] 부적합한 flag (%d)", __FILE__, __LINE__, flag);
#ifdef _LIB_DEBUG
		printf("[%s:%d] 부적합한 flag (%d)", __FILE__, __LINE__, flag);
#endif
		return(-8);
	}

	return(rc);
}
