TOP=..

include $(TOP)/make.cfg

ODIR=$(OBJDIR)/filesys

all: setup $(ODIR)/emul_handler.o $(ODIR)/nil_handler.o

setup:
	@echo "Setting up"
	@if [ ! -d $(ODIR) ]; then $(MKDIR) $(ODIR) ; fi
	@$(TOUCH) setup

clean:
	$(RM) $(ODIR) setup

$(ODIR)/%.o: %.s
	$(CC) $(CFLAGS) $^ -c -o $@ 2>&1|tee $*.err
	if test ! -s $*.err; then rm $*.err; fi

$(ODIR)/%.o: %.c
	$(CC) $(CFLAGS) $^ -c -o $@ 2>&1|tee $*.err
	if test ! -s $*.err; then rm $*.err; fi

