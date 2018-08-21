#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <libgen.h>
#include <sys/types.h>

#include "ComLib.h"
#include "jpcomd.h"

char	g_configpath	[MAX_PATH_LEN + 1];		// ȯ�� ���� ���
char	g_procname		[MAX_FILE_LEN + 1];		// ���μ�����

int		g_batsmnthnum;

sBatsMnth	g_sBatsMnth[MAX_BATSJOBS_NUM];

int LoadBatsMnth(sBatsMnth *p_sBatsMnth)
{
	int		i, j, line_num, proc_num;
	char	sTemp[256], read_buff[256], buff[256];
	char	fpath[MAX_PATH_LEN + 1];
	FILE	*fp;

	snprintf(fpath, MAX_PATH_LEN, "%s/batsmnth.conf", g_configpath);

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

		// �ּ��� �� newline ó��
		for(i = 0, j = 0; read_buff[i] != 0x00; i ++, j ++)  {
			if(read_buff[i] == '#' || read_buff[i] == '\n')  {
				buff[j] = 0x00;
				break;
			} else if(read_buff[i] == '\\')  {
				// '\' ���� ���ڴ� Ư�����ڷ� �ν����� �ʰ� �ش� ���ڷ� �ν��ϰ� �Ѵ�.
				// '#'�� �ּ� ǥ���ε� '#'�� ǥ���ϰ� ������ �տ� '\'�� �ٿ��ش�.
				i += 1;
				buff[j] = read_buff[i];
				continue;
			} else  {
				buff[j] = read_buff[i];
			}
		}

		RemoveDoubleSpace(buff);

		// �� �� ���� Ȯ��
		TRIM(buff);
		if(strlen(buff) == 0)
			continue;

		// ���� Ȯ�� (����� ':'�� ��ġ�� Ȯ��)
		if(buff[2] != ' ' || buff[5] != ':')  {
			LOG_SYS("[%s] batsmnth.conf ���� ����. [%d][%s]", g_procname, line_num, buff);
			return(-2);
		}

		p_sBatsMnth->day = s2l(&buff[0], 2);

		memcpy(sTemp, &buff[3], 2);	memcpy(sTemp + 2, &buff[6], 2);
		p_sBatsMnth->stime = s2l(sTemp, 4);

		for(i = 0; ; i ++)  {
			if((buff[9 + i] != ' ') && (buff[9 + i] != 0x00))
				p_sBatsMnth->pname[i] = buff[9 + i];
			else
				break;
		}

		if(buff[9 + i] == ' ')
			snprintf(p_sBatsMnth->args, MAX_PATH_LEN, "%s", &buff[9 + i + 1]);

		p_sBatsMnth->pid = 0;
		p_sBatsMnth->run_cnt = 0;
		p_sBatsMnth->status = 0;

#ifdef DEBUG
LOG_TRC("[%s] LoadBatsMnth [%s]", g_procname, buff);
LOG_TRC("[%s] LoadBatsMnth day[%02d] stime[%04d] pname[%s] args[%s] pid[%d]",
		g_procname, p_sBatsMnth->day, p_sBatsMnth->stime, p_sBatsMnth->pname,	p_sBatsMnth->args, p_sBatsMnth->pid);
#endif

		p_sBatsMnth ++;
		proc_num ++;
	}

	fclose(fp);

	return(proc_num);
}

//	����ġ �۾� ����
int RunBatsMnth(char *sDateTime)
{
	int		i;
	pid_t	pid;

	for(i = 0; i < g_batsmnthnum; i ++)  {
		if(g_sBatsMnth[i].day != s2l(sDateTime + 6, 2))
			continue;

		if(g_sBatsMnth[i].run_cnt == 0 && g_sBatsMnth[i].stime == s2l(sDateTime + 8, 4))  {
			if((pid = RunProgWithLog(g_sBatsMnth[i].pname, g_sBatsMnth[i].args)) <= 0)  {
				LOG_SYS("[%s] RunProgWithLog() fail.", g_procname);
			} else  {
				g_sBatsMnth[i].pid = pid;
				g_sBatsMnth[i].run_cnt ++;
				LOG_SYS("[%s] [%s] [%s] �⵿ ����.", g_procname, g_sBatsMnth[i].pname, g_sBatsMnth[i].args);
			}
		}
	}

	return(0);
}

// ����ġ �۾��� ���� ���¸� �ʱ�ȭ �Ѵ�.
int InitBatsMnth(void)
{
	int		i;

	for(i = 0; i < g_batsmnthnum; i ++)  {
		g_sBatsMnth[i].pid = 0;
		g_sBatsMnth[i].run_cnt = 0;
	}

	return(0);
}
