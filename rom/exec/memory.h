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
 * 31.01.2011: disabled, because creates problems:
 * 1. FindMem() can be called from within supervisor mode
 * 2. FreeMem() can be called from within RemTask() in order to free task structure itself.
 *    This eventually leads to trashing memory (semaphore is owned by removed task).
 * Can be turned on again only after addressing these issues.

#define MEM_LOCK	ObtainSemaphore(&PrivExecBase(SysBase)->MemListSem)
#define MEM_LOCK_SHARED ObtainSemaphoreShared(&PrivExecBase(SysBase)->MemListSem)
#define MEM_UNLOCK	ReleaseSemaphore(&PrivExecBase(SysBase)->MemListSem)
*/
#define MEM_LOCK	Forbid()
#define MEM_LOCK_SHARED Forbid()
#define MEM_UNLOCK	Permit()


/* Private Pool structure */
struct Pool 
{
    struct MinList PuddleList;
    ULONG Requirements;
    ULONG PuddleSize;
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

struct MemHeader *FindMem(APTR address, struct ExecBase *SysBase);
APTR stdAlloc(struct MemHeader *mh, IPTR byteSize, ULONG requirements, struct ExecBase *SysBase);
void stdDealloc(struct MemHeader *freeList, APTR memoryBlock, IPTR byteSize, struct ExecBase *SysBase);

APTR InternalAllocAbs(APTR location, IPTR byteSize, struct ExecBase *SysBase);
void InternalFreeMem(APTR location, IPTR byteSize, struct ExecBase *SysBase);
APTR AllocMemHeader(IPTR size, ULONG flags, struct ExecBase *SysBase);
void FreeMemHeader(APTR addr, struct ExecBase *SysBase);

APTR InternalAllocPooled(APTR poolHeader, IPTR memSize, ULONG flags, const char *function, APTR caller, struct ExecBase *SysBase);
void InternalFreePooled(APTR memory, IPTR memSize, const char *function, APTR caller, APTR stack, struct ExecBase *SysBase);

ULONG checkMemHandlers(struct checkMemHandlersState *cmhs, struct ExecBase *SysBase);

APTR nommu_AllocMem(IPTR byteSize, ULONG flags, struct ExecBase *SysBase);
APTR nommu_AllocAbs(APTR location, IPTR byteSize, struct ExecBase *SysBase);
void nommu_FreeMem(APTR memoryBlock, IPTR byteSize, struct ExecBase *SysBase);
IPTR nommu_AvailMem(ULONG attributes, struct ExecBase *SysBase);

#define BLOCK_TOTAL \
((sizeof(struct Block)+AROS_WORSTALIGN-1)&~(AROS_WORSTALIGN-1))

#endif
