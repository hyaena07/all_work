#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <libgen.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "ComLib.h"
#include "jplib.h"

/****************************************************************************
 * Prototype : int jpsq_init(char *, int, int)								*
 *																			*
 * 설명 : 시퀀스 파일을 초기화한다.											*
 *		  파일락을 확인하지 않기 때문에 신중하게 사용하여야함.				*
 *																			*
 * 인수 : char *sq_path														*
 *			시퀀스 파일의 전체 경로											*
 *		  int sq_val														*
 *			시퀀스 초기값													*
 *		  int opt															*
 *			JP_CREAT - 파일이 없으면 생성한다.								*
 *																			*
 * 리턴 : ( < 0 ) 실패														*
 *																			*
 * 작성 : 2011.12.28.														*
 * 수정 :																	*
 ****************************************************************************/
int jpsq_init(char *sq_path, int sq_val, int opt)
{
	int		i, fd;
	int		flags;
	char	sq_str[80 + 1];
	char	sTemp[1024];

	bzero(sq_str, sizeof(sq_str));
	snprintf(sq_str, sizeof(sq_str) - 13, "Sequence %s : ", basename(sq_path));

	// 파일 생성 옵션에 따라 open()의 flags 인수를 설정한다.
	if(opt & JP_CREAT)
		flags = O_RDWR | O_CREAT;
	else
		flags = O_RDWR;

	// 시퀀스 파일 열기
	if((fd = open(sq_path, flags, 0600)) < 0)  {
		snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] open():%s", __FILE__, __LINE__, strerror(errno));
#ifdef _LIB_DEBUG
		perror("[jpsq_init]open()");
#endif

		return(-1);
	}

	// 파일 생성 옵션이 없을 경우 시퀀스명 검증을 실행한다.
	if(!(opt & JP_CREAT))  {
		bzero(sTemp, sizeof(sTemp));

		if(read(fd, sTemp, 80) < 0)  {
			snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] read():%s", __FILE__, __LINE__, strerror(errno));
#ifdef _LIB_DEBUG
			perror("[jpsq_init]read()");
#endif

			close(fd);
			return(-2);
		}

		if(memcmp(sTemp, sq_str, strlen(sq_str)) != 0)  {
			snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] 시퀀스명 확인 실패. [%s]", __FILE__, __LINE__, sTemp);
#ifdef _LIB_DEBUG
			printf("[%s:%d] 시퀀스명 확인 실패. [%s]\n", __FILE__, __LINE__, sTemp);
#endif

			close(fd);
			return(-3);
		}
	}

	// 시퀀스값 저장
	sprintf(sq_str + strlen(sq_str), "%12d", sq_val);
	lseek(fd, 0, SEEK_SET);
		if(write(fd, sq_str, strlen(sq_str)) != strlen(sq_str))  {
		snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] write():%s", __FILE__, __LINE__, strerror(errno));
#ifdef _LIB_DEBUG
		perror("[jpsq_init]write()");
#endif
		close(fd);
		return(-4);
	}

	close(fd);
	return(0);
}

/****************************************************************************
 * Prototype : int jpsq_read(char *)										*
 *																			*
 * 설명 : 시퀀스 파일 읽기고 값을 1 증가.									*
 *																			*
 * 인수 : char *sq_path														*
 *			시퀀스 파일의 전체 경로											*
 *		  int opt															*
 *			JP_NOWAIT - 파일을 사용중이면 오류 리턴한다.					*
 *																			*
 * 리턴 : (0 < ) 실패. 그 외는 시퀀스값.									*
 *																			*
 * 작성 : 2011.12.29.														*
 * 수정 :																	*
 ****************************************************************************/
int jpsq_read(char *sq_path, int opt)
{
	int		i, fd, seq;
	char	sq_str[80 + 1];
	char	*seq_ptr = (char *)NULL;
	char	sTemp[1024];

	bzero(sq_str, sizeof(sq_str));
	snprintf(sq_str, sizeof(sq_str) - 13, "Sequence %s : ", basename(sq_path));

	if((fd = open(sq_path, O_RDWR)) < 0)  {
		snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] open():%s", __FILE__, __LINE__, strerror(errno));
#ifdef _LIB_DEBUG
		perror("[jpsq_open]open()");
#endif

		return(-1);
	}

	// 파일락을 설정한다. JP_NOWAIT 옵션일 경우 락에 실패하면 바로 리턴.
	// 언락을 close() 할 때 자동으로 되므로 따로 할 필요없다.
	while(1)  {
		if(lock_reg(fd, F_SETLK,  F_WRLCK, 0, SEEK_SET, 1) < 0)  {
			if(opt & JP_NOWAIT)  {
				snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] 시퀀스 파일 이미 사용중.", __FILE__, __LINE__);
#ifdef _LIB_DEBUG
				printf("[%s:%d] 시퀀스 파일 이미 사용중.\n", __FILE__, __LINE__);
#endif

				close(fd);
				return(-2);
			} else  {
				usleep(100000);
			}
		} else  {
			break;
		}
	}

	if(read(fd, sTemp, 80) < 0)  {
		snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] read():%s", __FILE__, __LINE__, strerror(errno));
#ifdef _LIB_DEBUG
		perror("[jpsq_read]read()");
#endif

		close(fd);
		return(-3);
	}
printf("--[%s]\n", sTemp);

	// 시퀀스명 검증
	if(memcmp(sTemp, sq_str, strlen(sq_str)) != 0)  {
		snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] 시퀀스명 확인 실패. [%s]", __FILE__, __LINE__, sTemp);
#ifdef _LIB_DEBUG
		printf("[%s:%d] 시퀀스명 확인 실패. [%s]\n", __FILE__, __LINE__, sTemp);
#endif

		close(fd);
		return(-4);
	}

	// 시퀀스값의 시작 포인터 구하기
	for(i = 0; sTemp[i] != (char)0; i++)  {
		if(sTemp[i] == ':')  {
			seq_ptr = &sTemp[i + 2];
			break;
		}
	}

	if(seq_ptr == (char *)NULL)  {
		snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] 시퀀스 파일 형식 오류. [%s]", __FILE__, __LINE__, sTemp);
#ifdef _LIB_DEBUG
		printf("[%s:%d] 시퀀스 파일 형식 오류. [%s]\n", __FILE__, __LINE__, sTemp);
#endif

		close(fd);
		return(-5);
	}

	// 현재 시퀀스값 구하기
	seq = atol(seq_ptr);

	// 증가된 시퀀스값 저장
	sprintf(sq_str + strlen(sq_str), "%12d", seq + 1);
printf("--[%s]\n", sTemp);
	lseek(fd, 0, SEEK_SET);
	if(write(fd, sq_str, strlen(sq_str)) != strlen(sq_str))  {
		snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] write():%s", __FILE__, __LINE__, strerror(errno));
#ifdef _LIB_DEBUG
		perror("[jpsq_read]write()");
#endif

		close(fd);
		return(-6);
	}

	close(fd);

	return(seq);
}
