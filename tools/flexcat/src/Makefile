#/***************************************************************************
#
# $Id$
#
# Copyright (C) 1993-1999 by Jochen Wiedmann and Marcin Orlowski
# Copyright (C) 2002-2010 by the FlexCat Open Source Team
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or (at
# your option) any later version.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#
#***************************************************************************/

###########################################################################
# This makefile is a very generic one. It tries to identify both, the host
# and the target operating system for which FlexCat should be compiled.
# However, this auto-detection can be easily overridden by directly
# specifying an option on the commandline while calling 'make'.
#
# Example:
#
# # to explicitly compile for AmigaOS3
# > make OS=os3
#
# # to compile for AmigaOS4 but without debugging
# > make OS=os4 DEBUG=
#

#############################################
# find out the HOST operating system
# on which this makefile is run
HOST ?= $(shell uname)
ifeq ($(HOST), AmigaOS)
  ifeq ($(shell uname -m), powerpc)
    HOST = AmigaOS4
  endif
  ifeq ($(shell uname -m), ppc)
    HOST = AmigaOS4
  endif
endif

# if no host is identifed (no uname tool)
# we assume a AmigaOS build
ifeq ($(HOST),)
  HOST = AmigaOS
endif

#############################################
# now we find out the target OS for
# which we are going to compile YAM in case
# the caller didn't yet define OS himself
ifndef (OS)
  ifeq ($(HOST), AmigaOS4)
    OS = os4
  else
  ifeq ($(HOST), AmigaOS)
    OS = os3
  else
  ifeq ($(HOST), MorphOS)
    OS = mos
  else
  ifeq ($(HOST), Linux)
    OS = unix
  else
  ifeq ($(HOST), Windows)
    OS = mingw32
  else
  ifeq ($(HOST), AROS)
    # now we find out which CPU system aros will be used
    ifeq ($(shell uname -m), powerpc)
      OS = aros-ppc
    endif
    ifeq ($(shell uname -m), ppc)
      OS = aros-ppc
    endif
    ifeq ($(shell uname -m), i386)
      OS = aros-i386
    endif
    ifeq ($(shell uname -m), i686)
      OS = aros-i686
    endif
    ifeq ($(shell uname -m), x86_64)
      OS = aros-x86_64
    endif
    ifeq ($(shell uname -m), arm)
      OS = aros-arm
    endif
  else
    OS = os4
  endif
  endif
  endif
  endif
  endif
  endif
endif

#############################################
# define common commands we use in this
# makefile. Please note that each of them
# might be overridden on the commandline.

# common commands
FLEXCAT = flexcat
EXPR    = expr
DATE    = date
RM      = rm -f #delete force
RMDIR   = rm -rf #delete force all
MKDIR   = mkdir -p #makedir force
CHMOD   = protect FLAGS=rwed
SED     = sed
CP      = copy
CC      = gcc
STRIP   = strip
OBJDUMP = objdump
MAKEINFO= makeinfo

# path definitions
CDUP  = /
CDTHIS=

# override some variables for non-native builds (cross-compiler)
ifneq ($(HOST), AmigaOS)
ifneq ($(HOST), AmigaOS4)
ifneq ($(HOST), MorphOS)

  # when we end up here this is either a unix or Aros host
  # so lets use unix kind of commands
  RM      = rm -f
  RMDIR   = rm -rf
  MKDIR   = mkdir -p
  CHMOD   = chmod 755
  CP      = cp -f

  CDUP  = ../
  CDTHIS= ./

endif
endif

###########################################################################
# CPU and DEBUG can be defined outside, defaults to above
# using e.g. "make DEBUG= CPU=-mcpu=603e" produces optimized non-debug
# PPC-603e version
#
# OPTFLAGS are disabled by DEBUG normally!
#
# ignored warnings are:
# none - because we want to compile with -Wall all the time

# target definition
TARGET = $(BINDIR)/flexcat$(EXE)

# Common Directories
PREFIX    = $(CDTHIS)
LOCALE    = $(PREFIX)$(CDUP)locale
TOOLS     = $(PREFIX)tools
LIB       = $(PREFIX)lib/$(OS)
OBJDIR    = .obj_$(OS)
DEPDIR    = .dep_$(OS)
BINDIR    = bin_$(OS)
VPATH     = $(OBJDIR)
GCCVER    = 4

# Common compiler/linker flags
WARN     = -W -Wall -Wwrite-strings -Wpointer-arith -Wsign-compare #-Wunreachable-code
OPTFLAGS = -O3 -fomit-frame-pointer -fno-strict-aliasing
DEBUG    = -DDEBUG -fno-omit-frame-pointer #-O0
DEBUGSYM = -g -gstabs
CFLAGS   = -I. -I./include $(CPU) $(WARN) $(OPTFLAGS) $(DEBUG) $(DEBUGSYM)
LDFLAGS  = $(CPU) $(DEBUGSYM)
LDLIBS   = -L$(LIB)

# different options per target OS
ifeq ($(OS), os4)

  ##############################
  # AmigaOS4

  # Compiler/link/strip commands
  ifneq ($(HOST), AmigaOS4)
    CC      = ppc-amigaos-gcc
    STRIP   = ppc-amigaos-strip
    OBJDUMP = ppc-amigaos-objdump
  endif

  # Compiler/Linker flags
  CRT      = newlib
  CPU      = -mcpu=powerpc -mstrict-align
  WARN     += -Wdeclaration-after-statement -Wdisabled-optimization -Wshadow
  REDEFINE = -DCoerceMethod=ICoerceMethod -DDoMethod=IDoMethod \
             -DDoSuperMethod=IDoSuperMethod -DDoSuperMethodA=IDoSuperMethodA
  CFLAGS   += -mcrt=$(CRT) -D__USE_INLINE__ -D__NEW_TIMEVAL_DEFINITION_USED__ \
              $(REDEFINE) -Wa,-mregnames -DAMIGA
  LDFLAGS  += -mcrt=$(CRT)
  LDLIBS   += -lauto -lunix
  CDUP     = ../
  CDTHIS   = ./

  SYSDEPOBJS = locale.o \
               getft.o \
               openlibs.o

else
ifeq ($(OS), os3)

  ##############################
  # AmigaOS3

  # Compiler/link/strip commands
  ifneq ($(HOST), AmigaOS)
    CC      = m68k-amigaos-gcc
    STRIP   = m68k-amigaos-strip
    OBJDUMP = m68k-amigaos-objdump
  endif

  # Compiler/Linker flags
  CPU     = -m68020-60 -msoft-float
  CFLAGS  += -I./include/netinclude -DNO_INLINE_STDARG -D__amigaos3__ -DAMIGA
  LDFLAGS += -noixemul
  LDLIBS  += -lamiga
  GCCVER  = 2

  SYSDEPOBJS = locale.o \
               getft.o \
               openlibs.o \
               strptime.o \
               vastubs.o

else
ifeq ($(OS), mos)

  ##############################
  # MorphOS

  # Compiler/link/strip commands
  ifneq ($(HOST), MorphOS)
    CC      = ppc-morphos-gcc
    STRIP   = ppc-morphos-strip
    OBJDUMP = ppc-morphos-objdump
  endif

  # Compiler/Linker flags
  CPU     = -mcpu=powerpc
  CFLAGS  += -noixemul -I./include/netinclude -DAMIGA
  LDFLAGS += -noixemul
  LDLIBS  += -lauto -lcodesets
  GCCVER  = 2

  SYSDEPOBJS = locale.o \
               asprintf.o \
               vasprintf.o \
               openlibs.o \
               strptime.o \

else
ifeq ($(OS), aros-i386)

  ##############################
  # AROS (i386)

  ifneq ($(HOST), AROS)
    CC      = i386-aros-gcc
    STRIP   = i386-aros-strip
    OBJDUMP = i386-aros-objdump
  endif

  # Compiler/Linker flags
  OPTFLAGS = -O2 -fomit-frame-pointer
  CFLAGS += -Wno-pointer-sign -DNO_INLINE_STDARG -D__BSD_VISIBLE=1 -DAMIGA
  LDLIBS += -lamiga -larossupport -larosc

  SYSDEPOBJS = locale.o \
               getft.o \
               asprintf.o \
               vasprintf.o \
               vastubs.o \
               openlibs.o

else
ifeq ($(OS), aros-ppc)

  ##############################
  # AROS (PPC)

  ifneq ($(HOST), AROS)
    CC      = ppc-aros-gcc
    STRIP   = ppc-aros-strip
    OBJDUMP = ppc-aros-objdump
  endif

  # Compiler/Linker flags
  OPTFLAGS = -O2 -fomit-frame-pointer
  CFLAGS += -Wno-pointer-sign -DNO_INLINE_STDARG -D__BSD_VISIBLE=1 -DAMIGA
  LDLIBS += -lamiga -larossupport -larosc

  SYSDEPOBJS = locale.o \
               getft.o \
               asprintf.o \
               vasprintf.o \
               vastubs.o \
               openlibs.o

else
ifeq ($(OS), aros-x86_64)

  ##############################
  # AROS (x86_64)

  ifneq ($(HOST), AROS)
    CC      = x86_64-aros-gcc
    STRIP   = x86_64-aros-strip
    OBJDUMP = x86_64-aros-objdump
  endif

  # Compiler/Linker flags
  OPTFLAGS = -O2 -fomit-frame-pointer
  CFLAGS += -Wno-pointer-sign -DNO_INLINE_STDARG -D__BSD_VISIBLE=1 -DAMIGA
  LDLIBS += -lamiga -larossupport -larosc

  SYSDEPOBJS = locale.o \
               getft.o \
               asprintf.o \
               vasprintf.o \
               vastubs.o \
               openlibs.o

else
ifeq ($(OS), aros-arm)

  ##############################
  # AROS (arm)

  ifneq ($(HOST), AROS)
    CC      = arm-aros-gcc
    STRIP   = arm-aros-strip
    OBJDUMP = arm-aros-objdump
  endif

  # Compiler/Linker flags
  OPTFLAGS = -O2 -fomit-frame-pointer
  CFLAGS += -Wno-pointer-sign -DNO_INLINE_STDARG -D__BSD_VISIBLE=1 -DAMIGA
  LDLIBS += -lamiga -larossupport -larosc

  SYSDEPOBJS = locale.o \
               getft.o \
               asprintf.o \
               vasprintf.o \
               vastubs.o \
               openlibs.o

else
ifeq ($(OS), mingw32)

  ##############################
  # Windows (mingw32)

  ifneq ($(HOST), Windows)
    CC      = i586-mingw32msvc-gcc
    STRIP   = i586-mingw32msvc-strip
    OBJDUMP = i586-mingw32msvc-objdump
  endif

  # Compiler/Linker flags
  EXE = .exe
  OPTFLAGS = -O2 -fomit-frame-pointer
  LDFLAGS =

  SYSDEPOBJS = locale_other.o \
               asprintf.o \
               vasprintf.o \
               strptime.o

else
ifeq ($(OS), unix)

  ##############################
  # Linux (unix)

  ifneq ($(HOST), Linux)
    CC      = x86_64-linux-gnu-gcc
    STRIP   = x86_64-linux-gnu-strip
    OBJDUMP = x86_64-linux-gnu-objdump
  endif

  # Compiler/Linker flags
  OPTFLAGS = -O2 -fomit-frame-pointer
  CFLAGS += -m32 -D_GNU_SOURCE
  LDFLAGS = -m32

  SYSDEPOBJS = locale_other.o

endif
endif
endif
endif
endif
endif
endif
endif
endif
endif

###########################################################################
# Here starts all stuff that is common for all target platforms and
# hosts.

CATFILES = FlexCat_cat.h \
           FlexCat_cat_other.h \
           locale.c \
           locale_other.c

OBJS = main.o \
       createcat.o \
       createcatsrc.o \
       createct.o \
       globals.o \
       readprefs.o \
       scancd.o \
       scanct.o \
       scanpo.o \
       showfuncs.o \
       swapfuncs.o \
       utils.o \
       $(SYSDEPOBJS)

CATALOGS = $(patsubst %.po,%.catalog,$(wildcard locale/*.po))

.PHONY: all
all: $(OBJDIR) $(BINDIR) $(DEPDIR) $(TARGET)

# for making a release we compile ALL target with no debug
release:
	@echo "  CC $<"
	make OS=os4 clean
	make OS=os4 DEBUG=
	@echo "  CC $<"
	make OS=os3 clean
	make OS=os3 DEBUG=
	@echo "  CC $<"
	make OS=mos clean
	make OS=mos DEBUG=
	@echo "  CC $<"
	make OS=aros-i386 clean
	make OS=aros-i386 DEBUG=
	@echo "  CC $<"
	make OS=aros-ppc clean
	make OS=aros-ppc DEBUG=
	@echo "  CC $<"
	make OS=aros-x86_64 clean
	make OS=aros-x86_64 DEBUG=
	@echo "  CC $<"
	make OS=unix clean
	make OS=unix DEBUG=
	@echo "  CC $<"
	make OS=mingw32 clean
	make OS=mingw32 DEBUG=

# make the object directories
$(OBJDIR):
	@echo "  MK $@"
	@$(MKDIR) $(OBJDIR)

# make the binary directory
$(BINDIR):
	@echo "  MK $@"
	@$(MKDIR) $(BINDIR)

# make the dependency directory
$(DEPDIR):
	@echo "  MK $@"
	@$(MKDIR) $(DEPDIR)

# for compiling single .c files
$(OBJDIR)/%.o: %.c
	@echo "  CC $<"
ifeq ($(GCCVER), 2)
	@$(CC) -MM -MG -o $(DEPDIR)/$(subst .c,.d,$<) $(CFLAGS) $<
	@$(SED) -i 's,^.*\.o:,$@:,g' $(DEPDIR)/$(subst .c,.d,$<)
else
	@$(CC) -MM -MP -MT '$@' -MF $(DEPDIR)/$(subst .c,.d,$<) $(CFLAGS) $<
endif
	@$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(CATFILES) $(addprefix $(OBJDIR)/,$(OBJS))
	@echo "  LD $@.debug"
	@$(CC) $(LDFLAGS) -o $@.debug $(addprefix $(OBJDIR)/,$(OBJS)) $(LDLIBS) -Wl,--cref,-M,-Map=$@.map
	@echo "  LD $@"
	@$(STRIP) --preserve-dates -R.comment -R.sdata2 -S -o $@ $@.debug
	@$(CHMOD) $@

locale.c: locale/FlexCat.pot sd/NoAutoC_c.sd
	@echo "  FC $@"
	@$(FLEXCAT) locale/FlexCat.pot locale.c=sd/NoAutoC_c.sd

FlexCat_cat.h: locale/FlexCat.pot sd/NoAutoC_h.sd
	@echo "  FC $@"
	@$(FLEXCAT) locale/FlexCat.pot FlexCat_cat.h=sd/NoAutoC_h.sd

locale_other.c: locale/FlexCat.pot sd/Hardcode_c.sd
	@echo "  FC $@"
	@$(FLEXCAT) locale/FlexCat.pot locale_other.c=sd/Hardcode_c.sd

FlexCat_cat_other.h: locale/FlexCat.pot sd/Hardcode_h.sd
	@echo "  FC $@"
	@$(FLEXCAT) locale/FlexCat.pot FlexCat_cat_other.h=sd/Hardcode_h.sd

locale/%.catalog: locale/%.po
	@echo "  MK $@"
	@$(FLEXCAT) POFILE $< CATALOG $@

.IGNORE: $(CATALOGS)

catalogs: $(CATALOGS)

.PHONY: clean
clean:
	-$(RM) $(addprefix $(OBJDIR)/,$(OBJS))
	-$(RM) $(BINDIR)/$(FLEXCAT)$(EXE)
	-$(RM) ../doc/*.guide

.PHONY: cleanall
cleanall: clean
	-$(RM) FlexCat_cat.h
	-$(RM) FlexCat_cat_other.h
	-$(RM) locale.c
	-$(RM) locale_other.c
	-$(RM) locale/*.catalog
	-$(RMDIR) $(OBJDIR)

.PHONY: doc
doc:
	$(MAKEINFO) ../doc/FlexCat_german.texinfo --amiga-39 --fill-column 76 --output ../doc/FlexCat_deutsch.guide
	$(MAKEINFO) ../doc/FlexCat_english.texinfo --amiga-39 --fill-column 76 --output ../doc/FlexCat_english.guide
	$(MAKEINFO) ../doc/FlexCat_spanish.texinfo --amiga-39 --fill-column 76 --output ../doc/FlexCat_español.guide
	$(MAKEINFO) ../doc/FlexCat_swedish.texinfo --amiga-39 --fill-column 76 --output ../doc/FlexCat_svenska.guide

## DEPENDENCY INCLUDE #################

-include $(OBJS:%.o=$(DEPDIR)/%.d)
