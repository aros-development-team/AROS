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

APTR
allocVecPooled(APTR pool,ULONG size)
{
  ULONG *mem;

  if((mem = AllocPooled(pool,size += sizeof(ULONG))))
    *mem++ = size;

  return mem;
}

/****************************************************************************/

void
freeVecPooled(APTR pool,APTR mem)
{
  FreePooled(pool,(LONG *)mem - 1,*((LONG *)mem - 1));
}

/****************************************************************************/

APTR
allocArbitratePooled(ULONG s)
{
  APTR mem;

  ObtainSemaphore(&CodesetsBase->poolSem);
  mem = AllocPooled(CodesetsBase->pool, s);
  ReleaseSemaphore(&CodesetsBase->poolSem);

  return mem;
}

/****************************************************************************/

void
freeArbitratePooled(APTR mem,ULONG s)
{
  ObtainSemaphore(&CodesetsBase->poolSem);
  FreePooled(CodesetsBase->pool, mem, s);
  ReleaseSemaphore(&CodesetsBase->poolSem);
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
