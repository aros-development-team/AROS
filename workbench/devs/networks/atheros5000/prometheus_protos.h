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

#ifndef PROMETHEUS_PROTOS_H
#define PROMETHEUS_PROTOS_H


#include "device.h"

ULONG GetPrometheusCount(struct DevBase *base);
struct BusContext *AllocPrometheusCard(ULONG index, struct DevBase *base);
VOID FreePrometheusCard(struct BusContext *context, struct DevBase *base);
BOOL AddPrometheusIntServer(APTR card, struct Interrupt *interrupt,
   struct DevBase *base);
VOID RemPrometheusIntServer(APTR card, struct Interrupt *interrupt,
   struct DevBase *base);
APTR AllocPrometheusDMAMem(UPINT size, struct DevBase *base);
VOID FreePrometheusDMAMem(APTR mem, UPINT size, struct DevBase *base);

#endif


