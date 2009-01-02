/*

Copyright (C) 2005 Neil Cafferkey

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

#ifndef UNIT_PROTOS_H
#define UNIT_PROTOS_H


#include "device.h"

struct DevUnit *CreateUnit(ULONG unit_num, APTR card,
   const struct TagItem *io_tags, UWORD bus, struct DevBase *base);
VOID DeleteUnit(struct DevUnit *unit, struct DevBase *base);
VOID ConfigureAdapter(struct DevUnit *unit, struct DevBase *base);
VOID GoOnline(struct DevUnit *unit, struct DevBase *base);
VOID GoOffline(struct DevUnit *unit, struct DevBase *base);
BOOL AddMulticastRange(struct DevUnit *unit, const UBYTE *lower_bound,
   const UBYTE *upper_bound, struct DevBase *base);
BOOL RemMulticastRange(struct DevUnit *unit, const UBYTE *lower_bound,
   const UBYTE *upper_bound, struct DevBase *base);
struct TypeStats *FindTypeStats(struct DevUnit *unit, struct MinList *list,
   ULONG packet_type, struct DevBase *base);
VOID FlushUnit(struct DevUnit *unit, UBYTE last_queue, BYTE error,
   struct DevBase *base);
VOID UpdateStats(struct DevUnit *unit, struct DevBase *base);


#endif


