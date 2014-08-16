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

.PHONY: all
all: src

.PHONY: src
src:
	@$(MAKE) -C src --no-print-directory

.PHONY: clean
clean:
	@$(MAKE) -C src --no-print-directory clean

.PHONY: cleanall
cleanall:
	@$(MAKE) -C src --no-print-directory cleanall

.PHONY: install
install:
	@$(MAKE) -C src --no-print-directory install

.PHONY: bumprev
bumprev:
	@sh tools/bumprev.sh all

.PHONY: release
release:
	@sh tools/mkrelease.sh
