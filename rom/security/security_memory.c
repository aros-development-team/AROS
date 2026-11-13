/*
    Copyright (C) 2002-2026, The AROS Development Team. All rights reserved.

    Desc: security.library private memory management (pooled, semaphore
          protected). Derived from MultiUser Memory.c (c) Geert Uytterhoeven.
*/

#include <proto/exec.h>
#include <proto/utility.h>

#include "security_intern.h"
#include "security_memory.h"

#define MEM_PUDDLESIZE  4096
#define MEM_THRESHSIZE  4096

/* The library is a singleton, so the pool can be module static */
static APTR                     Pool = NULL;
static struct SignalSemaphore   MemSemaphore;

BOOL InitMemory(struct SecurityBase *secBase)
{
    D(bug(DEBUG_NAME_STR " %s()\n", __func__);)

    InitSemaphore(&MemSemaphore);
    Pool = CreatePool(MEMF_PUBLIC | MEMF_CLEAR, MEM_PUDDLESIZE, MEM_THRESHSIZE);

    D(bug(DEBUG_NAME_STR " %s: Pool @ %p\n", __func__, Pool);)

    return (Pool != NULL);
}

void CleanUpMemory(struct SecurityBase *secBase)
{
    ObtainSemaphore(&MemSemaphore);
    if (Pool)
    {
        DeletePool(Pool);
        Pool = NULL;
    }
    ReleaseSemaphore(&MemSemaphore);
}

/* AllocMem() replacement: cleared memory from the pool */
APTR MAlloc(ULONG size)
{
    APTR block = NULL;

    if (size == 0)
        return NULL;

    ObtainSemaphore(&MemSemaphore);
    if (Pool && (block = AllocPooled(Pool, size)))
        memset(block, 0, size);
    ReleaseSemaphore(&MemSemaphore);

    D(if (!block) bug(DEBUG_NAME_STR " %s: FAILED, size = %lu\n", __func__, (unsigned long)size);)

    return block;
}

/* FreeMem() replacement */
void Free(APTR block, ULONG size)
{
    if (block)
    {
        ObtainSemaphore(&MemSemaphore);
        if (Pool)
            FreePooled(Pool, block, size);
        ReleaseSemaphore(&MemSemaphore);
    }
}

/* AllocVec() replacement: size is stored in front of the block */
APTR MAllocV(ULONG size)
{
    IPTR *block = NULL;

    if (size == 0)
        return NULL;

    ObtainSemaphore(&MemSemaphore);
    if (Pool && (block = AllocPooled(Pool, size + sizeof(IPTR))))
    {
        *(block++) = size + sizeof(IPTR);
        memset(block, 0, size);
    }
    ReleaseSemaphore(&MemSemaphore);

    D(if (!block) bug(DEBUG_NAME_STR " %s: FAILED, size = %lu\n", __func__, (unsigned long)size);)

    return block;
}

/* FreeVec() replacement */
void FreeV(APTR block)
{
    if (block)
    {
        IPTR *real = ((IPTR *)block) - 1;

        ObtainSemaphore(&MemSemaphore);
        if (Pool)
            FreePooled(Pool, real, *real);
        ReleaseSemaphore(&MemSemaphore);
    }
}
