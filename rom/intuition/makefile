TOP=..

include $(TOP)/make.cfg

FILES = closewindow modifyidcmp openwindow

all: setup $(OSGENDIR)/intuition_init.o $(OSGENDIR)/intuition_functable.o \
	$(OSGENDIR)/intui_x11.o \
	$(foreach f,$(FILES),$(OSGENDIR)/$(f).o)

setup :
	@if [ ! -d $(OSGENDIR) ]; then $(MKDIR) $(OSGENDIR) ; fi

clean:
	$(RM) $(OSGENDIR) *.err

$(OSGENDIR)/%.o: %.c
	$(CC) $(CFLAGS) $^ -c -o $@ 2>&1|tee $*.err
	if test ! -s $*.err; then rm $*.err; fi

intuition_functable.c : $(foreach f,$(FILES),$(f).c) \
	    $(TOP)/scripts/makefunctable.awk
	gawk -f $(TOP)/scripts/makefunctable.awk \
	    --assign lib=Intuition \
	    $(foreach f,$(FILES),$(f).c)
