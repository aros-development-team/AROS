/*

Copyright (C) 2006 Neil Cafferkey

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

#ifndef DEVICE_PROTOS_H
#define DEVICE_PROTOS_H


#include "device.h"

BYTE DevOpen(REG(a1, struct IOSana2Req *request),
   REG(d0, ULONG unit_num), REG(d1, ULONG flags),
   REG(BASE_REG, struct DevBase *base));
APTR DevClose(REG(a1, struct IOSana2Req *request),
   REG(BASE_REG, struct DevBase *base));
VOID DevBeginIO(REG(a1, struct IOSana2Req *request),
   REG(BASE_REG, struct DevBase *base));
VOID DevAbortIO(REG(a1, struct IOSana2Req *request),
   REG(BASE_REG, struct DevBase *base));
VOID CloseUnit(struct IOSana2Req *request, struct DevBase *base);
struct DevUnit *GetUnit(ULONG unit_num, struct DevBase *base);
BOOL WrapInt(struct Interrupt *interrupt, struct DevBase *base);
VOID UnwrapInt(struct Interrupt *interrupt, struct DevBase *base);

#endif


