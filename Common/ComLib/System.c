#include <utmp.h>
#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "ComLib.h"

/****************************************************************************
 * Prototype : char *GetTerminalName(void)									*
 *																			*
 * ���� : ���μ����� �����Ų �͹̳� �̸��� �����´�.						*
 *																			*
 * �μ� :																	*
 *																			*
 * ���� : (!= NULL) - �͹̳� �̸��� ��Ʈ�� ������							*
 *		  (== NULL) - ����													*
 *																			*
 * �ۼ� : 2001.08.16.														*
 * ���� :																	*
 ****************************************************************************/
char *GetTerminalName(void)
{
	char	*ptr, *ret;

	/* Terminal name�� �����´�.											*/
	ptr = ttyname(STDIN_FILENO);

	if(ptr == (char *)NULL)  {
		snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] ttyname():%s", __FILE__, __LINE__, strerror(errno));
#ifdef _LIB_DEBUG
		perror("[GetTerminalName]ttyname()");
#endif
		return((char *)NULL);
	}

	/* Terminal name�� ��θ� ��Ÿ���� �κ��� �����Ѵ�. (��: /dev/....)		*/
	ret = (char *)strstr(ptr + 1, "/");
	ret++;

	return(ret);
}

/****************************************************************************
 * Prototype : char *GetIP(void)											*
 *																			*
 * ���� : ���μ����� �����Ų �͹̳��� IP �ּҸ� �����´�.					*
 *		  ����) host ������ host name�̰�, host�� �Ҵ�� IP�� �������ϰ��	*
 *		  ��Ȯ�� IP �ּҸ� ������ �� ����.									*
 *																			*
 * �μ� :																	*
 *																			*
 * ���� : (!= NULL) - IP �ּ��� ��Ʈ�� ������								*
 *		  (== NULL) - ����													*
 *																			*
 * �ۼ� : 2001.08.16.														*
 * ���� : 2012.05.03.														*
 *			�ֶ󸮽� ���� �� utmp ����ü�� ut_home ����� ��� ���Ҵ�.	*
 ****************************************************************************/
char *GetIP(void)
{
	char			*term_name;
	static char		tmp[255], ip_addr[20];
	struct utmp		*buf;
	struct hostent	*he;

#if 0
	/* ���μ����� �����Ų �͹̳� �̸��� ������ IP �ּҸ� ã�ƾ��ϹǷ�
	  ���� �͹̳� �̸��� �����´�.											*/
	if((term_name = GetTerminalName()) == 0)
		return((char *)NULL);

	/* ����� ���� ��� ȭ���� utmp �� �͹̳� �̸��� ��ġ�ϴ� �����
	  ����� ã�´�. 														*/
	while(1)  {
		if((buf = getutent()) == (struct utmp *)NULL)  {
			snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] getutent():%s", __FILE__, __LINE__, strerror(errno));
#ifdef _LIB_DEBUG
			perror("[GetIP]getutent()");
#endif
			return((char *)NULL);
		}

		if(!strcmp(buf->ut_line, term_name))  {
			bzero(tmp, sizeof(tmp));
			strcpy(tmp, buf->ut_host);
			break;
		}
	}

	/* host ������ IP �ּ� ���������� �˻��Ѵ�.								*/
	if(!isIPAddr(tmp))
		return(tmp);

	/* host ������ ���� ���� localhost�� �����ϰ� ���� ó���� �Ѵ�.		*/
	if(tmp[0] == 0)
		strcpy(tmp, "localhost");

	/* host name���� IP �ּҸ� ã�´�.										*/
	if ((he = (struct hostent *)gethostbyname(tmp)) == (struct hostent *)NULL)
		return((char *)NULL);

	sprintf(ip_addr, "%u.%u.%u.%u"
		, (unsigned char)he->h_addr_list[0][0]
		, (unsigned char)he->h_addr_list[0][1]
		, (unsigned char)he->h_addr_list[0][2]
		, (unsigned char)he->h_addr_list[0][3]
	);
#endif

	return(ip_addr);
}
