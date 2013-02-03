#/***************************************************************************
#
# NListview.mcc - New Listview MUI Custom Class
# Registered MUI class, Serial Number: 1d51 (0x9d510020 to 0x9d51002F)
#
# Copyright (C) 1996-2001 by Gilles Masson
# Copyright (C) 2001-2005 by NList Open Source Team
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# NList classes Support Site:  http://www.sf.net/projects/nlist-classes
#
# $Id$
#
#***************************************************************************/

###########################################################################
# This makefile is a very generic one. It tries to identify both, the host
# and the target operating system for which YAM should be compiled.
# However, this auto-detection can be easily overridden by directly
# specifying an option on the commandline while calling 'make'.
#
# Example:
#
# # to explicitly compile for AmigaOS3
# > make OS=os3
#
# # to compile for AmigaOS4 but with debugging
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
  else
    OS = os4
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
FLEX    = flex
FC      = flexcat
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

# path definitions
CDUP  = /
CDTHIS=

# override some variables for non-native builds (cross-compiler)
ifneq ($(HOST), AmigaOS)
ifneq ($(HOST), AmigaOS4)
ifneq ($(HOST), MorphOS)

  # when we end up here this is either a unix or Aros host
  # so lets use unix kind of commands
  RM    = rm -f
  RMDIR = rm -rf
  MKDIR = mkdir -p
  CHMOD = chmod 755
  CP    = cp -f

  CDUP  = ../
  CDTHIS= ./

endif
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

# Common Directories
PREFIX   = $(CDTHIS)
OBJDIR   = .obj_$(OS)
BINDIR   = bin_$(OS)
VPATH    = $(OBJDIR)
DEPFILE  = Makefile.dep
DESTDIR  = MUI:Libs/MUI

# target definition
TARGET    = $(BINDIR)/NListview.mcc
TESTTARGET= $(BINDIR)/NListview-Test

# Common compiler/linker flags
WARN     = -W -Wall -Wwrite-strings -Wpointer-arith -Wsign-compare
OPTFLAGS = -O3 -fomit-frame-pointer -funroll-loops
DEBUG    = -DDEBUG -O0
DEBUGSYM = -g -gstabs
CFLAGS   = -I. -I../nlistviews_mcp -I../include $(CPU) $(WARN) \
           $(OPTFLAGS) $(DEBUG) $(DEBUGSYM) $(USER_CFLAGS) -c
LDFLAGS  = $(CPU) $(DEBUGSYM)
LDLIBS   =

###########################################################################
# object files definition
#

LOBJS = library.o

COBJS = NListview.o \
        Debug.o

TOBJS = #NListview-Test.o

MCCOBJS  = $(addprefix $(OBJDIR)/,$(LOBJS)) $(addprefix $(OBJDIR)/,$(COBJS))
TESTOBJS = $(addprefix $(OBJDIR)/,$(COBJS)) $(addprefix $(OBJDIR)/,$(TOBJS))

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
  CPU      = -mcpu=powerpc
  WARN     += -Wdeclaration-after-statement -Wdisabled-optimization
  CFLAGS   += -mcrt=$(CRT) -D__USE_INLINE__ -D__NEW_TIMEVAL_DEFINITION_USED__ \
              -DNO_PPCINLINE_STDARG -Wa,-mregnames
  LDFLAGS  += -mcrt=$(CRT)

  # additional object files required
  M68KSTUBS = $(OBJDIR)/mccclass_68k.o

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
  CFLAGS  += -noixemul -DNO_INLINE_STDARG -D__amigaos3__
  LDFLAGS += -noixemul
  LDLIBS  += -ldebug -lmui

  # additional object files required
  COBJS += vastubs.o

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
  CFLAGS  += -noixemul -DNO_PPCINLINE_STDARG
  LDFLAGS += -noixemul
  LDLIBS  += -ldebug

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
  OPTFLAGS = -O2 -fomit-frame-pointer -funroll-loops
  CFLAGS += -Wno-pointer-sign
  LDLIBS += -larossupport -lrom -lmui

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
  OPTFLAGS = -O2 -fomit-frame-pointer -funroll-loops
  CFLAGS += -Wno-pointer-sign
  LDLIBS += -larossupport -lrom -lmui

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
  OPTFLAGS = -O2 -fomit-frame-pointer -funroll-loops
  CFLAGS += -Wno-pointer-sign
  LDLIBS += -larossupport -lrom -lmui

endif
endif
endif
endif
endif
endif

# main target
.PHONY: all
ifeq ($(wildcard $(DEPFILE)),$(DEPFILE))
# great, we have a dependecies file, let's make our target
all: $(BINDIR) $(OBJDIR) $(M68KSTUBS) $(TARGET) #$(TESTTARGET)
else
# no dependecies, create it and then call make again
all: depend
	@make --no-print-directory all
endif

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

# make the object directories
$(OBJDIR):
	@echo "  MK $@"
	@$(MKDIR) $(OBJDIR)

# make the binary directories
$(BINDIR):
	@echo "  MK $@"
	@$(MKDIR) $(BINDIR)

# for compiling single .c files
$(OBJDIR)/%.o: %.c
	@echo "  CC $<"
	@$(CC) $(CFLAGS) $< -o $@

$(OBJDIR)/mccclass_68k.o: ../include/mccclass_68k.c
	@echo "  CC $<"
	@$(CC) $(CFLAGS) $< -o $@

# for linking the target
$(TARGET): $(M68KSTUBS) $(MCCOBJS)
	@echo "  LD $@.debug"
	@$(CC) -nostartfiles $(LDFLAGS) -o $@.debug $(MCCOBJS) $(M68KSTUBS) $(LDLIBS) -Wl,--cref,-M,-Map=$@.map
	@echo "  LD $@"
	@$(STRIP) --preserve-dates -R.comment -R.sdata2 -S -o $@ $@.debug

# for linking the target
$(TESTTARGET): $(TESTOBJS)
	@echo "  LD $@.debug"
	@$(CC) $(LDFLAGS) -o $@.debug $(TESTOBJS) $(LDLIBS) -Wl,--cref,-M,-Map=$@.map
	@echo "  LD $@"
	@$(STRIP) --preserve-dates -R.comment -R.sdata2 -S -o $@ $@.debug

.PHONY: dump
dump:
	-$(OBJDUMP) --section-headers --all-headers --reloc --disassemble-all $(TARGET) >$(TARGET).dump

.PHONY: clean
clean:
	-$(RM) $(TARGET) $(TARGET).debug $(TARGET).map
	-$(RM) $(TESTTARGET) $(TESTTARGET).debug $(TESTTARGET).map
	-$(RM) $(MCCOBJS) $(TESTOBJS) $(M68KSTUBS)

.PHONY: distclean
distclean: clean
	-$(RMDIR) $(OBJDIR)
	-$(RMDIR) $(BINDIR)

# install
.PHONY: install
install: all
	@echo "  IN $(TARGET)"
	@$(CP) $(TARGET) $(DESTDIR)

.PHONY: help
help:
	@echo "Cleaning targets:"
	@echo "  clean     - Cleanup working directory for clean compile"
	@echo "  distclean - Also cleanup autogenerated files"
	@echo ""
	@echo "Other generic targets:"
	@echo "  all      - Build $(TARGET)"
	@echo "  catalogs - Build all available catalogs"
	@echo ""
	@echo "Install targets:"
	@echo "  install  - Install $(TARGET) into $(DESTDIR)"
	@echo ""
	@echo "Parameters:"
	@echo "  make OS=os3|os4|mos|aros-i386|aros-i686|aros-ppc|aros-x86_64"
	@echo "  make DEBUG= : build $(TARGET) without debugging information"
	@echo ""

## DEPENDENCY GENERATION ##############

.PHONY: depend
depend:
	@echo "  MK $(DEPFILE)"
	@echo "# AUTOGENERATED! DO NOT EDIT!!!" >$(DEPFILE)
	@$(CC) -MM -MG $(CFLAGS) $(wildcard *.c) >>$(DEPFILE)
	@echo "# AUTOGENERATED! DO NOT EDIT!!!" >>$(DEPFILE)
	@$(SED) -i 's,^\(.*\)\.o:,$$\(OBJDIR\)/\1.o:,g' $(DEPFILE)

# include dependencies file
-include $(DEPFILE)
