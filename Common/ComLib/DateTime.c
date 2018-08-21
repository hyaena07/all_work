#include <time.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>

#include "ComLib.h"

/****************************************************************************
 * Prototype : float GetElapsedTime(struct timeval *)						*
 *																			*
 * ���� : ���α׷� ���� ������ ��� �ð��� �����ϴ� �������� ���.			*
 *		  ���� ������ ���ϴ� ���������� struct timeval *�� ���� ���		*
 *		  '0'���� �ʱ�ȭ�ؼ� �ָ� struct timeval *�� ���� ä���. �׸���	*
 *		  ������ ���� �������� ó�� ȣ�⶧ ���� ä���� struct timeval *��	*
 *		  �ָ� �ð� ���� �����ش�.											*
 *																			*
 * �μ� : struct timeval *tm												*
 *			���� ���� �ð��� �����ϱ� ���� �뵵								*
 *																			*
 * ���� : ó�� ȣ�� - (float)0												*
 *		  ���� ȣ�� - float�� �ð� ��(������ ��)							*
 *																			*
 * �ۼ� : 2000.12.11.														*
 * ���� : 2001.03.13.														*
 *			����� ��Ʈ������ �����ִ� ���� float������ �����ֵ��� ����		*
 ****************************************************************************/
float GetElapsedTime(struct timeval *tm)
{
	long			qsec, qusec;
	struct timeval	end;

	if(tm->tv_sec == 0 && tm->tv_usec == 0)  {
		/* ó�� ȣ��ɶ� ���ľ� �� ��ƾ										*/
		gettimeofday(tm, NULL);

		return((float)0);
	} else  {
		/* ����(�ι�°) ȣ��ɶ� ���ľ� �� ��ƾ								*/
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
 * ���� : ���� GetElapsedTime()�� �ð� ���� ���۰� ���� ��� ���ǹǷ�		*
 *		  �� �Լ��� ����ϴ� �ҽ��� �м��� �� �� �뵵�� �ľ��ϱ� �����Ͽ�	*
 *		  ���۰� ���� ����ϴ� �Լ��� ���� �������.						*
 *		  �� �Լ��� ���� �ð��� ����ϴ� �Լ��̴�.							*
 *																			*
 * �μ� : struct timeval *tm												*
 *			���� ���� �ð��� �����ϱ� ���� �뵵								*
 *																			*
 * ���� :																	*
 *																			*
 * �ۼ� : 2001.07.14. (������ε� ����ؼ� �̰� �������.)					*
 * ���� :																	*
 ****************************************************************************/
void GetElapsedTime_Begin(struct timeval *tm)
{
	gettimeofday(tm, (struct timezone *)NULL);
}

/****************************************************************************
 * Prototype : void GetElapsedTime_End(struct timeval *)					*
 *																			*
 * ���� : ���� GetElapsedTime()�� �ð� ���� ���۰� ���� ��� ���ǹǷ�		*
 *		  �� �Լ��� ����ϴ� �ҽ��� �м��� �� �� �뵵�� �ľ��ϱ� �����Ͽ�	*
 *		  ���۰� ���� ����ϴ� �Լ��� ���� �������.						*
 *		  �� �Լ��� ���� �ð����κ��� ����� �ð��� ����ϴ� �Լ��̴�.		*
 *																			*
 * �μ� : struct timeval *tm												*
 *			���� �ð�														*
 *																			*
 * ���� : float�� �ð� ��(������ ��)										*
 *																			*
 * �ۼ� : 2001.07.14.														*
 * ���� :																	*
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
 * ���� : ���� ��¥�� �ð��� "YYYYMMDDHHMMSS" ������ �����ͷ� ����			*
 *																			*
 * �μ� :																	*
 *																			*
 * ���� : char *															*
 *																			*
 * �ۼ� : 2010.01.28.														*
 * ���� :																	*
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
		, tmTime->tm_year + 1900				/* 1900����� ���۵�		*/
		, tmTime->tm_mon  + 1					/* 0~11 ����				*/
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
 * ���� : ���� ��¥�� �ð��� "YYYYMMDDHHMMSSssssss" ������ �����ͷ� ����	*
 *																			*
 * �μ� :																	*
 *																			*
 * ���� : char *															*
 *																			*
 * �ۼ� : 2017.09.28.														*
 * ���� :																	*
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
 * ���� : ���� ��¥�� ���� (YYYY/MM/DD)										*
 *																			*
 * �μ� :																	*
 *																			*
 * ���� : char *															*
 *																			*
 * �ۼ� : 2010.01.28.														*
 * ���� :																	*
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
		, tmTime->tm_year + 1900		/* 1900����� ���۵�				*/
		, tmTime->tm_mon  + 1			/* 0~11 ����						*/
		, tmTime->tm_mday
	);

	return(szTime);
}

/****************************************************************************
 * Prototype : char *getWeekName(void)										*
 *																			*
 * ���� : ���� ��¥�� ���� ���ϸ��� ����									*
 *																			*
 * �μ� :																	*
 *																			*
 * ���� : char *															*
 *																			*
 * �ۼ� : 2015.04.01.														*
 * ���� :																	*
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
 * ���� : ���� ��¥�� ���Ͽ� �ش��ϴ� ��ȣ ���� 							*
 *																			*
 * �μ� :																	*
 *																			*
 * ���� : int (0:�Ͽ��� ~ 6:�����)											*
 *																			*
 * �ۼ� : 2015.04.01.														*
 * ���� :																	*
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
 * ���� : ���� �ð��� mSec ������ ���� (HHMMSSsss)							*
 *																			*
 * �μ� :																	*
 *																			*
 * ���� : char *															*
 *																			*
 * �ۼ� : 2015.04.01.														*
 * ���� :																	*
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
 * ���� : ���� �ð��� mSec ������ ���� (HH:MM:SS.sss)						*
 *																			*
 * �μ� :																	*
 *																			*
 * ���� : char *															*
 *																			*
 * �ۼ� : 2015.04.01.														*
 * ���� :																	*
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
 * ���� : ��¥ ���															*
 *																			*
 * �μ� : char *date														*
 *			�������� (YYYYMMDD, NULL �̸� ���� ����)						*
 *		  int days															*
 *			��� �� (0:����, -2:��Ʋ ��, 4:4�� ��)							*
 *																			*
 * ���� : char *															*
 *			���� ��¥ (YYYYMMDD)											*
 *																			*
 * �ۼ� : 2015.04.01.														*
 * ���� :																	*
 ****************************************************************************/
char *CalcDate(char *date, int days)
{
	int			secpday;
	static char	szDate[15];
	time_t		lTime;
	struct tm 	tmTime;

	secpday = 60 * 60 * 24;				/* 1�ϴ� ��							*/

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
		, tmTime.tm_year + 1900		/* 1900����� ���۵�					*/
		, tmTime.tm_mon  + 1		/* 0~11 ����							*/
		, tmTime.tm_mday
	);

	return(szDate);
}

