#/***************************************************************************
#
# codesets.library - Amiga shared library for handling different codesets
# Copyright (C) 2001-2005 by Alfonso [alfie] Ranieri <alforan@tin.it>.
# Copyright (C) 2005-2014 codesets.library Open Source Team
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
# codesets.library project: http://sourceforge.net/projects/codesetslib/
#
# Most of the code included in this file was relicensed from GPL to LGPL
# from the source code of SimpleMail (http://www.sf.net/projects/simplemail)
# with full permissions by its authors.
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
RM      = delete force
RMDIR   = delete force all
MKDIR   = makedir
CHMOD   = protect FLAGS=rwed
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
  RM      = rm -f
  RMDIR   = rm -rf
  MKDIR   = mkdir -p
  CHMOD   = chmod 755

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

# target definition
TARGET = $(BINDIR)/b64d \
         $(BINDIR)/b64e \
         $(BINDIR)/UTF8ToStrHook \
         $(BINDIR)/DetectCodeset \
	 $(BINDIR)/demo1 \
	 $(BINDIR)/Convert

# Common compiler/linker flags
WARN     = -W -Wall -Wwrite-strings -Wpointer-arith -Wsign-compare
OPTFLAGS = -O3 -fomit-frame-pointer -funroll-loops
DEBUG    = -DDEBUG -O0
DEBUGSYM = -g -gstabs
CFLAGS   = -I. -I../include -I../../include $(CPU) $(WARN) $(OPTFLAGS) $(DEBUG) $(DEBUGSYM) $(USER_CFLAGS)
LDFLAGS  = $(CPU) $(DEBUGSYM)
LDLIBS   =

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
  REDEFINE =
  CFLAGS   += -mcrt=$(CRT) -D__USE_INLINE__ -D__NEW_TIMEVAL_DEFINITION_USED__ \
              $(REDEFINE) -Wa,-mregnames
  LDFLAGS  += -mcrt=$(CRT)

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
  CPU	    = -m68020-60 -msoft-float
  CFLAGS  += -noixemul
  LDFLAGS += -noixemul
  LDLIBS  += -ldebug -lmui
  NOINLINE = -DNO_INLINE_STDARG

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
  CFLAGS  += -noixemul -DUSE_INLINE_STDARG
  LDFLAGS += -noixemul
  LDLIBS  += -ldebug
  NOINLINE = -DNO_PPCINLINE_STDARG

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
  LDLIBS += -lmui

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
  LDLIBS += -lmui

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
  LDLIBS += -lmui

endif
endif
endif
endif
endif
endif

###########################################################################
# Here starts all stuff that is common for all target platforms and
# hosts.

# main target
.PHONY: all
all: $(BINDIR) $(OBJDIR) $(TARGET)

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

# make the object directories
$(BINDIR):
	@echo "  MK $@"
	@$(MKDIR) $(BINDIR)

# for compiling single .c files
$(OBJDIR)/%.o: %.c
	@echo "  CC $<"
	@$(CC) $(CFLAGS) -c $< -o $@

# for linking the target
$(BINDIR)/b64d: $(OBJDIR)/b64d.o
	@echo "  LD $@"
	@$(CC) $(LDFLAGS) -o $@.debug $< $(LDLIBS)
	@$(STRIP) --preserve-dates -R.comment -R.sdata2 -S -o $@ $@.debug
	@$(CHMOD) $@

$(BINDIR)/b64e: $(OBJDIR)/b64e.o
	@echo "  LD $@"
	@$(CC) $(LDFLAGS) -o $@.debug $< $(LDLIBS)
	@$(STRIP) --preserve-dates -R.comment -R.sdata2 -S -o $@ $@.debug
	@$(CHMOD) $@

$(BINDIR)/UTF8ToStrHook: $(OBJDIR)/UTF8ToStrHook.o
	@echo "  LD $@"
	@$(CC) $(LDFLAGS) -o $@.debug $< $(LDLIBS)
	@$(STRIP) --preserve-dates -R.comment -R.sdata2 -S -o $@ $@.debug
	@$(CHMOD) $@

$(BINDIR)/DetectCodeset: $(OBJDIR)/DetectCodeset.o
	@echo "  LD $@"
	@$(CC) $(LDFLAGS) -o $@.debug $< $(LDLIBS)
	@$(STRIP) --preserve-dates -R.comment -R.sdata2 -S -o $@ $@.debug

$(BINDIR)/demo1: demo1.c vastubs.c
	@echo "  CC demo1.c"
	@$(CC) $(CFLAGS) $(NOINLINE) -c demo1.c -o $(OBJDIR)/demo1.o
	@echo "  CC vastubs.c"
	@$(CC) $(CFLAGS) $(NOINLINE) -c vastubs.c -o $(OBJDIR)/vastubs.o
	@echo "  LD $@"
	@$(CC) $(LDFLAGS) -o $@.debug $(OBJDIR)/demo1.o $(OBJDIR)/vastubs.o $(LDLIBS)
	@$(STRIP) --preserve-dates -R.comment -R.sdata2 -S -o $@ $@.debug
	@$(CHMOD) $@

$(BINDIR)/Convert: $(OBJDIR)/Convert.o
	@echo "  LD $@"
	@$(CC) $(LDFLAGS) -o $@.debug $< $(LDLIBS)
	@$(STRIP) --preserve-dates -R.comment -R.sdata2 -S -o $@ $@.debug
	@$(CHMOD) $@

# cleanup target
.PHONY: clean
clean:
	-$(RM) $(TARGET) $(OBJDIR)/*.o

# clean all including .obj directory
.PHONY: cleanall
cleanall: clean
	-$(RMDIR) $(OBJDIR)

# clean all stuff, including our autotools
.PHONY: distclean
distclean: cleanall
	-$(RMDIR) $(BINDIR)
