#include <time.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>

#include "ComLib.h"

/****************************************************************************
 * Prototype : float GetElapsedTime(struct timeval *)						*
 *																			*
 * 설명 : 프로그램 실행 과정의 경과 시간을 측정하는 목적으로 사용.			*
 *		  측정 시작을 원하는 지점에서는 struct timeval *의 값을 모두		*
 *		  '0'으로 초기화해서 주면 struct timeval *의 값을 채운다. 그리고	*
 *		  측정을 끝낼 지점에서 처음 호출때 값이 채워진 struct timeval *을	*
 *		  주면 시간 차를 돌려준다.											*
 *																			*
 * 인수 : struct timeval *tm												*
 *			측정 시작 시간을 저장하기 위한 용도								*
 *																			*
 * 리턴 : 처음 호출 - (float)0												*
 *		  최종 호출 - float형 시간 값(단위는 초)							*
 *																			*
 * 작성 : 2000.12.11.														*
 * 수정 : 2001.03.13.														*
 *			결과를 스트링으로 돌려주던 것을 float형으로 돌려주도록 변경		*
 ****************************************************************************/
float GetElapsedTime(struct timeval *tm)
{
	long			qsec, qusec;
	struct timeval	end;

	if(tm->tv_sec == 0 && tm->tv_usec == 0)  {
		/* 처음 호출될때 거쳐야 할 루틴										*/
		gettimeofday(tm, NULL);

		return((float)0);
	} else  {
		/* 최종(두번째) 호출될때 거쳐야 할 루틴								*/
		gettimeofday(&end, NULL);
		if(end.tv_usec >= tm->tv_usec)  {
			qsec = end.tv_sec - tm->tv_sec;
			qusec = end.tv_usec - tm->tv_usec;
		} else  {
			qsec = (end.tv_sec - tm->tv_sec) - 1;
			qusec = 1000000 - (tm->tv_usec - end.tv_usec);
		}

		return(qsec + ((float)qusec / 1000000));
	}
}

/****************************************************************************
 * Prototype : void GetElapsedTime_Begin(struct timeval *)					*
 *																			*
 * 설명 : 위의 GetElapsedTime()이 시간 측정 시작과 끝에 모두 사용되므로		*
 *		  이 함수를 사용하는 소스를 분석할 때 그 용도를 파악하기 불편하여	*
 *		  시작과 끝을 담당하는 함수를 각각 만들었다.						*
 *		  이 함수는 시작 시간을 기록하는 함수이다.							*
 *																			*
 * 인수 : struct timeval *tm												*
 *			측정 시작 시간을 저장하기 위한 용도								*
 *																			*
 * 리턴 :																	*
 *																			*
 * 작성 : 2001.07.14. (토요일인데 출근해서 이거 만들었다.)					*
 * 수정 :																	*
 ****************************************************************************/
void GetElapsedTime_Begin(struct timeval *tm)
{
	gettimeofday(tm, (struct timezone *)NULL);
}

/****************************************************************************
 * Prototype : void GetElapsedTime_End(struct timeval *)					*
 *																			*
 * 설명 : 위의 GetElapsedTime()이 시간 측정 시작과 끝에 모두 사용되므로		*
 *		  이 함수를 사용하는 소스를 분석할 때 그 용도를 파악하기 불편하여	*
 *		  시작과 끝을 담당하는 함수를 각각 만들었다.						*
 *		  이 함수는 시작 시간으로부터 경과된 시간을 계산하는 함수이다.		*
 *																			*
 * 인수 : struct timeval *tm												*
 *			시작 시간														*
 *																			*
 * 리턴 : float형 시간 값(단위는 초)										*
 *																			*
 * 작성 : 2001.07.14.														*
 * 수정 :																	*
 *****************************************************************************/
float GetElapsedTime_End(struct timeval *tm)
{
	long			qsec, qusec;
	struct timeval	end;

	gettimeofday(&end, (struct timezone *)NULL);
	if(end.tv_usec >= tm->tv_usec)  {
		qsec = end.tv_sec - tm->tv_sec;
		qusec = end.tv_usec - tm->tv_usec;
	} else  {
		qsec = (end.tv_sec - tm->tv_sec) - 1;
		qusec = 1000000 - (tm->tv_usec - end.tv_usec);
	}

	return(qsec + ((float)qusec / 1000000));
}

/****************************************************************************
 * Prototype : char *getDateTime_14(void)									*
 *																			*
 * 설명 : 현재 날짜와 시간을 "YYYYMMDDHHMMSS" 형태의 포인터로 리턴			*
 *																			*
 * 인수 :																	*
 *																			*
 * 리턴 : char *															*
 *																			*
 * 작성 : 2010.01.28.														*
 * 수정 :																	*
 ****************************************************************************/
char *getDateTime_14(void)
{
	time_t		lTime;
	static char	szTime[14 + 1];
	struct tm	*tmTime;

	time(&lTime);
	tmTime = localtime(&lTime);

	sprintf(szTime
		, "%04d%02d%02d%02d%02d%02d"
		, tmTime->tm_year + 1900				/* 1900년부터 시작됨		*/
		, tmTime->tm_mon  + 1					/* 0~11 까지				*/
		, tmTime->tm_mday
		, tmTime->tm_hour
		, tmTime->tm_min
		, tmTime->tm_sec
	);

	return(szTime);
}

/****************************************************************************
 * Prototype : char *getDateTime_20(void)									*
 *																			*
 * 설명 : 현재 날짜와 시간을 "YYYYMMDDHHMMSSssssss" 형태의 포인터로 리턴	*
 *																			*
 * 인수 :																	*
 *																			*
 * 리턴 : char *															*
 *																			*
 * 작성 : 2017.09.28.														*
 * 수정 :																	*
 ****************************************************************************/
char *getDateTime_20(void)
{
	static char	szTime[20 + 1];
    struct timeval  ltTv;
    struct tm*      ltpTm;

    gettimeofday(&ltTv, 0x00);
    ltpTm = localtime(&ltTv.tv_sec);

	sprintf(szTime, "%04d%02d%02d%02d%02d%02d%06ld"
		, ltpTm->tm_year + 1900, ltpTm->tm_mon + 1, ltpTm->tm_mday
		, ltpTm->tm_hour, ltpTm->tm_min, ltpTm->tm_sec, ltTv.tv_usec
	);

	return(szTime);
}

/****************************************************************************
 * Prototype : char *getDate_s(void)										*
 *																			*
 * 설명 : 현재 날짜를 리턴 (YYYY/MM/DD)										*
 *																			*
 * 인수 :																	*
 *																			*
 * 리턴 : char *															*
 *																			*
 * 작성 : 2010.01.28.														*
 * 수정 :																	*
 ****************************************************************************/
char *getDate_s(void)
{
	time_t		lTime;
	static char	szTime[15];
	struct tm	*tmTime;

	time(&lTime);
	tmTime = localtime(&lTime);

	sprintf(szTime
		, "%04d/%02d/%02d"
		, tmTime->tm_year + 1900		/* 1900년부터 시작됨				*/
		, tmTime->tm_mon  + 1			/* 0~11 까지						*/
		, tmTime->tm_mday
	);

	return(szTime);
}

/****************************************************************************
 * Prototype : char *getWeekName(void)										*
 *																			*
 * 설명 : 현재 날짜의 영문 요일명을 리턴									*
 *																			*
 * 인수 :																	*
 *																			*
 * 리턴 : char *															*
 *																			*
 * 작성 : 2015.04.01.														*
 * 수정 :																	*
 ****************************************************************************/
char *getWeekName(void)
{
	time_t		lTime;
	static char	szWeek[15];
	struct tm	*tmTime;

	time(&lTime);
	tmTime = localtime(&lTime);

	bzero(szWeek, sizeof(szWeek));

	switch(tmTime->tm_wday)  {
		case 0	:
			strcpy(szWeek, "Sunday");
			break;
		case 1	:
			strcpy(szWeek, "Monday");
			break;
		case 2	:
			strcpy(szWeek, "Tuesday");
			break;
		case 3	:
			strcpy(szWeek, "Wednesday");
			break;
		case 4	:
			strcpy(szWeek, "Thursday");
			break;
		case 5	:
			strcpy(szWeek, "Friday");
			break;
		case 6	:
			strcpy(szWeek, "Saturday");
			break;
	}

	return(szWeek);
}

/****************************************************************************
 * Prototype : int getWeekNo(void)											*
 *																			*
 * 설명 : 현재 날짜의 요일에 해당하는 번호 리턴 							*
 *																			*
 * 인수 :																	*
 *																			*
 * 리턴 : int (0:일요일 ~ 6:토요일)											*
 *																			*
 * 작성 : 2015.04.01.														*
 * 수정 :																	*
 ****************************************************************************/
int getWeekNo(void)
{
	time_t		lTime;
	struct tm	*tmTime;

	time(&lTime);
	tmTime = localtime(&lTime);

	return(tmTime->tm_wday);
}

/****************************************************************************
 * Prototype : char *getTime(void)											*
 *																			*
 * 설명 : 현재 시간을 mSec 단위로 리턴 (HHMMSSsss)							*
 *																			*
 * 인수 :																	*
 *																			*
 * 리턴 : char *															*
 *																			*
 * 작성 : 2015.04.01.														*
 * 수정 :																	*
 ****************************************************************************/
char *getTime(void)
{
	long		qsec, qusec;
	static char	szTime[15];
	struct		timeval now;
	struct tm	*tmTime;

	gettimeofday(&now, NULL);
	tmTime = localtime(&now.tv_sec);

	sprintf(szTime
		, "%02d%02d%02d%03d"
		, tmTime->tm_hour
		, tmTime->tm_min
		, tmTime->tm_sec
		, now.tv_usec / 1000
	);

	return(szTime);
}

/****************************************************************************
 * Prototype : char *getTime_s(void)										*
 *																			*
 * 설명 : 현재 시간을 mSec 단위로 리턴 (HH:MM:SS.sss)						*
 *																			*
 * 인수 :																	*
 *																			*
 * 리턴 : char *															*
 *																			*
 * 작성 : 2015.04.01.														*
 * 수정 :																	*
 ****************************************************************************/
char *getTime_s(void)
{
	long		qsec, qusec;
	static char	szTime[15];
	struct		timeval now;
	struct tm	*tmTime;

	gettimeofday(&now, NULL);
	tmTime = localtime(&now.tv_sec);

	sprintf(szTime
		, "%02d:%02d:%02d.%03d"
		, tmTime->tm_hour
		, tmTime->tm_min
		, tmTime->tm_sec
		, now.tv_usec / 1000
	);

	return(szTime);
}

/****************************************************************************
 * Prototype : char *CalcDate(char *, int )									*
 *																			*
 * 설명 : 날짜 계산															*
 *																			*
 * 인수 : char *date														*
 *			기준일자 (YYYYMMDD, NULL 이면 오늘 기준)						*
 *		  int days															*
 *			계산 값 (0:당일, -2:이틀 전, 4:4일 후)							*
 *																			*
 * 리턴 : char *															*
 *			계산된 날짜 (YYYYMMDD)											*
 *																			*
 * 작성 : 2015.04.01.														*
 * 수정 :																	*
 ****************************************************************************/
char *CalcDate(char *date, int days)
{
	int			secpday;
	static char	szDate[15];
	time_t		lTime;
	struct tm 	tmTime;

	secpday = 60 * 60 * 24;				/* 1일당 초							*/

	if(date == (char *)NULL)  {
		time(&lTime);
	} else  {
		bzero(&tmTime, sizeof(struct tm));

		tmTime.tm_year	= s2l(date		, 4) - 1900;
		tmTime.tm_mon	= s2l(date + 4	, 2) - 1;
		tmTime.tm_mday	= s2l(date + 6	, 2);
		tmTime.tm_hour	= 10;

		lTime = mktime(&tmTime);
	}

	lTime = lTime - (lTime % secpday);
	lTime = lTime + (secpday * days);

	memcpy(&tmTime, localtime(&lTime), sizeof(struct tm));

	sprintf(szDate
		, "%04d%02d%02d"
		, tmTime.tm_year + 1900		/* 1900년부터 시작됨					*/
		, tmTime.tm_mon  + 1		/* 0~11 까지							*/
		, tmTime.tm_mday
	);

	return(szDate);
}

