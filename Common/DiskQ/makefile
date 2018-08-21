include $(HOME)/work/src/comenv

INCS	= ../inc
LIBS	= ../../../lib
DEFS	= $(G_DEFS)

CFLAGS	= $(G_CFLAGS) $(DEFS) -I$(INCS)

LIB		= DiskQ
ALB		= lib$(LIB).a
SLB		= lib$(LIB).so

OBJS	= d_common.o d_msgget.o d_msgsnd.o d_msgrcv.o d_msgctl.o

all		: $(G_LIBTYP)

A_LIB	: $(OBJS)
	$(AR) srv $(ALB) $?
	cp -f $(ALB) $(LIBS)

S_LIB	: $(OBJS)
	${CC} -shared -o $(SLB) $?

install	:
	cp -f $(SLB) $(LIBS)

clean	:
	rm -f $(OBJS) $(ALB) $(SLB)

$(OBJS)	: $(INCS)/DiskQ.h
