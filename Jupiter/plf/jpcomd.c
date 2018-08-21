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

char	g_syslogpath	[MAX_PATH_LEN + 1];		// System 로그 경로 및 접두사
char	g_trclogpath	[MAX_PATH_LEN + 1];		// Trace 로그 경로 및 접두사
char	g_tmplogpath	[MAX_PATH_LEN + 1];		// 자식 프로세스를 위한 임시 로그 경로 및 접두사
char	g_configpath	[MAX_PATH_LEN + 1];		// 환경 파일 경로
char	g_pidpath		[MAX_PATH_LEN + 1];		// pid 저장 파일 경로
char	g_fifostatus	[MAX_PATH_LEN + 1];		// 데몬 상태 전송용 fifo 경로 명
char	g_procname		[MAX_FILE_LEN + 1];		// 프로세스명

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
	LOG_SYS("[%s] signal %d 접수 !!!", g_procname, signo);

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
		LOG_SYS("[%s] signal SIGTERM 접수 !!!", g_procname);
		g_termflag = 1;
	}

	if(sigismember(&pend, SIGCHLD) == 1)  {
		LOG_SYS("[%s] signal SIGCHLD 접수 !!!", g_procname);
		g_chkrunflag = 1;
	}

	if(sigismember(&pend, SIGUSR1) == 1)  {
		LOG_SYS("[%s] signal SIGUSR1 접수 !!!", g_procname);
		g_reloadflag = 1;
	}

	if(sigismember(&pend, SIGUSR2) == 1)  {
		LOG_SYS("[%s] signal SIGUSR2 접수 !!!", g_procname);
	}

	DeleteSignal(&pend);

	return;
}

int CheckEnv(void)
{
	int		errflag = 0;

	if(getenv("JP_HOME") == (char *)NULL)  {
		printf(">> 환경변수 [JP_HOME]를 정의한 후에 실행하십시오.\n");
		errflag = 1;
	}

	if(getenv("JP_CONF") == (char *)NULL)  {
		printf(">> 환경변수 [JP_CONF]를 정의한 후에 실행하십시오.\n");
		errflag = 1;
	}

	if(getenv("JP_LOGS") == (char *)NULL)  {
		printf(">> 환경변수 [JP_LOGS]를 정의한 후에 실행하십시오.\n");
		errflag = 1;
	}

	if(getenv("JP_IPCS") == (char *)NULL)  {
		printf(">> 환경변수 [JP_IPCS]를 정의한 후에 실행하십시오.\n");
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
			// 실행 중인 프로그램에 대한 종료 검사를 한다.
			pid = waitpid(g_sSchdDaly[i].pid, &wait_status, WNOHANG);
			if(g_sSchdDaly[i].pid == pid)  {
				g_sSchdDaly[i].pid = 0;
				LOG_SYS("[%s] [%s] [%s] 종료 확인 [%d]", g_procname, g_sSchdDaly[i].pname, g_sSchdDaly[i].args, WEXITSTATUS(wait_status));
			}
		}
	}

	for(i = 0; i < g_batsdalynum; i ++)  {
		if(g_sBatsDaly[i].pid > 0)  {
			// 실행 중인 프로그램에 대한 종료 검사를 한다.
			pid = waitpid(g_sBatsDaly[i].pid, &wait_status, WNOHANG);
			if(g_sBatsDaly[i].pid == pid)  {
				g_sBatsDaly[i].pid = 0;
				g_sBatsDaly[i].status = wait_status;
				LOG_SYS("[%s] [%s] [%s] 종료 확인 [%d]", g_procname, g_sBatsDaly[i].pname, g_sBatsDaly[i].args, WEXITSTATUS(wait_status));
			}
		}
	}

	for(i = 0; i < g_batsweeknum; i ++)  {
		if(g_sBatsWeek[i].pid > 0)  {
			// 실행 중인 프로그램에 대한 종료 검사를 한다.
			pid = waitpid(g_sBatsWeek[i].pid, &wait_status, WNOHANG);
			if(g_sBatsWeek[i].pid == pid)  {
				g_sBatsWeek[i].pid = 0;
				g_sBatsWeek[i].status = wait_status;
				LOG_SYS("[%s] [%s] [%s] 종료 확인 [%d]", g_procname, g_sBatsWeek[i].pname, g_sBatsWeek[i].args, WEXITSTATUS(wait_status));
			}
		}
	}

	for(i = 0; i < g_batsmnthnum; i ++)  {
		if(g_sBatsMnth[i].pid > 0)  {
			// 실행 중인 프로그램에 대한 종료 검사를 한다.
			pid = waitpid(g_sBatsMnth[i].pid, &wait_status, WNOHANG);
			if(g_sBatsMnth[i].pid == pid)  {
				g_sBatsMnth[i].pid = 0;
				g_sBatsMnth[i].status = wait_status;
				LOG_SYS("[%s] [%s] [%s] 종료 확인 [%d]", g_procname, g_sBatsMnth[i].pname, g_sBatsMnth[i].args, WEXITSTATUS(wait_status));
			}
		}
	}

	for(i = 0; i < g_batsyearnum; i ++)  {
		if(g_sBatsYear[i].pid > 0)  {
			// 실행 중인 프로그램에 대한 종료 검사를 한다.
			pid = waitpid(g_sBatsYear[i].pid, &wait_status, WNOHANG);
			if(g_sBatsYear[i].pid == pid)  {
				g_sBatsYear[i].pid = 0;
				g_sBatsYear[i].status = wait_status;
				LOG_SYS("[%s] [%s] [%s] 종료 확인 [%d]", g_procname, g_sBatsYear[i].pname, g_sBatsYear[i].args, WEXITSTATUS(wait_status));
			}
		}
	}

	return(0);
}

void TermProc(void)
{
	int		i, wait_status;
	pid_t	pid;

	LOG_SYS("[%s] ---- 프로세스 종료 절차 시작 ...", g_procname);

	// 실행 중인 모든 프로세스에 종료 시그널 전송
	for(i = 0; i < 	g_schddalynum; i ++)  {
		if(g_sSchdDaly[i].pid > 0)  {
			kill(g_sSchdDaly[i].pid, SIGTERM);
			LOG_SYS("[%s] [%s] 정상 종료 신호 전송. (pid:%d)", g_procname, g_sSchdDaly[i].pname, g_sSchdDaly[i].pid);
		}
	}

	sleep(1);

	// 정상 종료 확인
	for(i = 0; i < 	g_schddalynum; i ++)  {
		if(g_sSchdDaly[i].pid > 0)  {
			pid = waitpid(g_sSchdDaly[i].pid, &wait_status, WNOHANG);
			if(g_sSchdDaly[i].pid == pid)  {
				g_sSchdDaly[i].pid = 0;
				LOG_SYS("[%s] [%s] [%s] 종료 확인 [%d].", g_procname, g_sSchdDaly[i].pname, g_sSchdDaly[i].args, wait_status);
			}
		}
	}

	// 데몬이 종료하기 직전이므로 모든 프로세스를 강제 종료 시킨다.
	for(i = 0; i < 	g_schddalynum; i ++)  {
		if(g_sSchdDaly[i].pid > 0)  {
			kill(g_sSchdDaly[i].pid, SIGKILL);
			LOG_SYS("[%s] [%s] 강제 종료 신호 전송. (pid:%d)", g_procname, g_sSchdDaly[i].pname, g_sSchdDaly[i].pid);
		}
	}

	return;
}

int InitProc(void)
{
	bzero(g_sSchdDaly, MAX_PROCSCHD_NUM * SZ_sSchdDaly);
	LOG_SYS("[%s] schddaly.conf 읽기 시작 ...", g_procname);
	if((g_schddalynum = LoadSchdDaly(g_sSchdDaly)) < 0)  {
		LOG_SYS("[%s] LoadSchdDaly() fail.", g_procname);
		return(-1);
	}

	bzero(g_sBatsDaly, MAX_BATSJOBS_NUM * SZ_sBatsDaly);
	LOG_SYS("[%s] batsdaly.conf 읽기 시작 ...", g_procname);
	if((g_batsdalynum = LoadBatsDaly(g_sBatsDaly)) < 0)  {
		LOG_SYS("[%s] LoadBatsDaly() fail.", g_procname);
		return(-2);
	}

	bzero(g_sBatsWeek, MAX_BATSJOBS_NUM * SZ_sBatsWeek);
	LOG_SYS("[%s] batsweek.conf 읽기 시작 ...", g_procname);
	if((g_batsweeknum = LoadBatsWeek(g_sBatsWeek)) < 0)  {
		LOG_SYS("[%s] LoadBatsWeek() fail.", g_procname);
		return(-3);
	}

	bzero(g_sBatsMnth, MAX_BATSJOBS_NUM * SZ_sBatsMnth);
	LOG_SYS("[%s] batsmnth.conf 읽기 시작 ...", g_procname);
	if((g_batsmnthnum = LoadBatsMnth(g_sBatsMnth)) < 0)  {
		LOG_SYS("[%s] LoadBatsMnth() fail.", g_procname);
		return(-4);
	}

	bzero(g_sBatsYear, MAX_BATSJOBS_NUM * SZ_sBatsYear);
	LOG_SYS("[%s] batsyear.conf 읽기 시작 ...", g_procname);
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

	dCurrWeek = getWeekNo();							// 현재 요일 셋팅
	sprintf(sCurrDateTime, "%.14s", getDateTime_14());	// 현재 날짜 셋팅

	// 플렛폼 시작 시 처리할 작업 들
	LOG_SYS("[%s] 플렛폼 시작 배치 시작.", g_procname);
	if(RunProgramList("plonboot.conf") < 0)  {
		LOG_SYS("[%s] RunProgramList() fail.", g_procname);
	}
	LOG_SYS("[%s] 플렛폼 시작 배치 종료.", g_procname);

	while(1)  {
		dTempWeek = getWeekNo();							// 현재 요일 셋팅
		sprintf(sTempDateTime, "%.14s", getDateTime_14());	// 현재 날짜 셋팅

		snprintf(g_syslogpath, MAX_PATH_LEN, "%s/sys_%.8s.log", getenv("JP_LOGS"), sTempDateTime);
		snprintf(g_trclogpath, MAX_PATH_LEN, "%s/trc_%.8s.log", getenv("JP_LOGS"), sTempDateTime);

		CheckSignal();

		// 종료 시그널 처리
		if(g_termflag)  {
			TermProc();
			break;
		}

		// 환경 재설정 처리
		if(g_reloadflag)  {
			LOG_SYS("[%s] schddaly.conf 읽기 시작 ...", g_procname);
			if(reLoadSchdDaly() < 0)
				LOG_SYS("[%s] reLoadSchdDaly() fail.", g_procname);

			bzero(g_sBatsDaly, MAX_BATSJOBS_NUM * SZ_sBatsDaly);
			LOG_SYS("[%s] batsdaly.conf 읽기 시작 ...", g_procname);
			if((g_batsdalynum = LoadBatsDaly(g_sBatsDaly)) < 0)
				LOG_SYS("[%s] LoadBatsDaly() fail.", g_procname);

			bzero(g_sBatsWeek, MAX_BATSJOBS_NUM * SZ_sBatsWeek);
			LOG_SYS("[%s] batsweek.conf 읽기 시작 ...", g_procname);
			if((g_batsweeknum = LoadBatsWeek(g_sBatsWeek)) < 0)
				LOG_SYS("[%s] LoadBatsWeek() fail.", g_procname);

			bzero(g_sBatsMnth, MAX_BATSJOBS_NUM * SZ_sBatsMnth);
			LOG_SYS("[%s] batsmnth.conf 읽기 시작 ...", g_procname);
			if((g_batsmnthnum = LoadBatsMnth(g_sBatsMnth)) < 0)
				LOG_SYS("[%s] LoadBatsMnth() fail.", g_procname);

			bzero(g_sBatsYear, MAX_BATSJOBS_NUM * SZ_sBatsYear);
			LOG_SYS("[%s] batsyear.conf 읽기 시작 ...", g_procname);
			if((g_batsyearnum = LoadBatsYear(g_sBatsYear)) < 0)
				LOG_SYS("[%s] LoadBatsYear() fail.", g_procname);

			g_reloadflag = 0;
		}

		// 종료 프로세스 검사 및 처리
		if(g_chkrunflag)  {
			CheckRunProc();

			g_chkrunflag = 0;
		}

		// 년 변경 시 처리할 작업 들
		if(memcmp(sCurrDateTime, sTempDateTime, 4) != 0)  {
			InitBatsYear();
			LOG_SYS("[%s] 년배치 작업의 실행 상태를 초기화.", g_procname);

			LOG_SYS("[%s] 년변경 배치 시작.", g_procname);

			LOG_SYS("[%s] 년변경 배치 종료.", g_procname);
		}

		// 월 변경 시 처리할 작업 들
		if(memcmp(sCurrDateTime, sTempDateTime, 6) != 0)  {
			InitBatsMnth();
			LOG_SYS("[%s] 월배치 작업의 실행 상태를 초기화.", g_procname);

			LOG_SYS("[%s] 월변경 배치 시작.", g_procname);

			LOG_SYS("[%s] 월변경 배치 종료.", g_procname);
		}

		// 주 변경 시 처리할 작업 들
		if(dCurrWeek == 6 && dTempWeek == 0)  {
			InitBatsWeek();
			LOG_SYS("[%s] 주배치 작업의 실행 상태를 초기화.", g_procname);

			LOG_SYS("[%s] 주변경 배치 시작.", g_procname);

			LOG_SYS("[%s] 주변경 배치 종료.", g_procname);
		}

		// 일 변경 시 처리할 작업 들
		if(memcmp(sCurrDateTime, sTempDateTime, 8) != 0)  {
			// 일별 배치 작업 결과를 초기화 한다.
			InitBatsDaly();
			LOG_SYS("[%s] 일배치 작업의 실행 상태를 초기화.", g_procname);

			LOG_SYS("[%s] 일변경 배치 시작.", g_procname);

			LOG_SYS("[%s] 일변경 배치 종료.", g_procname);
		}

		// 시 변경시 처리할 작업 들
		if(memcmp(sCurrDateTime, sTempDateTime, 10) != 0)  {
			LOG_SYS("[%s] 시변경 배치 시작.", g_procname);

			LOG_SYS("[%s] 시변경 배치 종료.", g_procname);
		}

		// 분 변경시 처리할 작업 들
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

		// 데몬관리 작업들
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

	// 필요한 환경변수가 모두 정의 되었는지 검사
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
		// 중복 실행을 막기 위해 이미 실행 중인 같은 프로세스가 있는지 검사한다.
		if(ReadPID(g_pidpath) >= 0)  {		// pid 파일이 존재하고
			if(LockFile(g_pidpath) < 0)  {	// pid 파일에 잠금 설정을 할 수 없다면 이미 실행 중인 프로세스가 있다고 판단.
				printf("[%s] is alrady run ...\n", g_procname);
				exit(1);
			} else  {						// pid 파일에 잠금 설정이 가능하면 이전 프로세스가 비정상 종료한 것으로 판다.
				unLockFile(g_pidpath);
			}
		}
	} else if(strcmp(argv[1], "stop") == 0)  {
		// 실행 중인 프로세스에 종료 시그널을 송신하고 종료한다.
		if((old_pid = ReadPID(g_pidpath)) >= 0)  {
			kill(old_pid, SIGTERM);
		}
		exit(0);
	} else if(strcmp(argv[1], "reload") == 0)  {
		// 실행 중인 프로세스에 USR1 시그널을 송신하고 종료한다.
		if((old_pid = ReadPID(g_pidpath)) >= 0)  {
			kill(old_pid, SIGUSR1);
		}
		exit(0);
	} else if(strcmp(argv[1], "status") == 0)  {
		// 실행 중인 프로세스의 상태를 전송 받아 화면에 출력한다.
		if(PrintStatus(g_fifostatus) < 0)  {
			printf("PrintStatus() fail. [%s]\n", g_fifostatus);
		}
		exit(0);
	} else  {
		printf("Usage: %s {start|stop|reload|status}\n", argv[0]);
		exit(1);
	}

	LOG_SYS("[%s] ---- 프로세스 초기화 시작 ...", g_procname);
	if(InitProc() < 0)  {
		printf("InitProc() fail.\n");
		exit(1);
	}

	toBackground();

	// 중복 실행을 막기위해 현재 프로세스의 pid를 저장하고 잠금 설정을 한다.
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

	// 프로그램 메인 루틴을 실행한다.
	ProcMain();

	// 중복 실행을 막기위해 저장한 pid를 삭제한다.
	DelePID(g_pidpath);

	LOG_SYS("[%s] ==================== Stop ====================", g_procname);

	exit(0);
}

