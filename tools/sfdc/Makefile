# Copyright © 2003, The AROS Development Team. All rights reserved.
# $Id$

include $(TOP)/config/make.cfg

MAIN		= main.pl
CLASSES		= $(wildcard [A-Z]*.pl)
PODS		= SFDCompile.pod

all : $(SFDC)

$(SFDC) : $(MAIN) $(CLASSES) $(PODS)
	@$(SED) "s/SFDC_VERSION/$(VERSION)/g" $^ | sed "s/SFDC_DATE/$(DATE)/g" >$@
	@$(CHMOD) +x $@
	
clean:
	@$(RM) -f $(SFDC)
