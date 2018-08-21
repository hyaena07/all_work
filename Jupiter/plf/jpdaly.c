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

char	g_configpath	[MAX_PATH_LEN + 1];		// ȯ�� ���� ���
char	g_procname		[MAX_FILE_LEN + 1];		// ���μ�����

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
		if(strlen(buff) == 0)  {
			continue;
		}

		// ���� Ȯ�� (����� ':'�� ��ġ�� Ȯ��)
		if(buff[2] != ':' || buff[5] != ' ')  {
			LOG_SYS("[%s] batsdaly.conf ���� ����. [%d][%s]", g_procname, line_num, buff);
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

//	�Ϲ�ġ �۾� ����
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
				LOG_SYS("[%s] [%s] [%s] �⵿ ����.", g_procname, g_sBatsDaly[i].pname, g_sBatsDaly[i].args);
			}
		}
	}

	return(0);
}

// �Ϲ�ġ �۾��� ���� ���¸� �ʱ�ȭ �Ѵ�.
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
		if(buff[2] != ':' || buff[5] != ' ' || buff[8] != ':' || buff[11] != ' ')  {
			LOG_SYS("[%s] schddaly.conf ���� ����. [%d][%s]", g_procname, line_num, buff);
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

		// ���� ��Ͽ� �����ϴ� ���α׷��� ���� ��Ͽ��� ���ܵǰ� ���α׷��� ���� ���� ���
		// ���ο� ��Ͽ� ���� �ð��� ���� �ð��� '8'�� '9'�� ä���� ���ο� ����� ó���� �� ����ǵ��� �Ѵ�.
		if(j == proc_num && g_sSchdDaly[i].pid > 0)  {
			t_sSchdDaly[j].stime = 8888;
			t_sSchdDaly[j].etime = 9999;
			memcpy(t_sSchdDaly[j].pname, g_sSchdDaly[i].pname, sizeof(t_sSchdDaly[j].pname));
			memcpy(t_sSchdDaly[j].args , g_sSchdDaly[i].args , sizeof(t_sSchdDaly[j].args ));
			t_sSchdDaly[j].pid = g_sSchdDaly[i].pid;
			t_sSchdDaly[j].run_cnt = g_sSchdDaly[i].run_cnt;
			t_sSchdDaly[j].kil_cnt = g_sSchdDaly[i].kil_cnt;

			proc_num ++;
LOG_TRC("[%s] reLoadSchdDaly() pname[%s] args[%s] ����.", g_procname, t_sSchdDaly[j].pname, t_sSchdDaly[j].args);
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

		// ���۽ð��� ����ð��� ������ �׻�
		// ���۽ð����� ����ð��� ũ��, ����ð��� ���۽ð� ���� ũ�ų� ���� && ����ð����� �۰ų� ���� ���
		// ���۽ð����� ����ð��� �۰�, ����ð��� ���۽ð� ���� ũ�ų� ���� || ����ð����� �۰ų� ���� ���
		if( (stime == etime) ||
			(stime < etime && (stime <= ntime && etime >= ntime)) ||
			(stime > etime && (stime <= ntime || etime >= ntime))
		)  {
//LOG_TRC("[%s] pname[%s] stime[%04d] etime[%04d] ntime[%04d]", g_procname, g_sSchdDaly[i].pname, stime, etime, ntime);
			if(g_sSchdDaly[i].pid == 0)  {
				// � �ð��� ���� ���� �ƴ� ���α׷��� �⵿ ��Ų��.

				if(cnt != 0 && g_sSchdDaly[i].run_cnt > 0)		// ������� 10�ʿ� �� ������
					continue;

				if((pid = RunProg(g_sSchdDaly[i].pname, g_sSchdDaly[i].args)) <= 0)  {
					LOG_SYS("[%s] RunProg() fail.", g_procname);
					exit(0);
				} else  {
					g_sSchdDaly[i].pid = pid;
					g_sSchdDaly[i].kil_cnt = 0;
					LOG_SYS("[%s] [%s] [%s] �⵿ ����. (%d retry)", g_procname, g_sSchdDaly[i].pname, g_sSchdDaly[i].args, g_sSchdDaly[i].run_cnt);
				}

				g_sSchdDaly[i].run_cnt ++;
			}
		} else  {
			if(g_sSchdDaly[i].pid > 0)  {
				// � �ð����� ��� ���� ���� ���μ������� ���� ��ȣ�� ������.
				if(g_sSchdDaly[i].kil_cnt > 3)  {
					kill(g_sSchdDaly[i].pid, SIGKILL);
					LOG_SYS("[%s] [%s] ���� ���� ����. (%d retry)", g_procname, g_sSchdDaly[i].pname, g_sSchdDaly[i].kil_cnt);
				} else  {
					kill(g_sSchdDaly[i].pid, SIGTERM);
					LOG_SYS("[%s] [%s] ���� ��ȣ ����. (%d retry)", g_procname, g_sSchdDaly[i].pname, g_sSchdDaly[i].kil_cnt);

					g_sSchdDaly[i].kil_cnt ++;
				}

				g_sSchdDaly[i].run_cnt = 0;
			}
		}
	}

	return(0);
}

