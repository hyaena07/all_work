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
 * ���� : ���ں� ���� ���� �ʱ�ȭ (���� ���ڰ� �ٸ��� �ʱ�ȭ)				*
 *																			*
 * �μ� : char *jn_path														*
 *			���� ���� ��ü ���												*
 *		  int opt															*
 *			0		 - ���� ���� ������ ����, ������ ��� ���.				*
 *			JP_CREAT - ���� ������ ������ �����ϰ� �����Ѵ�.				*
 *																			*
 * ���� : ( < 0 ) ����														*
 *																			*
 * �ۼ� : 2017.07.19.														*
 * ���� :																	*
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

	// ������ ���ϰ� �ε��� ������ ��ü ��θ� �����.
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
 * ���� : ���� ������ �ʱ�ȭ�Ѵ�.											*
 *			���� ���� �� �������� ����										*
 *																			*
 * �μ� : char *jn_path														*
 *			���� ���� ��ü ���												*
 *		  int opt															*
 *			0		 - ���� ���� ������ ����, ������ ��� ���.				*
 *			JP_CREAT - ���� ������ ������ �����ϰ� �����Ѵ�.				*
 *																			*
 * ���� : ( < 0 ) ����														*
 *																			*
 * �ۼ� : 2012.01.02.														*
 * ���� :																	*
 ****************************************************************************/
int jpjn_init(char *jn_path, int opt)
{
	int		flags;
	int		dat_fd, idx_fd;
	char	dat_path[MAX_PATH_LEN + 1], idx_path[MAX_PATH_LEN + 1];

	// ������ ���ϰ� �ε��� ������ ��ü ��θ� �����.
	snprintf(dat_path, MAX_PATH_LEN, "%s.jndat", jn_path);
	snprintf(idx_path, MAX_PATH_LEN, "%s.jnidx", jn_path);

	// ���� ���� �ɼǿ� ���� open()�� flags �μ��� �����Ѵ�.
	if(opt & JP_CREAT)
		flags = O_RDWR | O_CREAT | O_TRUNC;
	else
		flags = O_RDWR | O_CREAT;

	// �����Ϳ� �ε��� ���� ����
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
 * ���� : ���� ���Ͽ� ����.													*
 *																			*
 * �μ� : char *jn_path														*
 *			���� ���� ��ü ���												*
 *		  char	*data														*
 *			���� ���Ͽ� �� ������ (text or bin)								*
 *		  int len															*
 *			������ ����														*
 *																			*
 * ���� : ( < 0 ) ����. �� �ܴ� �߰��� �ε��� ��ȣ							*
 *																			*
 * �ۼ� : 2012.01.02.														*
 * ���� : 2018.05.29.														*
 *			���϶� �߰�														*
 ****************************************************************************/
int jpjn_write(char *jn_path, char *data, int len)
{
	int		dat_fd, idx_fd;
	long	seq;
	char	dat_path[MAX_PATH_LEN + 1], idx_path[MAX_PATH_LEN + 1];
	struct	stat idx_stat;
	struct	jpjn_idx	idx_buf;

	// ������ ���ϰ� �ε��� ������ ��ü ��θ� �����.
	snprintf(dat_path, MAX_PATH_LEN, "%s.jndat", jn_path);
	snprintf(idx_path, MAX_PATH_LEN, "%s.jnidx", jn_path);

	// �����Ϳ� �ε��� ���� ����
	if((dat_fd = open(dat_path, O_RDWR, 0600)) < 0)  {
		snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] open(dat):%s", __FILE__, __LINE__, strerror(errno));
#ifdef _LIB_DEBUG
		perror("[jpjn_write]open(dat)");
#endif

		return(-1);
	}

	// ���� �� ���� : close() �ϸ� �ڵ� ������
	jpjn_lock(dat_fd);

	if((idx_fd = open(idx_path, O_RDWR, 0600)) < 0)  {
		snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] open(idx):%s", __FILE__, __LINE__, strerror(errno));
#ifdef _LIB_DEBUG
		perror("[jpjn_write]open(idx)");
#endif

		close(dat_fd);
		return(-2);
	}

	// �ε��� ���� ũ�� ���� (�ε��� ����ü�� ������� �˻�)
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
		snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] �ε��� ���� ���� ����.", __FILE__, __LINE__);
#ifdef _LIB_DEBUG
		printf("[%s:%d] �ε��� ���� ���� ����.\n", __FILE__, __LINE__);
#endif

		close(dat_fd);
		close(idx_fd);
		return(-4);
	}

	// ���� �ε��� ����ü�� ����� ������ ��ȣ�� �����Ѵ�.
	// ������, �÷��׸� �����ϰ� ���̴� �������� �ʴ´�.
	if((seq = jpjn_next_idx(idx_fd, idx_stat.st_size, &idx_buf)) < 0)  {
#ifdef _LIB_DEBUG
		printf("[%s:%d] ���� �ε��� ����� ����.\n", __FILE__, __LINE__);
#endif

		close(dat_fd);
		close(idx_fd);
		return(-5);
	}

	// �����͸� �� ��ġ�� �̵�.
	lseek(dat_fd, idx_buf.jpjn_ofs, SEEK_SET);
	// ���� �����͸� ���Ͽ� ����.
	if(write(dat_fd, data, len) != len)  {
		snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] write(dat):%s", __FILE__, __LINE__, strerror(errno));
#ifdef _LIB_DEBUG
		perror("[jpjn_write]write(dat)");
#endif

		close(dat_fd);
		close(idx_fd);
		return(-6);
	}

	// �ε��� ����ü�� ���̸� �����ϰ� ���Ͽ� ����.
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
 * ���� : ���� ���Ͽ��� �б�.												*
 *																			*
 * �μ� : char *jn_path														*
 *			���� ���� ��ü ���												*
 *		  long seq															*
 *			���� ������ ��ȣ												*
 *		  char *data														*
 *			���� ������ ����												*
 *																			*
 * ���� : ( < 0 ) ����.														*
 *		  ( == 0) ������ ����.	�� �ܴ� ���� ������ ����					*
 *																			*
 * �ۼ� : 2012.01.02.														*
 * ���� : 2018.05.29.														*
 *			���϶� �߰�														*
 ****************************************************************************/
int jpjn_read(char *jn_path, long seq, char *data)
{
	int		rc, dat_fd, idx_fd;
	char	dat_path[MAX_PATH_LEN + 1], idx_path[MAX_PATH_LEN + 1];
	struct	stat idx_stat;
	struct	jpjn_idx	idx_buf;

	// ������ ���ϰ� �ε��� ������ ��ü ��θ� �����.
	snprintf(dat_path, MAX_PATH_LEN, "%s.jndat", jn_path);
	snprintf(idx_path, MAX_PATH_LEN, "%s.jnidx", jn_path);

	// �����Ϳ� �ε��� ���� ����
	if((dat_fd = open(dat_path, O_RDWR, 0600)) < 0)  {
		snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] open(dat):%s", __FILE__, __LINE__, strerror(errno));
#ifdef _LIB_DEBUG
		perror("[jpjn_read]open(dat)");
#endif

		return(-1);
	}

	// ���� �� ���� : close() �ϸ� �ڵ� ������
	jpjn_lock(dat_fd);

	if((idx_fd = open(idx_path, O_RDWR, 0600)) < 0)  {
		snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] open(idx):%s", __FILE__, __LINE__, strerror(errno));
#ifdef _LIB_DEBUG
		perror("[jpjn_read]open(idx)");
#endif

		close(dat_fd);
		return(-2);
	}

	// �ε��� ���� ũ�� ���� (�ε��� ����ü�� ������� �˻�)
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
		snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] �ε��� ���� ���� ����.", __FILE__, __LINE__);
#ifdef _LIB_DEBUG
		printf("[%s:%d] �ε��� ���� ���� ����.\n", __FILE__, __LINE__);
#endif

		close(dat_fd);
		close(idx_fd);
		return(-4);
	}

	// �ش� �ε��� ���� ��ġ�� �̵� �� �ε��� �б�
	lseek(idx_fd, sizeof(struct jpjn_idx) * seq, SEEK_SET);
	if((rc = read(idx_fd, &idx_buf, sizeof(struct jpjn_idx))) != sizeof(struct jpjn_idx))  {
		/* �������� �ش��ϴ� �����Ͱ� ���� ��� '0'�� �����Ѵ�.		*/
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

	// �ش� �����͸� ���� ��ġ�� �̵� �� ������ �б�
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

	// �ε��� ���� ���̰� '0'�̸� ù �ε����� �����Ѵ�.
	if(fsize == 0)  {
		seq = 0;
		idx_buf->jpjn_ofs = 0;
		idx_buf->jpjn_flg = 0;

		return(0);
	}

	seq = fsize / sizeof(struct jpjn_idx);

	// ������ �ε��� ���� ��ġ�� �̵�.
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
