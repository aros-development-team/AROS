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

  ObtainSemaphore(&((struct LibraryHeader *)CodesetsBase)->poolSem);
  mem = AllocPooled(((struct LibraryHeader *)CodesetsBase)->pool, s);
  ReleaseSemaphore(&((struct LibraryHeader *)CodesetsBase)->poolSem);

  return mem;
}

/****************************************************************************/

void
freeArbitratePooled(APTR mem,ULONG s)
{
  ObtainSemaphore(&((struct LibraryHeader *)CodesetsBase)->poolSem);
  FreePooled(((struct LibraryHeader *)CodesetsBase)->pool, mem, s);
  ReleaseSemaphore(&((struct LibraryHeader *)CodesetsBase)->poolSem);
}

/****************************************************************************/

APTR
allocArbitrateVecPooled(ULONG size)
{
  ULONG *mem;

  ObtainSemaphore(&((struct LibraryHeader *)CodesetsBase)->poolSem);
  mem = allocVecPooled(((struct LibraryHeader *)CodesetsBase)->pool, size);
  ReleaseSemaphore(&((struct LibraryHeader *)CodesetsBase)->poolSem);

  return mem;
}

/****************************************************************************/

void
freeArbitrateVecPooled(APTR mem)
{
  ObtainSemaphore(&((struct LibraryHeader *)CodesetsBase)->poolSem);
  freeVecPooled(((struct LibraryHeader *)CodesetsBase)->pool, mem);
  ReleaseSemaphore(&((struct LibraryHeader *)CodesetsBase)->poolSem);
}

/****************************************************************************/
