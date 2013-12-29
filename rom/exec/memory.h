/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang:
*/
#ifndef _MEMORY_H_
#define _MEMORY_H_

#include <exec/lists.h>
#include <exec/semaphores.h>
#include <exec/memory.h>
#include <stddef.h>

/*
 * EXPERIMENTAL: use semaphore protection instead of Forbid()/Permit() for
 * system memory allocation routines.
 * In case of problems use definitions below.
 *
 * Many m68k programs assume forbid state won't get broken.
 */
#ifndef __mc68000
#define MEM_LOCK	ObtainSemaphore(&PrivExecBase(SysBase)->MemListSem)
#define MEM_LOCK_SHARED ObtainSemaphoreShared(&PrivExecBase(SysBase)->MemListSem)
#define MEM_UNLOCK	ReleaseSemaphore(&PrivExecBase(SysBase)->MemListSem)
#else
#define MEM_LOCK	Forbid()
#define MEM_LOCK_SHARED Forbid()
#define MEM_UNLOCK	Permit()
#endif

#define POOL_MAGIC AROS_MAKE_ID('P','o','O','l')

/* Private Pool structure */
struct Pool 
{
    struct MinList PuddleList;
    ULONG Requirements;
    ULONG PuddleSize;
    ULONG PoolMagic;
};

struct ProtectedPool
{
    struct Pool     	   pool;
    struct SignalSemaphore sem;
};

struct Block
{
    struct MinNode Node;
    ULONG Size;
};

struct checkMemHandlersState
{
    struct Node           *cmhs_CurNode;
    struct MemHandlerData  cmhs_Data;
};

struct TraceLocation;

struct MemHeaderAllocatorCtx;

/* Returns existing or allocates new ctx */
struct MemHeaderAllocatorCtx * mhac_GetSysCtx(struct MemHeader * mh, struct ExecBase * SysBase);
void mhac_PoolMemHeaderSetup(struct MemHeader * mh, struct ProtectedPool * pool);
ULONG mhac_GetCtxSize();
/* Clears ctx if it already exists for a given MemHeader */
void mhac_ClearSysCtx(struct MemHeader * mh, struct ExecBase * SysBase);

struct MemHeader *FindMem(APTR address, struct ExecBase *SysBase);
APTR stdAlloc(struct MemHeader *mh, struct MemHeaderAllocatorCtx *mhac, IPTR byteSize, ULONG requirements, struct TraceLocation *loc, struct ExecBase *SysBase);
void stdDealloc(struct MemHeader *freeList, struct MemHeaderAllocatorCtx *mhac, APTR memoryBlock, IPTR byteSize, struct TraceLocation *loc, struct ExecBase *SysBase);

APTR InternalAllocAbs(APTR location, IPTR byteSize, struct ExecBase *SysBase);
void InternalFreeMem(APTR location, IPTR byteSize, struct TraceLocation *loc, struct ExecBase *SysBase);
APTR AllocMemHeader(IPTR size, ULONG flags, struct TraceLocation *loc, struct ExecBase *SysBase);
void FreeMemHeader(APTR addr, struct TraceLocation *loc, struct ExecBase *SysBase);

APTR InternalAllocPooled(APTR poolHeader, IPTR memSize, ULONG flags, struct TraceLocation *loc, struct ExecBase *SysBase);
void InternalFreePooled(APTR poolHeader, APTR memory, IPTR memSize, struct TraceLocation *loc, struct ExecBase *SysBase);

ULONG checkMemHandlers(struct checkMemHandlersState *cmhs, struct ExecBase *SysBase);

APTR nommu_AllocMem(IPTR byteSize, ULONG flags, struct TraceLocation *loc, struct ExecBase *SysBase);
APTR nommu_AllocAbs(APTR location, IPTR byteSize, struct ExecBase *SysBase);
void nommu_FreeMem(APTR memoryBlock, IPTR byteSize, struct TraceLocation *loc, struct ExecBase *SysBase);
IPTR nommu_AvailMem(ULONG attributes, struct ExecBase *SysBase);

#define PME_FREE_NO_CHUNK       1
#define PME_FREE_INV_POOL       2
#define PME_FREE_MXD_POOL       3
#define PME_ALLOC_INV_POOL      4
#define PME_DEL_POOL_INV_POOL   5

void PoolManagerAlert(ULONG code, ULONG flags, IPTR memSize, APTR memory, APTR poolHeaderMH, APTR poolHeader);

#define BLOCK_TOTAL \
((sizeof(struct Block)+AROS_WORSTALIGN-1)&~(AROS_WORSTALIGN-1))

#endif
