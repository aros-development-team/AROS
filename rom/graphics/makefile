# $Id$
TOP=../..

SPECIAL_CFLAGS=-I/usr/include

include $(TOP)/config/make.cfg

OBJDIR=$(OSGENDIR)/graphics
LIB=$(LIBDIR)/libgraphics.so

FILES = graphics_init graphics_functable intregions
FUNCTIONS = \
	addfont \
	allocbitmap \
	allocraster \
	andrectregion \
	askfont \
	bltbitmap \
	clearrectregion \
	clearregion \
	clonerastport \
	closefont \
	createrastport \
	deinitrastport \
	disposeregion \
	draw \
	drawellipse \
	eraserect \
	freebitmap \
	freeraster \
	freerastport \
	getapen \
	getbpen \
	getdrmd \
	getoutlinepen \
	initrastport \
	loadrgb32 \
	loadrgb4 \
	move \
	newregion \
	openfont \
	orrectregion \
	polydraw \
	readpixel \
	rectfill \
	remfont \
	scrollraster \
	setabpendrmd \
	setapen \
	setbpen \
	setdrmd \
	setfont \
	setoutlinepen \
	setrast \
	setrgb32 \
	setrpattrsa \
	setwritemask \
	text \
	textfit \
	textlength \
	waittof \
	writepixel \
	writepixelarray8 \
	xorrectregion

OBJS=$(foreach f,$(FILES) $(FUNCTIONS),$(OBJDIR)/$(f).o)

all: setup $(LIB)

$(LIB) : $(OBJS)
	@echo "Recreating $@..."
	@$(SHARED_AR) $@ $(OBJS)

setup :
	@if [ ! -d $(OBJDIR) ]; then $(MKDIR) $(OBJDIR) ; else true ; fi

clean:
	$(RM) $(OBJDIR) *.err

$(OBJDIR)/%.o: %.c
	$(CC) $(SHARED_CFLAGS) $(CFLAGS) $< -c -o $@ 2>&1|tee $*.err
	@if test ! -s $*.err; then rm $*.err ; else true ; fi

graphics_functable.c : $(foreach f,$(FUNCTIONS),$(f).c) \
	    $(TOP)/scripts/makefunctable.awk
	gawk -f $(TOP)/scripts/makefunctable.awk \
	    --assign lib=Graphics \
	    $^

$(OBJDIR)/%.d: %.c
	@if [ ! -d $(@D) ]; then $(MKDIR) $(@D) ; else true ; fi
	$(MKDEPEND) -f- -p$(@D)/ -- $(CFLAGS) -- $^ > $@

ifneq ($(TARGET),clean)
include $(foreach f,$(FILES) $(FUNCTIONS),$(OBJDIR)/$(f).d)
endif
