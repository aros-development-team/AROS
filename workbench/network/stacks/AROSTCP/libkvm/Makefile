CC= gcc -noixemul
AR= ar
AR_FLAGS= rs

OPTIMIZATION-OPTION = -O2
BASEREL-OPTION =
DEBUG-OPTION = -g
VERBOSITY = -Wall -Wno-unused -Wno-uninitialized
OBJECT-ONLY = -c

CFLAGS= $(OPTIMIZATION-OPTION) $(BASEREL-OPTION) $(DEBUG-OPTION)\
	 $(VERBOSITY) $(OBJECT-ONLY) 

INCLUDE_DIRS= -I ../include -I ../bsdsocket

SRCS = kvm.c kvm_open.c kvm_geterr.c kvm_read.c

LIB = libkvm.a

OBJS = kvm.o kvm_open.o kvm_geterr.o kvm_read.o

.SUFFIXES: .c .o  
#
# Generic rules
#
.c.o:
	$(CC) $(CFLAGS) $(OFLAGS) $(DEFS) $(INCLUDE_DIRS) $*.c

all: $(LIB)

#
# The library
#
$(LIB): $(OBJS)
	-$(RM) $@
	$(AR) $(AR_FLAGS) $@ $(OBJS)
clean:
	-$(RM) *.a *.o

install:

package:

