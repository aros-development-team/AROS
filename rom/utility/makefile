TOP=..
RM=rm -rf
CFLAGS=$(COMMON_CFLAGS) -I $(TOP)/include -I $(TOP)/amiga/include

all: libutility.a

clean:
	$(RM) *.o *.a

%.o: %.s
	$(CC) $(CFLAGS) $^ -c -o $@ 2>&1|tee $*.err
	if test ! -s $*.err; then rm $*.err; fi

%.o: %.c
	$(CC) $(CFLAGS) $^ -c -o $@ 2>&1|tee $*.err
	if test ! -s $*.err; then rm $*.err; fi

libutility.a: utility_init.o \
	  stricmp.o strnicmp.o applytagchanges.o findtagitem.o tolower.o \
	  utility_functable.o
	ar -r $@ $^
