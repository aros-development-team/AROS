TOP=..

include $(TOP)/make.cfg

all: setup $(OSGENDIR)/console.o $(OSGENDIR)/cdinputhandler.o \
	$(OSGENDIR)/rawkeyconvert.o

setup:
	@if [ ! -d $(OSGENDIR) ]; then $(MKDIR) $(OSGENDIR) ; fi

clean:
	$(RM) $(OSGENDIR)

$(OSGENDIR)/%.o: %.c
	$(CC) $(CFLAGS) $^ -c -o $@ 2>&1|tee $*.err
	if test ! -s $*.err; then rm $*.err; fi

