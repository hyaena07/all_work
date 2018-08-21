#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <libgen.h>
#include <sys/types.h>

#include "ComLib.h"
#include "jpcomd.h"

char	g_configpath	[MAX_PATH_LEN + 1];		// 환경 파일 경로
char	g_procname		[MAX_FILE_LEN + 1];		// 프로세스명

int		g_batsweeknum;

sBatsWeek	g_sBatsWeek[MAX_BATSJOBS_NUM];

int LoadBatsWeek(sBatsWeek *p_sBatsWeek)
{
	int		i, j, line_num, proc_num;
	char	sTemp[256], read_buff[256], buff[256];
	char	fpath[MAX_PATH_LEN + 1];
	FILE	*fp;

	snprintf(fpath, MAX_PATH_LEN, "%s/batsweek.conf", g_configpath);

	if((fp = fopen(fpath, "r")) == (FILE *)NULL)  {
		LOG_SYS("[%s] fopen() fail. [%s] [%s]", g_procname, strerror(errno), fpath);
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
		if(buff[1] != ' ' || buff[4] != ':')  {
			LOG_SYS("[%s] batsweek.conf 문법 오류. [%d][%s]", g_procname, line_num, buff);
			return(-1);
		}

		p_sBatsWeek->weekn = s2l(&buff[0], 1);

		memcpy(sTemp, &buff[2], 2);	memcpy(sTemp + 2, &buff[5], 2);
		p_sBatsWeek->stime = s2l(sTemp, 4);

		for(i = 0; ; i ++)  {
			if((buff[8 + i] != ' ') && (buff[8 + i] != 0x00))
				p_sBatsWeek->pname[i] = buff[8 + i];
			else
				break;
		}

		if(buff[8 + i] == ' ')
			snprintf(p_sBatsWeek->args, MAX_PATH_LEN, "%s", &buff[8 + i + 1]);

		p_sBatsWeek->pid = 0;
		p_sBatsWeek->run_cnt = 0;
		p_sBatsWeek->status = 0;

#ifdef DEBUG
LOG_TRC("[%s] LoadBatsWeek [%s]", g_procname, buff);
LOG_TRC("[%s] LoadBatsWeek weekn[%01d] stime[%04d] pname[%s] args[%s] pid[%d]",
		g_procname, p_sBatsWeek->weekn, p_sBatsWeek->stime, p_sBatsWeek->pname,	p_sBatsWeek->args, p_sBatsWeek->pid);
#endif

		p_sBatsWeek ++;
		proc_num ++;
	}

	fclose(fp);

	return(proc_num);
}

//	주배치 작업 실행
int RunBatsWeek(int weekn, char *sDateTime)
{
	int		i;
	pid_t	pid;

	for(i = 0; i < g_batsweeknum; i ++)  {
		if(g_sBatsWeek[i].weekn != weekn)
			continue;

		if(g_sBatsWeek[i].run_cnt == 0 && g_sBatsWeek[i].stime == s2l(sDateTime + 8, 4))  {
			if((pid = RunProgWithLog(g_sBatsWeek[i].pname, g_sBatsWeek[i].args)) <= 0)  {
				LOG_SYS("[%s] RunProgWithLog() fail.", g_procname);
			} else  {
				g_sBatsWeek[i].pid = pid;
				g_sBatsWeek[i].run_cnt ++;
				LOG_SYS("[%s] [%s] [%s] 기동 성공.", g_procname, g_sBatsWeek[i].pname, g_sBatsWeek[i].args);
			}
		}
	}

	return(0);
}

// 주배치 작업의 실행 상태를 초기화 한다.
int InitBatsWeek(void)
{
	int		i;

	for(i = 0; i < g_batsweeknum; i ++)  {
		g_sBatsWeek[i].pid = 0;
		g_sBatsWeek[i].run_cnt = 0;
	}

	return(0);
}
