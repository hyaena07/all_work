#include <stdio.h>
#include <fcntl.h>
#include <regex.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <stdarg.h>
#include <dirent.h>
#include <libgen.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>

#include "ComLib.h"
#include "jpcomd.h"

char	g_syslogpath	[MAX_PATH_LEN + 1];		// System �α� ��� �� ���λ�
char	g_trclogpath	[MAX_PATH_LEN + 1];		// Trace �α� ��� �� ���λ�
char	g_tmplogpath	[MAX_PATH_LEN + 1];		// �ڽ� ���μ����� ���� �ӽ� �α� ��� �� ���λ�
char	g_configpath	[MAX_PATH_LEN + 1];		// ȯ�� ���� ���
char	g_pidpath		[MAX_PATH_LEN + 1];		// pid ���� ���� ���
char	g_fifostatus	[MAX_PATH_LEN + 1];		// ���� ���� ���ۿ� fifo ��� ��
char	g_procname		[MAX_FILE_LEN + 1];		// ���μ�����

int		g_schddalynum;

int		g_batsdalynum;
int		g_batsweeknum;
int		g_batsmnthnum;
int		g_batsyearnum;

int		g_termflag		= 0;
int		g_reloadflag	= 0;
int		g_chkrunflag	= 0;

char	errmsg[MAX_ERRO_LEN + 1];

sSchdDaly	g_sSchdDaly[MAX_PROCSCHD_NUM];

sBatsDaly	g_sBatsDaly[MAX_BATSJOBS_NUM];
sBatsWeek	g_sBatsWeek[MAX_BATSJOBS_NUM];
sBatsWeek	g_sBatsMnth[MAX_BATSJOBS_NUM];
sBatsYear	g_sBatsYear[MAX_BATSJOBS_NUM];

void FuncSIGDUMY(int signo)
{
	LOG_SYS("[%s] signal %d ���� !!!", g_procname, signo);

	return;
}

int BlockSignal(void)
{
	sigset_t	bsig;

	sigemptyset(&bsig);

	Signal(SIGTERM, (void *)FuncSIGDUMY);
	Signal(SIGCHLD, (void *)FuncSIGDUMY);
	Signal(SIGUSR1, (void *)FuncSIGDUMY);
	Signal(SIGUSR2, (void *)FuncSIGDUMY);

	if(sigaddset(&bsig, SIGTERM) < 0)  {
		return(-1);
	}

	if(sigaddset(&bsig, SIGCHLD) < 0)  {
		return(-2);
	}

	if(sigaddset(&bsig, SIGUSR1) < 0)  {
		return(-3);
	}

	if(sigaddset(&bsig, SIGUSR2) < 0)  {
		return(-4);
	}

	if(sigprocmask(SIG_BLOCK, &bsig, (sigset_t *)NULL) < 0)  {
		return(-5);
	}

	return(0);
}

void DeleteSignal(sigset_t *pend)
{
	sigset_t	old_mask;

	sigprocmask(SIG_UNBLOCK, pend, &old_mask);

	sigprocmask(SIG_BLOCK, &old_mask, (sigset_t *)NULL);
}

void CheckSignal(void)
{
	sigset_t	pend;

	sigemptyset(&pend);

	if(sigpending(&pend) < 0)  {
		return;
	}

	if(sigismember(&pend, SIGTERM) == 1)  {
		LOG_SYS("[%s] signal SIGTERM ���� !!!", g_procname);
		g_termflag = 1;
	}

	if(sigismember(&pend, SIGCHLD) == 1)  {
		LOG_SYS("[%s] signal SIGCHLD ���� !!!", g_procname);
		g_chkrunflag = 1;
	}

	if(sigismember(&pend, SIGUSR1) == 1)  {
		LOG_SYS("[%s] signal SIGUSR1 ���� !!!", g_procname);
		g_reloadflag = 1;
	}

	if(sigismember(&pend, SIGUSR2) == 1)  {
		LOG_SYS("[%s] signal SIGUSR2 ���� !!!", g_procname);
	}

	DeleteSignal(&pend);

	return;
}

int CheckEnv(void)
{
	int		errflag = 0;

	if(getenv("JP_HOME") == (char *)NULL)  {
		printf(">> ȯ�溯�� [JP_HOME]�� ������ �Ŀ� �����Ͻʽÿ�.\n");
		errflag = 1;
	}

	if(getenv("JP_CONF") == (char *)NULL)  {
		printf(">> ȯ�溯�� [JP_CONF]�� ������ �Ŀ� �����Ͻʽÿ�.\n");
		errflag = 1;
	}

	if(getenv("JP_LOGS") == (char *)NULL)  {
		printf(">> ȯ�溯�� [JP_LOGS]�� ������ �Ŀ� �����Ͻʽÿ�.\n");
		errflag = 1;
	}

	if(getenv("JP_IPCS") == (char *)NULL)  {
		printf(">> ȯ�溯�� [JP_IPCS]�� ������ �Ŀ� �����Ͻʽÿ�.\n");
		errflag = 1;
	}

	if(errflag != 0)
		return(-1);
	else
		return(0);
}

int PrintStatus(char *fifo_path)
{
	char	startmsg[] = "Pleas Status";

	if(SendFIFO(fifo_path, startmsg, strlen(startmsg), 0, 2) != strlen(startmsg))  {
		LOG_SYS("[%s] SendFIFO() fail. %s", g_procname, errmsg);
		return(-1);
	}

	return(0);
}

int CheckRunProc(void)
{
	int		i, wait_status;
	pid_t	pid;

	for(i = 0; i < g_schddalynum; i ++)  {
		if(g_sSchdDaly[i].pid > 0)  {
			// ���� ���� ���α׷��� ���� ���� �˻縦 �Ѵ�.
			pid = waitpid(g_sSchdDaly[i].pid, &wait_status, WNOHANG);
			if(g_sSchdDaly[i].pid == pid)  {
				g_sSchdDaly[i].pid = 0;
				LOG_SYS("[%s] [%s] [%s] ���� Ȯ�� [%d]", g_procname, g_sSchdDaly[i].pname, g_sSchdDaly[i].args, WEXITSTATUS(wait_status));
			}
		}
	}

	for(i = 0; i < g_batsdalynum; i ++)  {
		if(g_sBatsDaly[i].pid > 0)  {
			// ���� ���� ���α׷��� ���� ���� �˻縦 �Ѵ�.
			pid = waitpid(g_sBatsDaly[i].pid, &wait_status, WNOHANG);
			if(g_sBatsDaly[i].pid == pid)  {
				g_sBatsDaly[i].pid = 0;
				g_sBatsDaly[i].status = wait_status;
				LOG_SYS("[%s] [%s] [%s] ���� Ȯ�� [%d]", g_procname, g_sBatsDaly[i].pname, g_sBatsDaly[i].args, WEXITSTATUS(wait_status));
			}
		}
	}

	for(i = 0; i < g_batsweeknum; i ++)  {
		if(g_sBatsWeek[i].pid > 0)  {
			// ���� ���� ���α׷��� ���� ���� �˻縦 �Ѵ�.
			pid = waitpid(g_sBatsWeek[i].pid, &wait_status, WNOHANG);
			if(g_sBatsWeek[i].pid == pid)  {
				g_sBatsWeek[i].pid = 0;
				g_sBatsWeek[i].status = wait_status;
				LOG_SYS("[%s] [%s] [%s] ���� Ȯ�� [%d]", g_procname, g_sBatsWeek[i].pname, g_sBatsWeek[i].args, WEXITSTATUS(wait_status));
			}
		}
	}

	for(i = 0; i < g_batsmnthnum; i ++)  {
		if(g_sBatsMnth[i].pid > 0)  {
			// ���� ���� ���α׷��� ���� ���� �˻縦 �Ѵ�.
			pid = waitpid(g_sBatsMnth[i].pid, &wait_status, WNOHANG);
			if(g_sBatsMnth[i].pid == pid)  {
				g_sBatsMnth[i].pid = 0;
				g_sBatsMnth[i].status = wait_status;
				LOG_SYS("[%s] [%s] [%s] ���� Ȯ�� [%d]", g_procname, g_sBatsMnth[i].pname, g_sBatsMnth[i].args, WEXITSTATUS(wait_status));
			}
		}
	}

	for(i = 0; i < g_batsyearnum; i ++)  {
		if(g_sBatsYear[i].pid > 0)  {
			// ���� ���� ���α׷��� ���� ���� �˻縦 �Ѵ�.
			pid = waitpid(g_sBatsYear[i].pid, &wait_status, WNOHANG);
			if(g_sBatsYear[i].pid == pid)  {
				g_sBatsYear[i].pid = 0;
				g_sBatsYear[i].status = wait_status;
				LOG_SYS("[%s] [%s] [%s] ���� Ȯ�� [%d]", g_procname, g_sBatsYear[i].pname, g_sBatsYear[i].args, WEXITSTATUS(wait_status));
			}
		}
	}

	return(0);
}

void TermProc(void)
{
	int		i, wait_status;
	pid_t	pid;

	LOG_SYS("[%s] ---- ���μ��� ���� ���� ���� ...", g_procname);

	// ���� ���� ��� ���μ����� ���� �ñ׳� ����
	for(i = 0; i < 	g_schddalynum; i ++)  {
		if(g_sSchdDaly[i].pid > 0)  {
			kill(g_sSchdDaly[i].pid, SIGTERM);
			LOG_SYS("[%s] [%s] ���� ���� ��ȣ ����. (pid:%d)", g_procname, g_sSchdDaly[i].pname, g_sSchdDaly[i].pid);
		}
	}

	sleep(1);

	// ���� ���� Ȯ��
	for(i = 0; i < 	g_schddalynum; i ++)  {
		if(g_sSchdDaly[i].pid > 0)  {
			pid = waitpid(g_sSchdDaly[i].pid, &wait_status, WNOHANG);
			if(g_sSchdDaly[i].pid == pid)  {
				g_sSchdDaly[i].pid = 0;
				LOG_SYS("[%s] [%s] [%s] ���� Ȯ�� [%d].", g_procname, g_sSchdDaly[i].pname, g_sSchdDaly[i].args, wait_status);
			}
		}
	}

	// ������ �����ϱ� �����̹Ƿ� ��� ���μ����� ���� ���� ��Ų��.
	for(i = 0; i < 	g_schddalynum; i ++)  {
		if(g_sSchdDaly[i].pid > 0)  {
			kill(g_sSchdDaly[i].pid, SIGKILL);
			LOG_SYS("[%s] [%s] ���� ���� ��ȣ ����. (pid:%d)", g_procname, g_sSchdDaly[i].pname, g_sSchdDaly[i].pid);
		}
	}

	return;
}

int InitProc(void)
{
	bzero(g_sSchdDaly, MAX_PROCSCHD_NUM * SZ_sSchdDaly);
	LOG_SYS("[%s] schddaly.conf �б� ���� ...", g_procname);
	if((g_schddalynum = LoadSchdDaly(g_sSchdDaly)) < 0)  {
		LOG_SYS("[%s] LoadSchdDaly() fail.", g_procname);
		return(-1);
	}

	bzero(g_sBatsDaly, MAX_BATSJOBS_NUM * SZ_sBatsDaly);
	LOG_SYS("[%s] batsdaly.conf �б� ���� ...", g_procname);
	if((g_batsdalynum = LoadBatsDaly(g_sBatsDaly)) < 0)  {
		LOG_SYS("[%s] LoadBatsDaly() fail.", g_procname);
		return(-2);
	}

	bzero(g_sBatsWeek, MAX_BATSJOBS_NUM * SZ_sBatsWeek);
	LOG_SYS("[%s] batsweek.conf �б� ���� ...", g_procname);
	if((g_batsweeknum = LoadBatsWeek(g_sBatsWeek)) < 0)  {
		LOG_SYS("[%s] LoadBatsWeek() fail.", g_procname);
		return(-3);
	}

	bzero(g_sBatsMnth, MAX_BATSJOBS_NUM * SZ_sBatsMnth);
	LOG_SYS("[%s] batsmnth.conf �б� ���� ...", g_procname);
	if((g_batsmnthnum = LoadBatsMnth(g_sBatsMnth)) < 0)  {
		LOG_SYS("[%s] LoadBatsMnth() fail.", g_procname);
		return(-4);
	}

	bzero(g_sBatsYear, MAX_BATSJOBS_NUM * SZ_sBatsYear);
	LOG_SYS("[%s] batsyear.conf �б� ���� ...", g_procname);
	if((g_batsyearnum = LoadBatsYear(g_sBatsYear)) < 0)  {
		LOG_SYS("[%s] LoadBatsYear() fail.", g_procname);
		return(-5);
	}

	return(0);
}

void ProcMain()
{
	int		dCurrWeek, dTempWeek;
	char	sCurrDateTime[14 + 1];
	char	sTempDateTime[14 + 1];
	char	sTemp[1024];

	dCurrWeek = getWeekNo();							// ���� ���� ����
	sprintf(sCurrDateTime, "%.14s", getDateTime_14());	// ���� ��¥ ����

	// �÷��� ���� �� ó���� �۾� ��
	LOG_SYS("[%s] �÷��� ���� ��ġ ����.", g_procname);
	if(RunProgramList("plonboot.conf") < 0)  {
		LOG_SYS("[%s] RunProgramList() fail.", g_procname);
	}
	LOG_SYS("[%s] �÷��� ���� ��ġ ����.", g_procname);

	while(1)  {
		dTempWeek = getWeekNo();							// ���� ���� ����
		sprintf(sTempDateTime, "%.14s", getDateTime_14());	// ���� ��¥ ����

		snprintf(g_syslogpath, MAX_PATH_LEN, "%s/sys_%.8s.log", getenv("JP_LOGS"), sTempDateTime);
		snprintf(g_trclogpath, MAX_PATH_LEN, "%s/trc_%.8s.log", getenv("JP_LOGS"), sTempDateTime);

		CheckSignal();

		// ���� �ñ׳� ó��
		if(g_termflag)  {
			TermProc();
			break;
		}

		// ȯ�� �缳�� ó��
		if(g_reloadflag)  {
			LOG_SYS("[%s] schddaly.conf �б� ���� ...", g_procname);
			if(reLoadSchdDaly() < 0)
				LOG_SYS("[%s] reLoadSchdDaly() fail.", g_procname);

			bzero(g_sBatsDaly, MAX_BATSJOBS_NUM * SZ_sBatsDaly);
			LOG_SYS("[%s] batsdaly.conf �б� ���� ...", g_procname);
			if((g_batsdalynum = LoadBatsDaly(g_sBatsDaly)) < 0)
				LOG_SYS("[%s] LoadBatsDaly() fail.", g_procname);

			bzero(g_sBatsWeek, MAX_BATSJOBS_NUM * SZ_sBatsWeek);
			LOG_SYS("[%s] batsweek.conf �б� ���� ...", g_procname);
			if((g_batsweeknum = LoadBatsWeek(g_sBatsWeek)) < 0)
				LOG_SYS("[%s] LoadBatsWeek() fail.", g_procname);

			bzero(g_sBatsMnth, MAX_BATSJOBS_NUM * SZ_sBatsMnth);
			LOG_SYS("[%s] batsmnth.conf �б� ���� ...", g_procname);
			if((g_batsmnthnum = LoadBatsMnth(g_sBatsMnth)) < 0)
				LOG_SYS("[%s] LoadBatsMnth() fail.", g_procname);

			bzero(g_sBatsYear, MAX_BATSJOBS_NUM * SZ_sBatsYear);
			LOG_SYS("[%s] batsyear.conf �б� ���� ...", g_procname);
			if((g_batsyearnum = LoadBatsYear(g_sBatsYear)) < 0)
				LOG_SYS("[%s] LoadBatsYear() fail.", g_procname);

			g_reloadflag = 0;
		}

		// ���� ���μ��� �˻� �� ó��
		if(g_chkrunflag)  {
			CheckRunProc();

			g_chkrunflag = 0;
		}

		// �� ���� �� ó���� �۾� ��
		if(memcmp(sCurrDateTime, sTempDateTime, 4) != 0)  {
			InitBatsYear();
			LOG_SYS("[%s] ���ġ �۾��� ���� ���¸� �ʱ�ȭ.", g_procname);

			LOG_SYS("[%s] �⺯�� ��ġ ����.", g_procname);

			LOG_SYS("[%s] �⺯�� ��ġ ����.", g_procname);
		}

		// �� ���� �� ó���� �۾� ��
		if(memcmp(sCurrDateTime, sTempDateTime, 6) != 0)  {
			InitBatsMnth();
			LOG_SYS("[%s] ����ġ �۾��� ���� ���¸� �ʱ�ȭ.", g_procname);

			LOG_SYS("[%s] ������ ��ġ ����.", g_procname);

			LOG_SYS("[%s] ������ ��ġ ����.", g_procname);
		}

		// �� ���� �� ó���� �۾� ��
		if(dCurrWeek == 6 && dTempWeek == 0)  {
			InitBatsWeek();
			LOG_SYS("[%s] �ֹ�ġ �۾��� ���� ���¸� �ʱ�ȭ.", g_procname);

			LOG_SYS("[%s] �ֺ��� ��ġ ����.", g_procname);

			LOG_SYS("[%s] �ֺ��� ��ġ ����.", g_procname);
		}

		// �� ���� �� ó���� �۾� ��
		if(memcmp(sCurrDateTime, sTempDateTime, 8) != 0)  {
			// �Ϻ� ��ġ �۾� ����� �ʱ�ȭ �Ѵ�.
			InitBatsDaly();
			LOG_SYS("[%s] �Ϲ�ġ �۾��� ���� ���¸� �ʱ�ȭ.", g_procname);

			LOG_SYS("[%s] �Ϻ��� ��ġ ����.", g_procname);

			LOG_SYS("[%s] �Ϻ��� ��ġ ����.", g_procname);
		}

		// �� ����� ó���� �۾� ��
		if(memcmp(sCurrDateTime, sTempDateTime, 10) != 0)  {
			LOG_SYS("[%s] �ú��� ��ġ ����.", g_procname);

			LOG_SYS("[%s] �ú��� ��ġ ����.", g_procname);
		}

		// �� ����� ó���� �۾� ��
		if(memcmp(sCurrDateTime, sTempDateTime, 12) != 0)  {
			if(RunBatsDaly(sTempDateTime) < 0)
				LOG_SYS("[%s] CheckBatsDaly() fail.", g_procname);

			if(RunBatsWeek(dTempWeek, sTempDateTime) < 0)
				LOG_SYS("[%s] CheckBatsWeek() fail.", g_procname);

			if(RunBatsMnth(sTempDateTime) < 0)
				LOG_SYS("[%s] CheckBatsMnth() fail.", g_procname);

			if(RunBatsYear(sTempDateTime) < 0)
				LOG_SYS("[%s] CheckBatsYear() fail.", g_procname);
		}

		// ������� �۾���
		if(CheckSchdDaly(sTempDateTime) < 0)  {
			LOG_SYS("[%s] CheckSchdDaly() fail.", g_procname);
		}

		dCurrWeek = dTempWeek;
		memcpy(sCurrDateTime, sTempDateTime, 14);

		sleep(1);
	}
}

int main(int argc, char *argv[])
{
	pid_t	old_pid;

	// �ʿ��� ȯ�溯���� ��� ���� �Ǿ����� �˻�
	if(CheckEnv() < 0)  {
		exit(1);
	}

	if(argc != 2)  {
//		printf("Usage: %s {start|stop|status|restart|condrestart|reload|configtest|probe}\n", argv[0]);
		printf("Usage: %s {start|stop|reload|status}\n", argv[0]);
		exit(1);
	}

	snprintf(g_procname, 	MAX_FILE_LEN, "%s",					basename(argv[0]));
	snprintf(g_pidpath, 	MAX_PATH_LEN, "%s/%s.pid",			getenv("JP_IPCS"), g_procname);
	snprintf(g_configpath,	MAX_PATH_LEN, "%s",					getenv("JP_CONF"));
	snprintf(g_syslogpath, 	MAX_PATH_LEN, "%s/sys_%.8s.log",	getenv("JP_LOGS"), getDateTime_14());
	snprintf(g_trclogpath, 	MAX_PATH_LEN, "%s/trc_%.8s.log",	getenv("JP_LOGS"), getDateTime_14());
	snprintf(g_fifostatus, 	MAX_PATH_LEN, "%s/FIFO_STATUS",		getenv("JP_IPCS"));

	Upp2Low(argv[1]);

	if(strcmp(argv[1], "start") == 0)  {
		// �ߺ� ������ ���� ���� �̹� ���� ���� ���� ���μ����� �ִ��� �˻��Ѵ�.
		if(ReadPID(g_pidpath) >= 0)  {		// pid ������ �����ϰ�
			if(LockFile(g_pidpath) < 0)  {	// pid ���Ͽ� ��� ������ �� �� ���ٸ� �̹� ���� ���� ���μ����� �ִٰ� �Ǵ�.
				printf("[%s] is alrady run ...\n", g_procname);
				exit(1);
			} else  {						// pid ���Ͽ� ��� ������ �����ϸ� ���� ���μ����� ������ ������ ������ �Ǵ�.
				unLockFile(g_pidpath);
			}
		}
	} else if(strcmp(argv[1], "stop") == 0)  {
		// ���� ���� ���μ����� ���� �ñ׳��� �۽��ϰ� �����Ѵ�.
		if((old_pid = ReadPID(g_pidpath)) >= 0)  {
			kill(old_pid, SIGTERM);
		}
		exit(0);
	} else if(strcmp(argv[1], "reload") == 0)  {
		// ���� ���� ���μ����� USR1 �ñ׳��� �۽��ϰ� �����Ѵ�.
		if((old_pid = ReadPID(g_pidpath)) >= 0)  {
			kill(old_pid, SIGUSR1);
		}
		exit(0);
	} else if(strcmp(argv[1], "status") == 0)  {
		// ���� ���� ���μ����� ���¸� ���� �޾� ȭ�鿡 ����Ѵ�.
		if(PrintStatus(g_fifostatus) < 0)  {
			printf("PrintStatus() fail. [%s]\n", g_fifostatus);
		}
		exit(0);
	} else  {
		printf("Usage: %s {start|stop|reload|status}\n", argv[0]);
		exit(1);
	}

	LOG_SYS("[%s] ---- ���μ��� �ʱ�ȭ ���� ...", g_procname);
	if(InitProc() < 0)  {
		printf("InitProc() fail.\n");
		exit(1);
	}

	toBackground();

	// �ߺ� ������ �������� ���� ���μ����� pid�� �����ϰ� ��� ������ �Ѵ�.
	if(SavePID(g_pidpath) < 0)  {
		LOG_SYS("[%s] SavePID() fail.", g_procname);
		exit(1);
	}

	if(LockFile(g_pidpath) < 0)  {
		LOG_SYS("[%s] LockFile() fail.", g_procname);
		exit(1);
	}

	LOG_SYS("[%s] ==================== Start ====================", g_procname);

	if(BlockSignal() < 0)  {
		printf("Signal Blocking fail.\n");
		exit(1);
	}

	// ���α׷� ���� ��ƾ�� �����Ѵ�.
	ProcMain();

	// �ߺ� ������ �������� ������ pid�� �����Ѵ�.
	DelePID(g_pidpath);

	LOG_SYS("[%s] ==================== Stop ====================", g_procname);

	exit(0);
}

