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
 * 설명 : Socket을 열고 클라이언트의 접속을 기다린다.						*
 *																			*
 * 인수 : int Port															*
 *			접속 대기 할 Port 번호											*
 *		  int BackLog														*
 *			접속 대기 서열의 수를 지정한다.									*
 *		  struct sockadd_in *Addr											*
 *			접속을 대기 할 Socket의 정보를 돌려주기 위함.					*
 *																			*
 * 리턴 : (!= -1) 정상 종료이며 소켓 디스크립터를 돌려준다.					*
 *		  (== -1) 실패														*
 *																			*
 * 작성 : 2001.03.27.														*
 * 수정 : 2010.04.15.														*
 *			PIN_WAIT 등 포트 사용으로 인한 오류 발생시 재시도 하도록 변경	*
 ****************************************************************************/
int Bind(int Port, int BackLog, struct sockaddr_in *BindAddr)
{
	int	sd;
	int	bind_retry = 10;	/* 포트 사용으로 인한 오류시 재시도 횟수		*/

	/* Socket을 생성한다.		*/
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
			/* 포트 사용으로 인한 재시도									*/
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
 * 설명 : Socket을 재사용 가능하게 열고 클라이언트의 접속을 기다린다.		*
 *																			*
 * 인수 : int Port															*
 *			접속 대기 할 Port 번호											*
 *		  int BackLog														*
 *			접속 대기 서열의 수를 지정한다.									*
 *		  struct sockadd_in *Addr											*
 *			접속을 대기 할 Socket의 정보를 돌려주기 위함.					*
 *																			*
 * 리턴 : (!= -1) 정상 종료이며 소켓 디스크립터를 돌려준다.					*
 *		  (== -1) 실패														*
 *																			*
 * 작성 : 2010.05.14.														*
 * 수정 : 																	*
 *																			*
 ****************************************************************************/
int Bind_Force(int Port, int BackLog, struct sockaddr_in *BindAddr)
{
	int	sd, optval;
	int	bind_retry = 10;	/* 포트 사용으로 인한 오류시 재시도 횟수		*/

	/* Socket을 생성한다.													*/
	if((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)  {
		snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] socket():%s", __FILE__, __LINE__, strerror(errno));
#ifdef _LIB_DEBUG
		perror("[BindF]socket");
#endif

		return(-1);
	}

	/* 소켓의 속성을 재사용 가능하도록 바꾼다.								*/
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
 * 설명 : 지정된 소켓 디스크립터로 접속을 시도하는 클라이언트가 있다면		*
 *		  접속을 맺는다.													*
 *																			*
 * 인수 : int sd															*
 *			접속을 맺을 소켓 디스크립터										*
 *		  long int sec														*
 *			접속을 기다릴 시간의 초											*
 *		  long int usec														*
 *			접속을 기다릴 마이크로 초										*
 *		  struct sockaddr_in *NewAddr										*
 *			접속 후 새롭게 생성될 Socket의 정보를 돌려주기 위함.			*
 *																			*
 * 리턴 : ( -1) 에러														*
 *		  (  0) Time Out													*
 *		  (> 0) 새롭게 맺어진 소켓 디스크립터								*
 *																			*
 * 작성 : 2001.03.27.														*
 * 수정 : 2002.05.02.														*
 *			sec와 usec이 모두 '0'이면 Time Out없이 무한 대기한다. 만일		*
 *			즉시 결과를 리턴 받고 싶을 경우 usec을 '1'로하면 지연시간이		*
 *			길지 않으므로 별 지장은 없을 것 같다.							*
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
 * 설명 : 문자열이 IP 주소로 적합한지 검사한다								*
 *		  (잘 찾아 보면 안만들어도 될것 같은데......)						*
 *																			*
 * 인수 : const char *str													*
 *		  검사할 문자열														*
 *																			*
 * 리턴 : ( -1) IP 주소로 부적합											*
 *		  (  0) IP 주소로 적합												*
 *																			*
 * 작성 : 2001.03.27.														*
 * 수정 : 2001.08.13.														*
 *			처음에는 구성 문자가 '.'과 '0'인가만 검사를 했는데 몇 가지		*
 *			검사 조건을 추가하였다.											*
 *		  2012.06.18.														*
 *			이제 필요없다.													*
 ****************************************************************************/
int isIPAddr(const char *str)
{
	int				count, flag;
	register int	i;

	/* IP 주소의 최소 길이(X.X.X) 보다 작으면 IP 주소로 부적합				*/
	if(strlen(str) < 5)
		return(-1);

	/* IP 주소에 '.'이 3개 있는가를 점사									*/
	for(i = 0, count = 0; str[i] != 0; i++)  {
		if(str[i] == '.')
			count++;
	}
	if(count != 3)
		return(-2);

	/* 숫자와 '.' 이 외의 다른 문자가 들어있는가를 검사						*/
	for(i = 0; str[i] != 0; i++)
		if(str[i] != '.' && (str[i] < '0' || str[i] > '9'))
			return(-3);

	/* '.'이 연달아 나오면 이것도 IP 주소로 부적합							*/
	for(i = 0, flag = 0; str[i] != 0; i++)  {
		if(str[i] == '.')  {
			if(flag == 1)
				return(-4);
			else
				flag = 1;
		} else
		flag = 0;
	}

	/* 숫자가 255보다 큰지 아닌지도 검사를 해야하는데 귀찮아서 생략			*/

	return(0);
}

/****************************************************************************
 * Prototype : int Connect(const char *, int, struct sockaddr_in *)			*
 *																			*
 * 설명 : Socket을 열고 서버에 접속한다.									*
 *																			*
 * 인수 : const char *Addr													*
 *			접속할 서버의 주소(IP or 도메인)								*
 *		  int Port															*
 *			접속할 서버의 Port												*
 *		  struct sockaddr_in *ConAddr										*
 *			접속된 Socket의 정보를 돌려주기 위함							*
 *																			*
 * 리턴 : (== -1) 실패														*
 *		  (!= -1) 접속된 Socket												*
 *																			*
 * 작성 : 2001.03.27.														*
 * 수정 :																	*
 ****************************************************************************/
int Connect(const char *Addr, int Port, struct sockaddr_in *ConAddr)
{
	int				sd, ret;
	struct hostent	*he;

	if((ret = isIPAddr(Addr)) == -1)  {
		/* Addr이 도메인 주소 형식이라고 판단한 경우.						*/
		if((he = (struct hostent *)gethostbyname(Addr)) == (struct hostent *)NULL)  {
			snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] gethostbyname():%s", __FILE__, __LINE__, strerror(errno));
#ifdef _LIB_DEBUG
			perror("[Connect]gethostbyname");
#endif
			return(-1);
		}
	}

	/* Socket을 생성한다.													*/
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
 * 설명 : Socket을 열고 sockaddr을 이용하여 서버에 접속한다.				*
 *																			*
 * 인수 : struct sockaddr_in *ConAddr										*
 *			접속할 Socket의 정보											*
 *																			*
 * 리턴 : (== -1) 실패														*
 *		  (!= -1) 접속된 Socket												*
 *																			*
 * 작성 : 2012.05.16.														*
 * 수정 :																	*
 ****************************************************************************/
int Connect_sa(struct sockaddr_in *ConAddr)
{
	int				sd, ret;
	struct hostent	*he;

	/* Socket을 생성한다.													*/
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
 * 설명 : Socket을 열고 서버에 접속한다. (timeout 설정 가능)				*
 *																			*
 * 인수 : const char *Addr													*
 *			접속할 서버의 주소(IP or 도메인)								*
 *		  int Port															*
 *			접속할 서버의 Port												*
 *		  struct sockaddr_in *ConAddr										*
 *			접속된 Socket의 정보를 돌려주기 위함							*
 *		  int sec															*
 *			접속 대기 시간 (초)												*
 *																			*
 * 리턴 : (== -1) 실패														*
 *		  (!= -1) 접속된 Socket												*
 *																			*
 * 작성 : 2012.05.11.														*
 * 수정 :																	*
 ****************************************************************************/
int ConnectTimeout(const char *Addr, int Port, struct sockaddr_in *ConAddr, int sec)
{
	int				sd, ret, flags;
	fd_set 			fds_r, fds_w;
	struct timeval	tv;

	struct hostent	*he;

	if((ret = isIPAddr(Addr)) == -1)  {
		/* Addr이 도메인 주소 형식이라고 판단한 경우.						*/
		if((he = (struct hostent *)gethostbyname(Addr)) == (struct hostent *)NULL)  {
			snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] gethostbyname():%s", __FILE__, __LINE__, strerror(errno));
#ifdef _LIB_DEBUG
			perror("[Connect]gethostbyname");
#endif
			return(-1);
		}
	}

	/* Socket을 생성한다.													*/
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

		/* Timeout 셋팅														*/
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
 * 설명 : Socket을 열고 sockaddr을 이용하여 서버에 접속한다. 				*
 *		  (timeout 설정 가능)												*
 *																			*
 * 인수 : struct sockaddr_in *ConAddr										*
 *			접속할 Socket의 정보											*
 *		  int sec															*
 *			접속 대기 시간 (초)												*
 *																			*
 * 리턴 : (== -1) 실패														*
 *		  (!= -1) 접속된 Socket												*
 *																			*
 * 작성 : 2012.05.16.														*
 * 수정 :																	*
 ****************************************************************************/
int ConnectTimeout_sa(struct sockaddr_in *ConAddr, int sec)
{
	int				sd, ret, flags;
	fd_set			fds_r, fds_w;
	struct timeval	tv;

	/* Socket을 생성한다.													*/
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

		/* Timeout 셋팅														*/
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
 * 설명 : 송신 전용 소켓의 라인 상태 검사.									*
 *		  (주의 : 반드시 송신 전용 소켓에서만 사용할 것.)					*
 *																			*
 * 인수 : int fd															*
 *			소켓 디스크립터													*
 *																			*
 * 리턴 : ( 0) - 정상														*
 *		  (-1) - 끊김														*
 *																			*
 * 작성 : 2000.05.26.														*
 * 수정 :																	*
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
 * 설명 : 지정된 소켓 디스크립터에 n byte만큼 송신.							*
 *																			*
 * 인수 : int sd															*
 *			소켓 디스크립터													*
 *		  const void *vptr													*
 *			쓸 내용이 있는 포인터											*
 *		  size_t n															*
 *			쓸 byte 수														*
 *		  int flags															*
 *			send()에 전달할 flags											*
 *																			*
 * 리턴 : (== n) - 성공														*
 *		  (!= n) - 실패														*
 *		  ( < 0) - 에러 발생												*
 *																			*
 * 작성 : 2001.03.14. (사탕 준비해야하는데 이런거나 만들고)					*
 * 수정 :																	*
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
 * 설명 : 지정된 소켓 디스크립터에서 n byte만큼 수신.						*
 *																			*
 * 인수 : int sd															*
 *			소켓 디스크립터													*
 *		  void *vptr														*
 *			읽은 내용을 저장할 포인터										*
 *		  size_t n															*
 *			읽을 byte 수													*
 *		  int flags															*
 *			send()에 전달할 flags											*
 *																			*
 * 리턴 : (== n) - 성공														*
 *		  (!= n) - 실패														*
 *		  ( < 0) - 에러 발생												*
 *																			*
 * 작성 : 2001.03.14.														*
 * 수정 :																	*
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
 * 설명 : 지정된 소켓 디스크립터에서 일정 시간 이내에 수신.					*
 *																			*
 * 인수 : int sd															*
 *			소켓 디스크립터													*
 *		  void *vptr														*
 *			읽은 내용을 저장할 포인터										*
 *		  size_t n															*
 *			읽을 byte 수													*
 *		  int flags															*
 *			send()에 전달할 flags											*
 *		  int sec															*
 *			대기 시간 (초)													*
 *																			*
 * 리턴 : (== n) - 성공														*
 *		  (!= n) - 실패														*
 *		  ( < 0) - 에러 발생												*
 *																			*
 * 작성 : 2010.04.08.														*
 * 수정 :																	*
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

	/* Timeout 셋팅															*/
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
 * 설명 : 지정된 소켓 디스크립터를 block mode로 설정.						*
 *																			*
 * 인수 : int sd															*
 *			소켓 디스크립터													*
 *																			*
 * 리턴 : ( >  0) - 성공													*
 *		  (== -1) - 실패													*
 *																			*
 * 작성 : 2001.03.26.														*
 * 수정 :																	*
 ****************************************************************************/
int SetBlockMode(int sd)
{
	int	fl;

	/* 소켓을 BLOCK모드로 지정												*/
	fl=fcntl(sd, F_GETFL, 0);
	if (fl != -1)
		return(fcntl(sd, F_SETFL, fl & ~O_NONBLOCK));

	return(-1);
}

/****************************************************************************
 * Prototype : int SetNonBlockMode(int)										*
 *																			*
 * 설명 : 지정된 소켓 디스크립터를 nonblock mode로 설정.					*
 *																			*
 * 인수 : int sd															*
 *			소켓 디스크립터													*
 *																			*
 * 리턴 : ( >  0) - 성공													*
 *		  (== -1) - 실패													*
 *																			*
 * 작성 : 2001.03.26.														*
 * 수정 :																	*
 ****************************************************************************/
int SetNonBlockMode(int sd)
{
	int	fl;

	/* 소켓을 NONBLOCK모드로 지정											*/
	fl=fcntl(sd, F_GETFL, 0);
	if (fl != -1)
	return(fcntl(sd, F_SETFL, fl | O_NONBLOCK | O_NDELAY));

	return(-1);
}

/****************************************************************************
 * Prototype : int SendFile(int, char *, int)								*
 *																			*
 * 설명 : 지정된 File을 소켓을 이용하여 전송한다.							*
 *																			*
 * 인수 : int sd															*
 *			소켓 디스크립터													*
 *		  char *fpath														*
 *			전송할 file의 전체 경로											*
 *		  int pksize														*
 *			File 전송에 사용할 패킷 사이즈									*
 *																			*
 * 리턴 : (>=  0) - 성공 (전송한 화일의 크기)								*
 *		  (== -1) - 실패													*
 *																			*
 * 작성 : 2002.05.02.														*
 * 수정 :																	*
 ****************************************************************************/
int SendFile(int sd, char *fpath, int pksize)
{
	int					fd, ret = 0;
	long				read_bytes, send_bytes, nsend;
	char				*buf;
	struct stat			statbuf;
	struct FTrans_hd	S_hd;

	if(pksize <= 0)  {
		snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] 부적당한 pksize", __FILE__, __LINE__);
#ifdef _LIB_DEBUG
		printf("[SendFile]부적당한 pksize.\n");
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
 * 설명 : 지정된 소켓으로부터 File을 전송 받는다.							*
 *																			*
 * 인수 : int sd															*
 *			소켓 디스크립터													*
 *		  char *fpath														*
 *			전송 받을 file의 전체 경로										*
 *			NULL이면 송신측에서 지정하는 화일명을 디폴트 디렉토리에 저장.	*
 *			'/'로 끝나면 저장할 디렉토리를 지정하는 것으로 판단하여			*
 *			송신측에서 지정하는 화일명으로 저장.							*
 *			'/'로 끝나지 않는 스트링이면 저장할 화일의 전체 경로명으로		*
 *			판단한다.														*
 *																			*
 * 리턴 : (>=  0) - 성공 (전송 받은 화일의 크기)							*
 *		  (== -1) - 실패													*
 *																			*
 * 작성 : 2002.05.02.														*
 * 수정 :																	*
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
