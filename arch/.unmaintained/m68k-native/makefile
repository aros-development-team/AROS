TOP=..

include $(TOP)/make.cfg

ODIR=$(OBJDIR)/m68k-native

all: setup $(LIBDIR)/libkernel.a

setup :
	@echo "Setting up"
	@if [ ! -d $(ODIR) ]; then $(MKDIR) $(ODIR) ; fi
	@$(TOUCH) setup

clean:
	$(RM) $(LIBDIR)/libkernel.a

$(ODIR)/%.o: %.s
	$(CC) $(CFLAGS) $^ -c -o $@ 2>&1|tee $*.err
	if test ! -s $*.err; then rm $*.err; fi

$(ODIR)/%.o: %.c
	$(CC) $(CFLAGS) $^ -c -o $@ 2>&1|tee $*.err
	if test ! -s $*.err; then rm $*.err; fi

libkernel.a: $(ODIR)/init.o $(ODIR)/supervisor.o $(ODIR)/switch.o \
	    $(ODIR)/dispatch.o $(ODIR)/disable.o $(ODIR)/enable.o \
	    $(ODIR)/forbid.o $(ODIR)/permit.o $(ODIR)/preparecontext.o \
	    $(ODIR)/special.o
	$(AR) $@ $^

