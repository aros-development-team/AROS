TOP=..
RM=rm -rf
CFLAGS=$(COMMON_CFLAGS) -noixemul -O2 -Wall -DSysBase=sysbase -I $(TOP)/include

all: libkernel.a

clean:
	$(RM) *.o *.a

%.o: %.s
	$(CC) $(CFLAGS) $^ -c -o $@

%.o: %.c
	$(CC) $(CFLAGS) $^ -c -o $@

libkernel.a: init.o supervisor.o switch.o dispatch.o disable.o \
	     enable.o forbid.o permit.o preparecontext.o special.o
	ar -r $@ $^

