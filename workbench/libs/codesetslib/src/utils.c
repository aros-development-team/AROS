/***************************************************************************

 codesets.library - Amiga shared library for handling different codesets
 Copyright (C) 2001-2005 by Alfonso [alfie] Ranieri <alforan@tin.it>.
 Copyright (C) 2005-2007 by codesets.library Open Source Team

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 codesets.library project: http://sourceforge.net/projects/codesetslib/

 $Id$

***************************************************************************/

#include "lib.h"

/****************************************************************************/

#if !defined(__amigaos4__)
APTR
allocVecPooled(APTR pool,ULONG size)
{
  ULONG *mem;

  if((mem = AllocPooled(pool,size += sizeof(ULONG))))
    *mem++ = size;

  return mem;
}
#endif

/****************************************************************************/

#if !defined(__amigaos4__)
void
freeVecPooled(APTR pool,APTR mem)
{
  FreePooled(pool,(LONG *)mem - 1,*((LONG *)mem - 1));
}
#endif

/****************************************************************************/

APTR
reallocVecPooled(APTR pool, APTR mem, ULONG oldSize, ULONG newSize)
{
  ULONG *newMem;

  if((newMem = allocVecPooled(pool, newSize)) != NULL)
  {
    memcpy(newMem, mem, oldSize);

    freeVecPooled(pool, mem);
  }

  return newMem;
}

/****************************************************************************/

APTR
allocArbitrateVecPooled(ULONG size)
{
  ULONG *mem;

  ObtainSemaphore(&CodesetsBase->poolSem);
  mem = allocVecPooled(CodesetsBase->pool, size);
  ReleaseSemaphore(&CodesetsBase->poolSem);

  return mem;
}

/****************************************************************************/

void
freeArbitrateVecPooled(APTR mem)
{
  ObtainSemaphore(&CodesetsBase->poolSem);
  freeVecPooled(CodesetsBase->pool, mem);
  ReleaseSemaphore(&CodesetsBase->poolSem);
}

/****************************************************************************/

APTR
reallocArbitrateVecPooled(APTR mem, ULONG oldSize, ULONG newSize)
{
  ObtainSemaphore(&CodesetsBase->poolSem);
  mem = reallocVecPooled(CodesetsBase->pool, mem, oldSize, newSize);
  ReleaseSemaphore(&CodesetsBase->poolSem);

  return mem;
}

/****************************************************************************/
