TOP=..

include $(TOP)/make.cfg

OBJDIR=$(GENDIR)/filesys

all: setup $(OBJDIR)/emul_handler.o $(OBJDIR)/nil_handler.o

setup:
	@if [ ! -d $(OBJDIR) ]; then $(MKDIR) $(OBJDIR) ; fi

clean:
	$(RM) $(OBJDIR)

$(OBJDIR)/%.o: %.c
	$(CC) $(CFLAGS) $^ -c -o $@ 2>&1|tee $*.err
	if test ! -s $*.err; then rm $*.err; fi

