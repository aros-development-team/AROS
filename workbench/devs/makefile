TOP=..

include $(TOP)/make.cfg

OBJDIR = $(GENDIR)/exe

DEVS = $(DEVSDIR)/ram.handler \
	$(DEVSDIR)/nil.handler

all: setup $(OSGENDIR)/console.o $(OSGENDIR)/cdinputhandler.o \
	$(OSGENDIR)/rawkeyconvert.o $(DEVS)

setup:
	@if [ ! -d $(OSGENDIR) ]; then $(MKDIR) $(OSGENDIR) ; fi
	@if [ ! -d $(DEVSDIR) ]; then $(MKDIR) $(DEVSDIR) ; fi

clean:
	$(RM) $(OSGENDIR)
	$(RM) $(DEVSDIR)

$(OSGENDIR)/%.o: %.c
	$(CC) $(CFLAGS) $^ -c -o $@ 2>&1|tee $*.err
	if test ! -s $*.err; then rm $*.err; fi

$(OBJDIR)/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(DEVSDIR)/ram.handler: $(OBJDIR)/ram_handler.o
	$(CC) $(ILDFLAGS) $^ -o $@

$(DEVSDIR)/nil.handler: $(OBJDIR)/nil_handler.o
	$(CC) $(ILDFLAGS) $^ -o $@
