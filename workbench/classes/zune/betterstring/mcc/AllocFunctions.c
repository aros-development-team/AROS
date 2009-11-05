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

#include "Debug.h"

static APTR sharedPool;
#if !defined(__amigaos4__) && !defined(__MORPHOS__)
static struct SignalSemaphore sharedPoolSema;
#endif

BOOL CreateSharedPool(void)
{
  BOOL success = FALSE;

  ENTER();

  #if defined(__amigaos4__)
  sharedPool = AllocSysObjectTags(ASOT_MEMPOOL, ASOPOOL_MFlags, MEMF_SHARED,
                                                ASOPOOL_Puddle, 512,
                                                ASOPOOL_Threshold, 256,
                                                ASOPOOL_Name, "BetterString.mcc shared pool",
                                                ASOPOOL_Protected, TRUE,
                                                TAG_DONE);
  #elif defined(__MORPHOS__)
  sharedPool = CreatePool(MEMF_SEM_PROTECTED, 512, 256);
  #else
  sharedPool = CreatePool(MEMF_ANY, 512, 256);
  InitSemaphore(&sharedPoolSema);
  #endif

  if(sharedPool != NULL)
    success = TRUE;

  RETURN(success);
  return success;
}

void DeleteSharedPool(void)
{
  ENTER();

  if(sharedPool != NULL)
  {
    #if defined(__amigaos4__)
    FreeSysObject(ASOT_MEMPOOL, sharedPool);
    #else
    DeletePool(sharedPool);
    #endif
    sharedPool = NULL;
  }

  LEAVE();
}

APTR SharedPoolAlloc(ULONG length)
{
  ULONG *mem;

  ENTER();

  #if !defined(__amigaos4__) && !defined(__MORPHOS__)
  ObtainSemaphore(&sharedPoolSema);
  #endif

  length = (length + sizeof(ULONG) + sizeof(ULONG) - 1) & ~(sizeof(ULONG)-1);
  if((mem = AllocPooled(sharedPool, length)))
  {
    *mem = length;
    mem += 1;
  }

  #if !defined(__amigaos4__) && !defined(__MORPHOS__)
  ReleaseSemaphore(&sharedPoolSema);
  #endif

  RETURN(mem);
  return mem;
}

VOID SharedPoolFree(APTR mem)
{
  ULONG *memptr, length;

  ENTER();

  memptr = &((ULONG *)mem)[-1];
  length = *memptr;

  #if !defined(__amigaos4__) && !defined(__MORPHOS__)
  ObtainSemaphore(&sharedPoolSema);
  #endif

  FreePooled(sharedPool, memptr, length);

  #if !defined(__amigaos4__) && !defined(__MORPHOS__)
  ReleaseSemaphore(&sharedPoolSema);
  #endif

  LEAVE();
}

APTR SharedPoolExpand(APTR mem, ULONG extra)
{
  ULONG length = ((ULONG *)mem)[-1] - sizeof(ULONG);
  ULONG sz = strlen((char *)mem) + extra;

  ENTER();

  if(length <= sz)
  {
    APTR new_mem = SharedPoolAlloc(sz + 20);

    CopyMem(mem, new_mem, length);
    SharedPoolFree(mem);

    mem = new_mem;
  }

  RETURN(mem);
  return mem;
}
