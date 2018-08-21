#ifndef _JPCOMD_H
#define _JPCOMD_H

#define	MAX_PROCSCHD_NUM	100
#define	MAX_BATSJOBS_NUM	100

void	LOG_SYS		(char *, ...);
void	LOG_TRC		(char *, ...);
void	LOG_TMP		(char *, ...);

int		LockFile	(char *);
int		unLockFile	(char *);

struct stConfigParse  {
	char ini_name[64];
	char ini_data[256];
	char ini_pat[256];
};

int	ConfigParse(char *, struct stConfigParse *);

typedef	struct  {
				int		stime;
				int		etime;
				char	pname[MAX_FILE_LEN];
				char	args[MAX_PATH_LEN];
				pid_t	pid;
	unsigned	int		run_cnt;
	unsigned	int		kil_cnt;
}	sSchdDaly;
#define SZ_sSchdDaly	(sizeof(sSchdDaly))

typedef struct  {
				int		stime;
				char	pname[MAX_FILE_LEN];
				char	args[MAX_PATH_LEN];
				pid_t	pid;
	unsigned	int		run_cnt;
				int		status;
}	sBatsDaly;
#define SZ_sBatsDaly	(sizeof(sBatsDaly))

typedef struct  {
				int		weekn;
				int		stime;
				char	pname[MAX_FILE_LEN];
				char	args[MAX_PATH_LEN];
				pid_t	pid;
	unsigned	int		run_cnt;
				int		status;
}	sBatsWeek;
#define SZ_sBatsWeek	(sizeof(sBatsWeek))

typedef struct  {
				int		day;
				int		stime;
				char	pname[MAX_FILE_LEN];
				char	args[MAX_PATH_LEN];
				pid_t	pid;
	unsigned	int		run_cnt;
				int		status;
}	sBatsMnth;
#define SZ_sBatsMnth	(sizeof(sBatsMnth))

typedef struct  {
				int		mmdd;
				int		stime;
				char	pname[MAX_FILE_LEN];
				char	args[MAX_PATH_LEN];
				pid_t	pid;
	unsigned	int		run_cnt;
				int		status;
}	sBatsYear;
#define SZ_sBatsYear	(sizeof(sBatsYear))

typedef struct  {
				int		year;
				int		mont;
				int		mday;
				int		wday;
				int		hour;
				int		mint;
				char	pname[MAX_FILE_LEN];
				char	args[MAX_PATH_LEN];
				pid_t	pid;
	unsigned	int		run_cnt;
				int		status;
}	sBatchJob;
#define SZ_sBatchJob	(sizeof(sBatchJob))

#endif	// _JPCOMD_H

