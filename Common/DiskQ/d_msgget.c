#include "DiskQ.h"

/****************************************************************************
 * Prototype : int MakeDataFile(char *, unsigned long, mode_t)				*
 *																			*
 * 설명 : Data File을 정해진 크기만큼 생성한다.								*
 *																			*
 * 인수 : char *dpath														*
 *			Data File의 경로명 (fd != 0) 일때만 유효						*
 *		  unsigned long size												*
 *			Data File의 크기												*
 *		  mode_t mode														*
 *			Data File의 권한 설정											*
 *																			*
 * 리턴 : ( >  0) Data File의 fd											*
 *		  (== -1) 실패														*
 *																			*
 * 작성 : 2001.12.19.														*
 * 수정 :																	*
 ****************************************************************************/
int MakeDataFile(char *dpath, unsigned long size, mode_t mode)
{
	int fd, nwritten;
	long nleft;
	char buf[513];
	mode_t old_umask;

	old_umask = umask(000);
	if((fd = open(dpath, O_RDWR | O_CREAT | O_TRUNC, mode)) < 0)  {
#ifdef _LIB_DEBUG
		perror("[MakeDataFile]open");
#endif
		umask(old_umask);
		return(-1);
	}
	umask(old_umask);

	memset(buf, 0x20, 512);
	buf[512] = (char)NULL;

	nleft = size;

	while(nleft > 0)  {
		if((nwritten = write(fd, buf, ((nleft > 512) ? 512 : nleft))) <= 0)  {
#ifdef _LIB_DEBUG
			perror("[MakeDataFile]write");
#endif
			close(fd);
			return(-2);
		}
		nleft -= nwritten;
	}

	return(fd);
}

/****************************************************************************
 * Prototype : int MakeControlFile(struct d_msqid_ds *, const char *,		*
 *				int, long)													*
 *																			*
 * 설명 : Control File을 생성하고 File 구조를 만든다.						*
 *																			*
 * 인수 : struct d_msqid *qh												*
 *			Queue header 정보를 돌려줄 구조체 포인터						*
 *		  char *cpath														*
 *			Control File의 경로												*
 *		  int flag															*
 *			Message Queue 생성에 필요한 flag들								*
 *																			*
 * 리턴 : ( >  0) Control File의 fd											*
 *		  (== -1) 실패														*
 *																			*
 * 작성 : 2001.12.20.														*
 * 수정 :																	*
 ****************************************************************************/
int MakeControlFile(struct d_msqid_ds *qh, char *cpath, int flag, long size)
{
	int i, fd;
	mode_t old_umask;
	struct d_msghd_ds mh;

	bzero((void *)qh, QHEAD_SZ);
	qh->d_msg_perm.uid = geteuid();
	qh->d_msg_perm.gid = getegid();
	qh->d_msg_perm.cuid = getuid();
	qh->d_msg_perm.cgid = getgid();
	qh->d_msg_perm.mode = (mode_t)flag;

	qh->d_msg_cbytes = (ulong)0;
	qh->d_msg_qnum = (ulong)0;
	qh->d_msg_qbytes = size;
	qh->d_msg_ds = -1;
	qh->d_msg_de = -1;
	qh->d_msg_es = 0;
	qh->d_msg_ee = INIT_MSG_NUM - 1;
	qh->d_msg_lspid = (pid_t)0;
	qh->d_msg_lrpid = (pid_t)0;
	qh->d_msg_stime = (time_t)0;
	qh->d_msg_rtime = (time_t)0;
	qh->d_msg_ctime = time((time_t)NULL);
	qh->d_msg_tcnt = (double)0;

	old_umask = umask(000);
	if((fd = open(cpath, O_RDWR | O_CREAT, CTL_PERM)) < 0)  {
#ifdef _LIB_DEBUG
		perror("[MakeControlFile]open");
#endif
		umask(old_umask);
		return(-1);
	}
	umask(old_umask);

	if(write(fd, (void *)qh, (size_t)QHEAD_SZ) != QHEAD_SZ)  {
#ifdef _LIB_DEBUG
		perror("[MakeControlFile]write(1)");
#endif
		close(fd);
		unlink(cpath);
		return(-2);
	}

	bzero((void *)&mh, MHEAD_SZ);
	mh.d_msg_mtype = 0;
	mh.d_msg_alive = 0;
	mh.d_msg_whence = 0;
	mh.d_msg_mbytes = 0;
	mh.d_msg_uid = 0;
	mh.d_msg_stime = 0;

	for(i = 0; i < INIT_MSG_NUM; i++)  {
		mh.d_msg_id = i;
		if(i == (INIT_MSG_NUM - 1))
			mh.d_msg_nextid = -1;
		else
			mh.d_msg_nextid = i + 1;

		if(write(fd, &mh, MHEAD_SZ) != MHEAD_SZ)  {
#ifdef _LIB_DEBUG
			perror("[MakeControlFile]write(2)");
#endif
			close(fd);
			unlink(cpath);
			return(-3);
		}
	}

	return(fd);
}

/****************************************************************************
 * Prototype : int AddQList(char *, char *, int, int, ulong, int, int		*
 *				struct ipc_perm)											*
 *																			*
 * 설명 : 프로세스 내에서 열려 있는 Queue 목록에 새로운 Queue 정보를		*
 *		  추가한다. (cfd == 0) 이면 제거된 Queue로 인식하여 새로운 Queue	*
 *		  정보를 추가하는데 사용할 수 있다.									*
 *																			*
 * 인수 : char *qname														*
 *			Queue의 고유한 이름												*
 *		  char *qpath														*
 *			Queue 관련 File들이 있는 경로									*
 *		  int cfd															*
 *			Control File의 fd												*
 *		  int dfd															*
 *			Data File의 fd													*
 *		  ulong qbytes														*
 *			Queue의 전체 크기												*
 *		  int shmid															*
 *			공유메모리 ID													*
 *		  int semid															*
 *			차단표시기 ID													*
 *		  struct ipc_perm perm												*
 *			Queue의 접근 권한 설정 상태										*
 *																			*
 * 리턴 : ( >  0) 프로세스 내부에서 사용할 qid로서 배열의 index 값보다 1만큼*
 *			큰 값임에 주의한다.												*
 *		  (== -1) 실패														*
 *																			*
 * 작성 : 2001.12.20.														*
 * 수정 :																	*
 ****************************************************************************/
int AddQList(char *qname, char *qpath, int cfd, int dfd, ulong qbytes, int shmid, int semid, struct ipc_perm perm)
{
	int i;
	struct d_msqinfo_ds *ptr;

	ptr = D_msqinfo;
	for(i = 0; i < D_OpenQNum && ptr != (struct d_msqinfo_ds *)NULL; i++)  {
		if(ptr->d_msg_cfd == 0)
			break;
		ptr ++;
	}

	if(i == D_OpenQNum)  {
		if((D_msqinfo = (struct d_msqinfo_ds *)realloc(D_msqinfo, sizeof(struct d_msqinfo_ds) * (D_OpenQNum + 1))) == (void *)NULL)  {
#ifdef _LIB_DEBUG
			perror("[AddQList]remalloc");
#endif
			return(-1);
		}
		ptr = D_msqinfo + D_OpenQNum;
	}

	memcpy(&(ptr->d_msg_perm), &perm, sizeof(struct ipc_perm));
	strcpy(ptr->d_msg_qname, qname);
	strcpy(ptr->d_msg_fpath, qpath);
	ptr->d_msg_cfd = cfd;
	ptr->d_msg_dfd = dfd;
	ptr->d_msg_qbytes = qbytes;
	ptr->d_msg_shmid = shmid;
	ptr->d_msg_semid = semid;

	return(++D_OpenQNum);
}

/****************************************************************************
 * Prototype : int d_msgget(char *, char *, int flag, long)					*
 *																			*
 * 설명 : Message Queue를 만들고 식별을 위한 id를 돌려준다.					*
 *																			*
 * 인수 : char *qname														*
 *			Message Queue의 이름											*
 *		  char *path														*
 *			관련된 파일들이 존재할 경로										*
 *		  int flag															*
 *			Message Queue 생성에 필요한 flag들								*
 *			IPC_CREAT : 없으면 생성											*
 *			O_EXCL    : 이미 존재한다면 에러								*
 *		  long size															*
 *			Message Queue를 위한 공간의 크기 (byte)							*
 *																			*
 * 리턴 : (!= -1) Message Queue의 식별자 (id)								*
 *		  (== -1) 실패														*
 *																			*
 * 에러 : EACCES															*
 *			접근 권한이 없음.												*
 *		  EEXIST															*
 *			메세지  큐가 존재하며 flag가 IPC_CREAT와 IPC_EXCL를 모두		*
 *			가지고 있다.													*
 *		  ENOENT															*
 *			메세지  큐가  존재하지  않으며 flag에 IPC_CREAT가 존재하지		*
 *			않는다.															*
 *		  EINVAL															*
 *			Queue가 이미 만들어져 있는 경우 지정한 size와 존재하는 Queue의	*
 *			size가 있치하지 않는다.											*
 *																			*
 * 작성 : 2001.12.19.														*
 * 수정 : 2002.04.09														*
 *			Control File을 만들거나 읽을때 LOCK()을 사용한다.				*
 ****************************************************************************/
int d_msgget(char *qname, const char *path, int flag, long size)
{
	int msq_id, cfd, dfd, qid, shmid, semid;
	char cpath[QPATH_LEN + QNAME_LEN + 3 + 3];
	char dpath[QPATH_LEN + QNAME_LEN + 3 + 3];
	char ipath[QPATH_LEN + QNAME_LEN + 3 + 3];

	char tmp[QPATH_LEN + 1];
	key_t ipckey;
	mode_t old_umask;
	struct d_msqid_ds qheader;
	struct shmid_ds shminfo;
	struct d_msgwhd_ds *shmhead;

	if(strlen(qname) > QNAME_LEN || strlen(qname) < 1)  {
#ifdef _LIB_DEBUG
		printf("[d_msgget]Queue 이름이 부적절.\n");
#endif
		return(-1);
	}

	if(strlen(path) > QPATH_LEN)  {
#ifdef _LIB_DEBUG
		printf("[d_msgget]Queue File 경로가 한계 초과.\n");
#endif
		return(-2);
	}

	bzero(tmp, sizeof(tmp));
	if(path[strlen(path) - 1] == '/')
		strncpy(tmp, path, strlen(path) - 1);
	else
		strncpy(tmp, path, strlen(path));

	sprintf(cpath, "%s/%s.%s", tmp, qname, CTL_EXP);
	sprintf(dpath, "%s/%s.%s", tmp, qname, DAT_EXP);
	sprintf(ipath, "%s/%s.%s", tmp, qname, IPC_EXP);

	old_umask = umask(000);
	if(creat(ipath, 0666) < 0)  {
#ifdef _LIB_DEBUG
		perror("[d_msgget]creat");
#endif
		umask(old_umask);
		return(-3);
	}
	umask(old_umask);

	if((ipckey = ftok(ipath, 'M')) == -1)  {
#ifdef _LIB_DEBUG
		perror("[d_msgget]ftok(1)");
#endif
		return(-4);
	}

	if((shmid = shmget(ipckey, sizeof(struct d_msgwhd_ds) + (sizeof(struct d_msgwait_ds) * MAX_WAIT), IPC_CREAT | SHM_R | SHM_W | 0666)) < 0)  {
#ifdef _LIB_DEBUG
		perror("[d_msgget]shmget");
#endif
		return(-5);
	}

	if(shmctl(shmid, IPC_STAT, &shminfo) == 0)  {
		if(shminfo.shm_lpid == 0  && shminfo.shm_atime == 0 && shminfo.shm_dtime == 0)  {
			if((shmhead = shmat(shmid, 0, 0)) == (void *)-1)  {
#ifdef _LIB_DEBUG
				perror("[FindSndWait]shmat");
#endif
				shmdt(shmhead);
				return(-6);
			}

			shmhead->d_msg_ss = -1;
			shmhead->d_msg_se = -1;
			shmhead->d_msg_rs = -1;
			shmhead->d_msg_re = -1;

			shmdt(shmhead);
		}
	} else  {
#ifdef _LIB_DEBUG
		perror("[d_msgget]shmctl");
#endif
		return(-7);
	}

	if((ipckey = ftok(ipath, 'P')) == -1)  {
#ifdef _LIB_DEBUG
		perror("[d_msgget]ftok(2)");
#endif
		return(-8);
	}

	if((semid = semget(ipckey, 3, IPC_CREAT | 0666)) < 0)  {
#ifdef _LIB_DEBUG
		perror("[d_msgget]semget");
#endif
		return(-9);
	}

	LOCK(semid);
	if((cfd = GetQHeader(cpath, &qheader, 0)) <= 0)  {
	/* file이 존재하지 않아서 에러가 발생한 경우만 다음으로 진행하고, 그렇지 않으면 종료 */
		if(errno != ENOENT)  {
			UNLOCK(semid);
			return(-10);
		}

		if(!(flag & IPC_CREAT))  {
			/* Queue가 존재하지 않을 경우 중에서 "IPC_CREAT" flag가 없으면 에러 종료한다. */
			UNLOCK(semid);
			errno = ENOENT;
			return(-11);
		}

		if((cfd = MakeControlFile(&qheader, cpath, flag, size)) < 0)  {
#ifdef _LIB_DEBUG
			printf("[d_msgget]MakeControlFile() fail.\n");
#endif
			unlink(cpath);
			UNLOCK(semid);
			return(-12);
		}

		if((dfd = MakeDataFile(dpath, size, DAT_PERM)) <= 0)  {
#ifdef _LIB_DEBUG
			printf("[d_msgget]MakeDataFile() fail.\n");
#endif
			unlink(cpath);
			unlink(dpath);
			UNLOCK(semid);
			return(-13);
		}
	} else  {
		if((flag & IPC_CREAT) && (flag & IPC_EXCL))  {
			/* Queue가 가 존재하며 flag가 IPC_CREAT와 IPC_EXCL를 모두 가지고 있다. */
			errno = EEXIST;
			UNLOCK(semid);
			return(-14);
		}

		if(size != qheader.d_msg_qbytes)  {
			errno = EINVAL;
			UNLOCK(semid);
			return(-15);
		}

		if(CheckPerm(qheader.d_msg_perm, 0) < 0)  {
			errno = EACCES;
			UNLOCK(semid);
			return(-16);
		}

		if((dfd = CheckDataFile(dpath, 0, size)) <= 0)  {
#ifdef _LIB_DEBUG
			printf("[d_msgget]CheckDataFile() fail.\n");
#endif
			UNLOCK(semid);
			return(-17);
		}
	}
	UNLOCK(semid);

	if((qid = AddQList(qname, tmp, cfd, dfd, (ulong)size, shmid, semid, qheader.d_msg_perm)) <= 0)  {
#ifdef _LIB_DEBUG
		printf("[d_msgget]AddQList() fail.\n");
#endif
		return(-18);
	}

	return(qid);
}
