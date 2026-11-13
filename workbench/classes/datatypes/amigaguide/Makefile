#
# $PROJECT: amigaguide.datatype
#
# $VER: Makefile 50.2 (07.06.03)
#
# $AUTHOR: Stefan Ruppert <stefan@ruppert-it.de>
#

BASENAME   = amigaguide
NAME       = $(BASENAME).datatype
BCLASS     = $(BASENAME).datatype
CREVFILE   = amigaguide_rev.h

DATATYPE   = $(BCLASS)

HEADER     = classbase.h classdata.h

CSRCS      = amigaguideclass.c nodeclass.c file.c util.c process.c navigator.c
SRCS       = classbase.c $(CSRCS)
OBJS       = classbase.o amigaguideclass.o nodeclass.o file.o util.o process.o navigator.o

AUTODOC    = amigaguide_dtc.doc

BCLASSDEST = Sys:Classes/Datatypes/

DIRS       =

##############################################################################
#
# GNU C definition
#

#DEFINES = -D__MORPHOS_DIRECTCALL -DUSE_INLINE_STDARG -DDEBUG
DEFINES = -D__MORPHOS_DIRECTCALL -DUSE_INLINE_STDARG
INCLUDES= -I./include -I../include -I../../include

#LDLIBS  = -labox -lmath -lc -lm -ldebug
LDLIBS  = -labox -lmath -lc -lm
LDFLAGS	= -fl libnix

#GCCOPTS     = -g -Wall -mmultiple -noixemul $(INCLUDES) $(DEFINES)
GCCOPTS     = -O2 -Wall -mmultiple -noixemul $(INCLUDES) $(DEFINES)
LDOPTS      = -noixemul $(LDFLAGS)
DEBUG       =

all: $(DIRS) $(DATATYPE)

%.o: %.c
	gcc $(GCCOPTS) $(DEBUG) $< -c -o $@

$(DATATYPE) : $(OBJS)
	$(LD) -L/gg/morphos/lib -o $@ $(LDFLAGS) $^ $(LDLIBS)
	protect $@ FLAGS=E SUB
#	copy $@ $(BCLASSDEST)
#	remlibs ${@F}

$(BASENAME): $(BASENAME).s
	sc $? link nostartup noicons
	delete $(BASENAME).lnk $(BASENAME).o
	ed $(BASENAME)

autodoc:
	makedoc FILES $(SRCS) BEGIN "/************ " END "******" AUTODOC $(AUTODOC) VERBOSE

##############################################################################
#
# explicit dependencies
#

classbase.o: $(CREVFILE)                # revision dependency

$(OBJS): $(HEADER)                      # header dependency


##############################################################################
#
# create directories
#

$(DIRS):
	mkdir $@

##############################################################################
#
# clean up
#

clean:
	-rm $(OBJS) $(DATATYPE)

install:
	strip -R .comment $(DATATYPE) -o MOSSYS:Classes/Datatypes/$(DATATYPE)
	copy $(BASENAME) MOSSYS:Devs/Datatypes/
	avail flush >nil:
	adddatatypes $(BASENAME)

archive: clean
	lha -r u RAM:AmigaGuideDT // $(BASENAME) MOSSYS:Classes/Datatypes/$(DATATYPE)
