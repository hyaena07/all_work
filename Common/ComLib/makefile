include ../../comenv

INCS	= ../inc
LIBS	= ../../../lib
DEFS	= $(G_DEFS)

CFLAGS	= $(G_CFLAGS) $(DEFS) -I$(INCS)

LIB		= ComLib
ALB		= lib$(LIB).a
SLB		= lib$(LIB).so

OBJS	= DateTime.o File.o IPC.o Log.o Network.o Process.o String.o System.o

all		: $(G_LIBTYP)

A_LIB	: $(OBJS)
	$(AR) rc $(ALB) $?

S_LIB	: $(OBJS)
	${CC} -shared -m64 -o $(SLB) $?

install	:
	cp -f $(SLB) $(LIBS)

clean	:
	rm -f $(OBJS) $(ALB) $(SLB)

$(OBJS)	: $(INCS)/ComLib.h
