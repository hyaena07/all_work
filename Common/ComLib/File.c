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
 * 설명 : 화일을 복사한다.													*
 *																			*
 * 인수 : const void *name1													*
 *			원본 화일명 (경로 포함 가능)									*
 *		  const void *name2													*
 *			복사본 화일명 (경로 포함 가능)									*
 *																			*
 * 리턴 : ( 0) - 성공														*
 *		  (-1) - 원본 화일 열기 실패										*
 *		  (-2) - 원본 화일 정보 가져오기 실패								*
 *		  (-3) - 복사본 화일 열기 실패										*
 *		  (-4) - 복사 실패													*
 *																			*
 * 작성 : 1999.12.17.														*
 * 수정 : 2001.03.14.														*
 *			복사본 화일을 열고 복사에 실패한 경우 복사본 화일을 삭제하고	*
 *			종료한다.														*
 *		  2002.05.02.														*
 *			복사본 화일과 같은 이름의 화일이 존재하면 에러 처리.			*
 ****************************************************************************/
int CopyFile(const void *name1, const void *name2)
{
	int			infile, outfile, nread;
	char		buffer[512];
	mode_t		old_umask;
	struct stat	infilestat;

	/* 원본 화일 열기														*/
	if((infile = open(name1, O_RDONLY)) < 0)  {
		snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] open():%s", __FILE__, __LINE__, strerror(errno));
#ifdef _LIB_DEBUG
		perror("[CopyFile]open(1)");
#endif

		return(-1);
	}

	/* 원본 화일 정보 가져오기												*/
	if(fstat(infile, (struct stat *)&infilestat) < 0)  {
		snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] fstat():%s", __FILE__, __LINE__, strerror(errno));
#ifdef _LIB_DEBUG
		perror("[CopyFile]fstat()");
#endif

		close(infile);
		return(-2);
	}

	/* 복사본 화일 생성														*/
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
 * 설명 : Memory Maped I/O를 이용한 화일 복사								*
 *																			*
 * 인수 : const void *name1													*
 *			원본 화일명 (경로 포함 가능)									*
 *		  const void *name2													*
 *			복사본 화일명 (경로 포함 가능)									*
 *																			*
 * 리턴 : ( 0) - 성공														*
 *		  (-1) - 원본 화일 열기 실패										*
 *		  (-2) - 원본 화일 정보 가져오기 실패								*
 *		  (-3) - 복사본 화일 열기 실패										*
 *		  (-4) - 복사본 화일 확장 실패										*
 *		  (-5) - 원본 화일을 위한 memory map 실패							*
 *		  (-6) - 복사본 화일을 위한 memory map 실패							*
 *																			*
 * 작성 : 2000.05.16.														*
 * 수정 : 2001.03.14.														*
 *			복사본 화일을 열고 복사에 실패한 경우 복사본 화일을 삭제하고	*
 *			종료한다.														*
 *		  2002.05.02.														*
 *			복사본 화일과 같은 이름의 화일이 존재하면 에러 처리.			*
 *****************************************************************************/
int CopyFile2(const void *name1, const void *name2)
{
	int			infile, outfile;
	char		*ptr_src, *ptr_dst;
	mode_t		old_umask;
	struct stat	statbuf;

	/* 원본과 복사본 화일 열기												*/
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

	/* 복사본 화일을 열고 길이를 확장한다.									*/
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

	/* 원본과 복사본 화일을 위한 memory map을 구성한다.						*/
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

	/* 실제로 복사하는 부분													*/
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
 * 설명 : open된 화일의 읽기 및 쓰기 잠금을 설정 또는 해제한다.				*
 *		  각각의 용도에 따라 mylib.h에 마크로를 정의하여 사용한다.			*
 *		  이 lock은 화일이 close()되거나 프로세스가 종료하면 해제되며, 화일	*
 *		  전체를 잠글때는 offset=0, whence=SEEK_SET, len=0을 사용.			*
 *																			*
 * 인수 : int fd															*
 *			file descriptor													*
 *		  int cmd															*
 *			F_GETLK - 잠김 봉쇄 상태인지를 결정한다.						*
 *			F_SETLK - 잠금을 설정.											*
 *			F_SETLKW - 잠금을 설정. 읽기, 쓰기 요청이 있을때 요청			*
 *					   프로그램은 sleep 상태가 된다.						*
 *		  off_t offset														*
 *			잠그거나 풀릴 영역의 시작 바이트								*
 *		  int whence														*
 *		  off_t len															*
 *			영역의 크기														*
 *																			*
 * 리턴 : fcntl의 그것과 같음												*
 *		  성공 - 0															*
 *		  실패 - -1															*
 *																			*
 * 작성 : 2000.03.31.														*
 * 수정 : 2001.12.12.														*
 *			테스트 완료. 테스트 관건은 쓰기위해 잠글때 write_lock(),		*
 *			읽기위해 잠글때 read_lock()를 쓴다는 것이다.					*
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
 * 설명 : 지정된 디스크립터에 n byte만큼 쓴다.								*
 *																			*
 * 인수 : int fd															*
 *			디스크립터														*
 *		  const void *vptr													*
 *			쓸 내용이 있는 포인터											*
 *		  size_t n															*
 *			쓸 byte 수														*
 *																			*
 * 리턴 : (== n) - 성공														*
 *		  (!= n) - 실패														*
 *		  ( < 0) - 에러 발생												*
 *																			*
 * 작성 : 2001.03.14. (사탕 준비해야하는데 이런거나 만들고 ...)				*
 * 수정 :																	*
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
 * 설명 : 지정된 디스크립터에서 n byte만큼 읽는다.							*
 *																			*
 * 인수 : int sd															*
 *			디스크립터														*
 *		  void *vptr														*
 *			읽은 내용을 저장할 포인터										*
 *		  size_t n															*
 *			읽을 byte 수													*
 *																			*
 * 리턴 : (== n) - 성공														*
 *		  (!= n) - 실패														*
 *		  ( < 0) - 에러 발생												*
 *																			*
 * 작성 : 2001.03.14.														*
 * 수정 :																	*
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
 * 설명 : File Lock 설정													*
 *																			*
 * 인수 : char *fpath														*
 *			파일 전체 경로													*
 *																			*
 * 리턴 : (== 0) - 성공														*
 *		  ( < 0) - 실패														*
 *																			*
 * 작성 : 2011.11.04.														*
 * 수정 :																	*
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
 * 설명 : File Lock 설정													*
 *																			*
 * 인수 : char *fpath														*
 *			파일 전체 경로													*
 *																			*
 * 리턴 : (== 0) - 성공														*
 *		  ( < 0) - 실패														*
 *																			*
 * 작성 : 2011.11.04.														*
 * 수정 :																	*
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
