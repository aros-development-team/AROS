TOP=..

include $(TOP)/make.cfg

all: setup $(OSGENDIR)/utility_init.o \
	    $(OSGENDIR)/stricmp.o $(OSGENDIR)/strnicmp.o $(OSGENDIR)/applytagchanges.o \
	    $(OSGENDIR)/findtagitem.o $(OSGENDIR)/tolower.o \
	    $(OSGENDIR)/utility_functable.o

setup :
	@if [ ! -d $(OSGENDIR) ]; then $(MKDIR) $(OSGENDIR) ; fi

clean:
	$(RM) $(OSGENDIR) *.err $(LIBDIR)/libutility.a

$(OSGENDIR)/%.o: %.c
	$(CC) $(CFLAGS) $^ -c -o $@ 2>&1|tee $*.err
	if test ! -s $*.err; then rm $*.err; fi

