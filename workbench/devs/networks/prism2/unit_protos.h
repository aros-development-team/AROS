/*

Copyright (C) 2004-2011 Neil Cafferkey

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

struct DevUnit *CreateUnit(ULONG index, APTR card,
   const struct TagItem *io_tags, UWORD bus, struct DevBase *base);
VOID DeleteUnit(struct DevUnit *unit, struct DevBase *base);
BOOL InitialiseAdapter(struct DevUnit *unit, BOOL reinsertion,
   struct DevBase *base);
VOID ConfigureAdapter(struct DevUnit *unit, struct DevBase *base);
VOID ReconfigureAdapter(struct DevUnit *unit, struct DevBase *base);
VOID GoOnline(struct DevUnit *unit, struct DevBase *base);
VOID GoOffline(struct DevUnit *unit, struct DevBase *base);
VOID SetOptions(struct DevUnit *unit, const struct TagItem *tag_list,
   struct DevBase *base);
VOID SetKey(struct DevUnit *unit, ULONG index, ULONG type, const UBYTE *key,
   ULONG key_length, const UBYTE *rx_counter, struct DevBase *base);
BOOL AddMulticastRange(struct DevUnit *unit, const UBYTE *lower_bound,
   const UBYTE *upper_bound, struct DevBase *base);
BOOL RemMulticastRange(struct DevUnit *unit, const UBYTE *lower_bound,
   const UBYTE *upper_bound, struct DevBase *base);
struct TypeStats *FindTypeStats(struct DevUnit *unit, struct MinList *list,
   ULONG packet_type, struct DevBase *base);
VOID FlushUnit(struct DevUnit *unit, UBYTE last_queue, BYTE error,
   struct DevBase *base);
BOOL StatusInt(REG(a1, struct DevUnit *unit), REG(a6, APTR int_code));
VOID UpdateStats(struct DevUnit *unit, struct DevBase *base);
struct TagItem *GetNetworkInfo(struct DevUnit *unit, APTR pool,
   struct DevBase *base);
VOID UpdateSignalQuality(struct DevUnit *unit, struct DevBase *base);
VOID StartScan(struct DevUnit *unit, const TEXT *ssid,
   struct DevBase *base);


#endif


