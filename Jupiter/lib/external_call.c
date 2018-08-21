#include <stdio.h>
#include <string.h>

#include "ComLib.h"
#include "jplib.h"

/****************************************************************************
 * Prototype : int jpec_call(struct jpec_data *, int)						*
 *																			*
 * ���� : �ܺ����α׷� ���� �� ������ ��ȯ									*
 *																			*
 * �μ� : struct jpec_data *ecdt											*
 *			������ ����ü													*
 *		  int timeout														*
 *			���� : ��														*
 *																			*
 * ���� : ( < 0) - ����														*
 *																			*
 * �ۼ� : 2017.05.26.														*
 * ���� :																	*
 ****************************************************************************/
int jpec_call(struct jpec_data *ecdt, int timeout)
{
	int		rc;
	int		handle, sfd, cfd;
	char	cmd[JPEC_NAME_LEN + 1];		// �ܺ����α׷���
	char	arg[16 + 1];				// ���� ��Ʈ��ȣ
	struct sockaddr_in sAddr, cAddr;

	if((sfd = jpec_bind(&sAddr)) < 0)  {
		return(-1);
	}

	// �����ϸ� ��Ʈ ��ȣ Ȯ��
	handle = ntohs(sAddr.sin_port);

	// �ܺ����α׷� ���� (��Ʈ ��ȣ ����)
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
		snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] [%s %s] ���� ����!!!", __FILE__, __LINE__, cmd, arg);
		close(sfd);
		return(-3);
	}

	// �ܺ����α׷� ���� ���
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

	// RunProg()���� �����Ų ���α׷��� �װ� SIGCHLD�� �� ������ ����Ѵ�. �ִ� 10�ʱ��� ... (2017.07.20.)
	sleep(10);

	return(0);
}


/****************************************************************************
 * Prototype : jpec_bind(struct sockaddr_in *)								*
 *																			*
 * ���� : �ܺ����α׷��� ������ ��ȯ�� ���� ���� ���� ����					*
 *																			*
 * �μ� : struct sockaddr_in *BindAddr										*
 *			�ּ� ����														*
 *																			*
 * ���� : ( < 0) - ����														*
 *																			*
 * �ۼ� : 2017.05.26.														*
 * ���� :																	*
 ****************************************************************************/
int jpec_bind(struct sockaddr_in *BindAddr)
{
	int	sd, port = JPEC_PORT_NUM;

	// Socket�� �����Ѵ�.
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
			// ��Ʈ ������� ���� ��õ�
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
 * ���� : �ܺ����α׷����� ���� �õ�										*
 *																			*
 * �μ� : int port															*
 *			TCP ��Ʈ ��ȣ													*
 *																			*
 * ���� : ( < 0) - ����														*
 *		  (== n) - ���� ��ũ����											*
 *																			*
 * �ۼ� : 2017.05.29.														*
 * ���� :																	*
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
 * ���� : �ܺ����α׷����� ���� �õ�										*
 *																			*
 * �μ� : int sd															*
 *			���� ��ũ����													*
 *																			*
 * ���� : 0																	*
 *																			*
 * �ۼ� : 2017.05.29.														*
 * ���� :																	*
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
 * ���� : �ܺ����α׷����� ������ ����										*
 *																			*
 * �μ� : int sd															*
 *			���� ��ũ����													*
 *		  char *buff														*
 *			���� ������ (struct jpec_data)									*
 *																			*
 * ���� : ( < 0) - ����														*
 *		  (== n) - ���ŵ����� ����											*
 *																			*
 * �ۼ� : 2017.05.29.														*
 * ���� :																	*
 ****************************************************************************/
int jpec_recv(int sd, char *buff)
{
	size_t	n = sizeof(struct jpec_data);

	return(Recvn(sd, buff, n, 0));
}


/****************************************************************************
 * Prototype : jpec_send(int, char *)										*
 *																			*
 * ���� : �ܺ����α׷����� ������ �۽�										*
 *																			*
 * �μ� : int sd															*
 *			���� ��ũ����													*
 *		  char *buff														*
 *			�۽� ������ (struct jpec_data)									*
 *																			*
 * ���� : ( < 0) - ����														*
 *		  (== n) - �۽ŵ����� ����											*
 *																			*
 * �ۼ� : 2017.05.29.														*
 * ���� :																	*
 ****************************************************************************/
int jpec_send(int sd, char *buff)
{
	size_t	n = sizeof(struct jpec_data);

	return(Sendn(sd, buff, n, 0));
}
