/*******************<FILE HEADER>*********************************************
 *--<SYSTEM>-----------------------------------------------------------------*
 *   DISK QUEUE                                                              *
 *                                                                           *
 *--<SUBSYSTEM>--------------------------------------------------------------*
 *   �ý��� - ����                                                           *
 *                                                                           *
 *--<FILE NAME>--------------------------------------------------------------*
 *   d_msgrcv.c                                                              *
 *                                                                           *
 *--<DESCRIPTION>------------------------------------------------------------*
 *                                                                           *
 *   msgrcv() ��� ����                                                      *
 *                                                                           *
 *--<SYNTAX>-----------------------------------------------------------------*
 *                                                                           *
 *.Calling Sequence                                                          *
 *                                                                           *
 *.Parameters                                                                *
 *                                                                           *
 *.Return Values                                                             *
 *                                                                           *
 *--<MODIFICATION HISTORY>---------------------------------------------------*
 *   (Rev.)  (Date)      (ID/NAME)      (Comment)                            *
 *    R1.0   2001.12.20  (PS/Joo M.K)    Original                            *
 *                                                                           *
 *--<SLAVE>------------------------------------------------------------------*
 *.System                                                                    *
 *                                                                           *
 *.Application                                                               *
 *                                                                           *
 *.Library                                                                   *
 *                                                                           *
 *****************************************************************************/
#include "DiskQ.h"

/****************************************************************************
 * Prototype : int d_msgrcv(int msqid, struct msgbuf *, int msgsz,			*
 *			long msgtyp, int msgflg											*
 *																			*
 * ���� : Message Queue����  msgtyp �� �ش��ϴ� Message�� �����´�.			*
 *																			*
 * �μ� : const int msqid													*
 *			Message Queue�� ���� ��ȣ										*
 *		  struct msgbuf *buf												*
 *			Message�� ���� ���� ������										*
 *			struct msgbuf {													*
 *				long mtype;													*
 *				char mtext[1];												*
 *			};																*
 *		  int msgsz															*
 *			���� ���� ������ ���� (byte)									*
 *		  long msgtyp														*
 *			ã���� �ϴ� Message Type										*
 *		  int msgflg														*
 *			Message ���ۿ� �ʿ��� flag��									*
 *			IPC_NOWAIT : ã�� msgtyp �� Message��  ���� ��� ���ŷ����		*
 *						 �ʰ�  �ٷ� �����Ѵ�.								*
 *			IPC_NOERROR: �޼����� ������ msgsz ���� ��� �ʰ�����			*
 *						 �߶󳽴�.  ������ ���ϸ� ERROR �� ó��				*
 *																			*
 * ���� : ( >  0) ������ ���� Message ����									*
 *		  (== -1) ����														*
 *																			*
 * �ۼ� : 2001.12.21.														*
 * ���� :																	*
 ****************************************************************************/

int d_msgrcv(int msqid, void *buf, int msgsz, long msgtyp, int msgflg)
{
	struct msgbuf  {
		long mtype;
		char mtext[1];
	};

	struct msgbuf *msgbuf = (struct msgbuf *)buf;

	char	q_temp[80];						/* temp buffer					*/
	char	cpath[QPATH_LEN + QNAME_LEN + 3 + 1];

	struct d_msghd_ds	*d_msgheader = NULL; /* Message header ����ü		*/
	struct d_msghd_ds	d_msgheader_1;       /* ã�� Message header ����ü	*/

	/* 20020124 jmk start */
	void   *shmptr;
	struct d_msgwhd_ds	*wheadptr;			/* ��⿭�� ���۰� �� �ε���	*/
	struct d_msgwait_ds	*waitptr;			/* ��⿭�� ����				*/
	int    flag = 0;						/* ó������ �д°�				*/
	/* 20020124 jmk end   */

	struct	d_msqid_ds  d_msqid;			/* Queue Header					*/
	struct	stat  fst;
	int		msg_cnt;						/* Message Count				*/

	int		cfd;							/* Control File Fd				*/
	int		ix, kx, ret, rbytes, nextid, beforeid;

	sigset_t	new, old;

	/*==================== Queue ID  Ȯ�� ====================*/
	if (msqid <= 0 || msqid > D_OpenQNum)
	{
		errno = EIDRM;
		printf ("[d_msgrcv] msqid Check : msqid=%d ����! \n", msqid);
		return(-1);
	}

	/*==================== Queue ���� Ȯ�� ====================*/
	/* Queue Data File ���� Ȯ�� */
	sprintf (q_temp, "%s/%s.%s", D_msqinfo[msqid-1].d_msg_fpath,
								 D_msqinfo[msqid-1].d_msg_qname, DAT_EXP);
	if (access(q_temp, R_OK) != 0)
	{
		printf ("[d_msgrcv] accecc(1) : Queue Data ������ Ȯ���ϼ���! \n");
		return(-2);
	}

	/* Queue Control File ���� Ȯ�� */
	sprintf (q_temp, "%s/%s.%s", D_msqinfo[msqid-1].d_msg_fpath,
								 D_msqinfo[msqid-1].d_msg_qname, CTL_EXP);

	if (access(q_temp, R_OK) != 0)
	{
		printf ("[d_msgrcv] access(2) : Queue Control ������ Ȯ���ϼ���! \n");
		return(-3);
	}
#if 0
	/* Permission Check */
	if(CheckPerm(D_msqid.d_msg_perm, 1) < 0)  {
		errno = EACCES;
		printf ("Queue Data ������ Ȯ���ϼ���! ");
		return(-4);
	}
#endif

	flag = 0;
	while (1)
	{

		/* ��⿭ ���� 20020124 jmk start*/
		if (flag ==	0)
		{
			RCV_WAIT_LOCK(D_msqinfo[msqid-1].d_msg_semid);

			if ((shmptr	= shmat(D_msqinfo[msqid-1].d_msg_shmid, 0, 0)) == (void	*)-1)
			{
				perror("[d_msgrcv] shmat");
				RCV_WAIT_UNLOCK(D_msqinfo[msqid-1].d_msg_semid);
				return(-5);
			}

			wheadptr	= (struct d_msgwhd_ds *)shmptr;
			waitptr		= (struct d_msgwait_ds	*)((char *)shmptr +
						   sizeof(struct d_msgwhd_ds));

			if(LOCK(D_msqinfo[msqid-1].d_msg_semid) < 0)  {
#ifdef _LIB_DEBUG
				printf("[wait_for_rcv]LOCK(1) fail.\n");
#endif
				shmdt(shmptr);
				RCV_WAIT_UNLOCK(D_msqinfo[msqid-1].d_msg_semid);
				return(-6);
			}

			if (wheadptr->d_msg_rs != -1 && wheadptr->d_msg_re != -1)
			{
				nextid = wheadptr->d_msg_rs;

				while(1)
				{
					if (nextid == -1) break;
					if(waitptr[nextid].use == 1)
					{
						if (waitptr[nextid].mtype == msgtyp)
						{
							flag = 1; break;
						}
						nextid = waitptr[nextid].nextid;
					}
				}

				if (flag ==	1)
				{
					UNLOCK(D_msqinfo[msqid-1].d_msg_semid);
					RCV_WAIT_UNLOCK(D_msqinfo[msqid-1].d_msg_semid);

					if(FLOCK(D_msqinfo[msqid-1].d_msg_cfd) < 0)  {
#ifdef _LIB_DEBUG
						printf("[d_msgrcv]FLOCK fail.\n");
#endif
						return(-7);
					}

					if ((ret = wait_for_rcv(D_msqinfo[msqid-1].d_msg_shmid,
							   D_msqinfo[msqid-1].d_msg_semid, msgtyp,
							   D_msqinfo[msqid-1].d_msg_fpath, 0, msqid)) >	0)
					{
						shmdt(shmptr);

						continue;

					} else {

						shmdt(shmptr);
						printf ("[d_msgrcv] wait_for_rcv(0)	\n");
						return(-8);
					}
				}
			}

			if (flag == 0)
			{
				RCV_WAIT_UNLOCK(D_msqinfo[msqid-1].d_msg_semid);
				UNLOCK(D_msqinfo[msqid-1].d_msg_semid);

				shmdt(shmptr);

			}
		}
		/* ��⿭ ���� 20020124 jmk	end	*/

		if (d_msgheader) free(d_msgheader);

		if(FLOCK(D_msqinfo[msqid-1].d_msg_cfd) < 0)  {
#ifdef _LIB_DEBUG
			printf("[d_msgrcv]FLOCK fail.\n");
#endif
			return(-9);
		}

		if (stat (q_temp, &fst) != 0)
		{
			printf ("[d_msgrcv]	stat : Queue Control ������	Ȯ���ϼ���!\n");
			return(-10);
		}

		/*-----	Message	Count -----*/
		msg_cnt	= ((fst.st_size - QHEAD_SZ) / MHEAD_SZ);
		sprintf(cpath, "%s/%s.%s", D_msqinfo[msqid-1].d_msg_fpath,
								   D_msqinfo[msqid-1].d_msg_qname, CTL_EXP);
		/*==================== Queue Header	���� Get ====================*/
		if((cfd = GetQHeader(cpath, &d_msqid, msqid)) < 0)  {
			printf ("[d_msgrcv] GetQHeader : Queue Control ������ Ȯ���ϼ���!\n");
			return(-11);
		}

		lseek(D_msqinfo[msqid-1].d_msg_cfd, QHEAD_SZ, SEEK_SET);
		lock_sig_all(&new, &old);
		d_msgheader = (struct d_msghd_ds *)	malloc(fst.st_size - QHEAD_SZ);

		/*==================== Control File	�� Read	====================*/
		if (read (D_msqinfo[msqid-1].d_msg_cfd, d_msgheader, (fst.st_size - QHEAD_SZ)) < 0)
		{
			printf ("[d_msgrcv] read() : Queue Control ������ Ȯ���ϼ���!\n");
			FUNLOCK(D_msqinfo[msqid-1].d_msg_cfd);
			unlock_sig_all(&old);
			if (d_msgheader) free(d_msgheader);
			return(-12);
		}

		/*-----	��ȿ�� Data	���� -----*/
		if (d_msqid.d_msg_ds != -1)
		{
			/*=====	mtype �� ���� ���� ����	���� ���� Message	ã�� =====*/
			if (msgtyp == 0)
			{
				/*----- ���� ���� ���� mtext ã�� -----*/
				memcpy (&d_msgheader_1, &d_msgheader[d_msqid.d_msg_ds], MHEAD_SZ);
				/*--------------- Data	File ���� Data �б� -----------------*/
				beforeid = -1;

				if ((rbytes = read_data(msqid, d_msgheader, &d_msgheader_1,
				   msgbuf, fst.st_size,	msgsz, msgflg, &d_msqid, beforeid))	< 0)
				{
					printf ("[d_msgrcv] read_data(1)\n");
					FUNLOCK(D_msqinfo[msqid-1].d_msg_cfd);
					unlock_sig_all(&old);
					if (d_msgheader) free(d_msgheader);
					return(-13);
				}

				if (flag ==	1)
					RCV_WAIT_UNLOCK(D_msqinfo[msqid-1].d_msg_semid);

				FUNLOCK(D_msqinfo[msqid-1].d_msg_cfd);
				msgbuf->mtype = d_msgheader_1.d_msg_mtype;

				if (d_msgheader)	free(d_msgheader);

				if (is_sig_catch() > 0)
				{
					errno = EINTR;
					unlock_sig_all(&old);
					return(-14);

				} else unlock_sig_all(&old);

				/* Blocking	�� d_msgsnd() wakeup */

				while (1)
				{
					ret	= wakeup_snd(D_msqinfo[msqid-1].d_msg_shmid,
									 D_msqinfo[msqid-1].d_msg_semid, msgsz,
									 D_msqinfo[msqid-1].d_msg_fpath);

					if (ret	== 0) break;
					else if	(ret ==	-1) {
						continue;
					} else {
						break;
					}
				}

				return(rbytes);

			/*=====	mtype �� �´� �� �� ���� ���� ���� Message ã�� =====*/
			} else if (msgtyp > 0) {

				/*----- mtype �� ������ Message	Searching -----*/
				ret = 0;
				for (ix = 0; ix < msg_cnt; ix++)
				{
					if (ix == 0)
					{
						beforeid = -1;
						if(d_msgheader[d_msqid.d_msg_ds].d_msg_mtype == msgtyp){
							memcpy(&d_msgheader_1,
								   &d_msgheader[d_msqid.d_msg_ds], MHEAD_SZ);
							ret++;	break;
						}
						beforeid = d_msqid.d_msg_ds;
						nextid = d_msgheader[d_msqid.d_msg_ds].d_msg_nextid;
					} else {
						if (nextid == -1) break;
						if (d_msgheader[nextid].d_msg_mtype == msgtyp) {
							memcpy (&d_msgheader_1,	&d_msgheader[nextid],
									MHEAD_SZ);
							ret++;	break;
						}
						beforeid = nextid;
						nextid = d_msgheader[nextid].d_msg_nextid;

					}
				}
				/* Message ���� */

				if (ret)
				{
					/*------------ Data File ���� Data �б� --------------*/
					if ((rbytes=read_data(msqid, d_msgheader, &d_msgheader_1,
						 msgbuf, fst.st_size, msgsz, msgflg, &d_msqid, beforeid)) < 0)
					{
						printf ("[d_msgrcv] read_data(2)\n");
						FUNLOCK(D_msqinfo[msqid-1].d_msg_cfd);
						unlock_sig_all(&old);
						if (d_msgheader) free(d_msgheader);
						return(-15);
					}
					if (flag == 1)
						RCV_WAIT_UNLOCK(D_msqinfo[msqid-1].d_msg_semid);

					FUNLOCK(D_msqinfo[msqid-1].d_msg_cfd);
					msgbuf->mtype =	d_msgheader_1.d_msg_mtype;
					if (is_sig_catch() > 0)
					{
						errno = EINTR;
						unlock_sig_all(&old);
						if (d_msgheader) free(d_msgheader);
						return(-16);

					} else unlock_sig_all(&old);

					/* Blocking	�� d_msgsnd() wakeup */
					while (1)
					{
						ret = wakeup_snd(D_msqinfo[msqid-1].d_msg_shmid,
									 D_msqinfo[msqid-1].d_msg_semid, msgsz,
									 D_msqinfo[msqid-1].d_msg_fpath);

						if (ret == 0) break;
						else if (ret == -1)	{
							continue;
						} else {
							break;
						}
					}
					if (d_msgheader) free(d_msgheader);
					return(rbytes);

                /* Message ���� IPC_NOWAIT�� �ƴ� */
				} else if (msgflg != IPC_NOWAIT) {

					if (is_sig_catch() > 0)
					{
						errno = EINTR;
						unlock_sig_all(&old);
						if (d_msgheader) free(d_msgheader);
						return(-17);

					} else unlock_sig_all(&old);

					/*---------- Blocking Checking ----------*/
					if ((ret = wait_for_rcv(D_msqinfo[msqid-1].d_msg_shmid,
							   D_msqinfo[msqid-1].d_msg_semid, msgtyp,
							   D_msqinfo[msqid-1].d_msg_fpath, flag, msqid)) > 0)
					{
						flag = 1;		/* 20020124 jmk */
						unlock_sig_all(&old);
						if (d_msgheader) free(d_msgheader);
						continue;

					} else {
						printf ("[d_msgrcv] wait_for_rcv(1)\n");
						if (d_msgheader) free(d_msgheader);
						return(-18);
					}

				} else  {

					errno = ENOMSG;
					FUNLOCK(D_msqinfo[msqid-1].d_msg_cfd);
					if (d_msgheader) free(d_msgheader);
					return(0);
				}

			} else if (msgtyp < 0) {

				/*-----	mtype ���� ���밪�� ���� Message Searching -----*/

				for(ix	= 0; ix < msg_cnt; ix++)
				{
					if (ix == 0)
					{
						msgtyp = d_msgheader[d_msqid.d_msg_ds].d_msg_mtype;
						nextid = d_msgheader[d_msqid.d_msg_ds].d_msg_nextid;

					} else if (msgtyp > d_msgheader[nextid].d_msg_mtype) {

						msgtyp = d_msgheader[nextid].d_msg_mtype;
						nextid = d_msgheader[nextid].d_msg_nextid;
					}
				}

				/* mtype�� ���� ���� Message �� ���� ���� ���� Message */
				for(ix = 0; ix < msg_cnt; ix++)
				{
					if (ix == 0)
					{
						beforeid = -1;
						if (d_msgheader[d_msqid.d_msg_ds].d_msg_mtype
							== msgtyp)
						{
							memcpy(&d_msgheader_1,
								   &d_msgheader[d_msqid.d_msg_ds], MHEAD_SZ);
							break;
						}
						beforeid = d_msqid.d_msg_ds;
						nextid=d_msgheader[d_msqid.d_msg_ds].d_msg_nextid;

					} else {
						if (d_msgheader[nextid].d_msg_mtype == msgtyp)
						{
							memcpy (&d_msgheader_1, &d_msgheader[nextid],
									MHEAD_SZ);
							break;
						}
						beforeid = nextid;
						nextid = d_msgheader[nextid].d_msg_nextid;
					}
				}

				/*---------- Data File ����	Data �б� -----------*/
				if ((rbytes = read_data (msqid, d_msgheader, &d_msgheader_1,
				   msgbuf, fst.st_size, msgsz, msgflg, &d_msqid, beforeid)) < 0)
				{
					printf ("[d_msgrcv] read_data(3)\n");
					FUNLOCK(D_msqinfo[msqid-1].d_msg_cfd);
					unlock_sig_all(&old);
					if (d_msgheader) free(d_msgheader);
					return(-19);
				}
				if (flag == 1)
					RCV_WAIT_UNLOCK(D_msqinfo[msqid-1].d_msg_semid);

				FUNLOCK(D_msqinfo[msqid-1].d_msg_cfd);
				msgbuf->mtype = d_msgheader_1.d_msg_mtype;

				if (is_sig_catch() > 0)
				{
					errno = EINTR;
					unlock_sig_all(&old);
					if (d_msgheader) free(d_msgheader);
					return(-20);

				} else unlock_sig_all(&old);

				/* Blocking	�� d_msgsnd() wakeup */
				while (1)
				{
					ret = wakeup_snd(D_msqinfo[msqid-1].d_msg_shmid,
									 D_msqinfo[msqid-1].d_msg_semid, msgsz,
									 D_msqinfo[msqid-1].d_msg_fpath);

					if (ret == 0) break;
					else if (ret == -1)	{
						continue;
					} else {
						break;
					}
				}

				if (d_msgheader) free(d_msgheader);
				return(rbytes);

			}

		/*----- ��ȿ�� Data ���� . ��� -----*/
		} else if (!(msgflg & IPC_NOWAIT)) {

			if (is_sig_catch() > 0)
			{
				errno = EINTR;
				unlock_sig_all(&old);
				FUNLOCK(D_msqinfo[msqid-1].d_msg_cfd);
				if (d_msgheader) free(d_msgheader);
				return(-21);

			} else unlock_sig_all(&old);

			/*---------- Blocking Checking ----------*/

			if ((ret = wait_for_rcv(D_msqinfo[msqid-1].d_msg_shmid,
					   D_msqinfo[msqid-1].d_msg_semid, msgtyp,
					   D_msqinfo[msqid-1].d_msg_fpath, flag, msqid)) > 0)
			{
				flag = 1;				/*	20020124 jmk */
				unlock_sig_all(&old);
				if (d_msgheader) free(d_msgheader);
				continue;

			} else {
				printf ("[d_msgrcv] wait_for_rcv(2)\n");
				unlock_sig_all(&old);
				RCV_WAIT_UNLOCK(D_msqinfo[msqid-1].d_msg_semid);
				FUNLOCK(D_msqinfo[msqid-1].d_msg_cfd);
				if (d_msgheader) free(d_msgheader);
				return(-22);
			}

		} else {
			errno = ENOMSG;
			FUNLOCK(D_msqinfo[msqid-1].d_msg_cfd);
			unlock_sig_all(&old);
			if (d_msgheader) free(d_msgheader);
			return(0);
		}
	}
}
/************************ < END OF "d_msgrcv"> ********************************/

/*******************<FUNCTION HEADER>*****************************************
 *                                                                           *
 *        >Read_data<                                                        *
 *                                                                           *
 *        -- Data File ���� �ش� Message ���� �� Control File ����  --       *
 *                                                                           *
 *****************************************************************************/

int	read_data(msqid, d_msgheader, d_msgheader_1, buf,
			  fsize, msgsz, msgflg, d_msqid, beforeid)
int		msqid;								/* Message Queue ID			*/
struct	d_msghd_ds	*d_msgheader;			/* Message Header			*/
struct	d_msghd_ds	*d_msgheader_1;			/* Message Header			*/
void	*buf;
int		fsize;								/* Control File Size		*/
int		msgsz;								/* Message Size				*/
int		msgflg;								/* Message Flag				*/
struct	d_msqid_ds *d_msqid;				/* Message Header			*/
int		beforeid;							/* ���� ������				*/
{
	struct msgbuf  {
		long mtype;
		char mtext[1];
	};
	struct flock ctl_lock;
	struct timeval tv;
	sigset_t new, old;
	int nsize;
	int ix;

	struct msgbuf *msgbuf = (struct	msgbuf *)buf;

	/*=====	Data File Read ======*/
	lseek(D_msqinfo[msqid-1].d_msg_dfd,	d_msgheader_1->d_msg_whence, SEEK_SET);

	if (msgsz < d_msgheader_1->d_msg_mbytes)
	{
		if (msgflg == MSG_NOERROR)
		{
			if ((nsize = read (D_msqinfo[msqid-1].d_msg_dfd, msgbuf->mtext,
							   msgsz)) < 0)
			{
				printf ("[read_data] read(1) \n");
				return(-1);
			}
		} else {

			printf ("[read_data] MSG_NOERROR ��������(msgsz[%d] ��msglength[%d])\n",msgsz, d_msgheader_1->d_msg_mbytes);
			errno = E2BIG;
			return(-2);
		}

	} else {

		if ((nsize = read (D_msqinfo[msqid-1].d_msg_dfd, msgbuf->mtext,
						   d_msgheader_1->d_msg_mbytes)) < 0)
		{
			printf("[read_data] read(2)\n");
			return(-3);
		}
	}

	msgbuf->mtext[nsize] = 0x00;

	/*---- Message Header ���� -----*/
	if (beforeid != -1)
		d_msgheader[beforeid].d_msg_nextid = d_msgheader_1->d_msg_nextid;
	if (d_msqid->d_msg_ee != -1)
		d_msgheader[d_msqid->d_msg_ee].d_msg_nextid = d_msgheader_1->d_msg_id;

	d_msgheader[d_msgheader_1->d_msg_id].d_msg_alive = 0;
	d_msgheader[d_msgheader_1->d_msg_id].d_msg_nextid = -1;	/*empty end nextid*/

	lseek(D_msqinfo[msqid-1].d_msg_cfd, QHEAD_SZ, SEEK_SET);

	if (write (D_msqinfo[msqid-1].d_msg_cfd, d_msgheader, (fsize - QHEAD_SZ)) < 0)
	{
		printf ("[read_data] write(2) \n");
		return(-4);
	}

	/*---- Queue  Header ���� -----*/
	if(d_msqid->d_msg_es == -1)
		d_msqid->d_msg_es = d_msqid->d_msg_ee = d_msgheader_1->d_msg_id;
	d_msqid->d_msg_ee = d_msgheader_1->d_msg_id;

	if (d_msqid->d_msg_ds == d_msqid->d_msg_de)
	{
		d_msqid->d_msg_ds = -1;	d_msqid->d_msg_de = -1;

	} else {
		if (beforeid ==	-1)
			d_msqid->d_msg_ds = d_msgheader_1->d_msg_nextid;
		if(d_msgheader_1->d_msg_nextid == -1)
			d_msqid->d_msg_de = beforeid;

	}

	gettimeofday(&tv, 0);
	d_msqid->d_msg_cbytes	= d_msqid->d_msg_cbytes - d_msgheader_1->d_msg_mbytes;
	d_msqid->d_msg_qnum		= d_msqid->d_msg_qnum	- 1;
	d_msqid->d_msg_lrpid	= getpid();
	d_msqid->d_msg_rtime	= tv.tv_sec;

	lseek(D_msqinfo[msqid-1].d_msg_cfd,  0, SEEK_SET);
	if (write(D_msqinfo[msqid-1].d_msg_cfd, d_msqid, QHEAD_SZ)	< 0)
	{
		printf ("[read_data] write(1) \n");
		return(-5);
	}

	return(nsize);
}
/************************ < END OF "read_data"> ******************************/


/*******************<FUNCTION HEADER>*****************************************
 *                                                                           *
 *        >wait_for_rcv<                                                     *
 *                                                                           *
 *        -- Receive �� ���� ����Ÿ�� ������� Blocking ó�� --              *
 *                                                                           *
 *****************************************************************************/
int	wait_for_rcv(int shmid, int semid, long	mtype, char *qpath, int flag, int msqid)
{
	int ix, bix, beforeid, ffd;
	long t_size;
	char fifoname[QPATH_LEN + QNAME_LEN + 4	+ 3];
	char fifopath[QPATH_LEN + QNAME_LEN + 4	+ 3];
	mode_t old_umask;
	void   *shmptr;
	struct d_msgwhd_ds		*wheadptr;
	struct d_msgwait_ds		*tmpptr	= NULL;

	sprintf(fifoname, "%s", mkfifoname(qpath));
	sprintf(fifopath, "%s/%s.%s", qpath, fifoname, FIFO_EXP);
	old_umask =	umask(000);
	if (mkfifo(fifopath, 0666) != 0)
	{
		shmdt(shmptr);
		umask(old_umask);
		perror("[wait_for_rcv] mkfifo");
		return(-1);
	}
	umask(old_umask);

	if ((shmptr = shmat(shmid, 0, 0)) == (void *)-1)
	{
		perror("[wait_for_rcv] shmat");
		return(-2);
	}

	wheadptr	= (struct d_msgwhd_ds *)shmptr;
	tmpptr		= (struct d_msgwait_ds *)((char *)shmptr + sizeof(struct d_msgwhd_ds));

	if(LOCK(semid) < 0)  {
#ifdef _LIB_DEBUG
		printf("[wait_for_rcv]LOCK(1) fail.\n");
#endif
		shmdt(shmptr);
		return(-3);
	}

	for	(ix = 0; ix < MAX_WAIT; ix++)
	{
		if(tmpptr[ix].use == 0)
		{
			tmpptr[ix].use = 1;
			tmpptr[ix].mtype = mtype;
			strcpy(tmpptr[ix].fifoname, fifoname);
			tmpptr[ix].nextid = -1;
			break;
		}
	}

	if (ix == MAX_WAIT)
	{
#ifdef _LIB_DEBUG
		printf("[wait_for_rcv]��⿭ ������ ��á����...\n");
#endif
		UNLOCK(semid);
		RCV_WAIT_UNLOCK(semid);
		shmdt(shmptr);
		unlink(fifopath);
		return(-4);

	} else {

		/*--- recv end index ���� ---*/
		if (wheadptr->d_msg_rs == -1 || wheadptr->d_msg_re == -1)
		{
			wheadptr->d_msg_rs = wheadptr->d_msg_re = ix;

		} else {
			tmpptr[wheadptr->d_msg_re].nextid = ix;		/*	�������� ���� index		*/
			wheadptr->d_msg_re = ix;
		}
	}

	UNLOCK(semid);
	shmdt(shmptr);
	if (flag == 1)
		RCV_WAIT_UNLOCK(semid);

	FUNLOCK(D_msqinfo[msqid-1].d_msg_cfd);

	if ((ffd = open(fifopath, O_WRONLY)) < 0)
	{
#ifdef _LIB_DEBUG
		perror("[wait_for_rcv]open");
#endif
		FLOCK(D_msqinfo[msqid-1].d_msg_cfd);
		return(-5);
	}

	if (write(ffd, &t_size, sizeof(int)) <= 0)
	{
#ifdef _LIB_DEBUG
		perror("[wait_for_rcv]write");
#endif
		close(ffd);
		return(-6);
	}

	FLOCK(D_msqinfo[msqid-1].d_msg_cfd);
	if (RCV_WAIT_LOCK(semid) < 0)
	{
		printf (" [wati_for_rcv] RCV_WAIT_LOCK Fail!!!\n");
		return(-7);
	}

	close(ffd);
	return(1);
}
/************************< END OF "wait_for_rcv">*****************************/

/*******************<FUNCTION HEADER>*****************************************
 *                                                                           *
 *        >wakeup_snd<                                                       *
 *                                                                           *
 *        -- Receive�� �� Blocking �Ǿ� �ִ� d_msgsnd() �� ������            *
 *             �ش� Fifo�� ���� Message �۽� --                              *
 *                                                                           *
 *****************************************************************************/
int	wakeup_snd(int shmid, int semid, int size, char *qpath)
{
	int		ix, jx, bix, ffd, use, beforeid, ret	= 0;
	char	fifopath[QPATH_LEN + QNAME_LEN + 4 + 3];
	void	*shmptr;
	struct	d_msgwhd_ds		*wheadptr;
	struct	d_msgwait_ds	*tmpptr	= NULL;
	struct	timeval tv;
	fd_set	fds;

	if (LOCK(semid) < 0)
	{
#ifdef _LIB_DEBUG
		printf("[wakeup_snd] LOCK fail.\n");
#endif
		return(-1);
	}

	if ((shmptr = shmat(shmid, 0, 0)) == (void *)-1)
	{
#ifdef _LIB_DEBUG
		perror("[wakeup_snd] shmat");
#endif
		return(-2);
	}

	wheadptr	= (struct d_msgwhd_ds *)shmptr;
	tmpptr		= (struct d_msgwait_ds *) ((char *)shmptr + sizeof(struct d_msgwhd_ds));

	if (wheadptr->d_msg_ss == -1)
	{
		UNLOCK(semid);
		shmdt(shmptr);
		return(-3);

	} else ix = wheadptr->d_msg_ss;

	sprintf(fifopath, "%s/%s.%s", qpath, tmpptr[ix].fifoname, FIFO_EXP);

	if ((ffd = open(fifopath, O_RDWR)) < 0)
	{
#ifdef _LIB_DEBUG
		perror("[wakeup_snd] open");
#endif
		ret = -1;
	}

	if (ret == 0)
	{
		FD_ZERO(&fds);
		FD_SET(ffd, &fds);
		tv.tv_sec = 1;
		tv.tv_usec = 0;

		if(select(ffd + 1, &fds, NULL, NULL, &tv) <= 0)
			ret = -1;
	}

	close(ffd);
	unlink(fifopath);

	if(wheadptr->d_msg_ss == ix)  {
		if(wheadptr->d_msg_se == ix)
		{
			wheadptr->d_msg_ss = wheadptr->d_msg_se = -1;
			tmpptr[ix].nextid = -1;

		} else {
			wheadptr->d_msg_ss = tmpptr[ix].nextid;
		}
	} else {
		bix = beforeid = wheadptr->d_msg_ss;
		while(beforeid != ix)  {
			bix = beforeid;
			beforeid = tmpptr[beforeid].nextid;
		}
		tmpptr[bix].nextid = tmpptr[ix].nextid;
		if(wheadptr->d_msg_se == ix)
			wheadptr->d_msg_se = bix;
	}
	tmpptr[ix].use = 0;

	shmdt(shmptr);
	UNLOCK(semid);
	return(ret);
}
/**************************< END OF "wakeup_snd">*****************************/
