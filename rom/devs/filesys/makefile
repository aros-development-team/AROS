TOP=..
RM=rm -rf
CFLAGS=$(COMMON_CFLAGS) -I $(TOP)/include -I $(TOP)/amiga/include

all: emul_handler.o nil_handler.o

clean:
	$(RM) *.o

%.o: %.s
	$(CC) $(CFLAGS) $^ -c -o $@ 2>&1|tee $*.err
	if test ! -s $*.err; then rm $*.err; fi

%.o: %.c
	$(CC) $(CFLAGS) $^ -c -o $@ 2>&1|tee $*.err
	if test ! -s $*.err; then rm $*.err; fi

