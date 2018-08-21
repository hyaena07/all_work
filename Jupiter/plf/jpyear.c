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

int		g_batsyearnum;

sBatsYear	g_sBatsYear[MAX_BATSJOBS_NUM];

int LoadBatsYear(sBatsYear *p_sBatsYear)
{
	int		i, j, line_num, proc_num;
	char	sTemp[256], read_buff[256], buff[256];
	char	fpath[MAX_PATH_LEN + 1];
	FILE	*fp;

	snprintf(fpath, MAX_PATH_LEN, "%s/batsyear.conf", g_configpath);

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
		if(buff[2] != '/' || buff[8] != ':')  {
			LOG_SYS("[%s] batsyear.conf ���� ����. [%d][%s]", g_procname, line_num, buff);
			return(-1);
		}

		memcpy(sTemp, &buff[0], 2);	memcpy(sTemp + 2, &buff[3], 2);
		p_sBatsYear->mmdd = s2l(sTemp, 4);

		memcpy(sTemp, &buff[6], 2);	memcpy(sTemp + 2, &buff[9], 2);
		p_sBatsYear->stime = s2l(sTemp, 4);

		for(i = 0; ; i ++)  {
			if((buff[12 + i] != ' ') && (buff[12 + i] != 0x00))
				p_sBatsYear->pname[i] = buff[12 + i];
			else
				break;
		}

		if(buff[12 + i] == ' ')
			snprintf(p_sBatsYear->args, MAX_PATH_LEN, "%s", &buff[12 + i + 1]);

		p_sBatsYear->pid = 0;
		p_sBatsYear->run_cnt = 0;
		p_sBatsYear->status = 0;

#ifdef DEBUG
LOG_TRC("[%s] LoadBatsYear [%s]", g_procname, buff);
LOG_TRC("[%s] LoadBatsYear mmdd[%04d] stime[%04d] pname[%s] args[%s] pid[%d]",
		g_procname, p_sBatsYear->mmdd, p_sBatsYear->stime, p_sBatsYear->pname,	p_sBatsYear->args, p_sBatsYear->pid);
#endif

		p_sBatsYear ++;
		proc_num ++;
	}

	fclose(fp);

	return(proc_num);
}

//	���ġ �۾� ����
int RunBatsYear(char *sDateTime)
{
	int		i;
	pid_t	pid;

	for(i = 0; i < g_batsyearnum; i ++)  {
		if(g_sBatsYear[i].mmdd != s2l(sDateTime + 4, 4))
			continue;

		if(g_sBatsYear[i].run_cnt == 0 && g_sBatsYear[i].stime == s2l(sDateTime + 8, 4))  {
			if((pid = RunProgWithLog(g_sBatsYear[i].pname, g_sBatsYear[i].args)) <= 0)  {
				LOG_SYS("[%s] RunProgWithLog() fail.", g_procname);
			} else  {
				g_sBatsYear[i].pid = pid;
				g_sBatsYear[i].run_cnt ++;
				LOG_SYS("[%s] [%s] [%s] �⵿ ����.", g_procname, g_sBatsYear[i].pname, g_sBatsYear[i].args);
			}
		}
	}

	return(0);
}

// ���ġ �۾��� ���� ���¸� �ʱ�ȭ �Ѵ�.
int InitBatsYear(void)
{
	int		i;

	for(i = 0; i < g_batsyearnum; i ++)  {
		g_sBatsYear[i].pid = 0;
		g_sBatsYear[i].run_cnt = 0;
	}

	return(0);
}
