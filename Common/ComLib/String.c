#include <stdio.h>
#include <string.h>

#include "ComLib.h"

/****************************************************************************
 * Prototype : char *MemStr(char *, char *, int)							*
 *																			*
 * 설명 : 메모리 영역 내에서 일치하는 문자열을 검색한다.					*
 *		  strstr()과 유사한 기능이지만 NULL 문자를 포함한 메모리			*
 *		  에서도 사용 가능하다.												*
 *																			*
 * 인수 : char *ptr															*
 *			검색을 시작할 포인터 (NULL 문자 포함 가능)						*
 *		  char *str															*
 *			검색할 문자열의 포인터 (NULL 문자 포함 불가)					*
 *		  int n																*
 *			검색될 영역의 길이												*
 *																			*
 * 리턴 : 성공 - 일치하는 포인터											*
 *		  실패 - NULL														*
 *																			*
 * 작성 : 1999.12.17.														*
 * 수정 : 2000.12.13.														*
 *			앞부분은 같고 뒤는 다른 문자열이 있을 경우 일치하는 부분까지	*
 *			검사를 하고 다시 검사를 시작할때 같았던 부분 이후부터 검사를	*
 *			하는 오류를 수정												*
 *			무슨 말인지 이해가 안가면 "back"의 용도를 잘 보면...			*
 *		  2001.03.06.														*
 *			함수 이름을 M_strstr()에서 MemStr()로 변경						*
 *****************************************************************************/
char *MemStr(char *ptr, char *str, int n)
{
	int				back = 0;
	char			*top;
	register int	i, j;

	for(i = 0, j = 0; i < n; i++)  {
		if(ptr[i] == str[j])  {
			if(back == 0)
				back = i;
			if(j == 0)
				top = ptr + i;
			j++;
			if(str[j] == '\0')
				return(top);
			continue;
		}

		if(back != 0)  {
			i = back;
			back = 0;
		}
		j = 0;
	}

	return(NULL);
}

/****************************************************************************
 * Prototype : char *LTrim(char *)											*
 *																			*
 * 설명 : 왼쪽 Space, Tab 문자 제거											*
 *																			*
 * 인수 : char *ptr															*
 *			공백을 제거할 문자열 포인터										*
 *																			*
 * 리턴 : 전달 받은 포인터													*
 *																			*
 * 작성 : 2001.03.30.														*
 * 수정 : 2001.05.24.														*
 *			Space만을 처리하였는데 Tab 문자도 함께 처리하도록 수정			*
 ****************************************************************************/
char *LTrim(char *str)
{
	int				stat = 0;
	register int	i = 0;
	register int	j = 0;

	while(str[i] != 0)  {
		if(stat == 0 && (str[i] == ' ' || str[i] == '\t'))  {
			i++;
			continue;
		}

		str[j] = str[i];
		stat = 1;

		i++;
		j++;
	}

	return(str);
}

/****************************************************************************
 * Prototype : char *RTrim(char *)											*
 *																			*
 * 설명 : 오른쪽 Space, Tab 문자 제거										*
 *																			*
 * 인수 : char *ptr															*
 *			공백을 제거할 문자열 포인터										*
 *																			*
 * 리턴 : 전달 받은 포인터													*
 *																			*
 * 작성 : 2001.03.30.														*
 * 수정 : 2001.05.24.														*
 *			Space만을 처리하였는데 Tab 문자도 함께 처리하도록 수정			*
 ****************************************************************************/
char *RTrim(char *str)
{
	int				end = 0;
	register int	i = 0;

	while(str[i] != 0)  {
		if(str[i] != ' ' && str[i] != '\t')
			end = i;
		i++;
	}
	str[end + 1] = 0;

	return(str);
}

/****************************************************************************
 * Prototype : char *RemoveAllSpace(char *)									*
 *																			*
 * 설명 : 문자열 내의 모든 Space, Tab 문자 제거								*
 *																			*
 * 인수 : char *ptr															*
 *			공백을 제거할 문자열 포인터										*
 *																			*
 * 리턴 : 전달 받은 포인터													*
 *																			*
 * 작성 : 2001.05.24.														*
 * 수정 :																	*
 ****************************************************************************/
char *RemoveAllSpace(char *str)
{
	register int	i, j;

	for(i = 0, j = 0; str[i] != 0; i++)  {
		if(str[i] == ' ' || str[i] == '\t')
			continue;
		str[j++] = str[i];
	}

	str[j] = 0;
}

/****************************************************************************
 * Prototype : char *RemoveDoubleSpace(char *)								*
 *																			*
 * 설명 : 문자열 내의 모든 중복된 Space, Tab 문자를 단일 Space로 바꾼다		*
 *																			*
 * 인수 : char *ptr															*
 *			공백을 제거할 문자열 포인터										*
 *																			*
 * 리턴 : 전달 받은 포인터													*
 *																			*
 * 작성 : 2001.05.24.														*
 * 수정 :																	*
 ****************************************************************************/
char *RemoveDoubleSpace(char *str)
{
	int				pSpace = 0;
	register int	i, j;

	for(i = 0, j = 0; str[i] != 0; i++)  {
		if(str[i] == ' ' || str[i] == '\t')  {
			/* pSpace == 0 이면 직전 문자가 공백 문자가 아닐 경우
			   pSpace == 1 이면 직전 문자가 공백 문자일 경우				*/
			if(pSpace == 1)
				continue;
			str[j++] = ' ';
			pSpace = 1;
			continue;
		}
		str[j++] = str[i];
		pSpace = 0;
	}

	str[j] = 0;
}

/****************************************************************************
 * Prototype : int isDecimal(char *)										*
 *																			*
 * 설명 : 문자열의 구성이 10진수에 적합한가를 검사한다.						*
 *																			*
 * 인수 : char *str															*
 *			검사할 문자열 포인터											*
 *																			*
 * 리턴 : ( 0) - 부적합														*
 *		  ( 1) - 적합														*
 *																			*
 * 작성 : 2001.08.31.														*
 * 수정 :																	*
 ****************************************************************************/
int isDecimal(char *str)
{
	int	i;

	for(i = 0; str[i] != 0; i++)  {
		if((str[i] >= '0') && (str[i] <= '9'))
			continue;
		else
			return(0);
	}

	return(1);
}

/****************************************************************************
 * Prototype : char *Long2Str(long, int)									*
 *																			*
 * 설명 : 정수형 변수의 값을 스트링으로 변환한다.							*
 *		  (주의 : 최대 30자리까지 사용 가능)								*
 *																			*
 * 인수 : long Val															*
 *			변환할 정수 값													*
 *		  int Com															*
 *			자리수 구분을 위한 ','의 위치를 지정							*
 *			0 : 사용 안함													*
 *			n : n 자리마다 표시												*
 *																			*
 * 리턴 : 변환된 문자열 포인터												*
 *																			*
 * 작성 : 2001.10.18.														*
 * 수정 :																	*
 ****************************************************************************/
char *Long2Str(long Val, int Com)
{
	int			sign = 0;
	int			i, j = 0;
	static char	str[31];

	if(Val < 0)  {
		sign = 1;
		Val = -Val;
	}

	bzero(str, sizeof(str));
	for(i = 30; i >= 0; i--)  {
		str[i] = (Val % 10) + '0';
		if(Val < 10)
			break;
		Val /= 10;
		j++;
		if(j == Com)  {
			str[--i] = ',';
			j = 0;
		}
	}

	if(sign == 1)
		str[--i] = '-';

	return(str + i);
}

/****************************************************************************
 * Prototype : void Upp2Low(char *)											*
 *																			*
 * 설명 : 영문 대문자를 소문자로 변환										*
 *																			*
 * 인수 : char	*buff														*
 *			변환할 문자열. (반드시 NULL 포함)								*
 *																			*
 * 리턴 : 없음																*
 *																			*
 * 작성 : 2010.02.03.														*
 * 수정 :																	*
 ****************************************************************************/
void Upp2Low(char *buff)
{
	int	i;

	for(i = 0; buff[i] != 0; i ++)  {
		if(buff[i] >= 'A' && buff[i] <= 'Z')
			buff[i] += 'a' - 'A';
	}

	return;
}

/****************************************************************************
 * Prototype : void Low2Upp(char *)											*
 *																			*
 * 설명 : 영문 소문자를 대문자로 변환										*
 *																			*
 * 인수 : char	*buff														*
 *			변환할 문자열. (반드시 NULL 포함)								*
 *																			*
 * 리턴 : 없음																*
 *																			*
 * 작성 : 2010.02.03.														*
 * 수정 :																	*
 ****************************************************************************/
void Low2Upp(char *buff)
{
	int	i;

	for(i = 0; buff[i] != 0; i ++)  {
		if(buff[i] >= 'a' && buff[i] <= 'z')
			buff[i] -= 'a' - 'A';
	}

	return;
}

/****************************************************************************
 * Prototype : void s2l(char *, int)										*
 *																			*
 * 설명 : 문자열을 long형 숫자로 변환										*
 *																			*
 * 인수 : char	*str														*
 *			변환할 문자열													*
 *		  int nd															*
 *			변환할 문자열 길이												*
 *																			*
 * 리턴 : 없음																*
 *																			*
 * 작성 : 2015.04.01.														*
 * 수정 :																	*
 ****************************************************************************/
long s2l(char *str, int nd)
{
	char	sTemp[256];

	bzero(sTemp, sizeof(sTemp));

	strncpy(sTemp, str, nd);

	return(atol(sTemp));
}

/****************************************************************************
 * Prototype : void s2f(char *, int)										*
 *																			*
 * 설명 : 문자열을 float형 숫자로 변환										*
 *																			*
 * 인수 : char	*str														*
 *			변환할 문자열													*
 *		  int nd															*
 *			변환할 문자열 길이												*
 *																			*
 * 리턴 : 없음																*
 *																			*
 * 작성 : 2015.04.01.														*
 * 수정 :																	*
 ****************************************************************************/
float s2f(char *str, int nd)
{
	char	sTemp[256];

	bzero(sTemp, sizeof(sTemp));

	strncpy(sTemp, str, nd);

	return(atof(sTemp));
}
