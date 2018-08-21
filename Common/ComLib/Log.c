#include <time.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <dirent.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "ComLib.h"

/****************************************************************************
 * Prototype : int LogRotate(const char *, int , int)						*
 *																			*
 * 설명 : 로그 화일이 지정된 크기보다 크면 백업 로그를 만들며, 백업 로그	*
 *		  화일의 수를 지정할 수 있다.										*
 *																			*
 * 인수 : const char *fpath													*
 *			log 화일명 (경로 포함 가능)										*
 *		  int maxsize														*
 *			log file의 최대 크기 (단위는 KByte)								*
 *		  int nofl															*
 *			log file을 백업할 개수											*
 *																			*
 * 리턴 : ( 0) - 성공														*
 *		  (-1) - 실패														*
 *																			*
 * 작성 : 2000.12.01.														*
 * 수정 : 2000.12.29.														*
 *			로그 화일을 처음 작성할 경우 오픈 에러가 발생하는데				*
 *			이때 리턴값을 '0'을 돌려주어야 한다.							*
 ****************************************************************************/
int LogRotate(const char *fpath, int maxsize, int nofl)
{
	int			fd, i;
	char		source[256], target[256];
	struct stat	filestat;

	/* 로그화일의 최대 길이 지정이 0보다 작거나 같은면 실패					*/
	if(maxsize <= 0)  {
		snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] 로그화일의 최대 길이 지정이 0보다 작거나 같음", __FILE__, __LINE__);
#ifdef _LIB_DEBUG
		perror("[LogRotate]로그화일의 최대 길이 지정이 0보다 작거나 같음");
#endif

		return(-1);
	}

	if((fd = open(fpath, O_RDONLY)) <= 0)  {
		return(0);
	}

	if(fstat(fd, (struct stat *)&filestat) != 0)  {
		snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] fstat():%s", __FILE__, __LINE__, strerror(errno));
#ifdef _LIB_DEBUG
		perror("[LogRotate]fstat()");
#endif

		close(fd);
		return(-2);
	}
	close(fd);

	/* 화일 길이가 최대 길이보다 크면 백업 화일을 만든다.					*/
	if(filestat.st_size > maxsize * 1024)  {
		/* 백업 화일 수가 0이면 백업 화일을 만들지 않는다.					*/
		if(nofl <= 0)  {
			unlink(fpath);
		} else  {
			sprintf(target, "%s.%d", fpath, nofl);
			unlink(target);
			for(i = nofl; i > 1; i--)  {
				sprintf(target, "%s.%d", fpath, i);
				sprintf(source, "%s.%d", fpath, i - 1);
				rename(source, target);
			}
			sprintf(target, "%s.%d", fpath, i);
			rename(fpath, target);
		}
	}

	return(0);
}

/****************************************************************************
 * Prototype : int WriteLog(const void *, int, int,							*
 *							 const void *, va_list)							*
 *																			*
 * 설명 : 지정된 화일에 로그 메세지를 저장한다.								*
 *		  (주의 : 같은 이름이 존재할 경우 내용을 추가한다.)					*
 *																			*
 * 인수 : const void *fpath													*
 *			log 화일명 (경로 포함 가능)										*
 *		  int maxsize														*
 *			log file의 최대 크기 (단위는 KByte)								*
 *			이 값이 '0'이면 LogRotate를 하지 않는다.						*
 *		  int nofl															*
 *			log file을 백업할 개수											*
 *			이 값이 '0'이면 백업 화일을 만들지 않는다.						*
 *		  const void *fmt													*
 *			log message이거나 printf형식의 문장 포멧이다.					*
 *		  va_list args														*
 *			뒤에 올 argment들... (없을 수도 있지)							*
 *																			*
 * 리턴 : ( 0) - 성공														*
 *		  (-1) - 실패														*
 *																			*
 * 작성 : 2000.12.01.														*
 * 수정 : 2001.03.12.														*
 *			종전에는 메세지를 하나의 String으로만 받았는데 printf			*
 *			형식으로 받게 고쳤는데 마크로 정의를 할 수 없어서 문제다.		*
 *		  2001.03.13.														*
 *			함수 마크로를 만들 수 없어서 아래와 같이 수정하고 껍데기		*
 *			함수를 만들어 사용하기로 했다.									*
 *																			*
 *			예)																*
 *			int ACC_LOG(char *fmt, ...)										*
 *			{																*
 *				int ret;													*
 *				va_list args;												*
 *																			*
 *				va_start(args, fmt);										*
 *				ret = WriteLog(FILE_ACC_LOG, MAX_LOGSIZE, \					*
 *							NUM_OF_LOG, fmt, args);							*
 *				va_end(args);												*
 *				return(ret);												*
 *			}																*
 ****************************************************************************/
int WriteLog(const void *fpath, int maxsize, int nofl, const void *fmt, va_list args)
{
	char	msg[1024];
	FILE	*fp;

	vsprintf(msg, fmt, args);

	/* 로그 화일의 최대 길이가 0으로 지정되면 LogRotate를 하지 않는다.		*/
	if(maxsize > 0)  {
		if(LogRotate(fpath, maxsize, nofl) != 0)  {
			snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] LogRotate():%s", __FILE__, __LINE__, strerror(errno));
#ifdef _LIB_DEBUG
			printf("[WriteLog]LogRotate() fail.\n");
#endif

			return(-1);
		}
	}

	if((fp = fopen(fpath, "a")) == NULL)  {
		snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] fopen():%s", __FILE__, __LINE__, strerror(errno));
#ifdef _LIB_DEBUG
		perror("[WriteLog]fopen()");
#endif

		return(-2);
	}

	if(fseek(fp, SEEK_END, 0) == -1)  {
		snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] fseek():%s", __FILE__, __LINE__, strerror(errno));
#ifdef _LIB_DEBUG
		perror("[WriteLog]fseek()");
#endif

		fclose(fp);
		return(-3);
	}

	fprintf(fp, "[%s] %s\n", getTime_s(), msg);

	fclose(fp);

	return(0);
}

/****************************************************************************
 * Prototype : int WriteDailyLog(const char *, const char *,				*
 *								  const char *, ...)						*
 *																			*
 * 설명 : 일자별 로그 메세지를 저장한다.									*
 *																			*
 * 인수 : const char *fpath													*
 *			log 화일 경로 - 마지막 문자가 '/'이면 디렉토리 이름이되지만		*
 *			'/'이 아니면 화일 이름 앞에 붙는 접두사가 된다.					*
 *		  const char *extension												*
 *			log 화일명의 확장자												*
 *		  const char *fmt													*
 *			log message이거나 printf형식의 문장 포멧이다.					*
 *		  ...																*
 *			뒤에 올 argment들... (없을 수도 있지)							*
 *																			*
 * 리턴 : ( 0) - 성공														*
 *		  (-1) - 실패														*
 *																			*
 * 작성 : 2001.08.14.														*
 * 수정 : 2001.08.31.														*
 *			fpath의 마지막에 '/'이 없으면 추가하던 루틴은 삭제했는데,		*
 *			그 이유는 화일 이름의 앞에 접두사를 첨가할 수 있게 하기 위한	*
 *			깊은 뜻이 있다.													*
 *		  2001.11.15.														*
 *			멍청하게도 va_end()를 빼먹어서 추가했다.						*
 *		  2003.06.17.														*
 *			log를 작성할 디렉토리가 존재하는지 검사하고, 만일 존재하지		*
 *			않는다면 디렉토리를 만든다.										*
 ****************************************************************************/
int WriteDailyLog(const char *fpath, const char *extension, const char *fmt, ...)
{
	int		i, ret;
	char	tmp[255], dir_path[255];
	va_list	args;
	DIR		*dp;

	va_start(args, fmt);

	bzero(tmp, sizeof(tmp));
	strncpy(tmp, fpath, strlen(fpath));

	/* 2003.06.07 추가 - 디렉토리가 존재하지 않을 경우 디렉토리를 만든다.	*/
	for(i = sizeof(tmp) - 1; tmp[i] != '/'; i--);
	bzero(dir_path, sizeof(dir_path));
	strncpy(dir_path, tmp, i + 1);

	if((dp = opendir(dir_path)) == (DIR *)NULL)  {
		if(mkdir(dir_path, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP) != 0)  {
			snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] mkdir():%s", __FILE__, __LINE__, strerror(errno));
#ifdef _LIB_DEBUG
			perror("[WriteDailyLog]mkdir()");
#endif

			return(-1);
		}
	} else
		closedir(dp);
	/* 2003.06.07 추가 끝 													*/

	/* 로그 화일의 경로를 지정할때 마직막에 '/'가 없으면 추가한다.			*/
	/* 2001.08.31 수정 시작
	if(tmp[strlen(tmp) - 1] != '/')
		strcat(tmp, "/");
	2001.08.31 수정 끝														*/

	/* 지정된 경로와 일자, 확장자를 이용해 완전한 화일 경로를 작성한다.		*/
	sprintf(tmp, "%s%.8s.%s", tmp, getDateTime_14(), extension);

	ret = WriteLog(tmp, 0, 0, fmt, args);
	va_end(args);

	return(ret);
}
