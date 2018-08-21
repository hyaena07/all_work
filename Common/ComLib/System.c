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
 * 설명 : 프로세스를 실행시킨 터미널 이름을 가져온다.						*
 *																			*
 * 인수 :																	*
 *																			*
 * 리턴 : (!= NULL) - 터미널 이름의 스트링 포인터							*
 *		  (== NULL) - 실패													*
 *																			*
 * 작성 : 2001.08.16.														*
 * 수정 :																	*
 ****************************************************************************/
char *GetTerminalName(void)
{
	char	*ptr, *ret;

	/* Terminal name을 가져온다.											*/
	ptr = ttyname(STDIN_FILENO);

	if(ptr == (char *)NULL)  {
		snprintf(errmsg, MAX_ERRO_LEN, "[%s:%d] ttyname():%s", __FILE__, __LINE__, strerror(errno));
#ifdef _LIB_DEBUG
		perror("[GetTerminalName]ttyname()");
#endif
		return((char *)NULL);
	}

	/* Terminal name중 경로를 나타내는 부분을 삭제한다. (예: /dev/....)		*/
	ret = (char *)strstr(ptr + 1, "/");
	ret++;

	return(ret);
}

/****************************************************************************
 * Prototype : char *GetIP(void)											*
 *																			*
 * 설명 : 프로세스를 실행시킨 터미널의 IP 주소를 가져온다.					*
 *		  주의) host 정보가 host name이고, host에 할당된 IP가 여러개일경우	*
 *		  정확한 IP 주소를 가져올 수 없다.									*
 *																			*
 * 인수 :																	*
 *																			*
 * 리턴 : (!= NULL) - IP 주소의 스트링 포인터								*
 *		  (== NULL) - 실패													*
 *																			*
 * 작성 : 2001.08.16.														*
 * 수정 : 2012.05.03.														*
 *			솔라리스 포팅 중 utmp 구조체에 ut_home 멤버가 없어서 막았다.	*
 ****************************************************************************/
char *GetIP(void)
{
	char			*term_name;
	static char		tmp[255], ip_addr[20];
	struct utmp		*buf;
	struct hostent	*he;

#if 0
	/* 프로세스를 실행시킨 터미널 이름을 가지고 IP 주소를 찾아야하므로
	  먼저 터미널 이름을 가져온다.											*/
	if((term_name = GetTerminalName()) == 0)
		return((char *)NULL);

	/* 사용자 정보 기록 화일인 utmp 중 터미널 이름이 일치하는 사용자
	  기록을 찾는다. 														*/
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

	/* host 정보가 IP 주소 형식인지를 검사한다.								*/
	if(!isIPAddr(tmp))
		return(tmp);

	/* host 정보가 없는 경우는 localhost로 가정하고 이후 처리를 한다.		*/
	if(tmp[0] == 0)
		strcpy(tmp, "localhost");

	/* host name으로 IP 주소를 찾는다.										*/
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
