TOP=..
RM=rm -rf
CFLAGS=$(COMMON_CFLAGS) -I $(TOP)/include -I $(TOP)/amiga/include

all: libkernel.a

clean:
	$(RM) *.o *.a

%.o: %.s
	$(CC) $(CFLAGS) $^ -c -o $@ 2>&1|tee $*.err
	if test ! -s $*.err; then rm $*.err; fi

%.o: %.c
	$(CC) $(CFLAGS) $^ -c -o $@ 2>&1|tee $*.err
	if test ! -s $*.err; then rm $*.err; fi

libkernel.a: init.o supervisor.o switch.o dispatch.o disable.o \
	     enable.o forbid.o permit.o preparecontext.o special.o
	ar -r $@ $^
