/***************************************************************************

 BetterString.mcc - A better String gadget MUI Custom Class
 Copyright (C) 1997-2000 Allan Odgaard
 Copyright (C) 2005 by BetterString.mcc Open Source Team

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

#include <clib/alib_protos.h>
#include <exec/types.h>
#include <proto/exec.h>

#include "private.h"

void *MyAllocPooled(void *pool, unsigned long length)
{
  long *mem;

  if((mem = AllocPooled(pool, length+4)))
  {
    *mem = length+4;
    mem += 1;

    return(mem);
  }
  else
    return(NULL);
}

void MyFreePooled(void *pool, void *mem)
{
  long *memptr = (long *)mem;
  long length;

  memptr -= 1;
  length = *(memptr);

  FreePooled(pool, memptr, length);
}

APTR ExpandPool(APTR pool, APTR mem, ULONG extra)
{
	ULONG length;

	length = *(((ULONG *)mem)-1);
	if(length <= strlen((STRPTR)mem)+extra+4)
	{
		APTR new_mem;

		do
    {	
      new_mem = MyAllocPooled(pool, strlen((STRPTR)mem)+extra+20);
    }
    while(!new_mem);
		
    CopyMem(mem, new_mem, length);
		MyFreePooled(pool, mem);
		mem = new_mem;
	}
	
  return(mem);
}
