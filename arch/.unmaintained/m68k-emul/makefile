TOP=..

include $(TOP)/make.cfg

OBJDIR=$(GENDIR)/m68k-emul

all: setup $(LIBDIR)/libkernel.a

setup :
	@if [ ! -d $(OBJDIR) ]; then $(MKDIR) $(OBJDIR) ; fi

clean:
	$(RM) $(LIBDIR)/libkernel.a

$(OBJDIR)/%.o: %.s
	$(CC) $(CFLAGS) $< -c -o $@ 2>&1|tee $*.err
	if test ! -s $*.err; then rm $*.err; fi

$(OBJDIR)/%.o: %.c
	$(CC) $(CFLAGS) $< -c -o $@ 2>&1|tee $*.err
	if test ! -s $*.err; then rm $*.err; fi

libkernel.a: $(OBJDIR)/init.o $(OBJDIR)/supervisor.o $(OBJDIR)/switch.o \
	    $(OBJDIR)/dispatch.o $(OBJDIR)/disable.o $(OBJDIR)/enable.o \
	    $(OBJDIR)/forbid.o $(OBJDIR)/permit.o $(OBJDIR)/preparecontext.o \
	    $(OBJDIR)/special.o
	$(AR) $@ $^
