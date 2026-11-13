#   Copyright © 2025, The AROS Development Team. All rights reserved.
#   $Id$
#
#   Desc: Makefile to create genctbl
#

USER_CFLAGS := -Wall -Werror -Wunused -O2

-include $(TOP)/config/make.cfg
-include Makefile.deps

HOST_CC	       ?= gcc
HOST_STRIP     ?= strip
MECHO	       ?= echo
HOST_CFLAGS    ?= $(USER_CFLAGS)
GENCTBL        ?= genctbl
GENDIR         ?= ./
MKDIR          ?= mkdir

all : $(GENCTBL) $(GENDIR)/ucd/UnicodeData.txt $(GENDIR)/ucd/SpecialCasing.txt

$(PORTSSOURCEDIR) :
	@$(MKDIR) -p $@

$(GENDIR)/ucd :
	@$(MKDIR) -p $@

$(PORTSSOURCEDIR)/UnicodeData.txt : | $(PORTSSOURCEDIR)
	@$(ECHO) "Fetching unicode data..."
	@curl -o $@ https://www.unicode.org/Public/UCD/latest/ucd/UnicodeData.txt

$(GENDIR)/ucd/UnicodeData.txt : $(PORTSSOURCEDIR)/UnicodeData.txt $(GENDIR)/ucd
	@$(CP) $< $@

$(PORTSSOURCEDIR)/SpecialCasing.txt : | $(PORTSSOURCEDIR)
	@$(ECHO) "Fetching unicode special casing data..."
	@curl -o $@ https://www.unicode.org/Public/UCD/latest/ucd/SpecialCasing.txt

$(GENDIR)/ucd/SpecialCasing.txt : $(PORTSSOURCEDIR)/SpecialCasing.txt $(GENDIR)/ucd
	@$(CP) $< $@

$(GENCTBL) : genctbl.c $(GENMODULE_DEPS)
	@$(ECHO) "Compiling $(notdir $@)..."
	@$(HOST_CC) -g $(HOST_CFLAGS) -I$(GENINCDIR) -I$(TOP)/$(CURDIR) genctbl.c -o $@

clean :
	@$(RM) $(GENCTBL)

.PHONY : all clean
