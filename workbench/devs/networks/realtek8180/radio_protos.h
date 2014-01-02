/*

Copyright (C) 2011 Neil Cafferkey

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

#ifndef RADIO_PROTOS_H
#define RADIO_PROTOS_H


#include "device.h"

BOOL InitialiseRadio(struct DevUnit *unit, struct DevBase *base);
VOID GetPower(struct DevUnit *unit, struct DevBase *base);
VOID SetPower(struct DevUnit *unit, struct DevBase *base);
#if 1
VOID WriteCCK(struct DevUnit *unit, UWORD index, UWORD value,
   struct DevBase *base); 
VOID WriteOFDM(struct DevUnit *unit, UWORD index, UWORD value,
   struct DevBase *base);
#endif
UWORD ReadRadio(struct DevUnit *unit, UWORD index,
   struct DevBase *base);
VOID WriteRadio(struct DevUnit *unit, UWORD index, UWORD value,
   struct DevBase *base);


#endif


