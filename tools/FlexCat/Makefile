#   Copyright (C) 2000 AROS - The Amiga Replacement OS
#   $Id$
#
#   Desc: Makefile for FlexCat
#

# Things of interest.
SRC	:=  flexcat.c
SDFILES	:=  C_c_orig.sd C_h_orig.sd

-include $(TOP)/config/make.cfg

USER_CFLAGS := -Wunused -O3

# Try and get some defaults if we couldn't read make.cfg
HOST_CC	    ?= $(CC)
HOST_SRIP   ?= strip
MKDIR	    ?= mkdir
FLEXCAT	    ?= FlexCat
HOST_CFLAGS ?= $(USER_CFLAGS)
CP	    ?= cp
TOOLDIR	    ?= .
STRIP	    ?= strip
MECHO	    ?= echo

all : $(FLEXCAT)

real-all: $(FLEXCAT) copy_sd_files

$(FLEXCAT) : flexcat.c
	@$(MECHO) "Compiling $(notdir $@)..."
	@$(CC) $(HOST_CFLAGS) $(HOST_LDFLAGS) $< -o $@
	@$(STRIP) $@

SD_OUT := $(foreach f,$(SDFILES),$(TOOLDIR)/$(f))

copy_sd_files : $(SD_OUT)

$(SD_OUT) : $(SDFILES)
	@$(CP) $< $@

clean:
	@$(RM) -f $(FLEXCAT)
