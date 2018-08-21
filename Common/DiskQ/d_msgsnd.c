#include "DiskQ.h"

/****************************************************************************
 * Prototype : struct d_msgmap_ds *MakeDataArray(struct d_msgmap_ds *,		*
 *			int , struct d_msqid_ds)										*
 *																			*
 * ���� : Control File�� �о� �����Ͱ� �����ϴ� ��ġ�� ������� �ʴ� ��ġ�� *
 *		  ������ �����Ͽ� Map�� �����Ѵ�.									*
 *																			*
 * �μ� : struct d_msgmap_ds *ma											*
 *			Map�� ������ ����ü �迭�� �����ͷμ� ȣ���ϱ� ���� �ʿ���		*
 *			��ŭ�� �޸𸮸� Ȯ���� �Ŀ� ����Ͽ����ϸ�, �ʿ��� �޸�		*
 *			����� Queue�� �����ϴ� �޼����� ���� ���ϱ� 2�� 1�� ����		*
 *			��ŭ�� struct d_msgmap_ds�� ���� �� �־���Ѵ�.					*
 *		  int cfd															*
 *			Control File�� fd�μ� �� ���� '0'���� �۰ų� ������ ����		*
 *		  struct d_msqid_ds qh												*
 *			Queue header ����												*
 *																			*
 * ���� : (!= NULL) Data Map�� ���� �ּ�									*
 *			�� ���� �ּҴ� Map�� �����ϱ����� Ȯ���� �޸��� ����			*
 *			�ּҿʹ� �ٸ� ������, ��ũ�帮��Ʈ�� ������ message ���� ��		*
 *			���� ���ʿ� ����ִ� message Ȥ�� �� ���� ������ �������̴�.	*
 *		  (== NULL) ����													*
 *																			*
 * �ۼ� : 2002.01.03.														*
 * ���� :																	*
 ****************************************************************************/
struct d_msgmap_ds *MakeDataArray(struct d_msgmap_ds *ma, int cfd, struct d_msqid_ds qh)
{
	int i, j, anum, msgid;
	struct d_msgmap_ds	*blank_block = NULL;
	struct d_msgmap_ds	*prev = NULL;
	struct d_msgmap_ds	*curr = NULL;
	struct d_msgmap_ds	*next = NULL;
	struct d_msgmap_ds	*start = NULL;
	struct d_msghd_ds	mh;

	anum = qh.d_msg_qnum * 2 + 1;

	ma->d_msg_id = -1;
	ma->d_msg_dt = 0;
	ma->d_msg_soff = (ulong)0;
	ma->d_msg_eoff = qh.d_msg_qbytes - 1;
	ma->prev = NULL;
	ma->next = NULL;

	start = ma;

	blank_block	= ma + 1;
	msgid = qh.d_msg_ds;
	while(msgid >= 0)  {
		if(ReadMHead(cfd, msgid, &mh) < 0)  {
#ifdef _LIB_DEBUG
			printf("[MakeDataArray]ReadMHead() fail.\n");
#endif
			return(NULL);
		}

		curr = start;
		while(1)  {
			if(curr->d_msg_dt == 0 && curr->d_msg_soff <= mh.d_msg_whence && curr->d_msg_eoff >= (mh.d_msg_whence + mh.d_msg_mbytes - 1))  {
				if(curr->d_msg_soff	< mh.d_msg_whence)	{
					prev = curr;
					next = curr->next;
					curr = blank_block ++;
					curr->next = next;
					curr->prev = prev;
					prev->next = curr;
					if(next != (struct d_msgmap_ds *)NULL)  {
						next->prev = curr;
					}

					curr->d_msg_eoff = prev->d_msg_eoff;
					prev->d_msg_eoff = mh.d_msg_whence - 1;
				}
				if(curr->d_msg_eoff	> (mh.d_msg_whence + mh.d_msg_mbytes - 1))	{
					if(curr == start)  {
						/* ���� �ּ��� �տ�	�߰��Ǵ� ����̹Ƿ� ���� �ּҸ�	�����Ѵ�. */
						start = blank_block;
					}

					next = curr;
					prev = curr->prev;
					curr = blank_block ++;
					curr->next = next;
					curr->prev = prev;
					next->prev = curr;
					if(prev != (struct d_msgmap_ds *)NULL)	{
						prev->next = curr;
					}

					curr->d_msg_soff = next->d_msg_soff;
					next->d_msg_soff = mh.d_msg_whence + mh.d_msg_mbytes;
				}
				curr->d_msg_id = mh.d_msg_id;
				curr->d_msg_dt = 1;
				curr->d_msg_soff = mh.d_msg_whence;
				curr->d_msg_eoff = mh.d_msg_whence + mh.d_msg_mbytes - 1;

				break;
			}

			if(curr->next == NULL)  {
				return(NULL);
			}  else  {
				curr = curr->next;
			}
		}
		msgid = mh.d_msg_nextid;
	}

//view_map(start);

	return(start);
}

/****************************************************************************
 * Prototype : int MoveFData(struct d_msgmap_ds *, int, int)				*
 *																			*
 * ���� : message�� �հ� �ڿ� �� ������ ������ ��� message�� ������ �Ű�	*
 *		  �ΰ��� �� ������ ���������� Data File���� message�� ��ġ�� �ű��.*
 *																			*
 * �μ� : struct d_msgmap_ds *start											*
 *			Data File Map �� �ű�����ϴ� message ����						*
 *		  int cfd															*
 *			Control File�� fd												*
 *		  int dfd															*
 *			Data File�� fd													*
 *																			*
 * ���� : (==  0) ����														*
 *		  (== -1) ����														*
 *																			*
 * �ۼ� : 2001.12.28.														*
 * ���� :																	*
 ****************************************************************************/
int	MoveFData(struct d_msgmap_ds *curr, int cfd, int dfd)
{
	char *buf;
	struct d_msghd_ds	mh;
	struct d_msgmap_ds	*prev, *next;

	prev = curr->prev;
	next = curr->next;

	if(ReadMHead(cfd, curr->d_msg_id, &mh) < 0)  {
#ifdef _LIB_DEBUG
		printf("[MoveFData]ReadMHead() fail.\n");
#endif
		return(-1);
	}

	if((buf = (char *)malloc(mh.d_msg_mbytes)) == NULL)  {
#ifdef _LIB_DEBUG
		perror("[MoveFData]malloc");
#endif
		return(-2);
	}

	if(lseek(dfd, mh.d_msg_whence, SEEK_SET) < 0)  {
#ifdef _LIB_DEBUG
		perror("[MoveFData]lseek(2)");
#endif
		free(buf);
		return(-3);
	}
	if(read(dfd, buf, mh.d_msg_mbytes) != mh.d_msg_mbytes)  {
#ifdef _LIB_DEBUG
		perror("[MoveFData]read(2)");
#endif
		free(buf);
		return(-4);
	}

	if(lseek(dfd, prev->d_msg_soff, SEEK_SET) < 0)  {
#ifdef _LIB_DEBUG
		perror("[MoveFData]lseek(3)");
#endif
		free(buf);
		return(-5);
	}
	if(write(dfd, buf, mh.d_msg_mbytes) != mh.d_msg_mbytes)  {
#ifdef _LIB_DEBUG
		perror("[MoveFData]write(1)");
#endif
		free(buf);
		return(-6);
	}

	mh.d_msg_whence = prev->d_msg_soff;

	if(WriteMHead(cfd, curr->d_msg_id, &mh)	< 0)  {
#ifdef _LIB_DEBUG
		printf("[MoveFData]WriteMHead() fail.\n");
#endif
		free(buf);
		return(-7);
	}
	free(buf);

	prev->d_msg_dt = 1;
	prev->d_msg_eoff = prev->d_msg_soff + mh.d_msg_mbytes - 1;
	next->d_msg_soff = prev->d_msg_eoff + 1;

	prev->next = next;
	next->prev = prev;

	return(0);
}

/****************************************************************************
 * Prototype : int DataCompress_1(struct d_msgmap_ds *, int, int, int)		*
 *																			*
 * ���� : Data File�� ����ȭ ���� - message�� �հ� �ڿ� �� ������ ���� ��� *
 *		  message�� ������ �ű�� �ΰ��� �� ������ ���� �� �ִµ�, �̶�		*
 *		  ����� ������ size���� ũ�ų� ���� ��� MoveFData()�� �̿��Ͽ�	*
 *		  ���� �ű�� �۾��� �Ѵ�.											*
 *																			*
 * �μ� : struct d_msgmap_ds *start											*
 *			Data File Map�� ���� �ּ�										*
 *		  int cfd															*
 *			Control File�� fd												*
 *		  int dfd															*
 *			Data File�� fd													*
 *		  int size															*
 *			send�� �������� ����											*
 *																			*
 * ���� : (==  0) ����														*
 *		  (== -1) ����														*
 *			size ���� ũ�ų� ���� ũ���� �� ������ Ȯ������ ���߰ų� ����	*
 *			�� �Ϻο� ������ ����.											*
 *																			*
 * �ۼ� : 2001.12.28.														*
 * ���� :																	*
 ****************************************************************************/
int	DataCompress_1(struct d_msgmap_ds *start, int cfd, int dfd, int size)
{
	int d_size;
	struct d_msgmap_ds *curr, *prev, *next;

	prev = start;
	curr = prev->next;
	next = curr->next;
	while(1)  {
		if(prev->d_msg_dt == 0 && curr->d_msg_dt == 1 && next->d_msg_dt == 0)  {
			d_size = prev->d_msg_eoff -	prev->d_msg_soff + 1;
			d_size += next->d_msg_eoff - next->d_msg_soff + 1;
			if(d_size >= size)  {
				if(MoveFData(curr, cfd,	dfd) ==	0)  {
					return(0);
				} else  {
#ifdef _LIB_DEBUG
					printf("[DataCompress_1]MoveFData fail.]n");
#endif
					return(-1);
				}
			}
		}
		if(next->next == (struct d_msgmap_ds *)NULL)
			return(-1);
		prev = curr;
		curr = curr->next;
		next = curr->next;
	}
}

/****************************************************************************
 * Prototype : int MoveOneData(struct d_msgmap_ds *,						*
 *			struct d_msgmap_ds *, int, int)									*
 *																			*
 * ���� : �� ������ ���̳� �ڿ� �ִ� message�� �̿����� �ʴ� �� ��������	*
 *		  �Űܼ� �ʿ��� ��ŭ�� ������� Ȯ���ϸ�, message�� �ű�� �� ��	*
 *		  ������� �������� 3�� �ݺ��� ���� ��θ� ��� �ϳ��� ���������	*
 *		  �����Ѵ�.															*
 *																			*
 * �μ� : struct d_msgmap_ds *start											*
 *			Data File Map �� ���� �տ� �ִ� message Ȥ�� �� ������ ����		*
 *		  struct d_msgmap_ds *obj											*
 *			Data File Map �� �Űܾ��� message�� ����						*
 *		  int cfd															*
 *			Control File�� fd												*
 *		  int dfd															*
 *			Data File�� fd													*
 *																			*
 * ���� : (==  0) ����														*
 *		  (== -1) ����														*
 *			����̵Ǵ� message�� �ű� �� �ִ� �� ������ ã�� ���߰ų� ����	*
 *			�� �Ϻο� ������ �߻��ߴ�.										*
 *																			*
 * �ۼ� : 2001.12.28.														*
 * ���� :																	*
 ****************************************************************************/
int	MoveOneData(struct d_msgmap_ds *start, struct d_msgmap_ds *obj, int cfd, int dfd)
{
	ulong d_size;
	char *buf;
	struct d_msghd_ds	mh;
	struct d_msgmap_ds	*curr, *prev, *next, *tmp;

	d_size = obj->d_msg_eoff - obj->d_msg_soff + 1;

	prev = (struct d_msgmap_ds *)NULL;
	curr = start;
	next = curr->next;
	while(1)  {
		if(curr->d_msg_dt == 0 && (curr->d_msg_eoff - curr->d_msg_soff + 1) >= d_size)  {
			if(ReadMHead(cfd, obj->d_msg_id, &mh) <	0)	{
#ifdef _LIB_DEBUG
				printf("[MoveOneData]ReadMHead() fail.\n");
#endif
				return(-1);
			}

			if((buf = (char *)malloc(mh.d_msg_mbytes)) == NULL)  {
#ifdef _LIB_DEBUG
				perror("[MoveOneData]malloc");
#endif
				return(-2);
			}

			if(lseek(dfd, mh.d_msg_whence, SEEK_SET) < 0)  {
#ifdef _LIB_DEBUG
				perror("[MoveOneData]lseek(2)");
#endif
				free(buf);
				return(-3);
			}
			if(read(dfd, buf, mh.d_msg_mbytes) != mh.d_msg_mbytes)	{
#ifdef _LIB_DEBUG
				perror("[MoveOneData]read(2)");
#endif
				free(buf);
				return(-4);
			}

			if(lseek(dfd, curr->d_msg_soff, SEEK_SET) < 0)  {
#ifdef _LIB_DEBUG
				perror("[MoveOneData]lseek(3)");
#endif
				free(buf);
				return(-5);
			}
			if(write(dfd, buf, mh.d_msg_mbytes) != mh.d_msg_mbytes)	 {
#ifdef _LIB_DEBUG
				perror("[MoveOneData]write(1)");
#endif
				free(buf);
				return(-6);
			}

			mh.d_msg_whence = curr->d_msg_soff;

			if(WriteMHead(cfd, obj->d_msg_id, &mh) < 0)  {
#ifdef _LIB_DEBUG
				printf("[MoveOneData]WriteMHead() fail.\n");
#endif
				free(buf);
				return(-7);
			}
			free(buf);

			tmp = obj;
			if(tmp->prev != (struct	d_msgmap_ds *)NULL && tmp->prev->d_msg_dt == 0)  {
				tmp->prev->d_msg_eoff = tmp->d_msg_eoff;
				tmp->prev->next = tmp->next;
				if(tmp->next != (struct d_msgmap_ds *)NULL)  {
					tmp->next->prev = tmp->prev;
				}
				tmp =  tmp->prev;
			}
			if(tmp->next != (struct	d_msgmap_ds *)NULL && tmp->next->d_msg_dt == 0)  {
				tmp->next->d_msg_soff = tmp->d_msg_soff;
				tmp->next->prev = tmp->prev;
				if(tmp->prev != (struct d_msgmap_ds *)NULL)  {
					tmp->prev->next = tmp->next;
				}
				tmp = tmp->next;
			}
			tmp->d_msg_dt = 0;

			if((curr->d_msg_eoff - curr->d_msg_soff + 1) > d_size)	{
				obj->d_msg_dt = 0;
				obj->d_msg_soff = curr->d_msg_soff + d_size;
				obj->d_msg_eoff = curr->d_msg_eoff;
				obj->prev = curr;
				obj->next = curr->next;

				curr->d_msg_id = obj->d_msg_id;
				curr->d_msg_eoff = curr->d_msg_soff + d_size - 1;
				curr->next = obj;
			}
			curr->d_msg_dt = 1;
			return(0);
		}
		if(curr->next == (struct d_msgmap_ds *)NULL)  {
			return(-8);
		}

		prev = curr;
		curr = curr->next;
		next = curr->next;
		if(curr == obj->prev || curr == obj->next)  {
			if(curr->next == (struct d_msgmap_ds *)NULL)  {
				return(-9);
			}
			prev = curr;
			curr = curr->next;
			next = curr->next;
		}
	}
}

/****************************************************************************
 * Prototype : int DataCompress_2(struct d_msgmap_ds *, int, int, int)		*
 *																			*
 * ���� : Data File�� ����ȭ ���� - �� ������ ���̳� �ڿ� �ִ� message��	*
 *		  �Ű� �� ū ������� Ȯ���� �� �ִ� ��츦 ã�� MoveOneData()��	*
 *		  �̿��Ͽ� ���� �ű�� �۾��� �Ѵ�.									*
 *																			*
 * �μ� : struct d_msgmap_ds *start											*
 *			Data File Map�� ���� �ּ�										*
 *		  int cfd															*
 *			Control File�� fd												*
 *		  int dfd															*
 *			Data File�� fd													*
 *		  int size															*
 *			send�� �������� ����											*
 *																			*
 * ���� : (==  0) ����														*
 *		  (== -1) ����														*
 *			size ���� ũ�ų� ���� ũ���� �� ������ Ȯ������ ���߰ų� ����	*
 *			�� �Ϻο� ������ ����.											*
 *																			*
 * �ۼ� : 2001.12.28.														*
 * ���� :																	*
 ****************************************************************************/
int DataCompress_2(struct d_msgmap_ds *start, int cfd, int dfd, int size)
{
	ulong p_size, c_size, n_size;
	struct d_msgmap_ds *curr, *prev, *next;

	prev = (struct d_msgmap_ds *)NULL;
	curr = start;
	next = curr->next;
	while(1)  {
		c_size = curr->d_msg_eoff - curr->d_msg_soff + 1;
		if(prev != (struct d_msgmap_ds *)NULL)
			p_size = prev->d_msg_eoff - prev->d_msg_soff + 1;
		else
			p_size = 0;
		if(next != (struct d_msgmap_ds *)NULL)
			n_size = next->d_msg_eoff - next->d_msg_soff + 1;
		else
			n_size = 0;

		if(curr->d_msg_dt == 0)  {
			if(prev != (struct d_msgmap_ds *)NULL && prev->d_msg_dt	== 1)  {
				if(c_size + p_size >= size)	 {
					if(MoveOneData(start, prev, cfd, dfd) == 0)
						return(0);
				}
			}
			if(next != (struct d_msgmap_ds *)NULL && next->d_msg_dt == 1)  {
				if(c_size + n_size >= size)  {
					if(MoveOneData(start, next, cfd, dfd) == 0)
						return(0);
				}
			}

		} else  {
			if((prev != (struct	d_msgmap_ds	*)NULL && prev->d_msg_dt == 0) && (next != (struct d_msgmap_ds *)NULL && next->d_msg_dt == 0))	{
				if((p_size + c_size + n_size) >= size)  {
					if(MoveOneData(start, curr, cfd, dfd) == 0)
						return(0);
				}
			}
		}

		if(curr->next == (struct d_msgmap_ds *)NULL)
			return(-1);
		prev = curr;
		curr = curr->next;
		next = curr->next;
	}
}

/****************************************************************************
 * Prototype : int DataCompress_3(struct d_msgmap_ds *, int, int, int)		*
 *																			*
 * ���� : Data File�� ����ȭ ���� - ���� ���Ŀ� ����ϰ� �Ǵ� ������� ����	*
 *		  �տ� �ִ� message���� �����Ͽ� ����� �� ������ Ȯ���ɶ�����		*
 *		  message���� ������ �Ű� ���ӵǰ� ��ġ�Ѵ�.						*
*																			*
 * �μ� : struct d_msgmap_ds *start											*
 *			Data File Map�� ���� �ּ�										*
 *		  int cfd															*
 *			Control File�� fd												*
 *		  int dfd															*
 *			Data File�� fd													*
 *		  int size															*
 *			send�� �������� ����											*
 *																			*
 * ���� : (==  0) ����														*
 *		  (== -1) ����														*
 *																			*
 * �ۼ� : 2001.12.28.														*
 * ���� :																	*
 ****************************************************************************/
int DataCompress_3(struct d_msgmap_ds *start, int cfd, int dfd, int size)
{
	char *buf;
	struct d_msghd_ds	mh;
	struct d_msgmap_ds	*curr, *prev, *next, tmp;

	prev = (struct d_msgmap_ds *)NULL;
	curr = start;
	next = curr->next;
	while(1)  {
		if(curr->d_msg_dt == 0)	 {
			if(ReadMHead(cfd, next->d_msg_id, &mh) < 0)	 {
#ifdef _LIB_DEBUG
				printf("[DataCompress_3]ReadMHead()	fail.\n");
#endif
				return(-1);
			}

			if((buf	= (char	*)malloc(mh.d_msg_mbytes)) == NULL)	 {
#ifdef _LIB_DEBUG
				perror("[DataCompress_3]malloc");
#endif
				return(-2);
			}

			if(lseek(dfd, mh.d_msg_whence, SEEK_SET) < 0)  {
#ifdef _LIB_DEBUG
				perror("[DataCompress_3]lseek(1)");
#endif
				free(buf);
				return(-3);
			}
			if(read(dfd, buf, mh.d_msg_mbytes) != mh.d_msg_mbytes)	{
#ifdef _LIB_DEBUG
				perror("[DataCompress_3]read");
#endif
				free(buf);
				return(-4);
			}

			if(lseek(dfd, curr->d_msg_soff,	SEEK_SET) <	0)	{
#ifdef _LIB_DEBUG
				perror("[DataCompress_3]lseek(2)");
#endif
				free(buf);
				return(-5);
			}
			if(write(dfd, buf, mh.d_msg_mbytes)	!= mh.d_msg_mbytes)	 {
#ifdef _LIB_DEBUG
				perror("[DataCompress_3]write(1)");
#endif
				free(buf);
				return(-6);
			}
			free(buf);

			mh.d_msg_whence	= curr->d_msg_soff;

			if(WriteMHead(cfd, next->d_msg_id, &mh)	< 0)  {
#ifdef _LIB_DEBUG
				printf("[DataCompress_3]WriteMHead() fail.\n");
#endif
				return(-7);
			}

			tmp.d_msg_soff = curr->d_msg_soff;
			tmp.d_msg_eoff = curr->d_msg_eoff;

			curr->d_msg_eoff = curr->d_msg_soff	+ mh.d_msg_mbytes -	1;
			curr->d_msg_dt = 1;
			curr->d_msg_id = next->d_msg_id;

			next->d_msg_soff = curr->d_msg_eoff	+ 1;
			next->d_msg_eoff = next->d_msg_soff	+ tmp.d_msg_eoff - tmp.d_msg_soff;
			next->d_msg_dt = 0;
			next->d_msg_id = 0;

			if(next->next != (struct d_msgmap_ds *)NULL)  {
				if(next->next->d_msg_dt	== 0)  {
					next->d_msg_eoff = next->next->d_msg_eoff;
					next->next = next->next->next;
					if(next->next != (struct d_msgmap_ds *)NULL)  {
						next->next->prev = next;
					}
				}
			}

			if((next->d_msg_eoff - next->d_msg_soff	+ 1) >=	size)
				return(0);
		}

		if(next->next == (struct d_msgmap_ds *)NULL)
			return(-8);
		prev = curr;
		curr = curr->next;
		next = curr->next;
	}
}

/****************************************************************************
 * Prototype : int FindSpace(int, int , struct d_msqid_ds, int, int *,		*
 *			int *)															*
 *																			*
 * ���� : Data File�� Map�� �����Ͽ� �����͸� ������ ��ġ�� �����ϸ� �ʿ���	*
 *		  ��� �ʿ��� ������ Ȯ���ϱ����� Data Compress ������ ȣ���Ѵ�.	*
 *																			*
 * �μ� : int cfd															*
 *			Control File�� fd												*
 *		  int dfd															*
 *			Data File�� fd													*
 *		  struct d_msqid_ds qh												*
 *			Queue header�� ����.											*
 *		  int size															*
 *			send�� �������� ����											*
 *		  int *des_off														*
 *			�����Ͱ� ����� Data File�� offset								*
 *																			*
 * ���� : (==  0) ����														*
 *		  (== -1) ����														*
 *																			*
 * �ۼ� : 2001.12.26.														*
 * ���� :																	*
 ****************************************************************************/
int	FindSpace(int cfd, int dfd,	struct d_msqid_ds qh, int size,	int	*des_off)
{
	int	b_size = -1;
	struct d_msgmap_ds *marray,	*start;
	struct d_msgmap_ds *curr = NULL;

	if((marray = (struct d_msgmap_ds *)calloc(qh.d_msg_qnum	* 2	+ 1, sizeof(struct d_msgmap_ds))) == NULL)	{
#ifdef _LIB_DEBUG
		perror("[FindSpace]calloc");
#endif
		return(-1);
	}

	if((start =	MakeDataArray(marray, cfd, qh))	== NULL)  {
#ifdef _LIB_DEBUG
		printf("[FindSpace]MakeDataArray() fail.\n");
#endif
		free(marray);
		return(-2);
	}

	while(1)  {
		b_size = -1;
		*des_off = -1;
		curr = start;
		while(1)  {
			if(curr->d_msg_dt == 0 && (curr->d_msg_eoff	- curr->d_msg_soff + 1)	>= size)  {
				if(b_size == -1	|| b_size >	((curr->d_msg_eoff - curr->d_msg_soff +	1) - size))	 {
					b_size = (curr->d_msg_eoff - curr->d_msg_soff +	1) - size;
					*des_off = curr->d_msg_soff;
				}
			}
			if(curr->next == (struct d_msgmap_ds *)NULL)  {
				break;
			} else {
				curr = curr->next;
			}
		}

		if(*des_off	!= -1)	{
			free(marray);
			return(0);
		}

printf("Start Compress_1...\n");
		if(DataCompress_1(start, cfd, dfd, size) !=	0)	{
#ifdef _LIB_DEBUG
			printf("[FindSpace]DataCompress_1()	fail.\n");
#endif
printf("Start Compress_2...\n");
			if(DataCompress_2(start, cfd, dfd, size) !=	0)	{
#ifdef _LIB_DEBUG
				printf("[FindSpace]DataCompress_2()	fail.\n");
#endif
printf("Start Compress_3...\n");
				if(DataCompress_3(start, cfd, dfd, size) !=	0)	{
#ifdef _LIB_DEBUG
					printf("[FindSpace]DataCompress_3()	fail.\n");
#endif

					break;
				}
			}
		}
//view_map(start);
	}

	free(marray);

	if(*des_off	== -1)	{
		return(-3);
	} else	{
		return(0);
	}
}

/****************************************************************************
 * Prototype : int d_msgsnd_main(int, void *, int, int)						*
 *																			*
 * ���� : Message Queue�� Message�� �����Ѵ�.								*
 *																			*
 * �μ� : int qid															*
 *			Message Queue�� ���� ��ȣ										*
 *		  void *buf															*
 *			������ ������ ���� ������										*
 *			struct msgbuf  {												*
 *				long mtype;													*
 *				char mtext[1];												*
 *			};																*
 *		  int size															*
 *			������ ������ ���� (byte)										*
 *		  int flag															*
 *			Message ���ۿ� �ʿ��� flag��									*
 *			IPC_NOWAIT : Queue�� ���� ������ ������ ��� ���ŷ���� �ʰ�	*
 *						 EAGAIN ������ �����ش�.							*
 *																			*
 * ���� : (==  0) ����														*
 *		  (== -1) ����														*
 *																			*
 * �ۼ� : 2001.12.21.														*
 * ���� :																	*
 ****************************************************************************/
int d_msgsnd_main(int qid, void *buf, int size, int flag)
{
	int	des_id;
	int	des_off;
	char ch;
	sigset_t new, old;
/* for LINUX
	char cpath[QPATH_LEN + QNAME_LEN + strlen(CTL_EXP) + 1];
	char dpath[QPATH_LEN + QNAME_LEN + strlen(DAT_EXP) + 1];
*/
	char cpath[QPATH_LEN + QNAME_LEN + 3 + 1];
	char dpath[QPATH_LEN + QNAME_LEN + 3 + 1];
	struct d_msqid_ds qh;

	struct d_msqinfo_ds	*qinfo;
	struct d_msghd_ds	mh,	de_mh, es_mh;
	struct msgbuf  {
		long mtype;
		char mtext[1];
	};

	struct msgbuf *msg_buf = (struct msgbuf	*)buf;

	lock_sig_all(&new, &old);

	qinfo =	D_msqinfo +	(qid - 1);
	sprintf(cpath, "%s/%s.%s", qinfo->d_msg_fpath, qinfo->d_msg_qname, CTL_EXP);
	sprintf(dpath, "%s/%s.%s", qinfo->d_msg_fpath, qinfo->d_msg_qname, DAT_EXP);

	if(GetQHeader(cpath, &qh, qid) < 0)	 {
#ifdef _LIB_DEBUG
		printf("[d_msgsnd_main]GetQHeader()	fail.\n");
#endif
		unlock_sig_all(&old);
		return(-1);
	}

	/* send�� �������� ����	��ŭ�� �� ������ �ִ���	�˻��Ѵ�. */
	if(qh.d_msg_qbytes < (qh.d_msg_cbytes +	size))	{
#ifdef _LIB_DEBUG
		printf("[d_msgsnd_main]������ ������ �����մϴ�.\n");
#endif
		errno =	EAGAIN;
		unlock_sig_all(&old);
		return(-2);
	}

	if (is_sig_catch() > 0)
	{
		errno =	EINTR;
		unlock_sig_all(&old);
		return(-3);
	}

	if(FindSpace(qinfo->d_msg_cfd, qinfo->d_msg_dfd, qh, size, &des_off) < 0)  {
#ifdef _LIB_DEBUG
		printf("[d_msgsnd_main]������ ������ ã�� ���߽��ϴ�.\n");
#endif
		errno =	EAGAIN;
		unlock_sig_all(&old);
		return(-4);
	}

#if	0
printf("des_off	: [%d],	size : [%d]\n",	des_off, size);
fflush(stdin);
printf("UPDATE?	(Y/N):");
scanf("%c",	&ch);
//ch = getchar();
if(ch != 'y' &&	ch != 'Y')	{
	unlock_sig_all(&old);
	return(0);
}
#endif

	if (is_sig_catch() > 0)
	{
		errno =	EINTR;
		unlock_sig_all(&old);
		return(-5);
	}

	/* Data	File Update	start */
	if(lseek(qinfo->d_msg_dfd, des_off,	SEEK_SET) <	0)	{
#ifdef _LIB_DEBUG
		perror("[d_msgsnd_main]lseek(1)");
#endif
		unlock_sig_all(&old);
		return(-6);
	}

	if(write(qinfo->d_msg_dfd, msg_buf->mtext, size) !=	size)  {
#ifdef _LIB_DEBUG
		perror("[d_msgsnd_main]write(1)");
#endif
		unlock_sig_all(&old);
		return(-7);
	}
	/* Data	File Update	end	*/

	/* Control File	Update start */
	if(qh.d_msg_es == -1)  {
		des_id = qh.d_msg_qnum;
	} else	{
		des_id = qh.d_msg_es;
		if(ReadMHead(qinfo->d_msg_cfd, qh.d_msg_es,	&es_mh)	< 0)  {
#ifdef _LIB_DEBUG
			printf("[d_msgsnd_main]ReadMHead() fail.\n");
#endif
			unlock_sig_all(&old);
			return(-8);
		}
	}

	if(ReadMHead(qinfo->d_msg_cfd, qh.d_msg_de,	&de_mh)	< 0)  {
#ifdef _LIB_DEBUG
		printf("[d_msgsnd_main]ReadMHead() fail.\n");
#endif
		unlock_sig_all(&old);
		return(-9);
	}
	de_mh.d_msg_nextid = des_id;
	if(WriteMHead(qinfo->d_msg_cfd,	qh.d_msg_de, &de_mh) < 0)  {
#ifdef _LIB_DEBUG
		printf("[d_msgsnd_main]WriteMHead()	fail.\n");
#endif
		unlock_sig_all(&old);
		return(-10);
	}

	mh.d_msg_id	= des_id;
	mh.d_msg_mtype = msg_buf->mtype;
	mh.d_msg_alive = 1;
	mh.d_msg_whence	= des_off;
	mh.d_msg_mbytes	= size;
	mh.d_msg_uid = qh.d_msg_tcnt;
	mh.d_msg_stime = time((time_t)NULL);
	mh.d_msg_nextid	= -1;

	if(WriteMHead(qinfo->d_msg_cfd,	des_id,	&mh) < 0)  {
#ifdef _LIB_DEBUG
		printf("[d_msgsnd_main]WriteMHead()	fail.\n");
#endif
		unlock_sig_all(&old);
		return(-11);
	}

	qh.d_msg_cbytes	+= size;
	qh.d_msg_qnum += 1;
	if(qh.d_msg_tcnt >=	MAX_CNT)
		qh.d_msg_tcnt =	0;
	else
		qh.d_msg_tcnt += 1;

	if(qh.d_msg_ds == -1)
		qh.d_msg_ds	= des_id;
	qh.d_msg_de	= des_id;
	if((qh.d_msg_es	!= -1) && (qh.d_msg_es == qh.d_msg_ee))
		qh.d_msg_es	= qh.d_msg_ee =	-1;
	else if(qh.d_msg_es	!= -1)
		qh.d_msg_es	= es_mh.d_msg_nextid;
	qh.d_msg_lspid = geteuid();
	qh.d_msg_stime = time((time_t)NULL);

	if(lseek(qinfo->d_msg_cfd, 0, SEEK_SET)	< 0)  {
#ifdef _LIB_DEBUG
		perror("[d_msgsnd_main]lseek(3)");
#endif
		unlock_sig_all(&old);
		return(-12);
	}

	if(write(qinfo->d_msg_cfd, &qh,	QHEAD_SZ) != QHEAD_SZ)	{
#ifdef _LIB_DEBUG
		perror("[d_msgsnd_main]write(3)");
#endif
		unlock_sig_all(&old);
		return(-13);
	}

	unlock_sig_all(&old);
	return(0);
}

/****************************************************************************
 * Prototype : int wakeup_rcv(int, int, char *, int, long)					*
 *																			*
 * ���� : mtype�� �����ϴ� d_msgrcv() ��� ���� ���μ����� ������ �뺸�Ͽ�	*
 *		  ���� ó���� �� �� �ֵ��� �Ѵ�.									*
 *																			*
 * �μ� : int shmid															*
 *			��⿭ ������ ��ϵ� shared memory ID							*
 *		  int semid															*
 *			Semaphores ID													*
 *		  char *qpath														*
 *			Queue File ���													*
 *		  int size															*
 *			������ ������ ���� (byte)										*
 *		  long mtype														*
 *			�뺸 ����� �� mtype											*
 *																			*
 * ���� : (==  0) ����														*
 *		  (== -1) FIFO�� ����. (��⿭�� �ִ� ���μ����� ������ ��������.)	*
 *		  (== -2) ������ ����ڸ� ã�� ������.								*
 *		  (== -3) ����														*
 *																			*
 * �ۼ� : 2002.01.10.														*
 * ���� :																	*
 ****************************************************************************/
int wakeup_rcv(int shmid, int semid, char *qpath, int size, long mtype)
{
	int	i, pix,	bix, ffd, ret =	0;
	char fifopath[QPATH_LEN	+ QNAME_LEN	+ 4	+ 3];
	void *shmptr;
	struct d_msgwhd_ds		*shmhead;
	struct d_msgwait_ds		*waitptr;
	struct timeval tv;
	fd_set fds;

	if(LOCK(semid) < 0)	 {
#ifdef _LIB_DEBUG
		printf("[wakeup_rcv]LOCK() fail.\n");
#endif
		return(-1);
	}

	if((shmptr = shmat(shmid, 0, 0)) ==	(void *)-1)	 {
#ifdef _LIB_DEBUG
		perror("[wakeup_rcv]shmat");
#endif
		return(-2);
	}

	shmhead	= (struct d_msgwhd_ds *)shmptr;
	waitptr	= (struct d_msgwait_ds *)((char	*)shmptr + sizeof(struct d_msgwhd_ds));

	i =	shmhead->d_msg_rs;

	while(i	>= 0)  {
		if(waitptr[i].use == 1 && (waitptr[i].mtype	== 0 ||	mtype == waitptr[i].mtype || (waitptr[i].mtype < 0 && -waitptr[i].mtype	>= mtype)))	 {
			break;
		}
		if(i ==	shmhead->d_msg_re)	{
			i =	-1;
			break;
		} else	{
			i =	waitptr[i].nextid;
		}
	}

	if(i ==	-1)	 {
		UNLOCK(semid);
		shmdt(shmptr);

		return(-3);
	}

	sprintf(fifopath, "%s/%s.%s", qpath, waitptr[i].fifoname, FIFO_EXP);
	if((ffd	= open(fifopath, O_RDWR)) <	0)	{
#ifdef _LIB_DEBUG
		perror("[wakeup_rcv]open");
#endif
		ret	= -1;
	}

	if (ret	== 0)
	{
		FD_ZERO(&fds);
		FD_SET(ffd,	&fds);
		tv.tv_sec =	1;
		tv.tv_usec = 0;

		if(select(ffd +	1, &fds, NULL, NULL, &tv) <= 0)
			ret	= -1;
	}

	close(ffd);
	unlink(fifopath);

	if(shmhead->d_msg_rs ==	i)	{
		if(shmhead->d_msg_re ==	i)
			shmhead->d_msg_rs =	shmhead->d_msg_re =	-1;
		shmhead->d_msg_rs =	waitptr[i].nextid;
	} else {
		bix	= pix =	shmhead->d_msg_rs;
		while(pix != i)	 {
			bix	= pix;
			pix	= waitptr[pix].nextid;
		}
		waitptr[bix].nextid	= waitptr[i].nextid;
		if(shmhead->d_msg_re ==	i)
			shmhead->d_msg_re =	bix;
	}
	waitptr[i].use = 0;

	shmdt(shmptr);
	UNLOCK(semid);

	return(ret);
}

/****************************************************************************
 * Prototype : int wait_for_snd(int, int)									*
 *																			*
 * ���� : message�� �����ϱ����� ������ ������ ��� ȣ��Ǹ�, wakeup_snd()��*
 *		  ���� �����.													*
 *																			*
 * �μ� : int qid															*
 *			��� Queue ID													*
 *		  int fcall															*
 *			ó�� ȣ������ ��ȣ������ �����Ͽ� ó�� ȣ���� ���� ��⿭��	*
 *			�������� �ڽ��� ������ �߰��ϸ�, ��ȣ���� ���� ��⿭��		*
 *			ó���� �ڽ��� ������ �߰��Ѵ�. (0: ó�� ȣ��, 1: ��ȣ��)		*
 *																			*
 * ���� : (==  0) ����														*
 *		  (== -1) ����														*
 *																			*
 * �ۼ� : 2002.01.10.														*
 * ���� :																	*
 ****************************************************************************/
int wait_for_snd(int qid, int fcall)
{
	int	eix, pix, bix, ffd,	ret	= 1;
	long t_size;
	char fifoname[QPATH_LEN	+ QNAME_LEN	+ 4	+ 3];
	char fifopath[QPATH_LEN	+ QNAME_LEN	+ 4	+ 3];
	void *shmptr;
	mode_t old_umask;
	struct d_msgwhd_ds		*shmhead;
	struct d_msgwait_ds		*waitptr;

	sprintf(fifoname, "%s",	mkfifoname(D_msqinfo[qid - 1].d_msg_fpath));
	sprintf(fifopath, "%s/%s.%s", D_msqinfo[qid	- 1].d_msg_fpath, fifoname,	FIFO_EXP);
	old_umask =	umask(000);

	if(mkfifo(fifopath,	0666) != 0)	 {
		umask(old_umask);
#ifdef _LIB_DEBUG
		perror("[wait_for_snd]mkfifo");
#endif
		return(-1);
	}
	umask(old_umask);

	if((shmptr = shmat(D_msqinfo[qid - 1].d_msg_shmid, 0, 0)) == (void *)-1)  {
#ifdef _LIB_DEBUG
		perror("[wait_for_snd]shmat");
#endif
		return(-2);
	}

	shmhead	= (struct d_msgwhd_ds *)shmptr;
	waitptr	= (struct d_msgwait_ds *)((char	*)shmptr + sizeof(struct d_msgwhd_ds));

	if(LOCK(D_msqinfo[qid -	1].d_msg_semid)	< 0)  {
#ifdef _LIB_DEBUG
		printf("[wait_for_snd]LOCK(1) fail.\n");
#endif
		shmdt(shmptr);

		return(-3);
	}

	for(eix	= 0; eix < MAX_WAIT; eix++)	 {
		if(waitptr[eix].use	== 0)  {
			break;
		}
	}

	if(eix == MAX_WAIT)	 {
#ifdef _LIB_DEBUG
		printf("[wait_for_snd]��⿭ ������	��á����...\n");
#endif
		UNLOCK(D_msqinfo[qid - 1].d_msg_semid);
		shmdt(shmptr);
		unlink(fifopath);
		return(-4);
	}

	waitptr[eix].use = 1;
	strcpy(waitptr[eix].fifoname, fifoname);
	waitptr[eix].nextid	= -1;

	if(fcall ==	0)	{
		if(shmhead->d_msg_se ==	-1)	 {
			shmhead->d_msg_ss =	shmhead->d_msg_se =	eix;
		} else	{
			waitptr[shmhead->d_msg_se].nextid =	eix;
			shmhead->d_msg_se =	eix;
		}
	}  else	 {
		if(shmhead->d_msg_ss ==	-1)	 {
			shmhead->d_msg_ss =	shmhead->d_msg_se =	eix;
		} else	{
			waitptr[eix].nextid	= shmhead->d_msg_ss;
			shmhead->d_msg_ss =	eix;
		}
	}

	if(fcall ==	1)
		SND_WAIT_UNLOCK(D_msqinfo[qid -	1].d_msg_semid);

	UNLOCK(D_msqinfo[qid - 1].d_msg_semid);
	shmdt(shmptr);

	FUNLOCK(D_msqinfo[qid -	1].d_msg_cfd);

	if((ffd	= open(fifopath, O_WRONLY))	< 0)  {
#ifdef _LIB_DEBUG
		perror("[wait_for_snd]open");
#endif
		close(ffd);
		FLOCK(D_msqinfo[qid	- 1].d_msg_cfd);
		return(-5);
	}

	if(ret && (write(ffd, &t_size, sizeof(int))	<= 0))	{
#ifdef _LIB_DEBUG
		perror("[wait_for_snd]write");
#endif
		close(ffd);
		FLOCK(D_msqinfo[qid	- 1].d_msg_cfd);
		return(-6);
	}

	FLOCK(D_msqinfo[qid	- 1].d_msg_cfd);

	if(SND_WAIT_LOCK(D_msqinfo[qid - 1].d_msg_semid) < 0)  {
#ifdef _LIB_DEBUG
		printf("[FindSndWait]SND_WAIT_LOCK() fail.\n");
#endif
		return(-7);
	}

	close(ffd);
	return(0);
}

/****************************************************************************
 * Prototype : int FindSndWait(int, int)									*
 *																			*
 * ���� : wait_for_send ��⿭�� ������� ���μ����� �ִ��� �˻��Ѵ�.		*
 *																			*
 * �μ� : int shmid															*
 *			��⿭ ������ ��ϵ� shared memory ID							*
 *		  int semid															*
 *			Semaphores ID													*
 *																			*
 * ���� : (==  1) �ִ�.														*
 *		  (==  0) ����.														*
 *		  (== -1) ����.														*
 *																			*
 * �ۼ� : 2002.01.10.														*
 * ���� :																	*
 ****************************************************************************/
int FindSndWait(int shmid, int semid)
{
	int	i, ret = 0;
	struct d_msgwhd_ds *shmhead;

	if(SND_WAIT_LOCK(semid)	< 0)  {
#ifdef _LIB_DEBUG
		printf("[FindSndWait]SND_WAIT_LOCK() fail.\n");
#endif
		return(-1);
	}

	if((shmhead	= shmat(shmid, 0, 0)) == (void *)-1)  {
#ifdef _LIB_DEBUG
		perror("[FindSndWait]shmat");
#endif
		SND_WAIT_UNLOCK(semid);
		return(-2);
	}

	if(LOCK(semid) < 0)	 {
#ifdef _LIB_DEBUG
		printf("[FindSndWait]LOCK()	fail.\n");
#endif
		SND_WAIT_UNLOCK(semid);
		return(-3);
	}

	if(shmhead->d_msg_ss !=	-1)
		ret	= 1;

	UNLOCK(semid);

	shmdt(shmhead);

	SND_WAIT_UNLOCK(semid);

	return(ret);
}

/****************************************************************************
 * Prototype : int d_msgsnd(int, const void *, int, int)					*
 *																			*
 * ���� : Message Queue�� Message�� �����Ѵ�.								*
 *																			*
 * �μ� : int qid															*
 *			Message Queue�� ���� ��ȣ										*
 *		  void *buf															*
 *			������ ������ ���� ������										*
 *			struct msgbuf  {												*
 *				long mtype;													*
 *				char mtext[1];												*
 *			};																*
 *		  int flag															*
 *			Message ���ۿ� �ʿ��� flag��									*
 *			IPC_NOWAIT : Queue�� ���� ������ ������ ��� ���ŷ���� �ʰ�	*
 *						 EAGAIN ������ �����ش�.							*
 *		  int size															*
 *			������ ������ ���� (byte)										*
 *																			*
 * ���� : (!= -1) ���� ������ ���� (byte)									*
 *		  (== -1) ����														*
 *																			*
 * ���� : EAGAIN															*
 *			Queue�� ���� ������ �����ϴ�.									*
 *		  EACCES															*
 *			������ Queue�� ���� ������ ����.								*
 *		  EIDRM																*
 *			������ Queue�� ���ŵǾ���.										*
 *		  EINTR																*
 *			interrupt�� ���� ����Ǿ���.									*
 *																			*
 * �ۼ� : 2002.01.10.														*
 * ���� :																	*
 ****************************************************************************/
int d_msgsnd(int qid, void *buf, int size, int flag)
{
	int	ret, sndret, fcall,	wait = 0;
	struct d_msqinfo_ds	*qinfo;
	struct msgbuf  {
		long mtype;
		char mtext[1];
	};
	struct timeval s_tv;
	struct msgbuf *msg_buf = (struct msgbuf	*)buf;

	/* mtype '0'���� �۰ų�	���� ��� */
	if(msg_buf->mtype <= 0)	 {
		errno =	EINVAL;
		return(-1);
	}

	/* ��û	���� qid�� ����	�����ϴ� ������	�˻��Ѵ�. */
	if(qid <= 0	|| qid > D_OpenQNum)  {
		errno =	EIDRM;
		return(-2);
	}

	qinfo =	D_msqinfo +	(qid - 1);
	if(qinfo->d_msg_cfd	== 0)  {
		errno =	EIDRM;
		return(-3);
	}

	/* ��û	���� qid�� send	������ �ִ��� �˻��Ѵ�.	*/
	if(CheckPerm(qinfo->d_msg_perm,	SND_PERM_CHK) <	0)	{
#ifdef _LIB_DEBUG
		printf("[d_msgsnd]CheckPerm() fail.\n");
#endif
		errno =	EACCES;
		return(-4);
	}

	/* send�� �������� ���̰� ��ü queue �������	�۰ų� ������ �˻��Ѵ�.	*/
	if(qinfo->d_msg_qbytes < size)	{
#ifdef _LIB_DEBUG
		printf("[d_msgsnd]������ �޼����� ��ü queue ���̸�	�ʰ��Ͽ����ϴ�.\n");
#endif
		errno =	EINVAL;
		return(-5);
	}

/*
	while(1)  {
		if((wait = FindSndWait(qinfo->d_msg_shmid, qinfo->d_msg_semid))	< 0)  {
#ifdef _LIB_DEBUG
			printf("[d_msgsnd]FindSndWait()	fail.\n");
#endif
			return(-6);
		}

		if(wait	== 1)  {
			ret	= wakeup_snd(qinfo->d_msg_shmid, qinfo->d_msg_semid, size, qinfo->d_msg_fpath);
			if(ret == 0)  {
				break;
			} else if(ret == -1)  {
				continue;
			} else if(ret == -2)  {
				wait = 0;
				break;
			}  else	 {
#ifdef _LIB_DEBUG
				printf("[d_msgsnd]wakeup_snd(1)	fail.\n");
#endif
				break;
			}
		} else	{
			break;
		}
	}
*/

	if(FLOCK(D_msqinfo[qid - 1].d_msg_cfd) < 0)	 {
#ifdef _LIB_DEBUG
		printf("[d_msgsnd]FLOCK	fail.\n");
#endif
		return(-7);
	}

	if((wait = FindSndWait(qinfo->d_msg_shmid, qinfo->d_msg_semid))	< 0)  {
#ifdef _LIB_DEBUG
		printf("[d_msgsnd]FindSndWait()	fail.\n");
#endif
		return(-8);
	}

	bzero(&s_tv, sizeof(struct timeval));

	fcall =	0;
	while(1)  {
		if(wait	== 1)  {
			if(flag	& IPC_NOWAIT)  {
				errno =	EAGAIN;
				sndret = -1;
				break;
			} else	{
				/* ������ Ȯ���ɶ����� ���ŷ�ϱ� ���� ��ƾ	*/
				if(wait_for_snd(qid, fcall)	< 0)  {
#ifdef _LIB_DEBUG
					printf("[d_msgsnd]wait_for_snd fail.\n");
#endif
					sndret = -1;
					break;
				}  else	 {
					fcall =	1;
					wait = 0;
					continue;
				}
			}
		} else if(wait == 0)  {
			if(d_msgsnd_main(qid, buf, size, flag) < 0)	 {
				if(errno ==	EAGAIN)	 {
					wait = 1;
					continue;
				} else	{
#ifdef _LIB_DEBUG
					perror("[d_msgsnd]d_msgsnd_main");
#endif
					sndret = -1;
					break;
				}
			} else {
				sndret = 0;
				break;
			}
		}
	}

	if(fcall ==	1)
		SND_WAIT_UNLOCK(qinfo->d_msg_semid);

	FUNLOCK(D_msqinfo[qid -	1].d_msg_cfd);

	while(sndret ==	0)	{
		ret	= wakeup_rcv(qinfo->d_msg_shmid, qinfo->d_msg_semid, qinfo->d_msg_fpath, size, msg_buf->mtype);
		if(ret == 0)  {
			break;
		} else if(ret == -1)  {
			continue;
		}  else	 {
#ifdef _LIB_DEBUG
//			  printf("[d_msgsnd]wakeup_rcv() fail.\n");
#endif
			break;
		}
	}

	while(1)  {
		ret	= wakeup_snd(qinfo->d_msg_shmid, qinfo->d_msg_semid, size, qinfo->d_msg_fpath);
		if(ret == 0)  {
			break;
		} else if(ret == -1)  {
			continue;
		}  else	 {
#ifdef _LIB_DEBUG
//			  printf("[d_msgsnd]wakeup_snd(2) fail.\n");
#endif
			break;
		}
	}

	return(sndret);
}
