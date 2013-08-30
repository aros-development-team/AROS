#/***************************************************************************
#
# openurl.library - universal URL display and browser launcher library
# Copyright (C) 1998-2005 by Troels Walsted Hansen, et al.
# Copyright (C) 2005-2009 by openurl.library Open Source Team
#
# This library is free software; it has been placed in the public domain
# and you can freely redistribute it and/or modify it. Please note, however,
# that some components may be under the LGPL or GPL license.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
#
# openurl.library project: http://sourceforge.net/projects/openurllib/
#
# $Id$
#
#***************************************************************************/

.PHONY: all
all: library prefs cmd raPrefs

.PHONY: library
library:
	@$(MAKE) -C library

.PHONY: prefs
prefs:
	@$(MAKE) -C prefs

.PHONY: cmd
cmd:
	@$(MAKE) -C cmd

.PHONY: raPrefs
raPrefs:
	@$(MAKE) -C raPrefs

.PHONY: clean
clean:
	@$(MAKE) -C library clean
	@$(MAKE) -C prefs clean
	@$(MAKE) -C cmd clean
	@$(MAKE) -C raPrefs clean

.PHONY: cleanall
cleanall:
	@$(MAKE) -C library cleanall
	@$(MAKE) -C prefs cleanall
	@$(MAKE) -C cmd cleanall
	@$(MAKE) -C raPrefs cleanall

.PHONY: bumprev
bumprev:
	@sh tools/bumprev.sh all

.PHONY: release
release:
	@sh tools/mkrelease.sh
