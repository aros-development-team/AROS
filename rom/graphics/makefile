# $Id$
TOP=..

SPECIAL_CFLAGS=-I/usr/include

include $(TOP)/config/make.cfg

FILES = graphics_init graphics_functable
FUNCTIONS = \
	clonerastport \
	closefont \
	createrastport \
	deinitrastport \
	draw \
	drawellipse \
	eraserect \
	freerastport \
	getapen \
	getbpen \
	getdrmd \
	getoutlinepen \
	initrastport \
	move \
	openfont \
	polydraw \
	readpixel \
	rectfill \
	scrollraster \
	setabpendrmd \
	setapen \
	setbpen \
	setdrmd \
	setfont \
	setoutlinepen \
	setrast \
	text \
	textlength \
	writepixel

all: setup \
	$(foreach f,$(FILES),$(OSGENDIR)/$(f).o) \
	$(foreach f,$(FUNCTIONS),$(OSGENDIR)/$(f).o)

setup :
	@if [ ! -d $(OSGENDIR) ]; then $(MKDIR) $(OSGENDIR) ; else true ; fi

clean:
	$(RM) $(OSGENDIR) *.err

$(OSGENDIR)/%.o: %.c
	$(CC) $(CFLAGS) $< -c -o $@ 2>&1|tee $*.err
	if test ! -s $*.err; then rm $*.err ; else true ; fi

graphics_functable.c : $(foreach f,$(FUNCTIONS),$(f).c) \
	    $(TOP)/scripts/makefunctable.awk
	gawk -f $(TOP)/scripts/makefunctable.awk \
	    --assign lib=Graphics \
	    $^

$(OSGENDIR)/%.d: %.c
	@if [ ! -d $(OSGENDIR) ]; then $(MKDIR) $(OSGENDIR) ; else true ; fi
	@$(RM) $@
	@touch $@
	$(MKDEPEND) -p$(OSGENDIR)/

include $(foreach f,$(FILES),$(OSGENDIR)/$(f).d)
include $(foreach f,$(FUNCTIONS),$(OSGENDIR)/$(f).d)

