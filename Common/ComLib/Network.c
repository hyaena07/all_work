#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <libgen.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "ComLib.h"

/****************************************************************************
 * Prototype : int Bind(int, int, struct sockaddr_in *)						*
 *																			*
 * ���� : Socket�� ���� Ŭ���̾�Ʈ�� ������ ��ٸ���.						*
 *																			*
 * �μ� : int Port															*
 *			���� ��� �� Port ��ȣ											*
 *		  int BackLog														*
 *			���� ��� ������ ���� �����Ѵ�.									*
 *		  struct sockadd_in *Addr											*
 *			������ ��� �� Socket�� ������ �����ֱ� ����.					*
 *																			*
 * ���� : (!= -1) ���� �����̸� ���� ��ũ���͸� �����ش�.					*
 *		  (== -1) ����														*
 *																			*
 * �ۼ� : 2001.03.27.														*
 * ���� : 2010.04.15.														*
 *			PIN_WAIT �� ��Ʈ ������� ���� ���� �߻��� ��õ� �ϵ��� ����	*
 ****************************************************************************/
int Bind(int Port, int BackLog, struct sockaddr_in *BindAddr)
{
	int	sd;
	int	bind_retry = 10;	/* ��Ʈ ������� ���� ������ ��õ� Ƚ��		*/

	/* Socket�� �����Ѵ�.		*/
	if((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)  {
		snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] socket():%s", __FILE__, __LINE__, strerror(errno));
#ifdef _LIB_DEBUG
		perror("[Bind]socket");
#endif

		return(-1);
	}

	while(1)  {
		BindAddr->sin_family = AF_INET;
		BindAddr->sin_port = htons(Port);
		BindAddr->sin_addr.s_addr = INADDR_ANY;
		bzero(&(BindAddr->sin_zero), 8);

		if(bind(sd, (struct sockaddr *)BindAddr, (socklen_t)sizeof(struct sockaddr)) < 0)  {
			/* ��Ʈ ������� ���� ��õ�									*/
			if(bind_retry > 0 && errno == EADDRINUSE)  {
				bind_retry --;
				sleep(1);
				continue;
			}

			snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] bind():%s", __FILE__, __LINE__, strerror(errno));
#ifdef _LIB_DEBUG
			perror("[Bind]bind");
#endif

			close(sd);
			return(-2);
		}

		break;
	}

	if(listen(sd, BackLog) == -1)  {
		snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] listen():%s", __FILE__, __LINE__, strerror(errno));
#ifdef _LIB_DEBUG
		perror("[Bind]listen");
#endif

		close(sd);
		return(-3);
	}

	return(sd);
}

/****************************************************************************
 * Prototype : int Bind_Force(int, int, struct sockaddr_in *)				*
 *																			*
 * ���� : Socket�� ���� �����ϰ� ���� Ŭ���̾�Ʈ�� ������ ��ٸ���.		*
 *																			*
 * �μ� : int Port															*
 *			���� ��� �� Port ��ȣ											*
 *		  int BackLog														*
 *			���� ��� ������ ���� �����Ѵ�.									*
 *		  struct sockadd_in *Addr											*
 *			������ ��� �� Socket�� ������ �����ֱ� ����.					*
 *																			*
 * ���� : (!= -1) ���� �����̸� ���� ��ũ���͸� �����ش�.					*
 *		  (== -1) ����														*
 *																			*
 * �ۼ� : 2010.05.14.														*
 * ���� : 																	*
 *																			*
 ****************************************************************************/
int Bind_Force(int Port, int BackLog, struct sockaddr_in *BindAddr)
{
	int	sd, optval;
	int	bind_retry = 10;	/* ��Ʈ ������� ���� ������ ��õ� Ƚ��		*/

	/* Socket�� �����Ѵ�.													*/
	if((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)  {
		snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] socket():%s", __FILE__, __LINE__, strerror(errno));
#ifdef _LIB_DEBUG
		perror("[BindF]socket");
#endif

		return(-1);
	}

	/* ������ �Ӽ��� ���� �����ϵ��� �ٲ۴�.								*/
	optval = 1;
	if(setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(int)) < 0)  {
		snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] setsockopt():%s", __FILE__, __LINE__, strerror(errno));
#ifdef _LIB_DEBUG
		perror("[BindF]setsockopt");
#endif

		close(sd);
		return(-2);
	}

	BindAddr->sin_family = AF_INET;
	BindAddr->sin_port = htons(Port);
	BindAddr->sin_addr.s_addr = INADDR_ANY;
	bzero(&(BindAddr->sin_zero), 8);

	if(bind(sd, (struct sockaddr *)BindAddr, (socklen_t)sizeof(struct sockaddr)) < 0)  {
		snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] bind():%s", __FILE__, __LINE__, strerror(errno));
#ifdef _LIB_DEBUG
		perror("[BindF]bind");
#endif

		close(sd);
		return(-3);
	}

	if(listen(sd, BackLog) == -1)  {
		snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] listen():%s", __FILE__, __LINE__, strerror(errno));
#ifdef _LIB_DEBUG
		perror("[BindF]listen");
#endif

		close(sd);
		return(-4);
	}

	return(sd);
}

/****************************************************************************
 * Prototype : int Accept(int, long int, long int, struct sockaddr_in *)	*
 *																			*
 * ���� : ������ ���� ��ũ���ͷ� ������ �õ��ϴ� Ŭ���̾�Ʈ�� �ִٸ�		*
 *		  ������ �δ´�.													*
 *																			*
 * �μ� : int sd															*
 *			������ ���� ���� ��ũ����										*
 *		  long int sec														*
 *			������ ��ٸ� �ð��� ��											*
 *		  long int usec														*
 *			������ ��ٸ� ����ũ�� ��										*
 *		  struct sockaddr_in *NewAddr										*
 *			���� �� ���Ӱ� ������ Socket�� ������ �����ֱ� ����.			*
 *																			*
 * ���� : ( -1) ����														*
 *		  (  0) Time Out													*
 *		  (> 0) ���Ӱ� �ξ��� ���� ��ũ����								*
 *																			*
 * �ۼ� : 2001.03.27.														*
 * ���� : 2002.05.02.														*
 *			sec�� usec�� ��� '0'�̸� Time Out���� ���� ����Ѵ�. ����		*
 *			��� ����� ���� �ް� ���� ��� usec�� '1'���ϸ� �����ð���		*
 *			���� �����Ƿ� �� ������ ���� �� ����.							*
 ****************************************************************************/
int Accept(int sd, long int sec, long int usec, struct sockaddr_in *NewAddr)
{
	int				newsd, sin_size, ret;
	fd_set			sds;
	struct timeval	tv, *tv_ptr;

	FD_ZERO(&sds);
	FD_SET(sd, &sds);

	if(sec == 0 && usec == 0)  {
		tv_ptr = (struct timeval *)NULL;
	} else  {
		tv.tv_sec = sec;
		tv.tv_usec = usec;
		tv_ptr = &tv;
	}

	if((ret = select(sd + 1, &sds, NULL, NULL, tv_ptr)) <= 0)  {
		snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] select():%s", __FILE__, __LINE__, strerror(errno));
#ifdef _LIB_DEBUG
		perror("[Accept]select");
#endif
		return(ret);
	}

	if(!FD_ISSET(sd, &sds))
		return(0);

	sin_size = sizeof(struct sockaddr);
	if((newsd = accept(sd, (struct sockaddr *)NewAddr, &sin_size)) == -1)  {
		snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] accept():%s", __FILE__, __LINE__, strerror(errno));
#ifdef _LIB_DEBUG
		perror("[Accept]accept");
#endif
	}

	return(newsd);
}

/****************************************************************************
 * Prototype : int isIPAddr(const char *)									*
 *																			*
 * ���� : ���ڿ��� IP �ּҷ� �������� �˻��Ѵ�								*
 *		  (�� ã�� ���� �ȸ��� �ɰ� ������......)						*
 *																			*
 * �μ� : const char *str													*
 *		  �˻��� ���ڿ�														*
 *																			*
 * ���� : ( -1) IP �ּҷ� ������											*
 *		  (  0) IP �ּҷ� ����												*
 *																			*
 * �ۼ� : 2001.03.27.														*
 * ���� : 2001.08.13.														*
 *			ó������ ���� ���ڰ� '.'�� '0'�ΰ��� �˻縦 �ߴµ� �� ����		*
 *			�˻� ������ �߰��Ͽ���.											*
 *		  2012.06.18.														*
 *			���� �ʿ����.													*
 ****************************************************************************/
int isIPAddr(const char *str)
{
	int				count, flag;
	register int	i;

	/* IP �ּ��� �ּ� ����(X.X.X) ���� ������ IP �ּҷ� ������				*/
	if(strlen(str) < 5)
		return(-1);

	/* IP �ּҿ� '.'�� 3�� �ִ°��� ����									*/
	for(i = 0, count = 0; str[i] != 0; i++)  {
		if(str[i] == '.')
			count++;
	}
	if(count != 3)
		return(-2);

	/* ���ڿ� '.' �� ���� �ٸ� ���ڰ� ����ִ°��� �˻�						*/
	for(i = 0; str[i] != 0; i++)
		if(str[i] != '.' && (str[i] < '0' || str[i] > '9'))
			return(-3);

	/* '.'�� ���޾� ������ �̰͵� IP �ּҷ� ������							*/
	for(i = 0, flag = 0; str[i] != 0; i++)  {
		if(str[i] == '.')  {
			if(flag == 1)
				return(-4);
			else
				flag = 1;
		} else
		flag = 0;
	}

	/* ���ڰ� 255���� ū�� �ƴ����� �˻縦 �ؾ��ϴµ� �����Ƽ� ����			*/

	return(0);
}

/****************************************************************************
 * Prototype : int Connect(const char *, int, struct sockaddr_in *)			*
 *																			*
 * ���� : Socket�� ���� ������ �����Ѵ�.									*
 *																			*
 * �μ� : const char *Addr													*
 *			������ ������ �ּ�(IP or ������)								*
 *		  int Port															*
 *			������ ������ Port												*
 *		  struct sockaddr_in *ConAddr										*
 *			���ӵ� Socket�� ������ �����ֱ� ����							*
 *																			*
 * ���� : (== -1) ����														*
 *		  (!= -1) ���ӵ� Socket												*
 *																			*
 * �ۼ� : 2001.03.27.														*
 * ���� :																	*
 ****************************************************************************/
int Connect(const char *Addr, int Port, struct sockaddr_in *ConAddr)
{
	int				sd, ret;
	struct hostent	*he;

	if((ret = isIPAddr(Addr)) == -1)  {
		/* Addr�� ������ �ּ� �����̶�� �Ǵ��� ���.						*/
		if((he = (struct hostent *)gethostbyname(Addr)) == (struct hostent *)NULL)  {
			snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] gethostbyname():%s", __FILE__, __LINE__, strerror(errno));
#ifdef _LIB_DEBUG
			perror("[Connect]gethostbyname");
#endif
			return(-1);
		}
	}

	/* Socket�� �����Ѵ�.													*/
	if((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)  {
		snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] socket():%s", __FILE__, __LINE__, strerror(errno));
#ifdef _LIB_DEBUG
		perror("[Connect]socket");
#endif

		return(-2);
	}

	ConAddr->sin_family = AF_INET;
	ConAddr->sin_port = htons(Port);

	if(ret == -1)
		ConAddr->sin_addr = *((struct in_addr *)he->h_addr);
	else
		ConAddr->sin_addr.s_addr = inet_addr(Addr);

	bzero(&(ConAddr->sin_zero), 8);

	if(connect(sd, (struct sockaddr *)ConAddr, (socklen_t)sizeof(struct sockaddr)) == -1)  {
		snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] connect():%s", __FILE__, __LINE__, strerror(errno));
#ifdef _LIB_DEBUG
		perror("[Connect]connect");
#endif

		close(sd);
		return(-3);
	}

	return(sd);
}

/****************************************************************************
 * Prototype : int Connect_sa(struct sockaddr_in *)							*
 *																			*
 * ���� : Socket�� ���� sockaddr�� �̿��Ͽ� ������ �����Ѵ�.				*
 *																			*
 * �μ� : struct sockaddr_in *ConAddr										*
 *			������ Socket�� ����											*
 *																			*
 * ���� : (== -1) ����														*
 *		  (!= -1) ���ӵ� Socket												*
 *																			*
 * �ۼ� : 2012.05.16.														*
 * ���� :																	*
 ****************************************************************************/
int Connect_sa(struct sockaddr_in *ConAddr)
{
	int				sd, ret;
	struct hostent	*he;

	/* Socket�� �����Ѵ�.													*/
	if((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)  {
		snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] socket():%s", __FILE__, __LINE__, strerror(errno));
#ifdef _LIB_DEBUG
		perror("[Connect]socket");
#endif

		return(-1);
	}

	if(connect(sd, (struct sockaddr *)ConAddr, (socklen_t)sizeof(struct sockaddr)) == -1)  {
		snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] connect():%s", __FILE__, __LINE__, strerror(errno));
#ifdef _LIB_DEBUG
		perror("[Connect]connect");
#endif

		close(sd);
		return(-2);
	}

	return(sd);
}

/****************************************************************************
 * Prototype : int ConnectTimeout											*
 *					(const char *, int, struct sockaddr_in *, char *, int)	*
 *																			*
 * ���� : Socket�� ���� ������ �����Ѵ�. (timeout ���� ����)				*
 *																			*
 * �μ� : const char *Addr													*
 *			������ ������ �ּ�(IP or ������)								*
 *		  int Port															*
 *			������ ������ Port												*
 *		  struct sockaddr_in *ConAddr										*
 *			���ӵ� Socket�� ������ �����ֱ� ����							*
 *		  int sec															*
 *			���� ��� �ð� (��)												*
 *																			*
 * ���� : (== -1) ����														*
 *		  (!= -1) ���ӵ� Socket												*
 *																			*
 * �ۼ� : 2012.05.11.														*
 * ���� :																	*
 ****************************************************************************/
int ConnectTimeout(const char *Addr, int Port, struct sockaddr_in *ConAddr, int sec)
{
	int				sd, ret, flags;
	fd_set 			fds_r, fds_w;
	struct timeval	tv;

	struct hostent	*he;

	if((ret = isIPAddr(Addr)) == -1)  {
		/* Addr�� ������ �ּ� �����̶�� �Ǵ��� ���.						*/
		if((he = (struct hostent *)gethostbyname(Addr)) == (struct hostent *)NULL)  {
			snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] gethostbyname():%s", __FILE__, __LINE__, strerror(errno));
#ifdef _LIB_DEBUG
			perror("[Connect]gethostbyname");
#endif
			return(-1);
		}
	}

	/* Socket�� �����Ѵ�.													*/
	if((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)  {
		snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] socket(1):%s", __FILE__, __LINE__, strerror(errno));
#ifdef _LIB_DEBUG
		perror("[ConnectTimeout]socket(1)");
#endif
		return(-2);
	}

	flags = fcntl(sd, F_GETFL, 0);
	fcntl(sd, F_SETFL, flags | O_NONBLOCK);

	ConAddr->sin_family = AF_INET;
	ConAddr->sin_port = htons(Port);

	if(ret == -1)
		ConAddr->sin_addr = *((struct in_addr *)he->h_addr);
	else
		ConAddr->sin_addr.s_addr = inet_addr(Addr);

	bzero(&(ConAddr->sin_zero), 8);

	ret = connect(sd, (struct sockaddr *)ConAddr, (socklen_t)sizeof(struct sockaddr));
	if(ret < 0)  {
		if(errno != EINPROGRESS)  {
			snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] connect():%s", __FILE__, __LINE__, strerror(errno));
#ifdef _LIB_DEBUG
			perror("[ConnectTimeout]connect");
#endif

			fcntl(sd, F_SETFL, flags);
			close(sd);
			return(-3);
		}

		FD_ZERO(&fds_r);
		FD_SET(sd, &fds_r);
		fds_w = fds_r;

		/* Timeout ����														*/
		tv.tv_sec = sec;	tv.tv_usec = 1;

		ret = select(sd + 1, &fds_r, &fds_w, NULL, &tv);
		if(ret < 0)  {
			snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] select():%s", __FILE__, __LINE__, strerror(errno));
#ifdef _LIB_DEBUG
			perror("[ConnectTimeout]select");
#endif

			fcntl(sd, F_SETFL, flags);
			close(sd);
			return(-4);
		}

		if(FD_ISSET(sd, &fds_r) || FD_ISSET(sd, &fds_w))  {
			fcntl(sd, F_SETFL, flags);
			return(sd);
		} else  {
			fcntl(sd, F_SETFL, flags);
			close(sd);
			return(-5);
		}
	}

	fcntl(sd, F_SETFL, flags);
	return(sd);
}

/****************************************************************************
 * Prototype : int ConnectTimeout_as(struct sockaddr_in *, char *, int)		*
 *																			*
 * ���� : Socket�� ���� sockaddr�� �̿��Ͽ� ������ �����Ѵ�. 				*
 *		  (timeout ���� ����)												*
 *																			*
 * �μ� : struct sockaddr_in *ConAddr										*
 *			������ Socket�� ����											*
 *		  int sec															*
 *			���� ��� �ð� (��)												*
 *																			*
 * ���� : (== -1) ����														*
 *		  (!= -1) ���ӵ� Socket												*
 *																			*
 * �ۼ� : 2012.05.16.														*
 * ���� :																	*
 ****************************************************************************/
int ConnectTimeout_sa(struct sockaddr_in *ConAddr, int sec)
{
	int				sd, ret, flags;
	fd_set			fds_r, fds_w;
	struct timeval	tv;

	/* Socket�� �����Ѵ�.													*/
	if((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)  {
		snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] socket(1):%s", __FILE__, __LINE__, strerror(errno));
#ifdef _LIB_DEBUG
		perror("[ConnectTimeout]socket(1)");
#endif

		return(-1);
	}

	flags = fcntl(sd, F_GETFL, 0);
	fcntl(sd, F_SETFL, flags | O_NONBLOCK);

	ret = connect(sd, (struct sockaddr *)ConAddr, (socklen_t)sizeof(struct sockaddr));
	if(ret < 0)  {
		if(errno != EINPROGRESS)  {
			snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] connect():%s", __FILE__, __LINE__, strerror(errno));
#ifdef _LIB_DEBUG
			perror("[ConnectTimeout]connect");
#endif

			fcntl(sd, F_SETFL, flags);
			close(sd);
			return(-2);
		}

		FD_ZERO(&fds_r);
		FD_SET(sd, &fds_r);
		fds_w = fds_r;

		/* Timeout ����														*/
		tv.tv_sec = sec;	 tv.tv_usec = 1;

		ret = select(sd + 1, &fds_r, &fds_w, NULL, &tv);
		if(ret < 0)  {
			snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] select():%s", __FILE__, __LINE__, strerror(errno));
#ifdef _LIB_DEBUG
			perror("[ConnectTimeout]select");
#endif

			fcntl(sd, F_SETFL, flags);
			close(sd);
			return(-3);
		}

		if(FD_ISSET(sd, &fds_r) || FD_ISSET(sd, &fds_w))  {
			fcntl(sd, F_SETFL, flags);
			return(sd);
		} else  {
			fcntl(sd, F_SETFL, flags);
			close(sd);
			return(-4);
		}
	}

	fcntl(sd, F_SETFL, flags);
	return(-5);
}

/****************************************************************************
 * Prototype : int CheckDisconnect(int)										*
 *																			*
 * ���� : �۽� ���� ������ ���� ���� �˻�.									*
 *		  (���� : �ݵ�� �۽� ���� ���Ͽ����� ����� ��.)					*
 *																			*
 * �μ� : int fd															*
 *			���� ��ũ����													*
 *																			*
 * ���� : ( 0) - ����														*
 *		  (-1) - ����														*
 *																			*
 * �ۼ� : 2000.05.26.														*
 * ���� :																	*
 ****************************************************************************/
int CheckDisconnect(int fd)
{
	fd_set			cfds;
	struct timeval	tv;

	FD_ZERO(&cfds);
	FD_SET(fd, &cfds);

	tv.tv_sec = 0;
	tv.tv_usec = 0;

	return(select(fd + 1, &cfds, NULL, NULL, &tv));
}

/****************************************************************************
 * Prototype : int Sendn(int, const void *, size_t, int)					*
 *																			*
 * ���� : ������ ���� ��ũ���Ϳ� n byte��ŭ �۽�.							*
 *																			*
 * �μ� : int sd															*
 *			���� ��ũ����													*
 *		  const void *vptr													*
 *			�� ������ �ִ� ������											*
 *		  size_t n															*
 *			�� byte ��														*
 *		  int flags															*
 *			send()�� ������ flags											*
 *																			*
 * ���� : (== n) - ����														*
 *		  (!= n) - ����														*
 *		  ( < 0) - ���� �߻�												*
 *																			*
 * �ۼ� : 2001.03.14. (���� �غ��ؾ��ϴµ� �̷��ų� �����)					*
 * ���� :																	*
 ****************************************************************************/
int Sendn(int sd, const void *vptr, size_t n, int flags)
{
	int			nleft;
	int			nsend;
	const char	*ptr;

	ptr = vptr;
	nleft = n;

	while(nleft > 0)  {
		if((nsend = send(sd, ptr, nleft, flags)) <= 0)  {
			if(errno == EINTR)
				continue;

			snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] send():%s", __FILE__, __LINE__, strerror(errno));
#ifdef _LIB_DEBUG
			perror("[Sendn]send");
#endif
			return(nsend);
		}
		nleft -= nsend;
		ptr += nsend;
	}

	return(n);
}

/****************************************************************************
 * Prototype : int Recvn(int, void *, size_t, int)							*
 *																			*
 * ���� : ������ ���� ��ũ���Ϳ��� n byte��ŭ ����.						*
 *																			*
 * �μ� : int sd															*
 *			���� ��ũ����													*
 *		  void *vptr														*
 *			���� ������ ������ ������										*
 *		  size_t n															*
 *			���� byte ��													*
 *		  int flags															*
 *			send()�� ������ flags											*
 *																			*
 * ���� : (== n) - ����														*
 *		  (!= n) - ����														*
 *		  ( < 0) - ���� �߻�												*
 *																			*
 * �ۼ� : 2001.03.14.														*
 * ���� :																	*
 ****************************************************************************/
int Recvn(int sd, void *vptr, size_t n, int flags)
{
	int		nleft;
	int		nrecv;
	char	*ptr;

	ptr = vptr;
	nleft = n;

	while(nleft > 0)  {
		if((nrecv = recv(sd, ptr, nleft, flags)) < 0)  {
			if(errno == EINTR)
				continue;

			snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] recv():%s", __FILE__, __LINE__, strerror(errno));
#ifdef _LIB_DEBUG
			perror("[Recvn]recv");
#endif
			return(nrecv);
		} else if(nrecv == 0)  {
			break;
		}
		nleft -= nrecv;
		ptr += nrecv;
	}

	return(n - nleft);
}

/****************************************************************************
 * Prototype : int Recvw(int, void *, size_t, int, int)						*
 *																			*
 * ���� : ������ ���� ��ũ���Ϳ��� ���� �ð� �̳��� ����.					*
 *																			*
 * �μ� : int sd															*
 *			���� ��ũ����													*
 *		  void *vptr														*
 *			���� ������ ������ ������										*
 *		  size_t n															*
 *			���� byte ��													*
 *		  int flags															*
 *			send()�� ������ flags											*
 *		  int sec															*
 *			��� �ð� (��)													*
 *																			*
 * ���� : (== n) - ����														*
 *		  (!= n) - ����														*
 *		  ( < 0) - ���� �߻�												*
 *																			*
 * �ۼ� : 2010.04.08.														*
 * ���� :																	*
 ****************************************************************************/
int Recvw(int sd, void *vptr, size_t n, int flags, int sec)
{
	int				ret;
	int				nrecv;
	char			*ptr;
	fd_set			fds;
	struct timeval	tv;

	ptr = vptr;

	FD_ZERO(&fds);
	FD_SET(sd, &fds);

	/* Timeout ����															*/
	tv.tv_sec = sec;	tv.tv_usec = 0;

	while(1)  {
		ret = select(sd + 1, &fds, NULL, NULL, &tv);
		if(ret < 0)  {
			if(errno != EINTR)  {
				snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] select():%s", __FILE__, __LINE__, strerror(errno));
#ifdef _LIB_DEBUG
				perror("[Recvw]select");
#endif
				return(-1);
			} else  {
				continue;
			}
		} else if(ret == 0)  {
			return(0);
		} else if(FD_ISSET(sd, &fds))  {
			break;
		}
	}

	if((nrecv = recv(sd, ptr, n, flags)) < 0)  {
		snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] recv():%s", __FILE__, __LINE__, strerror(errno));
#ifdef _LIB_DEBUG
		perror("[Recvw]recv");
#endif

		return(-2);
	}

	return(nrecv);
}

/****************************************************************************
 * Prototype : int SetBlockMode(int)										*
 *																			*
 * ���� : ������ ���� ��ũ���͸� block mode�� ����.						*
 *																			*
 * �μ� : int sd															*
 *			���� ��ũ����													*
 *																			*
 * ���� : ( >  0) - ����													*
 *		  (== -1) - ����													*
 *																			*
 * �ۼ� : 2001.03.26.														*
 * ���� :																	*
 ****************************************************************************/
int SetBlockMode(int sd)
{
	int	fl;

	/* ������ BLOCK���� ����												*/
	fl=fcntl(sd, F_GETFL, 0);
	if (fl != -1)
		return(fcntl(sd, F_SETFL, fl & ~O_NONBLOCK));

	return(-1);
}

/****************************************************************************
 * Prototype : int SetNonBlockMode(int)										*
 *																			*
 * ���� : ������ ���� ��ũ���͸� nonblock mode�� ����.					*
 *																			*
 * �μ� : int sd															*
 *			���� ��ũ����													*
 *																			*
 * ���� : ( >  0) - ����													*
 *		  (== -1) - ����													*
 *																			*
 * �ۼ� : 2001.03.26.														*
 * ���� :																	*
 ****************************************************************************/
int SetNonBlockMode(int sd)
{
	int	fl;

	/* ������ NONBLOCK���� ����											*/
	fl=fcntl(sd, F_GETFL, 0);
	if (fl != -1)
	return(fcntl(sd, F_SETFL, fl | O_NONBLOCK | O_NDELAY));

	return(-1);
}

/****************************************************************************
 * Prototype : int SendFile(int, char *, int)								*
 *																			*
 * ���� : ������ File�� ������ �̿��Ͽ� �����Ѵ�.							*
 *																			*
 * �μ� : int sd															*
 *			���� ��ũ����													*
 *		  char *fpath														*
 *			������ file�� ��ü ���											*
 *		  int pksize														*
 *			File ���ۿ� ����� ��Ŷ ������									*
 *																			*
 * ���� : (>=  0) - ���� (������ ȭ���� ũ��)								*
 *		  (== -1) - ����													*
 *																			*
 * �ۼ� : 2002.05.02.														*
 * ���� :																	*
 ****************************************************************************/
int SendFile(int sd, char *fpath, int pksize)
{
	int					fd, ret = 0;
	long				read_bytes, send_bytes, nsend;
	char				*buf;
	struct stat			statbuf;
	struct FTrans_hd	S_hd;

	if(pksize <= 0)  {
		snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] �������� pksize", __FILE__, __LINE__);
#ifdef _LIB_DEBUG
		printf("[SendFile]�������� pksize.\n");
#endif

		return(-1);
	}

	if((fd = open(fpath, O_RDONLY)) < 0)  {
		snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] open():%s", __FILE__, __LINE__, strerror(errno));
#ifdef _LIB_DEBUG
	perror("[SendFile]open()");
#endif

		return(-2);
	}

	if(fstat(fd, (struct stat *)&statbuf) < 0)  {
		snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] fstat():%s", __FILE__, __LINE__, strerror(errno));
#ifdef _LIB_DEBUG
		perror("[SendFile]fstat()");
#endif

		close(fd);
		return(-3);
	}

	if(statbuf.st_size == 0)  {
		close(fd);
		return(0);
	}

	bzero(&S_hd, sizeof(struct FTrans_hd));
	strcpy(S_hd.fname, basename(fpath));
	S_hd.flen = htonl(statbuf.st_size);
	S_hd.fmode = htonl((long)statbuf.st_mode);
	S_hd.pksize = htonl(pksize);

	if(Sendn(sd, &S_hd, sizeof(struct FTrans_hd), 0) !=
	   sizeof(struct FTrans_hd))  {
		snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] Sendn():%s", __FILE__, __LINE__, strerror(errno));
#ifdef _LIB_DEBUG
		printf("[SendFile]Sendn(1) fail.\n");
#endif

		close(fd);
		return(-4);
	}

	nsend = 0;
	if((buf = (char *)malloc(pksize)) == (char *)NULL)  {
		snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] malloc():%s", __FILE__, __LINE__, strerror(errno));
#ifdef _LIB_DEBUG
		perror("[SendFile]malloc()");
#endif

		close(fd);
		return(-5);
	}

	while(1)  {
		if(nsend >= statbuf.st_size)
			break;

		if((read_bytes = read(fd, buf, pksize)) <= 0)  {
			snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] read():%s", __FILE__, __LINE__, strerror(errno));
#ifdef _LIB_DEBUG
			perror("[SendFile]read()");
#endif
			ret = -1;
		}

		send_bytes = (pksize > read_bytes) ? read_bytes : pksize;
		if(ret == 0)  {
			if(Sendn(sd, buf, send_bytes, 0) != send_bytes)  {
				snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] Sendn():%s", __FILE__, __LINE__, strerror(errno));
#ifdef _LIB_DEBUG
				perror("[SendFile]Sendn(2) fail.\n");
#endif

				close(fd);
				free(buf);
				return(nsend);
			}
			nsend += send_bytes;
		} else  {
			if(Sendn(sd, buf, 0, 0) != 0)  {
				snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] Sendn():%s", __FILE__, __LINE__, strerror(errno));
#ifdef _LIB_DEBUG
				perror("[SendFile]Sendn(3) fail.\n");
#endif
			}

			close(fd);
			free(buf);
			return(nsend);
		}
	}

	close(fd);
	free(buf);
	return(nsend);
}

/****************************************************************************
 * Prototype : int RecvFile(int, char *)									*
 *																			*
 * ���� : ������ �������κ��� File�� ���� �޴´�.							*
 *																			*
 * �μ� : int sd															*
 *			���� ��ũ����													*
 *		  char *fpath														*
 *			���� ���� file�� ��ü ���										*
 *			NULL�̸� �۽������� �����ϴ� ȭ�ϸ��� ����Ʈ ���丮�� ����.	*
 *			'/'�� ������ ������ ���丮�� �����ϴ� ������ �Ǵ��Ͽ�			*
 *			�۽������� �����ϴ� ȭ�ϸ����� ����.							*
 *			'/'�� ������ �ʴ� ��Ʈ���̸� ������ ȭ���� ��ü ��θ�����		*
 *			�Ǵ��Ѵ�.														*
 *																			*
 * ���� : (>=  0) - ���� (���� ���� ȭ���� ũ��)							*
 *		  (== -1) - ����													*
 *																			*
 * �ۼ� : 2002.05.02.														*
 * ���� :																	*
 ****************************************************************************/
int RecvFile(int sd, char *fpath)
{
	int					fd;
	long				flen, pksize, nrecv, recv_bytes;
	char				*buf, path[MAX_PATH_LEN + 1];
	mode_t				fmode, old_umask;
	struct FTrans_hd	R_hd;

	if(Recvn(sd, &R_hd, sizeof(struct FTrans_hd), 0) != sizeof(struct FTrans_hd))  {
		snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] Recvn():%s", __FILE__, __LINE__, strerror(errno));
#ifdef _LIB_DEBUG
		printf("[RecvFile]Recvn(1) fail.\n");
#endif

		return(-1);
	}

	if(fpath == (char *)NULL)  {
		sprintf(path, "./%s", R_hd.fname);
	} else if(fpath[strlen(fpath) - 1] == '/')  {
		sprintf(path, "%s%s", fpath, R_hd.fname);
	} else  {
		sprintf(path, "%s", fpath);
	}

	flen = ntohl(R_hd.flen);
	fmode = (mode_t)ntohl(R_hd.fmode);
	pksize = ntohl(R_hd.pksize);

	old_umask = umask(000);
	if((fd = open(path, O_WRONLY | O_CREAT | O_EXCL, fmode)) < 0)  {
		snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] open():%s", __FILE__, __LINE__, strerror(errno));
#ifdef _LIB_DEBUG
	perror("[RecvFile]open()");
#endif

		umask(old_umask);
		return(-2);
	}
	umask(old_umask);

	nrecv = 0;
	if((buf = (char *)malloc(pksize)) == (char *)NULL)  {
		snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] malloc():%s", __FILE__, __LINE__, strerror(errno));
#ifdef _LIB_DEBUG
	perror("[RecvFile]malloc()");
#endif

		close(fd);
		unlink(path);
		return(-3);
	}

	while(1)  {
		if(nrecv >= flen)
			break;

		if((recv_bytes = Recvn(sd, buf, (flen - nrecv) < pksize ? (flen - nrecv) : pksize, 0)) <= 0)  {
			snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] Recvn():%s", __FILE__, __LINE__, strerror(errno));
#ifdef _LIB_DEBUG
			printf("[RecvFile]Recvn(2) fail.\n");
#endif

			close(fd);
			free(buf);
			unlink(path);
			return(-4);
		}

		if(write(fd, buf, recv_bytes) != recv_bytes)  {
			snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] write():%s", __FILE__, __LINE__, strerror(errno));
#ifdef _LIB_DEBUG
			perror("[RecvFile]malloc()");
#endif

			close(fd);
			free(buf);
			unlink(path);
			return(-5);
		}

		nrecv += recv_bytes;
	}

	close(fd);
	free(buf);
	return(nrecv);
}
