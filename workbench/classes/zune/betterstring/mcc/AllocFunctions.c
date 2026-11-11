/***************************************************************************

 BetterString.mcc - A better String gadget MUI Custom Class
 Copyright (C) 1997-2000 Allan Odgaard
 Copyright (C) 2005-2018 BetterString.mcc Open Source Team

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

/* Create the library-wide pool and initialize its semaphore on AROS. Return
 * whether creation succeeded. Call once before allocation; concurrent
 * creation/deletion is not serialized here.
 */
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
                                                ASOPOOL_LockMem, FALSE,
                                                TAG_DONE);
  #elif defined(__MORPHOS__)
  sharedPool = CreatePool(MEMF_SEM_PROTECTED, 512, 256);
  #else
  sharedPool = CreatePool(MEMF_ANY, 512, 256);
  memset(&sharedPoolSema, 0, sizeof(sharedPoolSema));
  InitSemaphore(&sharedPoolSema);
  #endif

  if(sharedPool != NULL)
    success = TRUE;

  RETURN(success);
  return success;
}

/* Delete the pool when present and clear its pointer. Require all users and
 * server operations stopped; outstanding blocks become invalid. Does not
 * acquire the allocation semaphore.
 */
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

#if defined(__amigaos3__)
/* AmigaOS 3-only compatibility allocator: reserve payload plus a stored
 * ULONG size and return the payload or NULL. Require a live pool and
 * representable size; pair with the matching FreeVecPooled. Excluded on
 * AROS.
 */
static APTR AllocVecPooled(APTR poolHeader, ULONG memSize)
{
  ULONG *memory;

  ENTER();

  // add the number of bytes used to store the size information
  memSize += sizeof(ULONG);

  // allocate memory from the pool
  if((memory = (ULONG *)AllocPooled(poolHeader, memSize)) != NULL)
  {
    // and finally store the size of the memory block, including the size itself
    *memory++ = memSize;
  }

  RETURN(memory);
  return memory;
}
#endif

/* Allocate length bytes from the live shared pool, serialized by
 * sharedPoolSema on AROS. Return owned storage or NULL; no clearing is
 * promised. Release with SharedPoolFree before teardown.
 */
APTR SharedPoolAlloc(ULONG length)
{
  ULONG *mem;

  ENTER();

  #if !defined(__amigaos4__) && !defined(__MORPHOS__)
  ObtainSemaphore(&sharedPoolSema);
  #endif

  mem = AllocVecPooled(sharedPool, length);

  #if !defined(__amigaos4__) && !defined(__MORPHOS__)
  ReleaseSemaphore(&sharedPoolSema);
  #endif

  RETURN(mem);
  return mem;
}

#if defined(__amigaos3__)
/* AmigaOS 3-only compatibility free: recover the stored size before a valid
 * matching payload and return the block to its pool. No NULL/provenance
 * validation is added. Excluded on AROS.
 */
static void FreeVecPooled(APTR poolHeader, APTR memory)
{
  ULONG *mem = (ULONG *)memory;
  ULONG memSize;

  ENTER();

  // skip back over the stored size information
  memSize = *(--mem);

  // an return the memory block to the pool
  FreePooled(poolHeader, mem, memSize);

  LEAVE();
}
#endif

/* Return a matching allocation through FreeVecPooled under the AROS pool
 * semaphore. Require a live pool and valid allocation; this wrapper adds no
 * NULL or lifetime check.
 */
void SharedPoolFree(APTR mem)
{
  ENTER();

  #if !defined(__amigaos4__) && !defined(__MORPHOS__)
  ObtainSemaphore(&sharedPoolSema);
  #endif

  FreeVecPooled(sharedPool, mem);

  #if !defined(__amigaos4__) && !defined(__MORPHOS__)
  ReleaseSemaphore(&sharedPoolSema);
  #endif

  LEAVE();
}

struct ContentString
{
  ULONG size;
  char string[0];
};

#define STR_TO_CSTR(str)    (struct ContentString *)(((size_t)(str)) - sizeof(struct ContentString))
#define CSTR_TO_STR(cstr)   (&(cstr)->string[0])

/* Allocate a size-tracked shared-pool block and initialize its first
 * character to NUL. Require size at least one and non-overflowing total
 * allocation. Return the owned header or NULL; other bytes are not
 * initialized.
 */
static struct ContentString *AllocContentStringInternal(ULONG size)
{
  struct ContentString *cstr;

  ENTER();

  if((cstr = SharedPoolAlloc(sizeof(*cstr) + size)) != NULL)
  {
    cstr->size = size;
    cstr->string[0] = '\0';
  }

  RETURN(cstr);
  return cstr;
}

/* Return an empty size-tracked character buffer or NULL. Size includes
 * terminator space and must be at least one. Free via FreeContentString,
 * not FreeVec or SharedPoolFree on the character pointer.
 */
char *AllocContentString(ULONG size)
{
  struct ContentString *cstr;
  char *result = NULL;

  ENTER();

  if((cstr = AllocContentStringInternal(size)) != NULL)
  {
    result = CSTR_TO_STR(cstr);
  }

  RETURN(result);
  return result;
}

/* Accept NULL or a matching live content-string pointer and free its
 * enclosing shared-pool allocation. Does not wipe bytes or clear caller
 * aliases; aliases become invalid.
 */
void FreeContentString(char *str)
{
  ENTER();

  if(str != NULL)
  {
    struct ContentString *cstr = STR_TO_CSTR(str);

    SharedPoolFree(cstr);
  }

  LEAVE();
}

/* Return allocated character capacity, including terminator space, or zero
 * for NULL. Require a matching live content-string pointer; this is not
 * strlen or pointer validation.
 */
ULONG ContentStringSize(char *str)
{
  ULONG size = 0;

  ENTER();

  if(str != NULL)
  {
    struct ContentString *cstr = STR_TO_CSTR(str);

    size = cstr->size;
  }

  RETURN(size);
  return size;
}

/* Ensure length plus terminator plus extra fits, adding a 32-byte margin on
 * growth. On successful reallocation copy, free old storage and replace the
 * pointer; on failure retain existing storage and return FALSE. NULL input
 * allocates exactly extra bytes, which must include terminator space.
 * Require valid pointer storage and representable sizes; pool locking does
 * not serialize a string.
 */
BOOL ExpandContentString(char **str, ULONG extra)
{
  BOOL success = FALSE;

  ENTER();

  if(*str != NULL)
  {
    struct ContentString *cstr = STR_TO_CSTR(*str);
    ULONG newsize = strlen(cstr->string) + 1 + extra;

    // check if we have to expand our contents string
    if(cstr->size < newsize)
    {
      struct ContentString *newcstr;

      // add another 32 bytes for less expansions in the future
      newsize += 32;
      if((newcstr = AllocContentStringInternal(newsize)) != NULL)
      {
        strlcpy(newcstr->string, cstr->string, newsize);
        FreeContentString(*str);
        *str = CSTR_TO_STR(newcstr);

        success = TRUE;
      }
    }
    else
    {
      // no expansion necessary, instant success
      success = TRUE;
    }
  }
  else
  {
    // allocate a new content string
    if((*str = AllocContentString(extra)) != NULL)
      success = TRUE;
  }

  RETURN(success);
  return success;
}
