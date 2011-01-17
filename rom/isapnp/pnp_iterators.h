/* $Id$ */

/*
     ISA-PnP -- A Plug And Play ISA software layer for AmigaOS.
     Copyright (C) 2001 Martin Blom <martin@blom.org>
     
     This library is free software; you can redistribute it and/or
     modify it under the terms of the GNU Library General Public
     License as published by the Free Software Foundation; either
     version 2 of the License, or (at your option) any later version.
     
     This library is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
     Library General Public License for more details.
     
     You should have received a copy of the GNU Library General Public
     License along with this library; if not, write to the
     Free Software Foundation, Inc., 59 Temple Place - Suite 330, Cambridge,
     MA 02139, USA.
*/

#ifndef	ISA_PNP_pnp_iterators_h
#define ISA_PNP_pnp_iterators_h

#include "CompilerSpecific.h"

#include <exec/lists.h>
#include <exec/nodes.h>
#include <exec/types.h>


struct ISAPNP_Resource;
struct ISAPNPBase;

struct ResourceIterator;
struct ResourceIteratorList;
struct ResourceContext;


struct ResourceContext*
AllocResourceIteratorContext( void );

void
FreeResourceIteratorContext( struct ResourceContext* ctx );



struct ResourceIteratorList*
AllocResourceIteratorList( struct MinList*         resource_list,
                           struct ResourceContext* ctx );


BOOL
FreeResourceIteratorList( struct ResourceIteratorList* list,
                          struct ResourceContext*      ctx );



BOOL
IncResourceIteratorList( struct ResourceIteratorList* iter_list,
                         struct ResourceContext*      ctx );



struct ISAPNP_Resource*
CreateResource( struct ResourceIterator* iter,
                struct ISAPNPBase*       res );

BOOL
CreateResouces( struct ResourceIteratorList* ril,
                struct List*                 result,
                struct ISAPNPBase*           res );

#endif /* ISA_PNP_pnp_iterators_h */
