TOP=..
RM=rm -rf
CFLAGS=$(COMMON_CFLAGS) -Dmain=submain -I $(TOP)/include -I $(TOP)/amiga/include

all: dummy.library

clean:
	$(RM) *.o *.a dummy.library

dummy.library: dummylib.c
	$(CC) $(CFLAGS) -I .. $^ -c -o $@
