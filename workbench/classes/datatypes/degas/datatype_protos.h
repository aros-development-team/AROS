/*

Author: Neil Cafferkey
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

#ifndef DATATYPE_PROTOS_H
#define DATATYPE_PROTOS_H


#include <exec/types.h>

#include "datatype.h"

struct DTBase *LibInit(struct DTBase *lib_base REG("d0"),
   APTR seg_list REG("a0"), struct DTBase *base REG(BASE_REG));
struct DTBase *LibOpen(ULONG version REG("d0"),
   struct DTBase *base REG(BASE_REG));
APTR LibClose(struct DTBase *base REG(BASE_REG));
APTR LibExpunge(struct DTBase *base REG(BASE_REG));
APTR LibReserved();
struct IClass *ObtainClass(struct DTBase *base REG(BASE_REG));

#endif


