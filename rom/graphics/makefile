TOP=..

include $(TOP)/make.cfg

FILES = closefont draw move openfont rectfill scrollraster setapen setbpen \
	setdrmd setfont setrast text writepixel

all: setup $(OSGENDIR)/graphics_init.o $(OSGENDIR)/graphics_functable.o \
	$(OSGENDIR)/driver_x11.o \
	$(foreach f,$(FILES),$(OSGENDIR)/$(f).o)

setup :
	@if [ ! -d $(OSGENDIR) ]; then $(MKDIR) $(OSGENDIR) ; fi

clean:
	$(RM) $(OSGENDIR) *.err

$(OSGENDIR)/%.o: %.c
	$(CC) $(CFLAGS) $^ -c -o $@ 2>&1|tee $*.err
	if test ! -s $*.err; then rm $*.err; fi

graphics_functable.c : $(foreach f,$(FILES),$(f).c) \
	    $(TOP)/scripts/makefunctable.awk
	gawk -f $(TOP)/scripts/makefunctable.awk \
	    --assign lib=Graphics \
	    $(foreach f,$(FILES),$(f).c)
