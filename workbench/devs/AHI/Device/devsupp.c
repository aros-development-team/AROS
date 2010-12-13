/*
     AHI - Hardware independent audio subsystem
     Copyright (C) 1996-2005 Martin Blom <martin@blom.org>
     
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

// Not the best routines (fraction does not get saved between calls,
// loads of byte writes, no interpolation etc), but who cares? 

#include <config.h>

#include "ahi_def.h"
#include "devsupp.h"

/*
 * size	 Number of SAMPLES to fill. (Max 131071)
 * add	 Add interger.fraction in samples (2×16 bit)
 * src	 Source (AHIST_S16S)
 * offset Pointer to Source Offset in bytes (will be updated)
 * dest	Pointer to Destination (will be updated)
 */

struct sample
{
    WORD left;
    WORD right;
};

void
RecM8S( ULONG  size,
	ULONG  add,
	APTR   src,
	ULONG* offset,
	void** dest )
{
  Fixed64        offs = 0;
  Fixed64        step = ((Fixed64) add) << 16;
  struct sample* from = (struct sample*) (src + *offset);
  BYTE*          to   = *dest;
  ULONG          i;

  for( i = 0; i < size; ++i )
  {
    *to++ = from[ offs >> 32 ].left >> 8;

    offs += step;
  }
  
  *offset += ( offs >> 32 ) * sizeof( struct sample* );
  *dest   = to;
}

void
RecS8S( ULONG  size,
	ULONG  add,
	APTR   src,
	ULONG* offset,
	void** dest )
{
  Fixed64        offs = 0;
  Fixed64        step = ((Fixed64) add) << 16;
  struct sample* from = (struct sample*) (src + *offset);
  BYTE*          to   = *dest;
  ULONG          i;

  for( i = 0; i < size; ++i )
  {
    *to++ = from[ offs >> 32 ].left  >> 8;
    *to++ = from[ offs >> 32 ].right >> 8;

    offs += step;
  }
  
  *offset += ( offs >> 32 ) * sizeof( struct sample* );
  *dest   = to;
}

void
RecM16S( ULONG  size,
	 ULONG  add,
	 APTR   src,
	 ULONG* offset,
	 void** dest )
{
  Fixed64        offs = 0;
  Fixed64        step = ((Fixed64) add) << 16;
  struct sample* from = (struct sample*) (src + *offset);
  WORD*          to   = *dest;
  ULONG          i;

  for( i = 0; i < size; ++i )
  {
    *to++ = from[ offs >> 32 ].left;

    offs += step;
  }
  
  *offset += ( offs >> 32 ) * sizeof( struct sample* );
  *dest   = to;
}

void
RecS16S( ULONG  size,
	 ULONG  add,
	 APTR   src,
	 ULONG* offset,
	 void** dest )
{
  Fixed64        offs = 0;
  Fixed64        step = ((Fixed64) add) << 16;
  struct sample* from = (struct sample*) (src + *offset);
  WORD*          to   = *dest;
  ULONG          i;

  for( i = 0; i < size; ++i )
  {
    *to++ = from[ offs >> 32 ].left;
    *to++ = from[ offs >> 32 ].right;

    offs += step;
  }
  
  *offset += ( offs >> 32 ) * sizeof( struct sample* );
  *dest   = to;
}

void
RecM32S( ULONG  size,
	 ULONG  add,
	 APTR   src,
	 ULONG* offset,
	 void** dest )
{
  Fixed64        offs = 0;
  Fixed64        step = ((Fixed64) add) << 16;
  struct sample* from = (struct sample*) (src + *offset);
  LONG*          to   = *dest;
  ULONG          i;

  for( i = 0; i < size; ++i )
  {
    *to++ = from[ offs >> 32 ].left << 16;

    offs += step;
  }
  
  *offset += ( offs >> 32 ) * sizeof( struct sample* );
  *dest   = to;
}

void
RecS32S( ULONG  size,
	 ULONG  add,
	 APTR   src,
	 ULONG* offset,
	 void** dest )
{
  Fixed64        offs = 0;
  Fixed64        step = ((Fixed64) add) << 16;
  struct sample* from = (struct sample*) (src + *offset);
  LONG*          to   = *dest;
  ULONG          i;

  for( i = 0; i < size; ++i )
  {
    *to++ = from[ offs >> 32 ].left  << 16;
    *to++ = from[ offs >> 32 ].right << 16;

    offs += step;
  }
  
  *offset += ( offs >> 32 ) * sizeof( struct sample* );
  *dest   = to;
}
