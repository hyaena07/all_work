#include "DiskQ.h"

/****************************************************************************
 * Prototype : int MakeDataFile(char *, unsigned long, mode_t)				*
 *																			*
 * ���� : Data File�� ������ ũ�⸸ŭ �����Ѵ�.								*
 *																			*
 * �μ� : char *dpath														*
 *			Data File�� ��θ� (fd != 0) �϶��� ��ȿ						*
 *		  unsigned long size												*
 *			Data File�� ũ��												*
 *		  mode_t mode														*
 *			Data File�� ���� ����											*
 *																			*
 * ���� : ( >  0) Data File�� fd											*
 *		  (== -1) ����														*
 *																			*
 * �ۼ� : 2001.12.19.														*
 * ���� :																	*
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
 * ���� : Control File�� �����ϰ� File ������ �����.						*
 *																			*
 * �μ� : struct d_msqid *qh												*
 *			Queue header ������ ������ ����ü ������						*
 *		  char *cpath														*
 *			Control File�� ���												*
 *		  int flag															*
 *			Message Queue ������ �ʿ��� flag��								*
 *																			*
 * ���� : ( >  0) Control File�� fd											*
 *		  (== -1) ����														*
 *																			*
 * �ۼ� : 2001.12.20.														*
 * ���� :																	*
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
 * ���� : ���μ��� ������ ���� �ִ� Queue ��Ͽ� ���ο� Queue ������		*
 *		  �߰��Ѵ�. (cfd == 0) �̸� ���ŵ� Queue�� �ν��Ͽ� ���ο� Queue	*
 *		  ������ �߰��ϴµ� ����� �� �ִ�.									*
 *																			*
 * �μ� : char *qname														*
 *			Queue�� ������ �̸�												*
 *		  char *qpath														*
 *			Queue ���� File���� �ִ� ���									*
 *		  int cfd															*
 *			Control File�� fd												*
 *		  int dfd															*
 *			Data File�� fd													*
 *		  ulong qbytes														*
 *			Queue�� ��ü ũ��												*
 *		  int shmid															*
 *			�����޸� ID													*
 *		  int semid															*
 *			����ǥ�ñ� ID													*
 *		  struct ipc_perm perm												*
 *			Queue�� ���� ���� ���� ����										*
 *																			*
 * ���� : ( >  0) ���μ��� ���ο��� ����� qid�μ� �迭�� index ������ 1��ŭ*
 *			ū ���ӿ� �����Ѵ�.												*
 *		  (== -1) ����														*
 *																			*
 * �ۼ� : 2001.12.20.														*
 * ���� :																	*
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
 * ���� : Message Queue�� ����� �ĺ��� ���� id�� �����ش�.					*
 *																			*
 * �μ� : char *qname														*
 *			Message Queue�� �̸�											*
 *		  char *path														*
 *			���õ� ���ϵ��� ������ ���										*
 *		  int flag															*
 *			Message Queue ������ �ʿ��� flag��								*
 *			IPC_CREAT : ������ ����											*
 *			O_EXCL    : �̹� �����Ѵٸ� ����								*
 *		  long size															*
 *			Message Queue�� ���� ������ ũ�� (byte)							*
 *																			*
 * ���� : (!= -1) Message Queue�� �ĺ��� (id)								*
 *		  (== -1) ����														*
 *																			*
 * ���� : EACCES															*
 *			���� ������ ����.												*
 *		  EEXIST															*
 *			�޼���  ť�� �����ϸ� flag�� IPC_CREAT�� IPC_EXCL�� ���		*
 *			������ �ִ�.													*
 *		  ENOENT															*
 *			�޼���  ť��  ��������  ������ flag�� IPC_CREAT�� ��������		*
 *			�ʴ´�.															*
 *		  EINVAL															*
 *			Queue�� �̹� ������� �ִ� ��� ������ size�� �����ϴ� Queue��	*
 *			size�� ��ġ���� �ʴ´�.											*
 *																			*
 * �ۼ� : 2001.12.19.														*
 * ���� : 2002.04.09														*
 *			Control File�� ����ų� ������ LOCK()�� ����Ѵ�.				*
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
		printf("[d_msgget]Queue �̸��� ������.\n");
#endif
		return(-1);
	}

	if(strlen(path) > QPATH_LEN)  {
#ifdef _LIB_DEBUG
		printf("[d_msgget]Queue File ��ΰ� �Ѱ� �ʰ�.\n");
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
	/* file�� �������� �ʾƼ� ������ �߻��� ��츸 �������� �����ϰ�, �׷��� ������ ���� */
		if(errno != ENOENT)  {
			UNLOCK(semid);
			return(-10);
		}

		if(!(flag & IPC_CREAT))  {
			/* Queue�� �������� ���� ��� �߿��� "IPC_CREAT" flag�� ������ ���� �����Ѵ�. */
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
			/* Queue�� �� �����ϸ� flag�� IPC_CREAT�� IPC_EXCL�� ��� ������ �ִ�. */
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
