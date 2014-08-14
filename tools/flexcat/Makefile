#/***************************************************************************
#
# $Id$
#
# Copyright (C) 1993-1999 by Jochen Wiedmann and Marcin Orlowski
# Copyright (C) 2002-2012 by the FlexCat Open Source Team
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or (at
# your option) any later version.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#
#***************************************************************************/

.PHONY: all
all: src

.PHONY: src
src:
	@$(MAKE) -C src --no-print-directory

.PHONY: catalogs
catalogs:
	@$(MAKE) -C src --no-print-directory catalogs

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
