#include <time.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "ComLib.h"
#include "jplib.h"

/****************************************************************************
 * Prototype : int jpjn_init_daily(char *)									*
 *																			*
 * 설명 : 일자별 저널 파일 초기화 (생성 일자가 다르면 초기화)				*
 *																			*
 * 인수 : char *jn_path														*
 *			저널 파일 전체 경로												*
 *		  int opt															*
 *			0		 - 기존 파일 없으면 생성, 있으면 계속 사용.				*
 *			JP_CREAT - 기존 파일이 있으면 삭제하고 생성한다.				*
 *																			*
 * 리턴 : ( < 0 ) 실패														*
 *																			*
 * 작성 : 2017.07.19.														*
 * 수정 :																	*
 ****************************************************************************/
int jpjn_init_daily(char *jn_path)
{
	int		rc;
	char	dat_path[MAX_PATH_LEN + 1], idx_path[MAX_PATH_LEN + 1];
    time_t  clock;
    struct  tm  *tm, td;
    struct  stat    st;

    if((rc = jpjn_init(jn_path, 0)) < 0)  {
    	return(rc);
    }

	// 데이터 파일과 인덱스 파일의 전체 경로를 만든다.
	snprintf(dat_path, MAX_PATH_LEN, "%s.jndat", jn_path);
	snprintf(idx_path, MAX_PATH_LEN, "%s.jnidx", jn_path);

    clock = time(0);
    tm = localtime(&clock);
    memcpy(&td, tm, sizeof(struct tm));

    if(stat(idx_path, &st) < 0)  {
		snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] stat():%s", __FILE__, __LINE__, strerror(errno));
#ifdef _LIB_DEBUG
		perror("[jpjn_init_daily]stat()");
#endif
		return(-1);
    }

    tm = localtime(&st.st_ctime);
    if(tm->tm_mon != td.tm_mon || tm->tm_mday != td.tm_mday)  {
	    if((rc = jpjn_init(jn_path, JP_CREAT)) < 0)  {
	    	return(rc);
	    }
    }

    return(0);
}

/****************************************************************************
 * Prototype : int jpjn_init(char *, int)									*
 *																			*
 * 설명 : 저널 파일을 초기화한다.											*
 *			파일 락을 걸 수없음에 유의										*
 *																			*
 * 인수 : char *jn_path														*
 *			저널 파일 전체 경로												*
 *		  int opt															*
 *			0		 - 기존 파일 없으면 생성, 있으면 계속 사용.				*
 *			JP_CREAT - 기존 파일이 있으면 삭제하고 생성한다.				*
 *																			*
 * 리턴 : ( < 0 ) 실패														*
 *																			*
 * 작성 : 2012.01.02.														*
 * 수정 :																	*
 ****************************************************************************/
int jpjn_init(char *jn_path, int opt)
{
	int		flags;
	int		dat_fd, idx_fd;
	char	dat_path[MAX_PATH_LEN + 1], idx_path[MAX_PATH_LEN + 1];

	// 데이터 파일과 인덱스 파일의 전체 경로를 만든다.
	snprintf(dat_path, MAX_PATH_LEN, "%s.jndat", jn_path);
	snprintf(idx_path, MAX_PATH_LEN, "%s.jnidx", jn_path);

	// 파일 생성 옵션에 따라 open()의 flags 인수를 설정한다.
	if(opt & JP_CREAT)
		flags = O_RDWR | O_CREAT | O_TRUNC;
	else
		flags = O_RDWR | O_CREAT;

	// 데이터와 인덱스 파일 열기
	if((dat_fd = open(dat_path, flags, 0600)) < 0)  {
		snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] open(dat):%s", __FILE__, __LINE__, strerror(errno));
#ifdef _LIB_DEBUG
		perror("[jpjn_init]open(dat)");
#endif

		return(-2);
	}

	if((idx_fd = open(idx_path, flags, 0600)) < 0)  {
		snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] open(idx):%s", __FILE__, __LINE__, strerror(errno));
#ifdef _LIB_DEBUG
		perror("[jpjn_init]open(idx)");
#endif

		close(dat_fd);
		return(-3);
	}

	close(dat_fd);
	close(idx_fd);
	return(0);
}

/****************************************************************************
 * Prototype : int jpjn_write(char *, char *, int)							*
 *																			*
 * 설명 : 저널 파일에 쓰기.													*
 *																			*
 * 인수 : char *jn_path														*
 *			저널 파일 전체 경로												*
 *		  char	*data														*
 *			저널 파일에 쓸 데이터 (text or bin)								*
 *		  int len															*
 *			데이터 길이														*
 *																			*
 * 리턴 : ( < 0 ) 실패. 그 외는 추가한 인덱스 번호							*
 *																			*
 * 작성 : 2012.01.02.														*
 * 수정 : 2018.05.29.														*
 *			파일락 추가														*
 ****************************************************************************/
int jpjn_write(char *jn_path, char *data, int len)
{
	int		dat_fd, idx_fd;
	long	seq;
	char	dat_path[MAX_PATH_LEN + 1], idx_path[MAX_PATH_LEN + 1];
	struct	stat idx_stat;
	struct	jpjn_idx	idx_buf;

	// 데이터 파일과 인덱스 파일의 전체 경로를 만든다.
	snprintf(dat_path, MAX_PATH_LEN, "%s.jndat", jn_path);
	snprintf(idx_path, MAX_PATH_LEN, "%s.jnidx", jn_path);

	// 데이터와 인덱스 파일 열기
	if((dat_fd = open(dat_path, O_RDWR, 0600)) < 0)  {
		snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] open(dat):%s", __FILE__, __LINE__, strerror(errno));
#ifdef _LIB_DEBUG
		perror("[jpjn_write]open(dat)");
#endif

		return(-1);
	}

	// 파일 락 설정 : close() 하면 자동 해제됨
	jpjn_lock(dat_fd);

	if((idx_fd = open(idx_path, O_RDWR, 0600)) < 0)  {
		snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] open(idx):%s", __FILE__, __LINE__, strerror(errno));
#ifdef _LIB_DEBUG
		perror("[jpjn_write]open(idx)");
#endif

		close(dat_fd);
		return(-2);
	}

	// 인덱스 파일 크기 검증 (인덱스 구조체의 배수인지 검사)
	if(stat(idx_path, &idx_stat) < 0)  {
		snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] stat():%s", __FILE__, __LINE__, strerror(errno));
#ifdef _LIB_DEBUG
		perror("[jpjn_write]stat()");
#endif

		close(dat_fd);
		close(idx_fd);
		return(-3);
	}

	if(idx_stat.st_size != 0 && idx_stat.st_size % sizeof(struct jpjn_idx) != 0)  {
		snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] 인덱스 파일 길이 오류.", __FILE__, __LINE__);
#ifdef _LIB_DEBUG
		printf("[%s:%d] 인덱스 파일 길이 오류.\n", __FILE__, __LINE__);
#endif

		close(dat_fd);
		close(idx_fd);
		return(-4);
	}

	// 다음 인덱스 구조체를 만들고 시퀀스 번호를 리턴한다.
	// 오프셋, 플레그만 셋팅하고 길이는 셋팅하지 않는다.
	if((seq = jpjn_next_idx(idx_fd, idx_stat.st_size, &idx_buf)) < 0)  {
#ifdef _LIB_DEBUG
		printf("[%s:%d] 다음 인덱스 만들기 실패.\n", __FILE__, __LINE__);
#endif

		close(dat_fd);
		close(idx_fd);
		return(-5);
	}

	// 데이터를 쓸 위치로 이동.
	lseek(dat_fd, idx_buf.jpjn_ofs, SEEK_SET);
	// 저널 데이터를 파일에 쓴다.
	if(write(dat_fd, data, len) != len)  {
		snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] write(dat):%s", __FILE__, __LINE__, strerror(errno));
#ifdef _LIB_DEBUG
		perror("[jpjn_write]write(dat)");
#endif

		close(dat_fd);
		close(idx_fd);
		return(-6);
	}

	// 인덱스 구조체에 길이를 셋팅하고 파일에 쓴다.
	idx_buf.jpjn_len = len;
	if(write(idx_fd, &idx_buf, sizeof(struct jpjn_idx)) != sizeof(struct jpjn_idx))  {
		snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] write(idx):%s", __FILE__, __LINE__, strerror(errno));
#ifdef _LIB_DEBUG
		perror("[jpjn_write]write(idx)");
#endif

		close(dat_fd);
		close(idx_fd);
		return(-7);
	}

	close(dat_fd);
	close(idx_fd);
	return(seq);
}

/****************************************************************************
 * Prototype : int jpjn_read(char *, long, char *)							*
 *																			*
 * 설명 : 저널 파일에서 읽기.												*
 *																			*
 * 인수 : char *jn_path														*
 *			저널 파일 전체 경로												*
 *		  long seq															*
 *			읽을 시퀀스 번호												*
 *		  char *data														*
 *			읽은 데이터 버퍼												*
 *																			*
 * 리턴 : ( < 0 ) 실패.														*
 *		  ( == 0) 데이터 없음.	그 외는 읽은 데이터 길이					*
 *																			*
 * 작성 : 2012.01.02.														*
 * 수정 : 2018.05.29.														*
 *			파일락 추가														*
 ****************************************************************************/
int jpjn_read(char *jn_path, long seq, char *data)
{
	int		rc, dat_fd, idx_fd;
	char	dat_path[MAX_PATH_LEN + 1], idx_path[MAX_PATH_LEN + 1];
	struct	stat idx_stat;
	struct	jpjn_idx	idx_buf;

	// 데이터 파일과 인덱스 파일의 전체 경로를 만든다.
	snprintf(dat_path, MAX_PATH_LEN, "%s.jndat", jn_path);
	snprintf(idx_path, MAX_PATH_LEN, "%s.jnidx", jn_path);

	// 데이터와 인덱스 파일 열기
	if((dat_fd = open(dat_path, O_RDWR, 0600)) < 0)  {
		snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] open(dat):%s", __FILE__, __LINE__, strerror(errno));
#ifdef _LIB_DEBUG
		perror("[jpjn_read]open(dat)");
#endif

		return(-1);
	}

	// 파일 락 설정 : close() 하면 자동 해제됨
	jpjn_lock(dat_fd);

	if((idx_fd = open(idx_path, O_RDWR, 0600)) < 0)  {
		snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] open(idx):%s", __FILE__, __LINE__, strerror(errno));
#ifdef _LIB_DEBUG
		perror("[jpjn_read]open(idx)");
#endif

		close(dat_fd);
		return(-2);
	}

	// 인덱스 파일 크기 검증 (인덱스 구조체의 배수인지 검사)
	if(stat(idx_path, &idx_stat) < 0)  {
		snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] stat():%s", __FILE__, __LINE__, strerror(errno));
#ifdef _LIB_DEBUG
		perror("[jpjn_read]stat()");
#endif

		close(dat_fd);
		close(idx_fd);
		return(-3);
	}

	if(idx_stat.st_size != 0 && idx_stat.st_size % sizeof(struct jpjn_idx) != 0)  {
		snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] 인덱스 파일 길이 오류.", __FILE__, __LINE__);
#ifdef _LIB_DEBUG
		printf("[%s:%d] 인덱스 파일 길이 오류.\n", __FILE__, __LINE__);
#endif

		close(dat_fd);
		close(idx_fd);
		return(-4);
	}

	// 해당 인덱스 읽을 위치로 이동 후 인덱스 읽기
	lseek(idx_fd, sizeof(struct jpjn_idx) * seq, SEEK_SET);
	if((rc = read(idx_fd, &idx_buf, sizeof(struct jpjn_idx))) != sizeof(struct jpjn_idx))  {
		/* 시퀀스에 해당하는 데이터가 없을 경우 '0'을 리턴한다.		*/
		if(rc == 0)  {
			close(dat_fd);
			close(idx_fd);
			return(0);
		}

		snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] read(idx):%s", __FILE__, __LINE__, strerror(errno));
#ifdef _LIB_DEBUG
		perror("[jpjn_read]read(idx)");
#endif

		close(dat_fd);
		close(idx_fd);
		return(-5);
	}

	if(idx_buf.jpjn_flg != 0)  {
		close(dat_fd);
		close(idx_fd);
		return(0);
	}

	// 해당 데이터를 읽을 위치로 이동 후 데이터 읽기
	lseek(dat_fd, idx_buf.jpjn_ofs, SEEK_SET);
	if(read(dat_fd, data, idx_buf.jpjn_len) != idx_buf.jpjn_len)  {
		snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] read(dat):%s", __FILE__, __LINE__, strerror(errno));
#ifdef _LIB_DEBUG
		perror("[jpjn_read]read(dat)");
#endif

		close(dat_fd);
		close(idx_fd);
		return(-6);
	}

	close(dat_fd);
	close(idx_fd);
	return(idx_buf.jpjn_len);
}

int jpjn_next_idx(int fd, off_t fsize, struct jpjn_idx *idx_buf)
{
	long	seq;

	// 인덱스 파일 길이가 '0'이면 첫 인덱스로 셋팅한다.
	if(fsize == 0)  {
		seq = 0;
		idx_buf->jpjn_ofs = 0;
		idx_buf->jpjn_flg = 0;

		return(0);
	}

	seq = fsize / sizeof(struct jpjn_idx);

	// 마지막 인덱스 읽을 위치로 이동.
	lseek(fd, fsize - sizeof(struct jpjn_idx), SEEK_SET);
	if(read(fd, idx_buf, sizeof(struct jpjn_idx)) != sizeof(struct jpjn_idx))  {
		snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] read():%s", __FILE__, __LINE__, strerror(errno));
#ifdef _LIB_DEBUG
		perror("[jpjn_next_idx]read()");
#endif

		return(-1);
	}

	idx_buf->jpjn_ofs = idx_buf->jpjn_ofs + idx_buf->jpjn_len;
	idx_buf->jpjn_flg = 0;

	return(seq);
}

int jpjn_lock(int fd)
{
	if(lock_reg(fd, F_SETLKW,  F_WRLCK, 0, SEEK_SET, 1) < 0)  {
		snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] lock_reg():%s", __FILE__, __LINE__, strerror(errno));
#ifdef _LIB_DEBUG
		perror("[jpjn_lock]lock_reg()");
#endif

		return(-1);
	}

	return(0);
}

int jpjn_unLock(int fd)
{
	if(lock_reg(fd, F_SETLK,  F_UNLCK, 0, SEEK_SET, 1) < 0)  {
		snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] lock_reg():%s", __FILE__, __LINE__, strerror(errno));
#ifdef _LIB_DEBUG
		perror("[jpjn_unLock]lock_reg()");
#endif

		return(-1);
	}

	return(0);
}
