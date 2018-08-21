#include <stdio.h>
#include <string.h>

#include "ComLib.h"
#include "jplib.h"

/****************************************************************************
 * Prototype : int jpec_call(struct jpec_data *, int)						*
 *																			*
 * 설명 : 외부프로그램 실행 후 데이터 교환									*
 *																			*
 * 인수 : struct jpec_data *ecdt											*
 *			데이터 구조체													*
 *		  int timeout														*
 *			단위 : 초														*
 *																			*
 * 리턴 : ( < 0) - 실패														*
 *																			*
 * 작성 : 2017.05.26.														*
 * 수정 :																	*
 ****************************************************************************/
int jpec_call(struct jpec_data *ecdt, int timeout)
{
	int		rc;
	int		handle, sfd, cfd;
	char	cmd[JPEC_NAME_LEN + 1];		// 외부프로그램명
	char	arg[16 + 1];				// 서버 포트번호
	struct sockaddr_in sAddr, cAddr;

	if((sfd = jpec_bind(&sAddr)) < 0)  {
		return(-1);
	}

	// 성공하면 포트 번호 확보
	handle = ntohs(sAddr.sin_port);

	// 외부프로그램 실행 (포트 번호 전달)
	bzero(cmd, sizeof(cmd));
	memcpy(cmd, ecdt->jpec_name, JPEC_NAME_LEN);
	RTrim(cmd);

	sprintf(arg, "%d", handle);

	if((rc = RunProg(cmd, arg)) < 0)  {
		snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] RunProg() fail.", __FILE__, __LINE__);
		close(sfd);
		return(-2);
	}

	if(rc == 0)  {
		snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] [%s %s] 실행 실패!!!", __FILE__, __LINE__, cmd, arg);
		close(sfd);
		return(-3);
	}

	// 외부프로그램 접속 대기
	cfd = Accept(sfd, 1, 0, &cAddr);
	if(cfd <= 0)  {
		close(sfd);
		return(-4);
	}

	rc = Sendn(cfd, ecdt, sizeof(struct jpec_data), 0);
	if(rc != sizeof(struct jpec_data))  {
		close(cfd);
		close(sfd);
		return(-5);
	}

	rc = Recvw(cfd, ecdt, sizeof(struct jpec_data), 0, timeout);
	if(rc != sizeof(struct jpec_data))  {
		close(cfd);
		close(sfd);
		return(-6);
	}

	close(cfd);
	close(sfd);

	// RunProg()으로 실행시킨 프로그램이 죽고 SIGCHLD가 올 때까지 대기한다. 최대 10초까지 ... (2017.07.20.)
	sleep(10);

	return(0);
}


/****************************************************************************
 * Prototype : jpec_bind(struct sockaddr_in *)								*
 *																			*
 * 설명 : 외부프로그램과 데이터 교환을 위한 서버 소켓 열기					*
 *																			*
 * 인수 : struct sockaddr_in *BindAddr										*
 *			주소 정보														*
 *																			*
 * 리턴 : ( < 0) - 실패														*
 *																			*
 * 작성 : 2017.05.26.														*
 * 수정 :																	*
 ****************************************************************************/
int jpec_bind(struct sockaddr_in *BindAddr)
{
	int	sd, port = JPEC_PORT_NUM;

	// Socket을 생성한다.
	if((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)  {
		snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] socket():%s", __FILE__, __LINE__, strerror(errno));
#ifdef _LIB_DEBUG
		perror("[Bind]socket");
#endif

		return(-1);
	}

	while(1)  {
		BindAddr->sin_family = AF_INET;
		BindAddr->sin_port = htons(port);
		BindAddr->sin_addr.s_addr = INADDR_ANY;
		bzero(&(BindAddr->sin_zero), 8);

		if(bind(sd, (struct sockaddr *)BindAddr, (socklen_t)sizeof(struct sockaddr)) < 0)  {
			// 포트 사용으로 인한 재시도
			if(errno == EADDRINUSE)  {
				port ++;
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


	if(listen(sd, 1) == -1)  {
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
 * Prototype : jpec_open(int)												*
 *																			*
 * 설명 : 외부프로그램에서 접속 시도										*
 *																			*
 * 인수 : int port															*
 *			TCP 포트 번호													*
 *																			*
 * 리턴 : ( < 0) - 실패														*
 *		  (== n) - 소켓 디스크립터											*
 *																			*
 * 작성 : 2017.05.29.														*
 * 수정 :																	*
 ****************************************************************************/
int jpec_open(int port)
{
	int sd;
	struct sockaddr_in cAddr;

	return(ConnectTimeout("127.0.0.1", port, &cAddr, 1));
}


/****************************************************************************
 * Prototype : jpec_close(int)												*
 *																			*
 * 설명 : 외부프로그램에서 접속 시도										*
 *																			*
 * 인수 : int sd															*
 *			소켓 디스크립터													*
 *																			*
 * 리턴 : 0																	*
 *																			*
 * 작성 : 2017.05.29.														*
 * 수정 :																	*
 ****************************************************************************/
int jpec_close(int sd)
{
	if(sd > 0)
		close(sd);

	return(0);
}


/****************************************************************************
 * Prototype : jpec_recv(int, char *)										*
 *																			*
 * 설명 : 외부프로그램에서 데이터 수신										*
 *																			*
 * 인수 : int sd															*
 *			소켓 디스크립터													*
 *		  char *buff														*
 *			수신 데이터 (struct jpec_data)									*
 *																			*
 * 리턴 : ( < 0) - 실패														*
 *		  (== n) - 수신데이터 길이											*
 *																			*
 * 작성 : 2017.05.29.														*
 * 수정 :																	*
 ****************************************************************************/
int jpec_recv(int sd, char *buff)
{
	size_t	n = sizeof(struct jpec_data);

	return(Recvn(sd, buff, n, 0));
}


/****************************************************************************
 * Prototype : jpec_send(int, char *)										*
 *																			*
 * 설명 : 외부프로그램에서 데이터 송신										*
 *																			*
 * 인수 : int sd															*
 *			소켓 디스크립터													*
 *		  char *buff														*
 *			송신 데이터 (struct jpec_data)									*
 *																			*
 * 리턴 : ( < 0) - 실패														*
 *		  (== n) - 송신데이터 길이											*
 *																			*
 * 작성 : 2017.05.29.														*
 * 수정 :																	*
 ****************************************************************************/
int jpec_send(int sd, char *buff)
{
	size_t	n = sizeof(struct jpec_data);

	return(Sendn(sd, buff, n, 0));
}
