# $Id$
TOP=../..

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
	loadrgb32 \
	loadrgb4 \
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
	setrpattrsa \
	setwritemask \
	text \
	textlength \
	waittof \
	writepixel

all: setup \
	$(foreach f,$(FILES),$(OSGENDIR)/$(f).o) \
	$(foreach f,$(FUNCTIONS),$(OSGENDIR)/$(f).o)

setup :
	@if [ ! -d $(OSGENDIR) ]; then $(MKDIR) $(OSGENDIR) ; else true ; fi

clean:
	$(RM) $(OSGENDIR) *.err

$(OSGENDIR)/%.o: %.c
	$(CC) $(SHARED_CFLAGS) $(CFLAGS) $< -c -o $@ 2>&1|tee $*.err
	@if test ! -s $*.err; then rm $*.err ; else true ; fi

graphics_functable.c : $(foreach f,$(FUNCTIONS),$(f).c) \
	    $(TOP)/scripts/makefunctable.awk
	gawk -f $(TOP)/scripts/makefunctable.awk \
	    --assign lib=Graphics \
	    $^

$(OSGENDIR)/%.d: %.c
	@if [ ! -d $(@D) ]; then $(MKDIR) $(@D) ; else true ; fi
	$(MKDEPEND) -f- -p$(@D)/ -- $(CFLAGS) -- $^ > $@

ifneq ($(TARGET),clean)
include $(foreach f,$(FILES) $(FUNCTIONS),$(OSGENDIR)/$(f).d)
endif
