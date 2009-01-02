/*
 * Copyright (C) 1993 AmiTCP/IP Group, <amitcp-group@hut.fi>
 *                    Helsinki University of Technology, Finland.
 *                    All rights reserved.
 * Copyright (C) 2005 - 2007 The AROS Dev Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 * MA 02111-1307, USA.
 *
 */

#include <conf.h>

#include <sys/param.h>
#include <sys/malloc.h>

#include <kern/amiga_includes.h>

struct SignalSemaphore malloc_semaphore = { 0 };
static BOOL initialized = FALSE;
static APTR mem_pool = NULL;

BOOL
malloc_init(void)
{
#if defined(__AROS__)
D(bug("[AROSTCP](kern_malloc.c) malloc_init()\n"));
#endif
  if (!initialized) {
    mem_pool = CreatePool(MEMF_PUBLIC, __MALLOC_POOLSIZE, __MALLOC_POOLSIZE_THRESHOLD);
    if(mem_pool == NULL)
    {
#if defined(__AROS__)
D(bug("[AROSTCP](kern_malloc.c) malloc_init: Failed to Allocate Pool\n"));
#endif
      return FALSE;
    }
#if defined(__AROS__)
D(bug("[AROSTCP](kern_malloc.c) malloc_init: Pool Allocated (%d bytes) @ 0x%p\n", __MALLOC_POOLSIZE, mem_pool));
#endif
    /*
     * Initialize malloc_semaphore for use.
     * Do not call bsd_malloc() or bsd_free() before this!
     */
    InitSemaphore(&malloc_semaphore);
    initialized = TRUE;
  }
  return TRUE;
}

VOID
malloc_deinit(void)
{
#if defined(__AROS__)
D(bug("[AROSTCP](kern_malloc.c) malloc_deinit()\n"));
#endif
  DeletePool(mem_pool);
#if defined(__AROS__)
D(bug("[AROSTCP](kern_malloc.c) malloc_deinit: Pool deleted\n"));
#endif
  initialized = FALSE;
  return;
}

void *
bsd_malloc(unsigned long size, int type, int flags)
{
  register void *mem;
#if defined(__AROS__)
D(bug("[AROSTCP](kern_malloc.c) bsd_malloc()\n"));
#endif

#ifdef DEBUG_MEM
  size += 4;
#endif
  ObtainSemaphore(&malloc_semaphore);
  mem = AllocVecPooled(mem_pool, size);
#ifdef DEBUG_MEM
  *((ULONG *)mem)++=0xbaadab00;
#endif
#if defined(__AROS__)
D(bug("[AROSTCP](kern_malloc.c) bsd_malloc: Allocated %d bytes @ 0x%p, from pool @ 0x%p\n", size, mem, mem_pool));
#endif
  ReleaseSemaphore(&malloc_semaphore);

  return mem;
}

void
bsd_free(void *addr, int type)
{
#if defined(__AROS__)
D(bug("[AROSTCP](kern_malloc.c) bsd_free(0x%p, pool @ 0x%p)\n", addr, mem_pool));
#endif

  ObtainSemaphore(&malloc_semaphore);
#ifdef DEBUG_MEM
  if (*--((ULONG *)addr) == 0xbaadab00)
  {
    *((ULONG *)addr) = 0xabadcafe;
#endif
    FreeVecPooled(mem_pool, addr);
#ifdef DEBUG_MEM
  }
  else
  {
#if defined(__AROS__)
D(bug("[AROSTCP](kern_malloc.c) bsd_free: Attempt to free non-allocated memory at 0x%p!!!\n", addr));
#endif
    log(LOG_CRIT,"Attempt to free non-allocated memory at 0x%p!!!", addr);
  }
#endif
  ReleaseSemaphore(&malloc_semaphore);
}

/*
 * bsd_realloc() cannot be used for reallocating
 * last freed block for obvious reasons
 *
 * This function is only called from one place, and there
 * it is used to shorten the allocated block. Only this particular
 * behaviour is implemented
 */

void *
bsd_realloc(void * mem, unsigned long size, int type, int flags)
{
  void *new_mem = NULL;
#if defined(__AROS__)
D(bug("[AROSTCP](kern_malloc.c) bsd_realloc(0x%p : %d bytes, pool @ 0x%p)\n", mem, size, mem_pool));
#endif

  ObtainSemaphore(&malloc_semaphore);
#ifdef DEBUG_MEM
  {
    ULONG *realmem = mem-4;
    unsigned long realsize = size+4;
    if (*realmem == 0xbaadab00)
    {
#else
#define realmem mem
#define realsize size
#endif
      new_mem = AllocVecPooled(mem_pool, realsize);
D(bug("[AROSTCP](kern_malloc.c) bsd_realloc: Allocated %d bytes @ 0x%p\n", realsize, new_mem));
      if(new_mem != NULL) {
#ifdef DEBUG_MEM
	*((ULONG *)new_mem)++ = 0xbaadab00;
#endif
	CopyMem(mem, new_mem, size);
	FreeVecPooled(mem_pool, realmem);
      }
#ifdef DEBUG_MEM
    }
    else
    {
#if defined(__AROS__)
D(bug("[AROSTCP](kern_malloc.c) bsd_free: Attempt to reallocate non-allocated memory at 0x%p!!!", mem));
#endif
      log(LOG_CRIT,"Attempt to reallocate non-allocated memory at 0x%p!!!", mem);
    }
  }
#endif
  ReleaseSemaphore(&malloc_semaphore);

  return new_mem;
}

