NAME = PrefInterp
OBJS = PrefInterp.o /parse.o

CC = SC

all: $(OBJS)
	$(CC) LINK $(OBJS) PNAME=$(NAME)
	Delete $(NAME).lnk

.c.o:
	$(CC) $*.c

clean:
	Delete FORCE QUIET $(OBJS) $(NAME)

bumprev:
	BumpRev 38 $(NAME)

revision: bumprev
	Delete FORCE QUIET $(NAME)_rev.i
