#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <libgen.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>

#include "ComLib.h"
#include "jpcomd.h"

char	g_configpath	[MAX_PATH_LEN + 1];		// 환경 파일 경로
char	g_procname		[MAX_FILE_LEN + 1];		// 프로세스명

int		g_batsdalynum;
int		g_schddalynum;

sBatsDaly	g_sBatsDaly[MAX_BATSJOBS_NUM];
sSchdDaly	g_sSchdDaly[MAX_PROCSCHD_NUM];

int LoadBatsDaly(sBatsDaly *p_sBatsDaly)
{
	int		i, j, line_num, proc_num;
	char	sTemp[256], read_buff[256], buff[256];
	char	fpath[MAX_PATH_LEN + 1];
	FILE	*fp;

	snprintf(fpath, MAX_PATH_LEN, "%s/batsdaly.conf", g_configpath);

	if((fp = fopen(fpath, "r")) == (FILE *)NULL)  {
		LOG_SYS("[%s] fopen() fail. [%s] [%s]", g_procname, strerror(errno), fpath);
		return(-1);
	}

	line_num = 0;
	proc_num = 0;

	while(1)  {
		bzero(read_buff,	sizeof(read_buff));
		bzero(buff,			sizeof(buff));

		if(fgets(read_buff, sizeof(read_buff), fp) == (char *)NULL)
			break;

		line_num ++;

		// 주석문 및 newline 처리
		for(i = 0, j = 0; read_buff[i] != 0x00; i ++, j ++)  {
			if(read_buff[i] == '#' || read_buff[i] == '\n')  {
				buff[j] = 0x00;
				break;
			} else if(read_buff[i] == '\\')  {
				// '\' 뒤의 문자는 특수문자로 인식하지 않고 해당 문자로 인식하게 한다.
				// '#'은 주석 표시인데 '#'을 표현하고 싶으면 앞에 '\'를 붙여준다.
				i += 1;
				buff[j] = read_buff[i];
				continue;
			} else  {
				buff[j] = read_buff[i];
			}
		}

		RemoveDoubleSpace(buff);

		// 빈 줄 여부 확인
		TRIM(buff);
		if(strlen(buff) == 0)  {
			continue;
		}

		// 문법 확인 (공백과 ':'의 위치만 확인)
		if(buff[2] != ':' || buff[5] != ' ')  {
			LOG_SYS("[%s] batsdaly.conf 문법 오류. [%d][%s]", g_procname, line_num, buff);
			return(-2);
		}

		memcpy(sTemp, &buff[0], 2);	memcpy(sTemp + 2, &buff[3], 2);
		p_sBatsDaly->stime = s2l(sTemp, 4);

		for(i = 0; ; i ++)  {
			if((buff[6 + i] != ' ') && (buff[6 + i] != 0x00))
				p_sBatsDaly->pname[i] = buff[6 + i];
			else
				break;
		}

		if(buff[6 + i] == ' ')
			snprintf(p_sBatsDaly->args, MAX_PATH_LEN, "%s", &buff[6 + i + 1]);

		p_sBatsDaly->pid = 0;
		p_sBatsDaly->run_cnt = 0;
		p_sBatsDaly->status = 0;

		LOG_TRC("[%s] LoadBatsDaly [%s]", g_procname, buff);
		LOG_TRC("[%s] LoadBatsDaly stime[%04d] pname[%s] args[%s] pid[%d]",
				g_procname, p_sBatsDaly->stime, p_sBatsDaly->pname,	p_sBatsDaly->args, p_sBatsDaly->pid);

		p_sBatsDaly ++;
		proc_num ++;
	}

	fclose(fp);

	return(proc_num);
}

//	일배치 작업 실행
int RunBatsDaly(char *sDateTime)
{
	int		i;
	pid_t	pid;

	for(i = 0; i < g_batsdalynum; i ++)  {
		if(g_sBatsDaly[i].run_cnt == 0 && g_sBatsDaly[i].stime == s2l(sDateTime + 8, 4))  {
			if((pid = RunProgWithLog(g_sBatsDaly[i].pname, g_sBatsDaly[i].args)) <= 0)  {
				LOG_SYS("[%s] RunProgWithLog() fail.", g_procname);
			} else  {
				g_sBatsDaly[i].pid = pid;
				g_sBatsDaly[i].run_cnt ++;
				LOG_SYS("[%s] [%s] [%s] 기동 성공.", g_procname, g_sBatsDaly[i].pname, g_sBatsDaly[i].args);
			}
		}
	}

	return(0);
}

// 일배치 작업의 실행 상태를 초기화 한다.
int InitBatsDaly(void)
{
	int		i;

	for(i = 0; i < g_batsdalynum; i ++)  {
		g_sBatsDaly[i].pid = 0;
		g_sBatsDaly[i].run_cnt = 0;
	}

	return(0);
}

int LoadSchdDaly(sSchdDaly *p_sSchdDaly)
{
	int		i, j, line_num, proc_num;
	char	sTemp[256], read_buff[256], buff[256];
	char	fpath[MAX_PATH_LEN + 1];
	FILE	*fp;

	snprintf(fpath, MAX_PATH_LEN, "%s/schddaly.conf", g_configpath);

	if((fp = fopen(fpath, "r")) == (FILE *)NULL)  {
		LOG_SYS("[%s] fopen() fail. [%s]", g_procname, strerror(errno));
		return(-1);
	}

	line_num = 0;
	proc_num = 0;

	while(1)  {
		bzero(read_buff,	sizeof(read_buff));
		bzero(buff,			sizeof(buff));

		if(fgets(read_buff, sizeof(read_buff) - 1, fp) == (char *)NULL)
			break;

		line_num ++;

		// 주석문 및 newline 처리
		for(i = 0, j = 0; read_buff[i] != 0x00; i ++, j ++)  {
			if(read_buff[i] == '#' || read_buff[i] == '\n')  {
				buff[j] = 0x00;
				break;
			} else if(read_buff[i] == '\\')  {
				// '\' 뒤의 문자는 특수문자로 인식하지 않고 해당 문자로 인식하게 한다.
				// '#'은 주석 표시인데 '#'을 표현하고 싶으면 앞에 '\'를 붙여준다.
				i += 1;
				buff[j] = read_buff[i];
				continue;
			} else  {
				buff[j] = read_buff[i];
			}
		}

		RemoveDoubleSpace(buff);

		// 빈 줄 여부 확인
		TRIM(buff);
		if(strlen(buff) == 0)
			continue;

		// 문법 확인 (공백과 ':'의 위치만 확인)
		if(buff[2] != ':' || buff[5] != ' ' || buff[8] != ':' || buff[11] != ' ')  {
			LOG_SYS("[%s] schddaly.conf 문법 오류. [%d][%s]", g_procname, line_num, buff);
			return(-2);
		}

		memcpy(sTemp, &buff[0], 2);	memcpy(sTemp + 2, &buff[3], 2);
		p_sSchdDaly->stime = s2l(sTemp, 4);

		memcpy(sTemp, &buff[6], 2);	memcpy(sTemp + 2, &buff[9], 2);
		p_sSchdDaly->etime = s2l(sTemp, 4);

		for(i = 0; ; i ++)  {
			if((buff[12 + i] != ' ') && (buff[12 + i] != 0x00))
				p_sSchdDaly->pname[i] = buff[12 + i];
			else
				break;
		}

		if(buff[12 + i] == ' ')
			snprintf(p_sSchdDaly->args, MAX_PATH_LEN, "%s", &buff[12 + i + 1]);

		p_sSchdDaly->pid		= 0;
		p_sSchdDaly->run_cnt	= 0;
		p_sSchdDaly->kil_cnt	= 0;

		LOG_TRC("[%s] LoadSchdDaly [%s]", g_procname, buff);
		LOG_TRC("[%s] LoadSchdDaly stime[%04d] etime[%04d] pname[%s] args[%s] pid[%d]",
				g_procname, p_sSchdDaly->stime, p_sSchdDaly->etime, p_sSchdDaly->pname,
				p_sSchdDaly->args, p_sSchdDaly->pid);

		p_sSchdDaly ++;
		proc_num ++;
	}

	fclose(fp);

	return(proc_num);
}

int reLoadSchdDaly(void)
{
	int			i, j, proc_num;
	sSchdDaly	t_sSchdDaly[MAX_PROCSCHD_NUM];

	bzero(t_sSchdDaly, MAX_PROCSCHD_NUM * SZ_sSchdDaly);
	if((proc_num = LoadSchdDaly(t_sSchdDaly)) < 0)  {
		LOG_SYS("[%s] LoadSchdDaly() fail.", g_procname);
		return(-1);
	}
LOG_TRC("[%s] reLoadSchdDaly() proc_num[%d]", g_procname, proc_num);

	for(i = 0; i < g_schddalynum; i ++)  {
		for(j = 0; j < proc_num; j ++)  {
			if(memcmp(g_sSchdDaly[i].pname, t_sSchdDaly[j].pname, sizeof(g_sSchdDaly[i].pname)) == 0 &&
				memcmp(g_sSchdDaly[i].args , t_sSchdDaly[j].args , sizeof(g_sSchdDaly[i].args )) == 0)  {
				t_sSchdDaly[j].pid = g_sSchdDaly[i].pid;
				break;
			}
		}

		// 과거 목록에 존재하던 프로그램이 현재 목록에서 제외되고 프로그램이 실행 중인 경우
		// 새로운 목록에 시작 시간과 종료 시간을 '8'과 '9'로 채워서 새로운 목록을 처리할 때 종료되도록 한다.
		if(j == proc_num && g_sSchdDaly[i].pid > 0)  {
			t_sSchdDaly[j].stime = 8888;
			t_sSchdDaly[j].etime = 9999;
			memcpy(t_sSchdDaly[j].pname, g_sSchdDaly[i].pname, sizeof(t_sSchdDaly[j].pname));
			memcpy(t_sSchdDaly[j].args , g_sSchdDaly[i].args , sizeof(t_sSchdDaly[j].args ));
			t_sSchdDaly[j].pid = g_sSchdDaly[i].pid;
			t_sSchdDaly[j].run_cnt = g_sSchdDaly[i].run_cnt;
			t_sSchdDaly[j].kil_cnt = g_sSchdDaly[i].kil_cnt;

			proc_num ++;
LOG_TRC("[%s] reLoadSchdDaly() pname[%s] args[%s] 제외.", g_procname, t_sSchdDaly[j].pname, t_sSchdDaly[j].args);
		}
	}

	g_schddalynum = proc_num;
	memcpy(g_sSchdDaly, t_sSchdDaly, SZ_sSchdDaly * proc_num);

	return(0);
}

int CheckSchdDaly(char *sDateTime)
{
	static int	cnt;
	int			i;
	int			stime, etime, ntime;
	char		sTemp[1024];
	pid_t		pid;

	sprintf(sTemp, "%.4s", sDateTime + 8);
	ntime = atoi(sTemp);

	if(++cnt >= 10)		cnt = 0;

	for(i = 0; i < g_schddalynum; i ++)  {
		stime = g_sSchdDaly[i].stime;

		etime = g_sSchdDaly[i].etime;

		// 시작시간과 종료시간이 같으면 항상
		// 시작시간보다 종료시간이 크고, 현재시간이 시작시간 보다 크거나 같고 && 종료시간보다 작거나 같은 경우
		// 시작시간보다 종료시간이 작고, 현재시간이 시작시간 보다 크거나 같고 || 종료시간보다 작거나 같은 경우
		if( (stime == etime) ||
			(stime < etime && (stime <= ntime && etime >= ntime)) ||
			(stime > etime && (stime <= ntime || etime >= ntime))
		)  {
//LOG_TRC("[%s] pname[%s] stime[%04d] etime[%04d] ntime[%04d]", g_procname, g_sSchdDaly[i].pname, stime, etime, ntime);
			if(g_sSchdDaly[i].pid == 0)  {
				// 운영 시간에 실행 중이 아닌 프로그램을 기동 시킨다.

				if(cnt != 0 && g_sSchdDaly[i].run_cnt > 0)		// 재실행은 10초에 한 번씩만
					continue;

				if((pid = RunProg(g_sSchdDaly[i].pname, g_sSchdDaly[i].args)) <= 0)  {
					LOG_SYS("[%s] RunProg() fail.", g_procname);
					exit(0);
				} else  {
					g_sSchdDaly[i].pid = pid;
					g_sSchdDaly[i].kil_cnt = 0;
					LOG_SYS("[%s] [%s] [%s] 기동 성공. (%d retry)", g_procname, g_sSchdDaly[i].pname, g_sSchdDaly[i].args, g_sSchdDaly[i].run_cnt);
				}

				g_sSchdDaly[i].run_cnt ++;
			}
		} else  {
			if(g_sSchdDaly[i].pid > 0)  {
				// 운영 시간에서 벗어난 실행 중인 프로세스에게 종료 신호를 보낸다.
				if(g_sSchdDaly[i].kil_cnt > 3)  {
					kill(g_sSchdDaly[i].pid, SIGKILL);
					LOG_SYS("[%s] [%s] 강제 종료 보냄. (%d retry)", g_procname, g_sSchdDaly[i].pname, g_sSchdDaly[i].kil_cnt);
				} else  {
					kill(g_sSchdDaly[i].pid, SIGTERM);
					LOG_SYS("[%s] [%s] 종료 신호 보냄. (%d retry)", g_procname, g_sSchdDaly[i].pname, g_sSchdDaly[i].kil_cnt);

					g_sSchdDaly[i].kil_cnt ++;
				}

				g_sSchdDaly[i].run_cnt = 0;
			}
		}
	}

	return(0);
}

