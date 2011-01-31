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

#define RTPOTx4(a)      ((a)>2?4:(a)>1?2:1)

#define RTPOTx10(a)     ((a)>=4?RTPOTx4(((a)+3)/4)*4:RTPOTx4(a))

#define RTPOTx100(a)    \
((a)>=0x10?RTPOTx10(((a)+0xf)/0x10)*0x10:RTPOTx10(a))

#define RTPOTx10000(a)  \
((a)>=0x100?RTPOTx100(((a)+0xff)/0x100)*0x100:RTPOTx100(a))

#define RTPOTx100000000(a)      \
((a)>=0x10000?RTPOTx10000(((a)+0xffff)/0x10000)*0x10000:RTPOTx10000(a))

#define ROUNDUP_TO_POWER_OF_TWO(a)      RTPOTx100(a)

/* Some defines for the memory handling functions. */

/* This is for the alignment of memchunk structures. */
#define MEMCHUNK_TOTAL	\
ROUNDUP_TO_POWER_OF_TWO(AROS_WORSTALIGN>sizeof(struct MemChunk)? \
AROS_WORSTALIGN:sizeof(struct MemChunk))

/* This allows to take the end of the MemHeader as the first MemChunk. */
#define MEMHEADER_TOTAL \
((sizeof(struct MemHeader)+MEMCHUNK_TOTAL-1)&~(MEMCHUNK_TOTAL-1))

/*
 * Mask for flags that describe physical properties of the memory
 * MEMF_31BIT is effective only on 64-bit machines. On 32-bit architectures
 * it will be ignored.
 * This is done for backwards compatibility on 32-bit machines, however
 * this flag perhaps still can be useful there for managing PCI DMA memory.
 * In this case it should have meaning on 32-bit machines too.
 *	Pavel Fedin <pavel_fedin@mail.ru>
 */
#if (__WORDSIZE == 64)
#define MEMF_PHYSICAL_MASK (MEMF_PUBLIC|MEMF_CHIP|MEMF_FAST|MEMF_LOCAL|MEMF_24BITDMA|MEMF_31BIT)
#else
#define MEMF_PHYSICAL_MASK (MEMF_PUBLIC|MEMF_CHIP|MEMF_FAST|MEMF_LOCAL|MEMF_24BITDMA)
#endif

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

APTR AllocMemHeader(IPTR size, ULONG flags, struct ExecBase *SysBase);
void FreeMemHeader(APTR addr, struct ExecBase *SysBase);

APTR InternalAllocPooled(APTR poolHeader, IPTR memSize, ULONG flags, struct ExecBase *SysBase);
void InternalFreePooled(APTR memory, IPTR memSize, struct ExecBase *SysBase);

ULONG checkMemHandlers(struct checkMemHandlersState *cmhs, struct ExecBase *SysBase);

APTR nommu_AllocMem(IPTR byteSize, ULONG flags, struct ExecBase *SysBase);
APTR nommu_AllocAbs(APTR location, IPTR byteSize, struct ExecBase *SysBase);
void nommu_FreeMem(APTR memoryBlock, IPTR byteSize, struct ExecBase *SysBase);
IPTR nommu_AvailMem(ULONG attributes, struct ExecBase *SysBase);

#define BLOCK_TOTAL \
((sizeof(struct Block)+AROS_WORSTALIGN-1)&~(AROS_WORSTALIGN-1))

#endif
