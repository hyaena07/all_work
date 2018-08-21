#ifndef _JPLIB_H
#define _JPLIB_H

/* Jupiter Library Options	*/
#define	JP_CREAT	00001		// 기존 파일이 있으면 삭제하고 생성한다.
#define	JP_NOWAIT	00010


/* for Journal File		*/
struct jpjn_idx  {
	long	jpjn_ofs;
	long	jpjn_len;
	int		jpjn_flg;
};

int	jpjn_init_daily	(char *);
int	jpjn_init		(char *, int);
int	jpjn_write		(char *, char *, int);
int	jpjn_read		(char *, long, char *);


/* for Sequence File	*/
int jpsq_init	(char *, int, int);
int jpsq_read	(char *, int);


/* for External Call	*/
#define	JPEC_NAME_LEN	64
#define	JPEC_DATA_MAX	(1024 * 10)
#define	JPEC_PORT_NUM	10000		// 미사용 포트 검색 시작값

struct jpec_data  {
	char	jpec_name[JPEC_NAME_LEN];
	long	jpec_i_len;
	char	jpec_i_buf[JPEC_DATA_MAX];
	long	jpec_o_len;
	char	jpec_o_buf[JPEC_DATA_MAX];
};

int jpec_call(struct jpec_data *, int);
int jpec_open(int);
int jpec_recv(int, char *);
int jpec_send(int, char *);
int jpec_close(int);


#endif	/* _JPLIB_H	*/
