#   Copyright © 1995-2001, The AROS Development Team. All rights reserved.
#   $Id$
#
#   Desc: Makefile for FlexCat
#

# Things of interest.
SRC	:=  flexcat.c
SDFILES	:=  C_c_orig.sd C_h_orig.sd gcccatcomp.sd

include $(TOP)/config/make.cfg

USER_CFLAGS := -Wunused -O3
SD_OUT	:=  $(foreach f,$(SDFILES),$(TOOLDIR)/$(f))

all : $(FLEXCAT) $(SD_OUT) $(TOOLDIR)/gcccatcomp.sh

$(FLEXCAT) : flexcat.c
	@$(MECHO) "Compiling $(notdir $@)..."
	@$(HOST_CC) $(HOST_CFLAGS) $(HOST_LDFLAGS) $< -o $@
	@$(STRIP) $@

$(TOOLDIR)/gcccatcomp.sh: gcccatcomp.sh
	@$(CP) $< $@

$(TOOLDIR)/%.sd : %.sd
	@$(CP) $< $@

clean:
	@$(RM) -f $(FLEXCAT)
