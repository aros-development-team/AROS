#   Copyright © 1995-2001, The AROS Development Team. All rights reserved.
#   $Id$
#
#   Desc: Makefile for FlexCat
#

# Things of interest.
SRC	:=  flexcat.c
SDFILES	:=  C_c_orig.sd C_h_orig.sd

-include $(TOP)/config/make.cfg

USER_CFLAGS := -Wunused -O3
SD_OUT	:=  $(foreach f,$(SDFILES),$(TOOLDIR)/$(f))

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


all : $(FLEXCAT) copy_sd_files

real-all: $(FLEXCAT) copy_sd_files

$(FLEXCAT) : flexcat.c
	@$(MECHO) "Compiling $(notdir $@)..."
	@$(CC) $(HOST_CFLAGS) $(HOST_LDFLAGS) $< -o $@
	@$(STRIP) $@

copy_sd_files : $(SD_OUT)

$(TOOLDIR)/%.sd : %.sd
	@$(CP) $< $@

clean:
	@$(RM) -f $(FLEXCAT)
