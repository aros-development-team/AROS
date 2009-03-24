/***************************************************************************

 codesets.library - Amiga shared library for handling different codesets
 Copyright (C) 2001-2005 by Alfonso [alfie] Ranieri <alforan@tin.it>.
 Copyright (C) 2005-2009 by codesets.library Open Source Team

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
#include "debug.h"

/****************************************************************************/

#if !defined(HAVE_ALLOCVECPOOLED)
APTR
allocVecPooled(APTR pool,ULONG size)
{
  ULONG *mem;

  ENTER();

  size += sizeof(ULONG);
  if((mem = AllocPooled(pool, size)))
    *mem++ = size;

  RETURN(mem);
  return mem;
}
#endif

/****************************************************************************/

#if !defined(HAVE_FREEVECPOOLED)
void
freeVecPooled(APTR pool,APTR mem)
{
  ENTER();

  FreePooled(pool,(LONG *)mem - 1,*((LONG *)mem - 1));

  LEAVE();
}
#endif

/****************************************************************************/

APTR
reallocVecPooled(APTR pool, APTR mem, ULONG oldSize, ULONG newSize)
{
  ULONG *newMem;

  ENTER();

  if((newMem = allocVecPooled(pool, newSize)) != NULL)
  {
    memcpy(newMem, mem, (oldSize < newSize) ? oldSize : newSize);

    freeVecPooled(pool, mem);
  }

  RETURN(newMem);
  return newMem;
}

/****************************************************************************/

APTR
allocArbitrateVecPooled(ULONG size)
{
  ULONG *mem;

  ENTER();

  ObtainSemaphore(&CodesetsBase->poolSem);
  mem = allocVecPooled(CodesetsBase->pool, size);
  ReleaseSemaphore(&CodesetsBase->poolSem);

  RETURN(mem);
  return mem;
}

/****************************************************************************/

void
freeArbitrateVecPooled(APTR mem)
{
  ENTER();

  ObtainSemaphore(&CodesetsBase->poolSem);
  freeVecPooled(CodesetsBase->pool, mem);
  ReleaseSemaphore(&CodesetsBase->poolSem);

  LEAVE();
}

/****************************************************************************/

APTR
reallocArbitrateVecPooled(APTR mem, ULONG oldSize, ULONG newSize)
{
  ENTER();

  ObtainSemaphore(&CodesetsBase->poolSem);
  mem = reallocVecPooled(CodesetsBase->pool, mem, oldSize, newSize);
  ReleaseSemaphore(&CodesetsBase->poolSem);

  RETURN(mem);
  return mem;
}

/****************************************************************************/

ULONG utf16_strlen(UTF16 *ptr)
{
  ULONG l;

  for (l=0; ptr[l]; l++);
  return l<<1;
}

/****************************************************************************/

ULONG utf32_strlen(UTF32 *ptr)
{
  ULONG l;

  for (l=0; ptr[l]; l++);
  return l<<2;
}
