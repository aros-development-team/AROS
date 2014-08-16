#/***************************************************************************
#
# NList MUI custom classes
#
# Copyright (C) 1996-2001 by Gilles Masson (NList.mcc)
# Copyright (C) 1999-2001 by Carsten Scholling (NListtree.mcc)
# Copyright (C) 2006      by Daniel Allsopp (NBitmap.mcc)
# Copyright (C) 2001-2014 NList Open Source Team
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
# NList classes Support Site:  http://www.sf.net/projects/nlist-classes
#
# $Id$
#
#***************************************************************************/

SUBDIRS = nbalance_mcc \
          nbitmap_mcc \
          nfloattext_mcc \
          nlist_mcc \
          nlisttree_mcc \
          nlisttree_mcp \
          nlistview_mcc \
          nlistviews_mcp \
          demo

.PHONY: all
all: $(SUBDIRS) catalogs

.PHONY: $(SUBDIRS)
$(SUBDIRS):
	@$(MAKE) -C $@

.PHONY: catalogs
catalogs:
	@$(MAKE) -C nlisttree_mcp catalogs
	@$(MAKE) -C nlistviews_mcp catalogs

.PHONY: clean
clean:
	@$(MAKE) -C nbalance_mcc clean
	@$(MAKE) -C nbitmap_mcc clean
	@$(MAKE) -C nfloattext_mcc clean
	@$(MAKE) -C nlist_mcc clean
	@$(MAKE) -C nlisttree_mcc clean
	@$(MAKE) -C nlisttree_mcp clean
	@$(MAKE) -C nlistview_mcc clean
	@$(MAKE) -C nlistviews_mcp clean
	@$(MAKE) -C demo clean

.PHONY: distclean
distclean:
	@$(MAKE) -C nbalance_mcc distclean
	@$(MAKE) -C nbitmap_mcc distclean
	@$(MAKE) -C nfloattext_mcc distclean
	@$(MAKE) -C nlist_mcc distclean
	@$(MAKE) -C nlisttree_mcc distclean
	@$(MAKE) -C nlisttree_mcp distclean
	@$(MAKE) -C nlistview_mcc distclean
	@$(MAKE) -C nlistviews_mcp distclean
	@$(MAKE) -C demo distclean

.PHONY: bumprev
bumprev:
	@sh tools/bumprev.sh all

.PHONY: release
release:
	@sh tools/mkrelease.sh $(RELEASEREV)
