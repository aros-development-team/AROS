# $Id$
TOP=../..

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

# ***** Native ***************
ifeq ($(FLAVOUR),native)

# Main function for exec.strap. Need this apart as an anchor for the link:
INIT_FILE = utility_init

FILES = utility_functable getnamespace intfindnamedobj

# Guarantee that this will go at the end of the file:
END_FILE = utility_endtag

# ***** Not Native ***************
else

FILES = utility_init utility_functable getnamespace intfindnamedobj

# ***** END ***************
endif

FUNCTIONS := \
	applytagchanges callhookpkt findtagitem nexttagitem stricmp strnicmp\
	tolower toupper getuniqueid date2amiga amiga2date allocatetagitems  \
	freetagitems clonetagitems gettagdata packbooltags taginarray       \
	refreshtagitemclones maptags addnamedobject allocnamedobjecta       \
	attemptremnamedobject findnamedobject namedobjectname               \
	releasenamedobject remnamedobject freenamedobject packstructuretags \
	unpackstructuretags checkdate filtertagitems filtertagchanges       \
	smult32 umult32 smult64 umult64 sdivmod32 udivmod32

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

ifeq ($(FLAVOUR),native)
ASM_PATH := $(TOP)/config/$(KERNEL)/utility
else
ASM_PATH := $(TOP)/config/$(KERNEL)
endif
ARCH_PATH := $(TOP)/config/$(ARCH)

ASM_FILES_TRY := $(foreach f,$(FUNCTIONS),$(ASM_PATH)/$(f).[cs] $(ARCH_PATH)/$(f).[cs])
ASM_FILES_EXIST := $(wildcard $(ASM_FILES_TRY))
ASM_FILES_EXIST := $(basename $(notdir $(ASM_FILES_EXIST)))

# Black magic: First create a negative list of the files (ie. only the
# ones which do *not* have a special assembler version for this CPU
# and then strip the path and extensions from all files in this list.
# Now the list contains the basenames of all files which must be taken
# from the local directory.
FUNCTIONS := $(filter-out $(ASM_FILES_EXIST),$(FUNCTIONS))

OBJDIR=$(OSGENDIR)/utility

# ***** Native ***************
ifeq ($(FLAVOUR),native)

LIBS = -L$(LIBDIR) -lutility -larosc
LIB=$(LIBDIR)/libutility.a
LIBLIST = utility arosc
LIBDEPS = $(foreach f,$(LIBLIST),$(LIBDIR)/lib$(f).a)
OBJS = $(foreach f,$(FILES) $(FUNCTIONS) $(ASM_FILES_EXIST),$(OBJDIR)/$(f).o)
INITFUNC = $(OBJDIR)/$(INIT_FILE).o
ENDFUNC = $(OBJDIR)/$(END_FILE).o

all: setup $(OSMODDIR)/utility.library

.PHONY: asm
asm: $(foreach f,$(FUNCTIONS) $(INIT_FILE) $(END_FILE),$(f).s) all

$(OSMODDIR)/utility.library: $(INITFUNC) $(ENDFUNC) $(LIBDEPS)
	@echo "Building $(notdir $@) ..."
	@$(CC) $(ILDFLAGS) -Xlinker -M $(INITFUNC) $(LIBS) $(ENDFUNC) -o $@ 2>&1|tee utilitylib.map
	@if test ! -s utilitylib.map; then rm utilitylib.map ; else true ; fi
	@strip $@

%.s: %.c
	@echo "Generating $@ ..."
	@$(CC) -S $(CFLAGS) $< -c -o $@

# Collect all (asm-)functions in a linklib for fast linking:
$(LIB): $(OBJS)
	@echo "Adding functions to $(notdir $@) ..."
	@$(AR) $@ $?
	@$(RANLIB) $@

# ***** Not Native ***************
else

LIB=$(LIBDIR)/libutility.so

OBJS = $(foreach f,$(FILES) $(FUNCTIONS),$(OBJDIR)/$(f).o)

all: setup $(LIB)

$(LIB) : $(OBJS)
	@echo "Recreating $@..."
	@$(SHARED_AR) $@ $(OBJS)

# ***** END ***************
endif

setup :
	@if [ ! -d $(OSGENDIR) ]; then $(MKDIR) $(OSGENDIR) ; else true ; fi
	@if [ ! -d $(OBJDIR) ]; then $(MKDIR) $(OBJDIR) ; else true ; fi
	@echo "CFLAGS=$(CFLAGS)"

clean:
	-$(RM) $(OBJDIR) *.err $(LIB)

$(OBJDIR)/%.o: %.c
	@echo "Compiling $(CURDIR)/$<"
	@$(CC) $(SHARED_CFLAGS) $(CFLAGS) $< -c -o $@ 2>&1|tee $*.err
	@if test ! -s $*.err; then rm $*.err ; else true ; fi

utility_functable.c: $(foreach f,$(FUNCTIONS),$(f).c) \
	    $(TOP)/scripts/makefunctable.awk
	gawk -f $(TOP)/scripts/makefunctable.awk \
	    --assign lib=Utility \
	    $^

$(OBJDIR)/utility_init.o: libdefs.h

libdefs.h: lib.conf $(TOP)/scripts/genlibdefs.gawk
	gawk -f $(TOP)/scripts/genlibdefs.gawk lib.conf > libdefs.h

$(OBJDIR)/%.d: %.c
	@if [ ! -d $(@D) ]; then $(MKDIR) $(@D) ; else true ; fi
	@echo "Makedepend $(CURDIR)/$<"
	@$(MKDEPEND) -f- -p$(@D)/ -- $(CFLAGS) -- $^ > $@

ifneq ($(TARGET),clean)
include $(foreach f,$(INIT_FILE) $(FILES) $(FUNCTIONS) $(END_FILE),$(OBJDIR)/$(f).d)
endif
