#ifndef _UNIT_H_
#define _UNIT_H_
/*
 * $Id$
 */
/*
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

#include <exec/types.h>
#include <exec/resident.h>
#include <exec/io.h>
#include <exec/ports.h>
#include <exec/errors.h>

#include <devices/sana2.h>
#include <devices/sana2specialstats.h>
#include <devices/newstyle.h>

#include <utility/utility.h>
#include <utility/tagitem.h>
#include <utility/hooks.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/battclock.h>

#include <stdlib.h>

#include "e1000_hw.h"
#include LC_LIBDEFS_FILE

struct e1000Unit *CreateUnit(LIBBASETYPEPTR, OOP_Object *);
void DeleteUnit(struct e1000Base *, struct e1000Unit *);
void FlushUnit(LIBBASETYPEPTR, struct e1000Unit *, UBYTE, BYTE);
BOOL AddressFilter(struct e1000Base *, struct e1000Unit *, UBYTE *);
VOID CopyPacket(struct e1000Base *, struct e1000Unit *, struct IOSana2Req *, UWORD, UWORD, struct eth_frame *);
VOID ReportEvents(struct e1000Base *, struct e1000Unit *, ULONG);
struct TypeStats *FindTypeStats(struct e1000Base *, struct e1000Unit *, struct MinList *, ULONG);

BOOL AddMulticastRange(LIBBASETYPEPTR, struct e1000Unit *, const UBYTE *, const UBYTE *);
BOOL RemMulticastRange(LIBBASETYPEPTR, struct e1000Unit *, const UBYTE *, const UBYTE *);

#endif //_UNIT_H_
