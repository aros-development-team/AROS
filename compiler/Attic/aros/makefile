TOP=..

include $(TOP)/make.cfg

OBJDIR=$(GENDIR)/aros

all: setup $(LIBDIR)/libaros.a

setup :
	@if [ ! -d $(OBJDIR) ]; then $(MKDIR) $(OBJDIR) ; fi

clean:
	$(RM) $(OBJDIR) *.err $(LIBDIR)/libaros.a

$(OBJDIR)/%.o: %.c
	$(SYS_CC) $(CFLAGS) $^ -c -o $@ 2>&1|tee $*.err
	if test ! -s $*.err; then rm $*.err; fi

$(OBJDIR)/rt_stubs.o: rt_stubs.s
	$(SYS_CC) $(CFLAGS) $^ -c -o $@ 2>&1|tee $*.err
	if test ! -s $*.err; then rm $*.err; fi

$(LIBDIR)/libaros.a: $(OBJDIR)/calcchecksum.o $(OBJDIR)/strcmp.o \
	    $(OBJDIR)/strlen.o $(OBJDIR)/strcpy.o $(OBJDIR)/qsort.o \
	    $(OBJDIR)/kprintf.o $(OBJDIR)/nastyfreemem.o \
	    $(OBJDIR)/rt.o $(OBJDIR)/rt_stubs.o $(OBJDIR)/purify.o \
	    $(OBJDIR)/stricmp.o
	$(AR) $@ $?
	$(RANLIB) $@

