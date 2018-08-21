/*******************<FILE HEADER>*********************************************
 *--<SYSTEM>-----------------------------------------------------------------*
 *   DISK QUEUE                                                              *
 *                                                                           *
 *--<SUBSYSTEM>--------------------------------------------------------------*
 *   �ý��� - ����                                                           *
 *                                                                           *
 *--<FILE NAME>--------------------------------------------------------------*
 *   d_msgctl.c                                                              *
 *                                                                           *
 *--<DESCRIPTION>------------------------------------------------------------*
 *                                                                           *
 *   msgctl() ��� ����                                                      *
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
 * Prototype : int d_msgrcv(int msqid, int command,							*
 *			struct msqid_ds *msq_stat)										*
 *																			*
 * ���� : Message Queue �� ���� ������ ��ų�, ������ �����ϰų�, ����		*
 *																			*
 * �μ� : const int msqid													*
 *			Message Queue�� ���� ��ȣ										*
 *		  int command														*
 *			����� �۾�														*
 *			IPC_STAT : ���������� msq_stat �� �ִ´�.						*
 *			IPC_SET  : Message Queue�� ����  ������� ���� �����Ѵ�.		*
 *					   msq_stat.msg_perm.uid								*
 *					   msq_stat.msg_perm.gid								*
 *					   msq_stat.msg_perm.mode								*
 *					   msq_stat.msg_qbyte									*
 *			IPC_RMID : Message Queue�� �����Ѵ�.							*
 *		  struct msqid_ds													*
 *																			*
 * ���� : (==  0) ����														*
 *		  (== -1) ����														*
 *																			*
 * �ۼ� : 2001.12.21.														*
 * ���� :																	*
 ****************************************************************************/
int d_msgctl(int msqid, int command, struct d_msqid_ds *msq_stat)
{
	struct d_msqid_ds   d_msqid;			/* Queue Header ����		*/

	char	q_full_name[256];				/* Queue File Name			*/
	int		cfd, dfd;						/* Control/Data File Fd		*/
	int		nleft, nwsz, size, ret;

	char	buf[BUFSIZ];

	/*==================== Queue ID  Ȯ�� ====================*/
	if (msqid <= 0 || msqid > D_OpenQNum)  {
		errno = EIDRM;
		printf ("[d_msgrcv] msqid Check : msqid=%d ����! \n", msqid);
		return(-1);
	}

	/*========== Queue Header ���� ���´� ==========*/
	sprintf(buf, "%s/%s.%s", D_msqinfo[msqid-1].d_msg_fpath, D_msqinfo[msqid-1].d_msg_qname, CTL_EXP);

	if((cfd = GetQHeader(buf, &d_msqid, msqid)) <= 0)  {
		printf ("[d_msgctl] GetQHeader \n");
		return(-2);
	}

	switch(command)  {
		case IPC_STAT:
/*
printf ("d_msqid.d_msg_perm.mode: [%#o]\n", d_msqid.d_msg_perm.mode & IPC_PERM);
printf ("d_msqid.d_msg_perm.uid : [%ld]\n",   d_msqid.d_msg_perm.uid);
printf ("d_msqid.d_msg_perm.gid : [%ld]\n",   d_msqid.d_msg_perm.gid);
printf ("d_msqid.d_msg_cbytes   : [%d]\n",    d_msqid.d_msg_cbytes);
printf ("d_msqid.d_msg_qbytes   : [%d]\n",    d_msqid.d_msg_qbytes);
printf ("d_msqid.d_msg_qnum     : [%d]\n",    d_msqid.d_msg_qnum  );
printf ("d_msqid.d_msg_lrpid    : [%d]\n",    d_msqid.d_msg_lrpid );
printf ("d_msqid.d_msg_rtime    : [%ld]\n",   d_msqid.d_msg_rtime );
printf ("d_msqid.d_msg_ctime    : [%ld]\n\n", d_msqid.d_msg_ctime );
*/
			memcpy (msq_stat, &d_msqid, sizeof (struct d_msqid_ds));

			break;
		case IPC_SET :
			/*-------- UID / GID ���� ----------*/
			if(d_msqid.d_msg_perm.uid != msq_stat->d_msg_perm.uid || d_msqid.d_msg_perm.gid != msq_stat->d_msg_perm.gid)  {
				d_msqid.d_msg_perm.uid  = msq_stat->d_msg_perm.uid;
				d_msqid.d_msg_perm.gid  = msq_stat->d_msg_perm.gid;

				sprintf (q_full_name, "%s/%s.%s", D_msqinfo[msqid-1].d_msg_fpath,
					D_msqinfo[msqid-1].d_msg_qname, CTL_EXP);
				chown (q_full_name, msq_stat->d_msg_perm.uid,
					msq_stat->d_msg_perm.gid);

				sprintf (q_full_name, "%s/%s.%s", D_msqinfo[msqid-1].d_msg_fpath,
					D_msqinfo[msqid-1].d_msg_qname, DAT_EXP);
				chown (q_full_name, msq_stat->d_msg_perm.uid, msq_stat->d_msg_perm.gid);

				sprintf (q_full_name, "%s/%s.%s", D_msqinfo[msqid-1].d_msg_fpath,
					D_msqinfo[msqid-1].d_msg_qname, IPC_EXP);
				chown (q_full_name, msq_stat->d_msg_perm.uid, msq_stat->d_msg_perm.gid);
			}
			/*-------- Mode ���� ----------*/
			d_msqid.d_msg_perm.mode = msq_stat->d_msg_perm.mode;

			/*------- Qbytes ���� ---------*/
			if(d_msqid.d_msg_qbytes != msq_stat->d_msg_qbytes)  {
				if(d_msqid.d_msg_qbytes > msq_stat->d_msg_qbytes)  {
					printf ("[d_msgctl] ���� Queue ���� ���� QSIZE�Դϴ�.!\n");
					return(-3);
				}

				nleft = msq_stat->d_msg_qbytes - d_msqid.d_msg_qbytes ;

				LOCK(D_msqinfo[msqid-1].d_msg_semid);

				dfd = D_msqinfo[msqid-1].d_msg_dfd;

				lseek(dfd, 0, SEEK_END);

				while(nleft > 0)  {
					memset(buf, 0x20, BUFSIZ); buf[BUFSIZ] = 0x00;
					if((nwsz=write(dfd, buf, ((nleft>BUFSIZ) ? BUFSIZ:nleft))) <= 0)  {
						printf ("[d_msgctl] write(1)  \n");
						UNLOCK(D_msqinfo[msqid-1].d_msg_semid);
						return(-4);
					}
					nleft -= nwsz;
				}

				d_msqid.d_msg_qbytes  = msq_stat->d_msg_qbytes;
				cfd = D_msqinfo[msqid-1].d_msg_cfd;

				lseek(cfd, 0, SEEK_SET);
				if((nwsz = write (cfd, &d_msqid, QHEAD_SZ)) < 0)  {
					printf ("[d_msgctl] Queue �������� ����!\n");
					UNLOCK(D_msqinfo[msqid-1].d_msg_semid);
					return(-5);
				}

				memcpy (&D_msqinfo[msqid-1], &d_msqid, QHEAD_SZ);
				UNLOCK(D_msqinfo[msqid-1].d_msg_semid);

				/* Blocking �� d_msgsnd() wakeup */
				while (1)
				{
					ret = wakeup_snd(D_msqinfo[msqid-1].d_msg_shmid,
									 D_msqinfo[msqid-1].d_msg_semid, size,
									 D_msqinfo[msqid-1].d_msg_fpath);

					if (ret == 0) break;
					else if (ret == -1) continue;
					else {
#ifdef _LIB_DEBUG
						printf("[d_msgrcv]wakeup_snd(1) fail.\n");
#endif
						break;
					}
				}
			}

			break;

		case IPC_RMID:
			/*---------- Control File ���� ---------*/
			sprintf(q_full_name, "%s%s.%s", D_msqinfo[msqid-1].d_msg_fpath,
				D_msqinfo[msqid-1].d_msg_qname, CTL_EXP);
			unlink(q_full_name);

			/*---------- Data    File ���� ---------*/
			sprintf(q_full_name, "%s%s.%s", D_msqinfo[msqid-1].d_msg_fpath,
				D_msqinfo[msqid-1].d_msg_qname, DAT_EXP);
			unlink(q_full_name);

			sprintf(q_full_name, "%s%s.%s", D_msqinfo[msqid-1].d_msg_fpath,
				D_msqinfo[msqid-1].d_msg_qname, IPC_EXP);
			unlink(q_full_name);

			/*----------- Send / Recv ���� Wakeup ----------*/
			wakeup_wait(D_msqinfo[msqid-1].d_msg_shmid,
						D_msqinfo[msqid-1].d_msg_semid,
						D_msqinfo[msqid-1].d_msg_fpath, 0);

			wakeup_wait(D_msqinfo[msqid-1].d_msg_shmid,
						D_msqinfo[msqid-1].d_msg_semid,
						D_msqinfo[msqid-1].d_msg_fpath, 1);

			/*----------- Shared Memory/Semaphore ���� ----------*/
			sprintf(q_full_name, "ipcrm -m %d; ipcrm -s %d",
				D_msqinfo[msqid-1].d_msg_shmid, D_msqinfo[msqid-1].d_msg_semid);
			system(q_full_name);

			break;
	}

	return(0);
}
/************************ < END OF d_msgctl > ********************************/
