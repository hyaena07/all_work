#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <limits.h>
#include <time.h>
#include <string.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <sys/stat.h>
#include <sys/types.h>

#define LOCK(a)				d_sem_opr(a, 0, 1)
#define UNLOCK(a)			d_sem_opr(a, 0, 2)

#define SND_WAIT_LOCK(a)	d_sem_opr(a, 1, 1)
#define SND_WAIT_UNLOCK(a)	d_sem_opr(a, 1, 2)

#define RCV_WAIT_LOCK(a)	d_sem_opr(a, 2, 1)
#define RCV_WAIT_UNLOCK(a)	d_sem_opr(a, 2, 2)

#define FLOCK(i)			d_locking(i, 1)
#define FUNLOCK(i)			d_locking(i, 2)

#define QNAME_LEN			 8					/* Queue Name�� �ִ� ���� ��						*/
#define QPATH_LEN			80					/* Queue ���� ���� ���								*/
#define FIFONAME_LEN		 8					/* FIFO �̸��� �ִ� ���� ��							*/

#define MAX_WAIT			  50				/* d_msgsnd(), d_msgrcv()�� ���� ��⿭�� �ִ� ��	*/
#define MAX_MESG			4096				/* Queue �� ���� �ִ� Message ��				*/
#define MAX_CNT				9007199254740992.0	/* �Է� �޼��� ���� ī��Ʈ�� ����� �ִ� ��			*/

#define INIT_MSG_NUM		10					/* MSQ ������ # of messages on queue				*/

#define QHEAD_SZ	sizeof(struct d_msqid_ds)	/* Queue Header Size								*/
#define MHEAD_SZ	sizeof(struct d_msghd_ds)	/* Message Header Size								*/

#define CTL_PERM	S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH
#define DAT_PERM	S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH

#define RCV_PERM_CHK	0001
#define SND_PERM_CHK	0002

#define ROOT_UID		0
#define ROOT_GID		0

#define CTL_EXP			"ctl"
#define DAT_EXP			"dat"
#define IPC_EXP			"ipc"
#define FIFO_EXP		"fifo"

/*=================== Control File��  Queue Header ����ü ====================*/
struct d_msqid_ds {
	struct ipc_perm	d_msg_perm;
	ulong			d_msg_cbytes;	/* current # bytes on queue				*/
	ulong			d_msg_qnum;		/* # of messages on queue				*/
	ulong			d_msg_qbytes;	/* max # of bytes on queue				*/
	int				d_msg_ds;		/* Message ���� ID						*/
	int				d_msg_de;		/* Message ��   ID						*/
	int				d_msg_es;		/* Empty   ���� ID						*/
	int				d_msg_ee;		/* Empty   ��   ID						*/
	pid_t			d_msg_lspid;	/* pid of last d_msgsnd()				*/
	pid_t			d_msg_lrpid;	/* pid of last d_msgrcv()				*/
	time_t			d_msg_stime;	/* last-d_msgsnd() time					*/
	time_t			d_msg_rtime;	/* last-d_msgrcv() time					*/
	time_t			d_msg_ctime;	/* last-change time						*/
	double			d_msg_tcnt;		/* total # of messages in queue			*/
};

/*=================== Control File��  Message Header ����ü ==================*/
struct d_msghd_ds {
	int				d_msg_id;		/* message id							*/
	long			d_msg_mtype;	/* message type							*/
	unsigned short	d_msg_alive;	/* message alive flag (0 or 1)			*/
	unsigned int	d_msg_whence;	/* message start offset	on data file	*/
	unsigned int	d_msg_mbytes;	/* message length						*/
	double			d_msg_uid;		/* message unique id					*/
	time_t			d_msg_stime;	/* message send time					*/
	int				d_msg_nextid;	/* ���� Message ID						*/

};

/*================== ���μ��� ������ ������ MSQ ���� ���� ====================*/
struct d_msqinfo_ds {
	struct	ipc_perm d_msg_perm;
	char	d_msg_qname[QNAME_LEN +	1];	/* MSQ �̸�							*/
	char	d_msg_fpath[QPATH_LEN +	1];	/* MSQ ���� ���ϵ��� ��ġ ���		*/
	int		d_msg_cfd;					/* Control file	fd					*/
	int		d_msg_dfd;					/* Data file fd						*/
	ulong	d_msg_qbytes;				/* max # of bytes on queue			*/
	int		d_msg_shmid;				/* Shared Memory ID					*/
	int		d_msg_semid;				/* Semaphore ID						*/
};

/*========== �޸𸮻󿡼� ������ ������ ���� �����ϱ����� ����ü =============*/
struct d_msgmap_ds  {
	int					 d_msg_id;	   /* message id						*/
	unsigned short		 d_msg_dt;	   /* Space(0)���� Data ����(1)���� ����*/
	ulong				 d_msg_soff;   /* ���� offset						*/
	ulong				 d_msg_eoff;   /* �� offset							*/
	struct d_msgmap_ds	*prev;		   /* ���� ��ũ ������					*/
	struct d_msgmap_ds	*next;		   /* ���� ��ũ ������					*/
};

/*========================= ���� �޸� ��� ���� ============================*/
struct d_msgwhd_ds  {
	int	d_msg_rs;
	int	d_msg_re;
	int	d_msg_ss;
	int	d_msg_se;
};

/*===================== ���� �޸𸮻��� ��⿭ ����ü ========================*/
struct d_msgwait_ds  {
	int		use;						/* 0:������� ����, 1:��� ��	*/
	long	mtype;
	char	fifoname[FIFONAME_LEN + 1];
	int		nextid;
};

struct d_msqinfo_ds	*D_msqinfo;			/* Message Queue	Infomation	*/

int	D_OpenQNum;							/* ������ MSQ ����				*/

/*===== d_common.c =====*/
int d_CheckControlFile(int, struct d_msqid_ds *);
int d_CheckPerm(struct ipc_perm, int);
char *d_MakeFIFOName(char *);
int d_ReadQHead(int, struct d_msqid_ds *);
int d_WriteQHead(int, struct d_msqid_ds *);
int d_ReadMHead(int, int, struct d_msghd_ds *);
int d_WriteMHead(int, int, struct d_msghd_ds *);
int d_TotalCntSet(int, double);
int d_msgview(int, int, int, double, unsigned int, int);
int d_sem_opr(int, int, int);
int d_LockSigAll(sigset_t *);
int d_UnLockSigAll(sigset_t *);
int d_IsSigCatch(void);
int d_FLock(int , int);
int wakeup_wait(int, int, char *, int);
void view_map(struct d_msgmap_ds *);
void print_qinfo();
void hd(char *, int);

/*===== d_msgget.c =====*/
int d_OpenControlFile(char *, struct d_msqid_ds *);
int d_MakeDataFile(char *, unsigned long, mode_t);
int d_MakeControlFile(struct d_msqid_ds *, char *, int, long);
int d_AddQList(char *, char *, int, int, ulong, int, int, struct ipc_perm);
int d_OpenDataFile(char *, unsigned long);
int d_msgget(char *, const char *, int, long);

/*===== d_msgsnd =====*/
int d_MoveFData(int, struct d_msgmap_ds *);
int d_DataCompress_1(int, struct d_msgmap_ds *, int);
int d_MoveOneData(int, struct d_msgmap_ds *, struct d_msgmap_ds *);
int d_DataCompress_2(int, struct d_msgmap_ds *, int);
int d_DataCompress_3(int, struct d_msgmap_ds *, int);
struct d_msgmap_ds *d_MakeDataArray(int, struct d_msgmap_ds *, struct d_msqid_ds);
int d_FindSpace(int, struct d_msqid_ds, int, int *);
int d_msgsnd_main(int, void *, int, int);
int d_WakeupRcv(int, int, long);
int d_WaitSnd(int, int);
int d_FindWaitSnd(int);
int d_msgsnd(int, void *,int, int);

/*===== d_msgrcv =====*/
int	d_msgrcv		();				/* msgrcv()								*/
int	read_data		();				/* Message Read / Control File Update	*/
int	wait_for_rcv	();				/* recv blocking						*/
int	wakeup_snd		();				/* blocking �� d_msgsnd() ȣ��			*/

/*=====	d_msgctl =====*/
int	d_msgctl		();				/* msgctl()								*/
