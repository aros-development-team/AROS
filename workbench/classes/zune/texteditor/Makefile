#/***************************************************************************
#
# TextEditor.mcc - Textediting MUI Custom Class
# Copyright (C) 1997-2000 Allan Odgaard
# Copyright (C) 2005-2014 TextEditor.mcc Open Source Team
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
# TextEditor class Support Site:  http://www.sf.net/projects/texteditor-mcc
#
# $Id$
#
#***************************************************************************/

.PHONY: all
all: mcc mcp demo

.PHONY: mcc
mcc:
	@$(MAKE) -C mcc

.PHONY: mcp
mcp:
	@$(MAKE) -C mcp

.PHONY: demo
demo:
	@$(MAKE) -C demo

.PHONY: clean
clean:
	@$(MAKE) -C mcc clean
	@$(MAKE) -C mcp clean
	@$(MAKE) -C demo clean

.PHONY: distclean
distclean:
	@$(MAKE) -C mcc distclean
	@$(MAKE) -C mcp distclean
	@$(MAKE) -C demo distclean

.PHONY: install
install:
	@$(MAKE) -C mcc install
	@$(MAKE) -C mcp install
	@$(MAKE) -C demo install

.PHONY: bumprev
bumprev:
	@sh tools/bumprev.sh all

.PHONY: release
release:
	@sh tools/mkrelease.sh
