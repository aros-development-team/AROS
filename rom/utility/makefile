TOP=..

include $(TOP)/make.cfg

ODIR=$(OBJDIR)/utility

all: setup $(LIBDIR)/libutility.a

setup :
	@echo "Setting up"
	@if [ ! -d $(ODIR) ]; then $(MKDIR) $(ODIR) ; fi
	@$(TOUCH) setup

clean:
	$(RM) $(ODIR) *.err $(LIBDIR)/libutility.a setup

$(ODIR)/%.o: %.s
	$(CC) $(CFLAGS) $^ -c -o $@ 2>&1|tee $*.err
	if test ! -s $*.err; then rm $*.err; fi

$(ODIR)/%.o: %.c
	$(CC) $(CFLAGS) $^ -c -o $@ 2>&1|tee $*.err
	if test ! -s $*.err; then rm $*.err; fi

$(LIBDIR)/libutility.a: $(ODIR)/utility_init.o \
	    $(ODIR)/stricmp.o $(ODIR)/strnicmp.o $(ODIR)/applytagchanges.o \
	    $(ODIR)/findtagitem.o $(ODIR)/tolower.o \
	    $(ODIR)/utility_functable.o
	$(AR) $@ $^
