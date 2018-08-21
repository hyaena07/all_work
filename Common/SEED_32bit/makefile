include ../../comenv

CC		= gcc
INC		= ../inc
LIBS	= ../../../lib
DEFS	= $(G_DEFS)

CFLAGS	= $(G_CFLAGS) $(DEFS) -I$(INC)

LIB		= SEED_KISA
ALB		= lib$(LIB).a
SLB		= lib$(LIB).so

OBJS	= SEED_KISA.o

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

$(OBJS)	: $(INC)/SEED_KISA.h
