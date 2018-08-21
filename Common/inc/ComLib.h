#ifndef _CCOMMON_H
#define _CCOMMON_H

#include <errno.h>
#include <stdarg.h>
#include <sys/shm.h>
#include <sys/time.h>
#include <sys/types.h>
#include <netinet/in.h>

#define MAX_PATH_LEN	1024
#define MAX_FILE_LEN	1024
#define	MAX_ERRO_LEN	1024

extern char	errmsg[MAX_ERRO_LEN + 1];
/****************************************************************************/
/* DateTime.c																*/
/****************************************************************************/
float	GetElapsedTime		(struct timeval *);
void	GetElapsedTime_Begin(struct timeval *);
float	GetElapsedTime_End	(struct timeval *);

char*	getDateTime_14		(void);
char*	getDate_s			(void);
char*	getWeekName			(void);
char*	getTime				(void);
char*	getTime_s			(void);
int		getWeekNo			(void);
char*	CalcDate			(char *, int);

/****************************************************************************/
/* File.c																	*/
/****************************************************************************/
#define Read_lock(fd, offset, len)	lock_reg(fd, F_SETLK,  F_RDLCK, offset, SEEK_SET, len)
#define Readw_lock(fd, offset, len)	lock_reg(fd, F_SETLKW, F_RDLCK, offset, SEEK_SET, len)
#define Write_lock(fd, offset, len)	lock_reg(fd, F_SETLK,  F_WRLCK, offset, SEEK_SET, len)
#define Writew_lock(fd, offset, len)lock_reg(fd, F_SETLKW, F_WRLCK, offset, SEEK_SET, len)
#define Un_lock(fd, offset, len)	lock_reg(fd, F_SETLK,  F_UNLCK, offset, SEEK_SET, len)

int		CopyFile			(const void *, const void *);
int		CopyFile2			(const void *, const void *);
int		lock_reg			(int, int, int, off_t, int, off_t);
ssize_t	Writen				(int, const void *, size_t);
ssize_t	Readn				(int, void *, size_t);
int		LockFile			(char *);
int		unLockFile			(char *);

/****************************************************************************/
/* IPC.c																	*/
/****************************************************************************/
#define	FF_BLOCK	0x0001

#define	JPS_NAM		"JPCOMD"
#define	JPS_MAX		250

#define	JPS_LOCK(semno)		SemOpr(JPS_NAM, JPS_MAX, semno, 1)
#define	JPS_UNLOCK(semno)	SemOpr(JPS_NAM, JPS_MAX, semno, 2)
#define	JPS_CHECK(semno)	SemOpr(JPS_NAM, JPS_MAX, semno, 3)

int		CreatMsq			(char *, int);
int		ClearMsq			(int, long);
int		CreatShm			(char *, int , int);
int		Signal				(int, void *);
int		SendFIFO			(char *, char *, int, int, int);
int		SemOpr				(char *, int, int, int);

/****************************************************************************/
/* Log.c																	*/
/****************************************************************************/
int		LogRotate			(const char *, int, int);
int		WriteLog			(const void *, int, int, const void *, va_list);
int		WriteDailyLog		(const char *, const char *, const char *, ...);

/****************************************************************************/
/* Network.c																*/
/****************************************************************************/
struct FTrans_hd  {
	char	fname[MAX_FILE_LEN];
	long	flen;
	long	fmode;
	long	pksize;
};

int		Bind				(int, int, struct sockaddr_in *);
int		Bind_Force			(int, int, struct sockaddr_in *);
int		Accept				(int, long int, long int, struct sockaddr_in *);
int		isIPAddr			(const char *);
int		Connect				(const char *, int, struct sockaddr_in *);
int		Connect_sa			(struct sockaddr_in *);
int		ConnectTimeout		(const char *, int, struct sockaddr_in *, int);
int		ConnectTimeout_sa	(struct sockaddr_in *, int);
int		CheckDisconnect		(int);
int		Sendn				(int, const void *, size_t, int);
int		Recvn				(int, void *, size_t, int);
int		Recvw				(int, void *, size_t, int, int);
int		SetBlockMode		(int);
int		SetNonBlockMode		(int);
int		SendFile			(int, char *, int);
int		RecvFile			(int, char *);

/****************************************************************************/
/* Process.c																*/
/****************************************************************************/
pid_t	SavePID				(const void *);
void	DelePID				(const void *);
pid_t	ReadPID				(const void *);
pid_t	RunProg				(const char *, const char *);
void	toBackground		(void);

/****************************************************************************/
/* String.c																	*/
/****************************************************************************/
#define TRIM(str)	RTrim(LTrim(str))

char*	MemStr				(char *, char *, int);
char*	LTrim				(char *);
char*	RTrim				(char *);
char*	RemoveAllSpace		(char *);
char*	RemoveDoubleSpace	(char *);
char*	Long2Str			(long, int);
void	Upp2Low				(char *);
void	Low2Upp				(char *);
int		isDecimal			(char *);
long	s2l					(char *, int);
float	s2f					(char *, int);

/****************************************************************************/
/* System																	*/
/****************************************************************************/
char*	GetTerminalName		(void);
char*	GetIP				(void);

#endif	/* _COMMON_H	*/
