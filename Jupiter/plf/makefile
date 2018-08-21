include ../../comenv

COMINC	= -I../../Common/inc
COMLIB	= -L../../../lib -lComLib
BINDIR	= ../../../bin

JPINCS	= -I../inc
JPLIBS	= -L../../../lib -ljplib

DEFS	= $(G_DEFS)

CFLAGS	= $(G_CFLAGS) $(DEFS) $(COMINC) $(JPINCS)
LFLAGS	= $(COMLIB) $(JPLIBS) $(G_LFLAGS)

EXEC	= jpcomd
OBJS	= jpcomd.o jpsysm.o jpdaly.o jpweek.o jpmnth.o jpyear.o

all		: $(EXEC)

$(EXEC)	: $(OBJS)
	$(CC) -o $(EXEC) $(OBJS) $(LFLAGS)

clean	:
	rm -rf $(EXEC) $(OBJS) core

install	: $(EXEC)
	cp -f $(EXEC) $(BINDIR)

