# $Id$
TOP=../..
COMPILE_KERNEL = yes # Must have a value, no matter which one

include $(TOP)/config/make.cfg

# BEGIN_DESC{makefile}
# This is the makefile for utility.library. Use it if you want to compile
# only this part of AROS.
# END_DESC{makefile}

# BEGIN_DESC{localmakevar}
# \item{FILES} This is a list of all files (without the .c) which
#	contain internal functions of the library (eg. libinit(),
#	libopen(), libexpunge() and the function table).
#
# \item{FUNCTIONS} This is a list of all files (without the .c)
#	with public functions of the library. The makefile will
#	strip all files from this list for which a special
#	version for this CPU does exist.
#
# END_DESC{localmakevar}
FILES = utility_init utility_functable getnamespace intfindnamedobj
FUNCTIONS := \
	applytagchanges callhookpkt findtagitem nexttagitem \
	stricmp strnicmp tolower toupper getuniqueid \
	date2amiga amiga2date allocatetagitems \
	freetagitems clonetagitems gettagdata packbooltags \
	refreshtagitemclones taginarray maptags \
	addnamedobject allocnamedobjecta attemptremnamedobject \
	findnamedobject namedobjectname releasenamedobject \
	remnamedobject freenamedobject \
	packstructuretags unpackstructuretags checkdate \
	filtertagitems filtertagchanges \
	smult32 umult32 smult64 umult64

# BEGIN_DESC{localmakevar}
# \item{ASM_PATH} The path to the assembler sources
#
# \item{ASM_FILES_TRY} Create a list with all files that might be
#	found as special CPU versions in $(ASM_PATH) with full
#	path.
#
# \item{ASM_FILES_EXIST} Create a list of all files from $(ASM_FILES_TRY)
#	which do really exist in $(ASM_PATH).
#
# END_DESC{localmakevar}

ASM_PATH := $(TOP)/config/$(KERNEL)

ASM_FILES_TRY := $(foreach f,$(FUNCTIONS),$(ASM_PATH)/$(f).s)
ASM_FILES_EXIST := $(wildcard $(ASM_FILES_TRY))

# Black magic: First create a negative list of the files (ie. only the
# ones which do *not* have a special assembler version for this CPU
# and then strip the path and extensions from all files in this list.
# Now the list contains the basenames of all files which must be taken
# from the local directory.
FUNCTIONS := $(filter-out $(ASM_FILES_EXIST),$(ASM_FILES_TRY))
FUNCTIONS := $(basename $(notdir $(FUNCTIONS)))

all: setup \
	$(foreach f,$(FILES) $(FUNCTIONS),$(OSGENDIR)/$(f).o)

setup :
	@if [ ! -d $(OSGENDIR) ]; then $(MKDIR) $(OSGENDIR) ; else true ; fi

clean:
	-$(RM) $(OSGENDIR) *.err $(LIBDIR)/libutility.a

$(OSGENDIR)/%.o: %.c
	$(CC) $(SHARED_CFLAGS) $(CFLAGS) $< -c -o $@ 2>&1|tee $*.err
	@if test ! -s $*.err; then rm $*.err ; else true ; fi

utility_functable.c: $(foreach f,$(FUNCTIONS),$(f).c) \
	    $(TOP)/scripts/makefunctable.awk
	gawk -f $(TOP)/scripts/makefunctable.awk \
	    --assign lib=Utility \
	    $^

$(OSGENDIR)/%.d: %.c
	@if [ ! -d $(@D) ]; then $(MKDIR) $(@D) ; else true ; fi
	$(MKDEPEND) -f- -p$(@D)/ -- $(CFLAGS) -- $^ > $@

ifneq ($(TARGET),clean)
include $(foreach f,$(FILES) $(FUNCTIONS),$(OSGENDIR)/$(f).d)
endif
