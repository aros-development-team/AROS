/***************************************************************************

 BetterString.mcc - A better String gadget MUI Custom Class
 Copyright (C) 1997-2000 Allan Odgaard
 Copyright (C) 2005-2009 by BetterString.mcc Open Source Team

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 BetterString class Support Site:  http://www.sf.net/projects/bstring-mcc/

 $Id$

***************************************************************************/

#include <string.h>

#include <proto/exec.h>

#include "private.h"

APTR MyAllocPooled(APTR pool, ULONG length)
{
  ULONG *mem;

  length = (length + sizeof(ULONG) + sizeof(ULONG) - 1) & ~(sizeof(ULONG)-1);
  if((mem = AllocPooled(pool, length)))
  {
    *mem = length;
    mem += 1;
  }

  return(mem);
}

VOID MyFreePooled(APTR pool, APTR mem)
{
  ULONG *memptr, length;

  memptr = &((ULONG *)mem)[-1];
  length = *memptr;

  FreePooled(pool, memptr, length);
}

APTR ExpandPool(APTR pool, APTR mem, ULONG extra)
{
  ULONG length = ((ULONG *)mem)[-1] - sizeof(ULONG);
  ULONG sz = strlen((char *)mem) + extra;

  if(length <= sz)
  {
    APTR new_mem = MyAllocPooled(pool, sz + 20);

    CopyMem(mem, new_mem, length);
    MyFreePooled(pool, mem);

    mem = new_mem;
  }

  return(mem);
}
