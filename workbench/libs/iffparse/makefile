# $Id$
TOP=../../..

USER_INCLUDES = -I..

include $(TOP)/config/make.cfg

OBJDIR=$(GENDIR)/workbench/libs/iffparse
DEPLIBS=$(LIBDIR)/libamiga.a $(LIBDIR)/libarosc.a
LIBS=-L$(LIBDIR) -lamiga -larosc

FILES = iffparse_init iffparse_functable basicfuncs bufferingfuncs \
	clipboardfuncs collectionhooks prophooks

FUNCTIONS = \
	allociff \
	alloclocalitem \
	closeclipboard \
	closeiff \
	collectionchunk \
	collectionchunks \
	currentchunk \
	dosstreamhandler \
	entryhandler \
	exithandler \
	findcollection \
	findlocalitem \
	findprop \
	findpropcontext \
	freeiff \
	freelocalitem \
	goodid \
	goodtype \
	idtostr \
	iffparse_init \
	initiff \
	initiffasclip \
	initiffasdos \
	localitemdata \
	openclipboard \
	openiff \
	parentchunk \
	parseiff \
	popchunk \
	propchunk \
	propchunks \
	pushchunk \
	readchunkbytes \
	readchunkrecords \
	setlocalitempurge \
	stopchunk \
	stopchunks \
	stophook \
	stoponexit \
	storeitemincontext \
	storelocalitem \
	writechunkbytes \
	writechunkrecords

all: setup $(SLIBDIR)/iffparse.library

$(SLIBDIR)/iffparse.library: $(DEPLIBS) \
	    $(foreach f,$(FILES) $(FUNCTIONS),$(OBJDIR)/$(f).o)
	$(CC) $(ILDFLAGS) $^ $(LIBS) -o $@

setup :
	@if [ ! -d $(OBJDIR) ]; then $(MKDIR) $(OBJDIR) ; else true ; fi
	@if [ ! -d $(SLIBDIR) ]; then $(MKDIR) $(SLIBDIR) ; else true ; fi

clean:
	$(RM) $(OBJDIR) *.err

$(OBJDIR)/%.o: %.c
	$(CC) $(CFLAGS) $< -c -o $@ 2>&1|tee $*.err
	@if test ! -s $*.err; then rm $*.err ; else true ; fi

iffparse_functable.c : $(foreach f,$(FUNCTIONS),$(f).c) \
	    $(TOP)/scripts/makefunctable.awk
	gawk -f $(TOP)/scripts/makefunctable.awk \
	    --assign lib=IFFParse \
	    $^

$(OBJDIR)/%.d: %.c
	@if [ ! -d $(@D) ]; then $(MKDIR) $(@D) ; else true ; fi
	$(MKDEPEND) -f- -p$(@D)/ -- $(CFLAGS) -- $^ > $@

ifneq ($(TARGET),clean)
include $(foreach f,$(FILES) $(FUNCTIONS),$(OBJDIR)/$(f).d)
endif
