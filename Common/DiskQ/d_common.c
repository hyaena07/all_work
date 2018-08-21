#include "DiskQ.h"

/****************************************************************************
 * Prototype : int GetQHeader(char *, struct d_msqid_ds *, int)				*
 *																			*
 * 설명 : Control File이 존재하는지 검사하고, Message Queue의 Queue 헤더	*
 *		  정보를 가져온다. d_msgget()에서 호출할 경우 Control File에대한	*
 *		  open()이 선행됨에 주의하여야한다.									*
 *																			*
 * 인수 : char *cpath														*
 *			Message Queue의 경로명											*
 *		  struct d_msqid_ds *info											*
 *			헤더 정보를 저장할 구조체 포인터								*
 *		  int qid															*
 *			Queue ID로서 이 값이 '0'이 아니면 이미 열려있는 Queue로			*
 *			판단하여 파일을 열지 않고 파일이 실제로 존제하는지 확인한다.	*
 *																			*
 * 리턴 : ( >  0) Control File의 fd											*
 *		  (== -1) 실패														*
 *																			*
 * 작성 : 2001.12.19.														*
 * 수정 :																	*
 ****************************************************************************/
int GetQHeader(char *cpath, struct d_msqid_ds *info, int qid)
{
	int fd;
	char fpath[QPATH_LEN + QNAME_LEN + 1];

	if(qid == 0)  {
		if((fd = open(cpath, O_RDWR)) < 0)  {
#ifdef _LIB_DEBUG
			if(errno != ENOENT)
				perror("[GetQHeader]open");
#endif
			return(-1);
		}
	} else  {
		sprintf(fpath, "%s/%s.%s", D_msqinfo[qid - 1].d_msg_fpath, D_msqinfo[qid - 1].d_msg_qname, CTL_EXP);
		if(access(fpath, R_OK | W_OK) < 0)  {
#ifdef _LIB_DEBUG
			perror("[GetQHeader]access");
#endif
			return(-2);
		}
		fd = D_msqinfo[qid - 1].d_msg_cfd;
	}

	if(lseek(fd, 0, SEEK_SET) < 0)  {
#ifdef _LIB_DEBUG
		perror("[GetQHeader]lseek");
#endif
		close(fd);
		return(-3);
	}

	if(read(fd, info, QHEAD_SZ) != QHEAD_SZ)  {
#ifdef _LIB_DEBUG
		perror("[GetQHeader]read");
#endif
		close(fd);
		return(-4);
	}

	if(qid != 0)  {
		memcpy(&D_msqinfo[qid - 1].d_msg_perm, &info->d_msg_perm, sizeof(struct ipc_perm));
		D_msqinfo[qid - 1].d_msg_qbytes = info->d_msg_qbytes;
	}

	return(fd);
}

/****************************************************************************
 * Prototype : int CheckDataFile(char *, int, unsigned long)				*
 *																			*
 * 설명 : Data File의 논리적 에러에 대해 검사한다. d_msgget()에서 최초로	*
 *		  호출할 경우 Data File에대한 open()이 선행됨에 주의하여야한다.		*
 *																			*
 * 인수 : char *dpath														*
 *			Data File의 경로명 (fd != 0) 일때만 유효						*
 *		  int fd															*
 *			Data File의 fd로서 이 값이 '0'이 아니면 File open을 하지		*
 *			않으므로 dpath의 값이 사용되지 않는다.							*
 *		  unsigned long size												*
 *			검사 대상으로서 Queue의 최대 사용량 만큼의 File 공간이 확보		*
 *			되어있는지를 검사할때 사용한다.									*
 *																			*
 * 리턴 : ( >  0) Data File의 fd											*
 *		  (== -1) 실패														*
 *																			*
 * 작성 : 2001.12.19.														*
 * 수정 :																	*
 ****************************************************************************/
int CheckDataFile(char *dpath, int fd, unsigned long size)
{
	struct stat buf;

	if(fd == 0)  {
		if((fd = open(dpath, O_RDWR)) < 0)  {
#ifdef _LIB_DEBUG
			perror("[CheckDataFile]open");
#endif
			return(-1);
		}
	}

	if(fstat(fd, &buf) < 0)  {
#ifdef _LIB_DEBUG
		perror("[CheckDataFile]fstat");
#endif
		close(fd);
		return(-2);
	}

	if(buf.st_size != (off_t)size)  {
#ifdef _LIB_DEBUG
		printf("[CheckDataFile]Data File의 크기가 맞지 않습니다.\n");
#endif
		close(fd);
		return(-3);
	}

	return(fd);
}

/****************************************************************************
 * Prototype : int CheckPerm(struct ipc_perm, int)							*
 *																			*
 * 설명 : Queue에대한 사용자의 접근 권한이 있는지를 검사한다.				*
 *																			*
 * 인수 : struct ipc_perm ip												*
 *			Queue의 접근 권한 설정 상태										*
 *		  int chklist														*
 *			접근 권한 검사 항목 ('|' 연산으로 중복 지정 가능)				*
 *			0            : 읽기 쓰기 중 하나라도 있으면 성공				*
 *			RCV_PERM_CHK : 읽기 권한이 있으면 성공							*
 *			SND_PERM_CHK : 쓰기 권한이 있으면 성공							*
 *																			*
 * 리턴 : (==  0) 성공														*
 *		  (== -1) 실패														*
 *																			*
 * 작성 : 2001.12.20.														*
 * 수정 :																	*
 ****************************************************************************/
int CheckPerm(struct ipc_perm ip, int chklist)
{
	int rcv_res = 0, snd_res = 0;
	int des = 2;  /* own(0), group(1), other(2) 구분 변수 */
	uid_t euid, egid;

	euid = geteuid();
	egid = getegid();

	if(euid == ROOT_UID || egid == ROOT_GID)  {
		/* effactive UID가 root이거나 GID가 root이면 무조건 권한이 있는것으로 판단 */
		return(0);
	}

#if 0
	if(euid == ip.uid)  {
		/* effactive UID가 소유자와 같다면 무조건 권한을 줄 것인가....... */
		return(0);
	}
#endif

	if(egid == ip.gid)
		des = 1;

	if(euid == ip.uid)
		des = 0;

	/* 읽기 권한에 대한 검사 */
	if((chklist & RCV_PERM_CHK) || !chklist)  {
		switch(des)  {
			case 0 : /* owner */
				if(ip.mode & S_IRUSR)
					rcv_res = 1;
				break;
			case 1 : /* group */
				if(ip.mode & S_IRGRP)
					rcv_res = 1;
				break;
			case 2 : /* other */
				if(ip.mode & S_IROTH)
				rcv_res = 1;
				break;
		}
	}

	/* 쓰기 권한에 대한 검사 */
	if((chklist & SND_PERM_CHK) || !chklist)  {
		switch(des)  {
			case 0 : /* owner */
				if(ip.mode & S_IWUSR)
					snd_res = 1;
				break;
			case 1 : /* group */
				if(ip.mode & S_IWGRP)
					snd_res = 1;
				break;
			case 2 : /* other */
				if(ip.mode & S_IWOTH)
					snd_res = 1;
				break;
		}
	}

	if((!chklist && (rcv_res || snd_res)) ||		/* chklist가 '0'이고, 읽기, 쓰기 중 하나 이상에 권한이 있을때 */
		((chklist & RCV_PERM_CHK) && rcv_res) ||	/* 읽기 권한 검사를 요청 받았으며, 읽기 권한이 있을때 */
		((chklist & SND_PERM_CHK) && snd_res) ||	/* 쓰기 권한 검사를 요청 받았으며, 쓰기 권한이 있을때 */
		((chklist & RCV_PERM_CHK & SND_PERM_CHK) && (rcv_res && snd_res)))
													/* 읽기,쓰기 권한 검사를 요청 받았으며, 읽기 쓰기 권한이 있을때 */
		return(0);

	return(-1);
}

/****************************************************************************
 * Prototype : char *mkfifoname(char *)										*
 *																			*
 * 설명 : 임시로 사용될 FIFO의 이름을 rand()를 이용하여 만들고 같은 이름이	*
 *		  존재하는지 검사하여 존재하지 않는 이름을 만들때까지 반복한다.		*
 *																			*
 * 인수 : char *path														*
 *			FIFO 파일을 생성할 디렉토리 경로								*
 *																			*
 * 리턴 : FIFO 이름															*
 *																			*
 * 작성 : 2002.01.09.														*
 * 수정 :																	*
 ****************************************************************************/
char *mkfifoname(char *path)
{
	int i;
/* for LINUX
    char fpath[QPATH_LEN + FIFONAME_LEN + strlen(FIFO_EXP) + 3];
*/
	char fpath[QPATH_LEN + FIFONAME_LEN + 4 + 3];
	static char name[FIFONAME_LEN + 1];

	while(1)  {
		for(i = 0; i < FIFONAME_LEN; i++)
			name[i] = 'a' + (int)((float)('z'-'a') * rand() / (RAND_MAX + 1.0));

		name[FIFONAME_LEN] = (char)NULL;

		sprintf(fpath, "%s/%s.%s", path, name, FIFO_EXP);

		if(access(fpath, F_OK) != 0)
			return(name);
	}
}

/****************************************************************************
 * Prototype : int ReadMHead(int, int, struct d_msghd_ds *)					*
 *																			*
 * 설명 : 지정한 Control File에서 지정한 하나의 Message header를 읽어온다.	*
 *																			*
 * 인수 : int cfd															*
 *			Control File fd													*
 *		  int mid															*
 *			Message 지정 번호												*
 *		  struct d_msghd_ds *mh												*
 *			Message header 저장 포인터										*
 *																			*
 * 리턴 : (==  0) 성공														*
 *		  (== -1) 실패														*
 *																			*
 * 작성 : 2002.01.17.														*
 * 수정 :																	*
 ****************************************************************************/
int ReadMHead(int cfd, int mid, struct d_msghd_ds *mh)
{
	if(lseek(cfd, QHEAD_SZ + (MHEAD_SZ * mid), SEEK_SET) < 0)  {
#ifdef _LIB_DEBUG
		perror("[ReadMHead]lseek");
#endif
		return(-1);
	}

	if(read(cfd, mh, MHEAD_SZ) != MHEAD_SZ)  {
#ifdef _LIB_DEBUG
		perror("[ReadMHead]read");
#endif
		return(-2);
	}

	return(0);
}

/****************************************************************************
 * Prototype : int WriteMHead(int, int, struct d_msghd_ds *)				*
 *																			*
 * 설명 : 지정한 Control File에서 지정한 하나의 Message header 정보를		*
 *		  갱신한다.															*
 *																			*
 * 인수 : int cfd															*
 *			Control File fd													*
 *		  int mid															*
 *			Message 지정 번호												*
 *		  struct d_msghd_ds *mh												*
 *			Message header 저장 포인터										*
 *																			*
 * 리턴 : (==  0) 성공														*
 *		  (== -1) 실패														*
 *																			*
 * 작성 : 2002.01.17.														*
 * 수정 :																	*
 ****************************************************************************/
int WriteMHead(int cfd, int mid, struct d_msghd_ds *mh)
{
	if(lseek(cfd, QHEAD_SZ + (MHEAD_SZ * mid), SEEK_SET) < 0)  {
#ifdef _LIB_DEBUG
		perror("[WriteMHead]lseek");
#endif
		return(-1);
	}

	if(write(cfd, mh, MHEAD_SZ) != MHEAD_SZ)  {
#ifdef _LIB_DEBUG
		perror("[WriteMHead]write");
#endif
		return(-2);
	}

	return(0);
}

/****************************************************************************
 * Prototype : int d_tcntset(int, double)									*
 *																			*
 * 설명 : 지정한 cfd를 Control File의 fd로 가정하여 Total Count를 cnt로		*
 *		  설정한다.															*
 *																			*
 * 인수 : int cfd															*
 *			Control File fd													*
 *		  int cnt															*
 *			설정할 Total Count												*
 *																			*
 * 리턴 : (==  0) 성공														*
 *		  (== -1) 실패														*
 *																			*
 * 작성 : 2002.02.25.														*
 * 수정 :																	*
 ****************************************************************************/
int d_tcntset(int cfd, double cnt)
{
	struct d_msqid_ds qh;

	if(lseek(cfd, 0, SEEK_SET) < 0)  {
#ifdef _LIB_DEBUG
		perror("[d_tcntset]lseek(1)");
#endif
		return(-1);
	}

	if(read(cfd, &qh, QHEAD_SZ) != QHEAD_SZ)  {
#ifdef _LIB_DEBUG
		perror("[d_tcntset]read");
#endif
		return(-2);
	}

	qh.d_msg_tcnt = cnt;

	if(lseek(cfd, 0, SEEK_SET) < 0)  {
#ifdef _LIB_DEBUG
		perror("[d_tcntset]lseek(2)");
#endif
		return(-3);
	}

	if(write(cfd, &qh, QHEAD_SZ) != QHEAD_SZ)  {
#ifdef _LIB_DEBUG
		perror("[d_tcntset]write");
#endif
		return(-4);
	}

	return(0);
}

/****************************************************************************
 * Prototype : int d_msgview(int, int, int, double, unsigned int, int)		*
 *																			*
 * 설명 : 지정한 cfd를 Control File의 fd로 가정하여 mid와 uid로 지정된		*
 *		   메세지를 읽어 텍스트 혹은 바이너리 형태로 화면에 출력한다.		*
 *																			*
 * 인수 : int cfd															*
 *			Control File fd													*
 *		  int dfd															*
 *			Data File fd													*
 *		  int mid															*
 *			Message ID														*
 *		  double uid														*
 *			Message Unique ID												*
 *		  unsinged int len													*
 *			출력할 길이 (== 0 이면 전체 출력)								*
 *		  int mode															*
 *			(== 0) 2진														*
 *			(== 1) 8진														*
 *			(== 2) 16진														*
 *			(== 3) ASCII													*
 *																			*
 * 리턴 : (==  0) 성공														*
 *		  (== -1) 실패														*
 *																			*
 * 작성 : 2002.02.25.														*
 * 수정 :																	*
 ****************************************************************************/
int d_msgview(int cfd, int dfd, int mid, double uid, unsigned int len, int mode)
{
	unsigned int i, j;
	unsigned int length;
	char *buf;
	struct d_msghd_ds mh;
	int CPL[4] = {8, 16, 16, 64};
	int LOD[4] = {71, 63, 47, 64};

	length = len;

	if(mode < 0 || mode > 3)  {
		errno = EINVAL;
		return(-1);
	}

	if(ReadMHead(cfd, mid, &mh) < 0)  {
#ifdef _LIB_DEBUG
		printf("[d_msgview]ReadMHead() fail.\n");
#endif
		return(-2);
	}

	if(mh.d_msg_alive != 1 || mh.d_msg_uid != uid)  {
#ifdef _LIB_DEBUG
		printf("[d_msgview]메세지가 존재하지 않습니다.\n");
#endif
		errno = EINVAL;
		return(-3);
	}

	if(lseek(dfd, mh.d_msg_whence, SEEK_SET) < 0)  {
#ifdef _LIB_DEBUG
		perror("[d_msgview]lseek");
#endif
		return(-4);
	}

	if(length == 0 || mh.d_msg_mbytes < length)
		length = mh.d_msg_mbytes;

	if((buf = malloc(length + 1)) == (char)NULL)  {
#ifdef _LIB_DEBUG
		perror("[d_msgview]malloc");
#endif
		return(-5);
	}
	bzero(buf, len + 1);

	if(read(dfd, buf, (size_t)length) != (ssize_t)length)  {
#ifdef _LIB_DEBUG
		perror("[d_msgview]read");
#endif
		free(buf);
		return(-6);
	}

	printf("Message ID : %d.%.0f\n\n", mid, uid);
	printf("[BYTES] DATA\n");
	printf("======= ");
	for(i = 0; i < LOD[mode]; i++)
		printf("=");

	for(i = 0; i < length; i++)  {
		if(i % CPL[mode] == 0)
			printf("\n[%05d] ", i + 1);

		switch(mode)  {
			case 0 :
				printf("%08b ", buf[i]);
				break;
			case 1 :
				printf("%03o ", buf[i]);
				break;
			case 2 :
				printf("%02X ", buf[i]);
				break;
			case 3 :
				if(buf[i] < '!' || buf[i] > '~')
					printf(" ");
				printf("%c", buf[i]);
				break;
		}
	}
	printf("\n");

	free(buf);
	return(0);
}

/*************************< FUNCTION HEADER >**********************************
 *                                                                            *
 *        > d_sem_opr <                                                       *
 *                                                                            *
 *       - Semaphore Create                                                   *
 *                                                                            *
 ******************************************************************************/
int d_sem_opr(semid, sem_num, flag)
int semid;				/* semid				*/
int flag;				/* 1:LOCK, 2:UNLOCK		*/
{
	int num;
	/*-----<LOCAL DATA> */
	static struct sembuf op_lock[2] = {
	   0, 0, 0,								/* wait for sem#0 to become 0          */
	   0, 1, SEM_UNDO						/* then increment sem#0 by 1           */
	};

	static struct sembuf op_unlock[1] = {
	   0, -1, (IPC_NOWAIT|SEM_UNDO)  /* decrement sem#0 by 1 (sets it to 0) */
	};

	op_lock[0].sem_num = op_lock[1].sem_num = sem_num;
	op_unlock[0].sem_num = sem_num;

	/*===== LOCK =====*/
	if(flag == 1)	{
		if(semop (semid, &op_lock[0], 2) < 0 && errno != 0) {
#ifdef _LIB_DEBUG
			printf ("[d_sem_opr] semop SEND LOCK \n");
#endif
			return(-1);
		}
	/*===== UNLOCK =====*/
	} else if(flag == 2)  {
		if(semop (semid, &op_unlock[0], 1) < 0 && errno != 0)  {
#ifdef _LIB_DEBUG
			printf ("[d_sem_opr] semop UNLOCK \n");
#endif
			return(-2);
		}
	}

	return(0);
}
/*************************<END OF "d_sem_opr">*********************************/

/*************************< FUNCTION HEADER >**********************************
 *                                                                            *
 *        > lock_sig_all <                                                    *
 *                                                                            *
 *       - Signal Pending -                                                   *
 *                                                                            *
 ******************************************************************************/

int lock_sig_all(sigset_t *new, sigset_t *old)
{
	sigemptyset(new);
	sigfillset(new);

	if (sigprocmask(SIG_BLOCK, new, old) < 0)  {
#ifdef _LIB_DEBUG
		perror("lock_sig_all");
#endif
		return(-1);
	}
	return(0);
}

int unlock_sig_all(sigset_t *old)
{
	if (sigprocmask(SIG_SETMASK, old, NULL) < 0)  {
#ifdef _LIB_DEBUG
		perror("unlock_sig_all");
#endif
		return(-2);
	}

	return(0);
}

int is_sig_catch(void)
{
	int i;
	sigset_t pend;

	if ( sigpending(&pend) < 0)
	{
#ifdef _LIB_DEBUG
		perror("is_sig_catch:sigpending");
#endif
		return(-1);
	}

	for ( i =1 ; i < NSIG; i++)
	{
		if ( sigismember(&pend, i ))
			return(1);
	}

	return(0);
}

#if 0
int d_locking(int index, int c)
{
	struct flock lock;
	int ret;

	if(D_msqinfo[index - 1].d_msg_cfd <= 0)
		return(-1);

	lock.l_start = 0;
	lock.l_whence = SEEK_SET;
	lock.l_len = 0;

	if( c == 1)  {
		/* write locking */
		lock.l_type = F_WRLCK;
		ret = fcntl(D_msqinfo[index - 1].d_msg_dfd, F_SETLKW, &lock); //jmk
		if (ret != 0) return(-2);
		ret = fcntl(D_msqinfo[index - 1].d_msg_cfd, F_SETLKW, &lock);
		if (ret != 0) return(-3);

		return(0);
/*
		return(fcntl(D_msqinfo[index - 1].d_msg_cfd, F_SETLKW, &lock));
*/
	} else if ( c == 2)  {
		lock.l_type =  F_UNLCK;
		ret = fcntl(D_msqinfo[index - 1].d_msg_dfd, F_SETLK, &lock); //jmk
		if (ret != 0) return(-4);
		ret = fcntl(D_msqinfo[index - 1].d_msg_cfd, F_SETLK, &lock);
		if (ret != 0) return(-5);
		return(0);
/*
		return(fcntl(D_msqinfo[index - 1].d_msg_cfd, F_SETLK, &lock));
*/
	} else
		return(-6);
}
#endif

int d_locking(int fd, int c)
{
	struct flock lock;
	int ret;

	if (fd <= 0) return(-1);

	lock.l_start = 0;
	lock.l_whence = SEEK_SET;
	lock.l_len = 0;

	if(c == 1)  {
		/* write locking */
		lock.l_type = F_WRLCK;
		return(fcntl(fd, F_SETLKW, &lock));
	} else if(c == 2)  {
		lock.l_type =  F_UNLCK;
		return(fcntl(fd, F_SETLK, &lock));
	} else
		return(-2);
}

/*************************< FUNCTION HEADER >**********************************
 *                                                                            *
 *        > wakeup_wait <                                                     *
 *                                                                            *
 *       - Send or Recv Open 대기시  Wakeup                                   *
 *                                                                            *
 ******************************************************************************/
                                                 /* flag : 0-Send, 1-Recv   */
int wakeup_wait(int shmid, int semid, char *qpath, int flag)
{
	int index, ix, nextid, ffd, size = -1;
	char fifopath[QPATH_LEN + QNAME_LEN + 4 + 3];
	void   *shmptr;
	struct d_msgwhd_ds  *wheadptr;
	struct d_msgwait_ds  *tmpptr = NULL;

	struct timeval tv;
	fd_set fds;

	if ((shmptr = shmat(shmid, 0, 0)) == (void *)-1)  {
		perror("[wakeup_wait] shmat");
		return(-1);
	}

	wheadptr	= (struct d_msgwhd_ds *)shmptr;
	tmpptr		= (struct d_msgwait_ds *) ((char *)shmptr + sizeof(struct d_msgwhd_ds));

	if (LOCK(semid) < 0)  {
#ifdef _LIB_DEBUG
		printf("[wakeup_wait] LOCK fail.\n");
#endif
		return(-2);
	}

	if ((flag == 0 ? wheadptr->d_msg_ss : wheadptr->d_msg_rs) == -1)  {
		//printf("[wakeup_wait] 대기열에 적당한 대기자가 없군요.\n");
		UNLOCK(semid);
		shmdt(shmptr);
		return(-3);
	} else
		index = (flag == 0 ? wheadptr->d_msg_ss : wheadptr->d_msg_rs);

	ix = index;
	while(1)  {
		sprintf(fifopath, "%s/%s.%s", qpath, tmpptr[ix].fifoname, FIFO_EXP);

		if ((ffd = open(fifopath, O_RDWR)) < 0)  {
			perror("[wakeup_wait] open");
		}

		FD_ZERO(&fds);
		FD_SET(ffd, &fds);
		tv.tv_sec = 1;
		tv.tv_usec = 0;

		select(ffd + 1, &fds, NULL, NULL, &tv);
		unlink(fifopath);

		close(ffd);
		ix = tmpptr[ix].nextid;
		if (ix == -1) break;
	}

	UNLOCK(semid);
	shmdt(shmptr);

	return(0);
}

/*******************<FUNCTION HEADER>*****************************************
 *                                                                           *
 *        >ctl_view<                                                         *
 *                                                                           *
 *        -- QUEUE File의 각각의 Message 별로 Control 정보를 Display --      *
 *                                                                           *
 *****************************************************************************/
int ctl_view(fd)
int   fd;                                     /* QUEUE Control File Fd      */
{
	int cnt = 0, ix , index;
	struct d_msghd_ds	d_msgheader;
	struct d_msqid_ds	d_msqid;
	struct timeval tv;
	struct tm *tm;			/* time struct       */
	char stime[20];

	lseek (fd, 0, SEEK_SET);
	read (fd, &d_msqid, QHEAD_SZ);

	index = d_msqid.d_msg_ds;

	if (index != -1)  {
		printf ("      ID           MTYPE      WHENCE    BYTES        DATE/TIME \n");
		printf (" ----------------  -----  ----------  -------  --------------------\n");
		while (1)  {
			if (index == -1) break;

			lseek(fd, QHEAD_SZ + (MHEAD_SZ * index), SEEK_SET);
			bzero(&d_msgheader, sizeof(struct d_msghd_ds));
			if (read(fd, &d_msgheader, MHEAD_SZ) < 0)  {
				return(-1);		/* Queue 정보를 읽는데 실패 */
			}

			tm = localtime(&d_msgheader.d_msg_stime);
			sprintf (stime, "%d.%02d.%02d %02d:%02d:%02d",
				tm->tm_year + 1900, tm->tm_mon +1, tm->tm_mday,
				tm->tm_hour, tm->tm_min, tm->tm_sec);

			printf ("%6d.%-10.0f  %5ld  %10d  %7d   %19s\n",
				d_msgheader.d_msg_id,		d_msgheader.d_msg_uid,
				d_msgheader.d_msg_mtype,	d_msgheader.d_msg_whence,
				d_msgheader.d_msg_mbytes,	stime);

			index = d_msgheader.d_msg_nextid;
		}
	} else {
		return(-2);				/* 현재 Queue 에 들어있는 메세지가 없음. */
	}

	return(0);
}

/*******************<FUNCTION HEADER>*****************************************
 *                                                                           *
 *        >msg_rm<                                                           *
 *                                                                           *
 *        -- Message 고유번호와 Message ID 에 해당하는 Message를 삭제 --     *
 *                                                                           *
 *****************************************************************************/
int msg_rm(fd, msgid, msguid)
int     fd;                                        /* Control File Fd       */
int     msgid;                                     /* Message ID            */
double  msguid;                                    /* Message Unique ID     */
{
	struct d_msqid_ds		d_msqid;				/* QUEUE   Header		*/
	struct d_msghd_ds		*d_msgheader = NULL;	/* Message Header		*/
	struct d_msghd_ds		d_msgheader_1;			/* Message Header		*/
	struct timeval tv;
	time_t clock;
	int    ix, cnt, beforeid, nextid;

	lseek (fd, -MHEAD_SZ, SEEK_END);
	read (fd, &d_msgheader_1, MHEAD_SZ);

	cnt = d_msgheader_1.d_msg_id + 1;

	d_msgheader = (struct d_msghd_ds *) malloc ((MHEAD_SZ * cnt) + 100);

	lseek (fd, 0, SEEK_SET);
	read (fd,  &d_msqid,	QHEAD_SZ);
	read (fd,  d_msgheader,	MHEAD_SZ*cnt);
	memcpy (&d_msgheader_1,	&d_msgheader[msgid], MHEAD_SZ);

	if (d_msgheader_1.d_msg_uid != msguid || d_msgheader_1.d_msg_alive == 0)  {
		errno = EINVAL;
		if (d_msgheader) free(d_msgheader);
		return(-1);			/* 해당 ID가 존재하지 않음 */
	}

	ix = 0;
	for (ix = 0; ix < cnt; ix++)  {
		if (ix == 0)  {
			beforeid = -1;
			if(d_msgheader[d_msqid.d_msg_ds].d_msg_id == msgid) break;
			beforeid = d_msqid.d_msg_ds;
			nextid = d_msgheader[d_msqid.d_msg_ds].d_msg_nextid;

		} else {
			if (nextid == -1) break;
			if (d_msgheader[nextid].d_msg_id == msgid) break;
			beforeid = nextid;
			nextid = d_msgheader[nextid].d_msg_nextid;
		}
	}

	/*----- Message Header 수정 -----*/
	if(beforeid != -1)
		d_msgheader[beforeid].d_msg_nextid = d_msgheader_1.d_msg_nextid;

	if(d_msqid.d_msg_ee != -1)
		d_msgheader[d_msqid.d_msg_ee].d_msg_nextid = d_msgheader_1.d_msg_id;

	d_msgheader[d_msgheader_1.d_msg_id].d_msg_alive = 0;
	d_msgheader[d_msgheader_1.d_msg_id].d_msg_nextid = -1; /*empty end nextid*/

	lseek (fd,  QHEAD_SZ, SEEK_SET);

	if(write (fd, d_msgheader, cnt * MHEAD_SZ) < 0)	 {
		printf ("[msg_rm] write(1) \n");
		if (d_msgheader) free(d_msgheader);
		return(-2);					/* 해당 메세지 삭제 실패 */
	}

	/*----- Queue   Header 수정 -----*/
	if(d_msqid.d_msg_es == -1)
		d_msqid.d_msg_es = d_msqid.d_msg_ee = d_msgheader_1.d_msg_id;
	d_msqid.d_msg_ee = d_msgheader_1.d_msg_id;

	if(d_msqid.d_msg_ds == d_msqid.d_msg_de)  {
		d_msqid.d_msg_ds = -1; d_msqid.d_msg_de = -1;
	} else  {
		if(beforeid == -1)
			d_msqid.d_msg_ds = d_msgheader_1.d_msg_nextid;
		if(d_msgheader_1.d_msg_nextid == -1)
			d_msqid.d_msg_de = beforeid;
	}

	gettimeofday(&tv, 0);
	d_msqid.d_msg_cbytes = d_msqid.d_msg_cbytes - d_msgheader_1.d_msg_mbytes;
	d_msqid.d_msg_qnum   = d_msqid.d_msg_qnum - 1;
	d_msqid.d_msg_lrpid  = getpid();
	d_msqid.d_msg_rtime  = tv.tv_sec;

	lseek(fd,  0, SEEK_SET);
	if (write (fd, &d_msqid, QHEAD_SZ) < 0)  {
		printf ("[msg_rm] write(2) \n");
		if (d_msgheader) free(d_msgheader);
		return(-3);					/* 해당 메세지 삭제 실패 */
	}

	if (d_msgheader) free(d_msgheader);
	return(0);
}

/************************************** 테스트 함수 **************************/
void view_map(struct d_msgmap_ds *ma)
{
	struct d_msgmap_ds *curr = ma;

	while(1)  {
		if(curr == (struct d_msgmap_ds *)NULL)
			break;
		printf("ID : [%4d]  사용여부 : [%1d]  시작 : [%7d]  끝 : [%7d]", curr->d_msg_id, curr->d_msg_dt, curr->d_msg_soff, curr->d_msg_eoff);
		if(curr->d_msg_dt == 0)
			printf(" (%d)\n", curr->d_msg_eoff - curr->d_msg_soff + 1);
		else
			printf("     <%d>\n", curr->d_msg_eoff - curr->d_msg_soff + 1);

		if(curr->next == NULL)
			break;
		curr = curr->next;
	}
}

void print_qinfo()
{
	int i;
	struct d_msqinfo_ds *ptr;

	printf("D_OpenQNum : [%d]\n", D_OpenQNum);

	for(i = 0; i < D_OpenQNum; i++)  {
		ptr = D_msqinfo + i;
		printf("----------------------\n");
		printf("qname : [%s]\n", ptr->d_msg_qname);
		printf("fpath : [%s]\n", ptr->d_msg_fpath);
		printf("cfd   : [%d]\n", ptr->d_msg_cfd);
		printf("dfd   : [%d]\n", ptr->d_msg_dfd);
	}
}

void hd(char *ptr, int size)
{
	int i;
	printf("\n");
	for(i = 0; i < size; i++)  {
		printf("%02X ", (unsigned char)*ptr);
		if(!((i + 1) % 16)) printf("\n");
		ptr++;
	}
	printf("\n");
}
