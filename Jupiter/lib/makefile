include ../../comenv

CMINCS	= ../../Common/inc
JPINCS	= ../inc
JPLIBS	= ../../../lib
DEFS	= $(G_DEFS) -D_LIB_DEBUG

CFLAGS	= $(G_CFLAGS) $(DEFS) -I$(CMINCS) -I$(JPINCS)

LIB		= jplib
ALB		= lib$(LIB).a
SLB		= lib$(LIB).so

OBJS	= journal_file.o sequence_file.o external_call.o

all		: $(G_LIBTYP)

A_LIB	: $(OBJS)
	$(AR) srv $(ALB) $?

S_LIB	: $(OBJS)
	${CC} -shared -m64 -o $(SLB) $?

install	:
	cp -f $(SLB) $(JPLIBS)

clean	:
	rm -f $(OBJS) $(ALB) $(SLB)

$(OBJS)	: $(JPINCS)/jplib.h
