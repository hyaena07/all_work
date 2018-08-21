.SUFFIXES	: .c .o

CC		= gcc
COMINC	= -I../../Common/inc
COMLIB	= -L../../../lib -lComLib
BINDIR	= $(JP_HOME)/bin

JPINCS	= -I../inc
JPLIBS	= -L../../../lib -ljplib

DEFS	= -DDEBUG

CFLAGS	= -g $(DEFS) $(COMINC) $(JPINCS)
LFLAGS	= $(JPLIBS) $(COMLIB)

EXEC	= seq_init seq_test \
		  jnl_init jnl_write jnl_read \
		  jpec_test jpec_subp

all		: $(EXEC)

$(EXEC)	: $(EXEC:=.o)
	$(CC) -o $@ $@.o $(LFLAGS)

clean	:
	rm -rf $(EXEC) $(EXEC:=.o) core

install	:
	cp $(EXEC) $(BINDIR)

