#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <libgen.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "ComLib.h"

/****************************************************************************
 * Prototype : int CopyFile(const void *, const void *)						*
 *																			*
 * ���� : ȭ���� �����Ѵ�.													*
 *																			*
 * �μ� : const void *name1													*
 *			���� ȭ�ϸ� (��� ���� ����)									*
 *		  const void *name2													*
 *			���纻 ȭ�ϸ� (��� ���� ����)									*
 *																			*
 * ���� : ( 0) - ����														*
 *		  (-1) - ���� ȭ�� ���� ����										*
 *		  (-2) - ���� ȭ�� ���� �������� ����								*
 *		  (-3) - ���纻 ȭ�� ���� ����										*
 *		  (-4) - ���� ����													*
 *																			*
 * �ۼ� : 1999.12.17.														*
 * ���� : 2001.03.14.														*
 *			���纻 ȭ���� ���� ���翡 ������ ��� ���纻 ȭ���� �����ϰ�	*
 *			�����Ѵ�.														*
 *		  2002.05.02.														*
 *			���纻 ȭ�ϰ� ���� �̸��� ȭ���� �����ϸ� ���� ó��.			*
 ****************************************************************************/
int CopyFile(const void *name1, const void *name2)
{
	int			infile, outfile, nread;
	char		buffer[512];
	mode_t		old_umask;
	struct stat	infilestat;

	/* ���� ȭ�� ����														*/
	if((infile = open(name1, O_RDONLY)) < 0)  {
		snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] open():%s", __FILE__, __LINE__, strerror(errno));
#ifdef _LIB_DEBUG
		perror("[CopyFile]open(1)");
#endif

		return(-1);
	}

	/* ���� ȭ�� ���� ��������												*/
	if(fstat(infile, (struct stat *)&infilestat) < 0)  {
		snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] fstat():%s", __FILE__, __LINE__, strerror(errno));
#ifdef _LIB_DEBUG
		perror("[CopyFile]fstat()");
#endif

		close(infile);
		return(-2);
	}

	/* ���纻 ȭ�� ����														*/
	old_umask = umask(000);
	if((outfile = open(name2, O_WRONLY | O_CREAT | O_EXCL, infilestat.st_mode)) < 0)  {
		snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] open():%s", __FILE__, __LINE__, strerror(errno));
#ifdef _LIB_DEBUG
		perror("[CopyFile]open(2)");
#endif

		close(infile);
		umask(old_umask);
		return(-3);
	}
	umask(old_umask);

	while((nread = read(infile, buffer, 512)) > 0)  {
		if(write(outfile, buffer, nread) < nread)  {
			snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] write():%s", __FILE__, __LINE__, strerror(errno));
#ifdef _LIB_DEBUG
			perror("[CopyFile]write()");
#endif

			close(infile);
			close(outfile);
			unlink(name2);
			return(-4);
		}
	}

	close(infile);
	close(outfile);
	return(0);
}

/****************************************************************************
 * Prototype : int CopyFile2(const void *, const void *)					*
 *																			*
 * ���� : Memory Maped I/O�� �̿��� ȭ�� ����								*
 *																			*
 * �μ� : const void *name1													*
 *			���� ȭ�ϸ� (��� ���� ����)									*
 *		  const void *name2													*
 *			���纻 ȭ�ϸ� (��� ���� ����)									*
 *																			*
 * ���� : ( 0) - ����														*
 *		  (-1) - ���� ȭ�� ���� ����										*
 *		  (-2) - ���� ȭ�� ���� �������� ����								*
 *		  (-3) - ���纻 ȭ�� ���� ����										*
 *		  (-4) - ���纻 ȭ�� Ȯ�� ����										*
 *		  (-5) - ���� ȭ���� ���� memory map ����							*
 *		  (-6) - ���纻 ȭ���� ���� memory map ����							*
 *																			*
 * �ۼ� : 2000.05.16.														*
 * ���� : 2001.03.14.														*
 *			���纻 ȭ���� ���� ���翡 ������ ��� ���纻 ȭ���� �����ϰ�	*
 *			�����Ѵ�.														*
 *		  2002.05.02.														*
 *			���纻 ȭ�ϰ� ���� �̸��� ȭ���� �����ϸ� ���� ó��.			*
 *****************************************************************************/
int CopyFile2(const void *name1, const void *name2)
{
	int			infile, outfile;
	char		*ptr_src, *ptr_dst;
	mode_t		old_umask;
	struct stat	statbuf;

	/* ������ ���纻 ȭ�� ����												*/
	if((infile = open(name1, O_RDONLY)) < 0)  {
	snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] open():%s", __FILE__, __LINE__, strerror(errno));
#ifdef _LIB_DEBUG
		perror("[CopyFile2]open(1)");
#endif

		return(-1);
	}

	if(fstat(infile, (struct stat *)&statbuf) < 0)  {
		snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] fstat():%s", __FILE__, __LINE__, strerror(errno));
#ifdef _LIB_DEBUG
		perror("[CopyFile2]fstat()");
#endif

		close(infile);
		return(-2);
	}

	/* ���纻 ȭ���� ���� ���̸� Ȯ���Ѵ�.									*/
	old_umask = umask(000);
	if((outfile = open(name2, O_WRONLY | O_CREAT | O_EXCL, statbuf.st_mode)) < 0)  {
		snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] open():%s", __FILE__, __LINE__, strerror(errno));
#ifdef _LIB_DEBUG
		perror("[CopyFile2]open(2)");
#endif

		close(infile);
		umask(old_umask);
		return(-3);
	}
	umask(old_umask);

	if(lseek(outfile, statbuf.st_size - 1, SEEK_SET) == -1)  {
		snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] lseek():%s", __FILE__, __LINE__, strerror(errno));
#ifdef _LIB_DEBUG
		perror("[CopyFile2]lseek()");
#endif

		close(outfile);
		close(infile);
		unlink(name2);
		return(-4);
	}

	if(write(outfile, " ", 1) != 1)  {
		snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] write():%s", __FILE__, __LINE__, strerror(errno));
#ifdef _LIB_DEBUG
		perror("[CopyFile2]write()");
#endif

		close(outfile);
		close(infile);
		unlink(name2);
		return(-5);
	}

	/* ������ ���纻 ȭ���� ���� memory map�� �����Ѵ�.						*/
	if((ptr_src = mmap((void *)0, statbuf.st_size, PROT_READ, MAP_SHARED, infile, 0)) == MAP_FAILED)  {
		snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] mmap():%s", __FILE__, __LINE__, strerror(errno));
#ifdef _LIB_DEBUG
		perror("[CopyFile2]mmap(1)");
#endif

		close(outfile);
		close(infile);
		unlink(name2);
		return(-6);
	}

	if((ptr_dst = mmap((void *)0, statbuf.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, outfile, 0)) == MAP_FAILED)  {
		snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] mmap():%s", __FILE__, __LINE__, strerror(errno));
#ifdef _LIB_DEBUG
		perror("[CopyFile2]mmap(2)");
#endif

		munmap(ptr_src, statbuf.st_size);

		close(outfile);
		close(infile);
		unlink(name2);
		return(-7);
	}

	/* ������ �����ϴ� �κ�													*/
	memcpy(ptr_dst, (char *)ptr_src, statbuf.st_size);

	munmap(ptr_dst, statbuf.st_size);
	munmap(ptr_src, statbuf.st_size);
	close(outfile);
	close(infile);
	return(0);
}

/****************************************************************************
 * Prototype : int lock_reg(int, int, off_t, int, off_t)					*
 *																			*
 * ���� : open�� ȭ���� �б� �� ���� ����� ���� �Ǵ� �����Ѵ�.				*
 *		  ������ �뵵�� ���� mylib.h�� ��ũ�θ� �����Ͽ� ����Ѵ�.			*
 *		  �� lock�� ȭ���� close()�ǰų� ���μ����� �����ϸ� �����Ǹ�, ȭ��	*
 *		  ��ü�� ��۶��� offset=0, whence=SEEK_SET, len=0�� ���.			*
 *																			*
 * �μ� : int fd															*
 *			file descriptor													*
 *		  int cmd															*
 *			F_GETLK - ��� ���� ���������� �����Ѵ�.						*
 *			F_SETLK - ����� ����.											*
 *			F_SETLKW - ����� ����. �б�, ���� ��û�� ������ ��û			*
 *					   ���α׷��� sleep ���°� �ȴ�.						*
 *		  off_t offset														*
 *			��װų� Ǯ�� ������ ���� ����Ʈ								*
 *		  int whence														*
 *		  off_t len															*
 *			������ ũ��														*
 *																			*
 * ���� : fcntl�� �װͰ� ����												*
 *		  ���� - 0															*
 *		  ���� - -1															*
 *																			*
 * �ۼ� : 2000.03.31.														*
 * ���� : 2001.12.12.														*
 *			�׽�Ʈ �Ϸ�. �׽�Ʈ ������ �������� ��۶� write_lock(),		*
 *			�б����� ��۶� read_lock()�� ���ٴ� ���̴�.					*
 ****************************************************************************/
int lock_reg(int fd, int cmd, int type, off_t offset, int whence, off_t len)
{
	struct flock	lock;

	lock.l_type		= type;			/* F_RDLCK, F_WRLCK, F_UNLCK 			*/
	lock.l_start	= offset;		/* byte offset, relative to l_whence	*/
	lock.l_whence	= whence;		/* SEEK_SET, SEEK_CUR, SEEK_END			*/
	lock.l_len		= len;			/* #byte (0 means to EOF)				*/

	if(fcntl(fd, cmd, &lock) < 0)  {
		snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] fcntl():%s", __FILE__, __LINE__, strerror(errno));
#ifdef _LIB_DEBUG
		perror("[lock_reg]fcntl()");
#endif

		return(-1);
	}

	return(0);
}

/****************************************************************************
 * Prototype : ssize_t Writen(int, const void *, size_t)					*
 *																			*
 * ���� : ������ ��ũ���Ϳ� n byte��ŭ ����.								*
 *																			*
 * �μ� : int fd															*
 *			��ũ����														*
 *		  const void *vptr													*
 *			�� ������ �ִ� ������											*
 *		  size_t n															*
 *			�� byte ��														*
 *																			*
 * ���� : (== n) - ����														*
 *		  (!= n) - ����														*
 *		  ( < 0) - ���� �߻�												*
 *																			*
 * �ۼ� : 2001.03.14. (���� �غ��ؾ��ϴµ� �̷��ų� ����� ...)				*
 * ���� :																	*
 ****************************************************************************/
ssize_t Writen(int fd, const void *vptr, size_t n)
{
	int			nleft;
	ssize_t		nwritten;
	const char	*ptr;

	ptr = vptr;
	nleft = n;

	while(nleft > 0)  {
		if((nwritten = write(fd, ptr, nleft)) <= (ssize_t)0)  {
			if(errno == EINTR)
				continue;

			snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] write():%s", __FILE__, __LINE__, strerror(errno));
#ifdef _LIB_DEBUG
			perror("[Writen]write()");
#endif
			return(nwritten);
		}
		nleft -= nwritten;
		ptr += nwritten;
	}

	return(n);
}

/****************************************************************************
 * Prototype : ssize_t Readn(int, void *, size_t)							*
 *																			*
 * ���� : ������ ��ũ���Ϳ��� n byte��ŭ �д´�.							*
 *																			*
 * �μ� : int sd															*
 *			��ũ����														*
 *		  void *vptr														*
 *			���� ������ ������ ������										*
 *		  size_t n															*
 *			���� byte ��													*
 *																			*
 * ���� : (== n) - ����														*
 *		  (!= n) - ����														*
 *		  ( < 0) - ���� �߻�												*
 *																			*
 * �ۼ� : 2001.03.14.														*
 * ���� :																	*
 ****************************************************************************/
ssize_t Readn(int sd, void *vptr, size_t n)
{
	int		nleft;
	char	*ptr;
	ssize_t	nread;

	ptr = vptr;
	nleft = n;

	while(nleft > 0)  {
		if((nread = read(sd, ptr, nleft)) < (ssize_t)0)  {
			if(errno == EINTR)
				continue;

			snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] read():%s", __FILE__, __LINE__, strerror(errno));
#ifdef _LIB_DEBUG
			perror("[Readn]read()");
#endif
			return(nread);
		} else if(nread == 0)
			break;
		nleft -= nread;
		ptr += nread;
	}

	return(n - nleft);
}

/****************************************************************************
 * Prototype : int LockFile(char *)											*
 *																			*
 * ���� : File Lock ����													*
 *																			*
 * �μ� : char *fpath														*
 *			���� ��ü ���													*
 *																			*
 * ���� : (== 0) - ����														*
 *		  ( < 0) - ����														*
 *																			*
 * �ۼ� : 2011.11.04.														*
 * ���� :																	*
 ****************************************************************************/
int LockFile(char *fpath)
{
	int	fd;

	if((fd = open(fpath, O_WRONLY)) < 0)  {
		snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] open():%s", __FILE__, __LINE__, strerror(errno));
#ifdef _LIB_DEBUG
		perror("[LockFile]open()");
#endif

		return(-1);
	}

	if(lock_reg(fd, F_SETLK,  F_WRLCK, 0, SEEK_SET, 1) < 0)  {
		snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] lock_reg():%s", __FILE__, __LINE__, strerror(errno));
#ifdef _LIB_DEBUG
		perror("[LockFile]lock_reg()");
#endif

		return(-2);
	}

	return(0);
}

/****************************************************************************
 * Prototype : int unLockFile(char *)										*
 *																			*
 * ���� : File Lock ����													*
 *																			*
 * �μ� : char *fpath														*
 *			���� ��ü ���													*
 *																			*
 * ���� : (== 0) - ����														*
 *		  ( < 0) - ����														*
 *																			*
 * �ۼ� : 2011.11.04.														*
 * ���� :																	*
 ****************************************************************************/
int unLockFile(char *fpath)
{
	int	fd;

	if((fd = open(fpath, O_WRONLY)) < 0)  {
		snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] open():%s", __FILE__, __LINE__, strerror(errno));
#ifdef _LIB_DEBUG
		perror("[unLockFile]open()");
#endif

		return(-1);
	}

	if(lock_reg(fd, F_SETLK,  F_UNLCK, 0, SEEK_SET, 1) < 0)  {
		snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] lock_reg():%s", __FILE__, __LINE__, strerror(errno));
#ifdef _LIB_DEBUG
		perror("[unLockFile]lock_reg()");
#endif

		return(-2);
	}

	return(0);
}
