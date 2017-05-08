/*

Copyright (C) 2004,2005 Neil Cafferkey

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston,
MA 02111-1307, USA.

*/

#ifndef PCI_H
#define PCI_H


#include <exec/types.h>
#include <utility/tagitem.h>

#include "device.h"


#define BAR_NO 1


struct BusContext
{
   struct DevUnit *unit;
   struct DevBase *device;
   VOID *card;
   UPINT io_base;
   const struct TagItem *unit_tags;
   BOOL have_card;
};


#endif
