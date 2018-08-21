#include <stdio.h>
#include <regex.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <libgen.h>
#include <sys/types.h>

#include "ComLib.h"
#include "jpcomd.h"

char	g_configpath	[MAX_PATH_LEN + 1];		// 환경 파일 경로
char	g_procname		[MAX_FILE_LEN + 1];		// 프로세스명
char	g_syslogpath	[MAX_PATH_LEN + 1];		// System 로그 경로 및 접두사
char	g_trclogpath	[MAX_PATH_LEN + 1];		// Trace 로그 경로 및 접두사
char	g_tmplogpath	[MAX_PATH_LEN + 1];		// 자식 프로세스를 위한 임시 로그 경로 및 접두사

int ConfigParse(char *ini_path, struct stConfigParse *ini_data)
{
	int						i, j, serror = 0, ecode, line_no;
	char					buff[255], read_buff[255];
	char					rpat[255];
	char					ebuff[255];
	char					value[255];
	regex_t					ini_re;
	regmatch_t				apm[5];
	FILE					*fp;
	struct stConfigParse	*ptr_data;

	// 데이터 초기화
	ptr_data = ini_data;
	while(1)  {
		ptr_data->ini_data[0] = 0;
		ptr_data ++;
		if(ptr_data->ini_name[0] == 0)  {
			break;
		}
	}

	bzero(buff,			sizeof(buff));
	bzero(read_buff,	sizeof(read_buff));

	if((fp = fopen(ini_path, "r")) == (FILE *)NULL)  {
//		PutLog(g_sys_log, LOG_NORMAL, "[ERR] [%s][%d]fopen() : %s\n", __FILE__, __LINE__, strerror(errno));
		return(-1);
	}

	for(line_no = 1; ; line_no ++)  {
		if(fgets(read_buff, sizeof(read_buff), fp) == (char *)NULL)  {
			break;
		}

		// 주석문 및 newline 처리
		for(i = 0, j = 0; read_buff[i] != 0; i ++, j ++)  {
			if(read_buff[i] == '#' || read_buff[i] == '\n')  {
				buff[j] = 0;
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

		// 빈 줄 여부 확인
		TRIM(buff);
		if(strlen(buff) == 0)  {
			continue;
		}

		// 기본적인 문장 형태를 검사한다.
		sprintf(rpat, "^[[:space:]]{0,}[0-9A-Za-z_/.]{1,}[[:space:]]{0,}=[[:space:]]{0,}[[:print:]]{1,}[[:space:]]{0,}$");
		if((ecode = regcomp(&ini_re, rpat, REG_EXTENDED)) != 0) {
			regerror(ecode, &ini_re, ebuff, 100);
//			PutLog(g_sys_log, LOG_NORMAL, "[ERR] [%s][%d]regcomp() : [%d]%s\n", __FILE__, __LINE__, ecode, ebuff);
		}
		if((ecode = regexec(&ini_re, buff, (size_t)0, apm, 0)) != 0)  {
			if(ecode != REG_NOMATCH)  {
				regerror(ecode, &ini_re, ebuff, 100);
//				PutLog(g_sys_log, LOG_NORMAL, "[ERR] [%s][%d]regexec() : [%d]%s\n", __FILE__, __LINE__, ecode, ebuff);
				regfree(&ini_re);
				continue;
			} else  {
//				PutLog(g_sys_log, LOG_NORMAL, "[ERR] [%s][%d]문법오류 : %d line [%s]\n", __FILE__, __LINE__, line_no, buff);
				serror = 1;
				regfree(&ini_re);
				break;
			}
		}

		// 마지막 과정
		ptr_data = ini_data;
		while(1)  {
			if(ptr_data->ini_name[0] == 0)  {
				break;
			}

			sprintf(rpat, "^[[:space:]]{0,}%s[[:space:]]{0,}=[[:space:]]{0,}(%s)[[:space:]]{0,}$", ptr_data->ini_name, ptr_data->ini_pat);
			if((ecode = regcomp(&ini_re, rpat, REG_EXTENDED)) != 0)  {
				regerror(ecode, &ini_re, ebuff, 100);
//				PutLog(g_sys_log, LOG_NORMAL, "[ERR] [%s][%d]regcomp() : [%d]%s\n", __FILE__, __LINE__, ecode, ebuff);
			}
			if((ecode = regexec(&ini_re, buff, (size_t)2, apm, REG_EXTENDED)) != 0)  {
				if(ecode != REG_NOMATCH)  {
					regerror(ecode, &ini_re, ebuff, 100);
//					PutLog(g_sys_log, LOG_NORMAL, "[ERR] [%s][%d]regexec() : [%d]%s\n", __FILE__, __LINE__, ecode, ebuff);
				}
				regfree(&ini_re);

				ptr_data ++;
				continue;
			} else  {
				bzero(ebuff, sizeof(ebuff));
				strncpy(ebuff, buff + apm[1].rm_so, apm[1].rm_eo - apm[1].rm_so);
				strcpy(ptr_data->ini_data, ebuff);
/*
#ifdef FILELOG
				makeLog(LOG_FILE, "LIB_", "%s(%d) %s : [%s]\n", __FILE__, __LINE__, ptr_data->ini_name, ebuff);
#endif
*/
				regfree(&ini_re);
			}

			ptr_data ++;
		}
	}

	ptr_data = ini_data;
	while(1)  {
		if(ptr_data->ini_data[0] == 0)  {
//			PutLog(g_sys_log, LOG_NORMAL, "[ERR] [%s][%d]항목이 없습니다.[%s]\n", __FILE__, __LINE__, ptr_data->ini_name);
			serror = 1;
			break;
		}

		ptr_data ++;
		if(ptr_data->ini_name[0] == 0)  {
			break;
		}
	}

	fclose(fp);

	if(serror == 0)
		return(0);
	else
		return(-2);
}

void LOG_SYS(char *fmt, ...)
{
	va_list	args;

	va_start(args, fmt);
	WriteLog(g_syslogpath, 0, 0, fmt, args);
	va_end(args);

	return;
}

void LOG_TRC(char *fmt, ...)
{
	va_list	args;

	va_start(args, fmt);
	WriteLog(g_trclogpath, 0, 0, fmt, args);
	va_end(args);

	return;
}

void LOG_TMP(char *fmt, ...)
{
	va_list	args;

	va_start(args, fmt);
	WriteLog(g_tmplogpath, 0, 0, fmt, args);
	va_end(args);

	return;
}

pid_t RunProgWithLog(const char *pname, const char *args)
{
	char	cmd[MAX_PATH_LEN + MAX_FILE_LEN + 1];
	char	buff[1024 + 1];
	pid_t	pid;
	FILE	*fp;

	pid = fork();
	switch(pid)  {
		case	0	:
			snprintf(g_tmplogpath, 	MAX_PATH_LEN, "%s/%s_%.8s.log", getenv("JP_LOGS"), basename(pname), getDateTime_14());
			sprintf(cmd, "%s %s", pname, args);

			LOG_TMP("----- 시작 [%s]", cmd);
			if((fp = popen(cmd, "r")) == (FILE *)NULL)  {
				exit(1);
			}
			while(1)  {
				bzero(buff, sizeof(buff));
				if(fgets(buff, 1024, fp) == (char *)NULL)  {
					LOG_TMP("----- 종료 [%s]", cmd);
					break;
				} else  {
					if(buff[strlen(buff) - 1] == '\n')
						buff[strlen(buff) - 1] = 0;
					LOG_TMP("%s", buff);
				}
			}
			pclose(fp);
			exit(0);
		case	-1	:
			exit(1);
		default		:
			return(pid);
	}

	return(0);
}

/*
pid_t RunProgWithLog(const char *pname, const char *args)
{
	pid_t	pid;

	pid = fork();
	switch(pid)  {
		case	0	:
LOG_SYS("pname[%s] args[%s]", pname, args);
			execlp(pname, pname, args, (char *)NULL);
		case	-1	:
			return(-1);
		default		:
			return(pid);
	}

	return(0);
}
*/

int RunProgramList(char *fname)
{
	int		i, j, wait_status;
	char	read_buff[256], buff[256];
	char	fpath[MAX_PATH_LEN + 1];
	char	pname[MAX_FILE_LEN], args[MAX_PATH_LEN];
	pid_t	pid;
	FILE	*fp;

	snprintf(fpath, MAX_PATH_LEN, "%s/%s", g_configpath, fname);

	if((fp = fopen(fpath, "r")) == (FILE *)NULL)  {
		LOG_SYS("[%s] fopen() fail. [%s]", g_procname, strerror(errno));
		return(-1);
	}

	while(1)  {
		bzero(read_buff,	sizeof(read_buff));
		bzero(buff,			sizeof(buff));
		bzero(pname,		sizeof(pname));
		bzero(args,			sizeof(args));

		if(fgets(read_buff, sizeof(read_buff) - 1, fp) == (char *)NULL)
			break;

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

		for(i = 0; ; i ++)  {
			if((buff[i] != ' ') && (buff[i] != 0x00))
				pname[i] = buff[i];
			else
				break;
		}

		if(buff[i] == ' ')
			snprintf(args, MAX_PATH_LEN, "%s", &buff[i + 1]);

		pid = RunProgWithLog(pname, args);
		if(pid <= 0)  {
			LOG_SYS("[%s] RunProgWithLog() fail. [%s] [%s]", g_procname, pname, args);
		} else  {
			LOG_SYS("[%s] [%s] [%s] 기동 성공", g_procname, pname, args);
			waitpid(pid, &wait_status, 0);
			LOG_SYS("[%s] [%s] [%s] 종료 확인 [%d]", g_procname, pname, args, WEXITSTATUS(wait_status));
		}
	}

	fclose(fp);

	return(0);
}
