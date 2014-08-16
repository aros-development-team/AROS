/***************************************************************************

 NList.mcc - New List MUI Custom Class
 Registered MUI class, Serial Number: 1d51 0x9d510030 to 0x9d5100A0
                                           0x9d5100C0 to 0x9d5100FF

 Copyright (C) 1996-2001 by Gilles Masson
 Copyright (C) 2001-2014 NList Open Source Team

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 NList classes Support Site:  http://www.sf.net/projects/nlist-classes

 $Id$

***************************************************************************/

#include <exec/types.h>

#define NO_PROTOS
#include "private.h"

/*
 *
 * Warning: the number of octets have to be a multiple of 4
 * and to be efficient, src and dest also have to be a multiple of 4
 *
 * extern void  NL_Move(void *dest,void *src,LONG len);
 *
 */

void NL_Move(struct TypeEntry **dest, struct TypeEntry **src, LONG count, LONG newpos)
{
  LONG i;

  for(i = 0; i < count; i++)
  {
    (*src)->entpos = newpos++;
    *dest++ = *src++;
  }
}


void NL_MoveD(struct TypeEntry **dest, struct TypeEntry **src, LONG count, LONG newpos)
{
  LONG i;

  for(i = 0; i < count; i++)
  {
    *--dest = *--src;
    (*dest)->entpos = --newpos;
  }
}
