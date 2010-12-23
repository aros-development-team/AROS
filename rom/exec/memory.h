/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
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

/* Mask for flags that describe physical properties of the memory */
#define MEMF_PHYSICAL_MASK (MEMF_PUBLIC|MEMF_CHIP|MEMF_FAST|MEMF_LOCAL|MEMF_24BITDMA)

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

struct MemHeader *FindMem(APTR address, struct ExecBase *SysBase);
APTR stdAlloc(struct MemHeader *mh, IPTR byteSize, ULONG requirements, struct ExecBase *SysBase);
void stdDealloc(struct MemHeader *freeList, APTR memoryBlock, IPTR byteSize, struct ExecBase *SysBase);

APTR AllocMemHeader(IPTR size, ULONG flags, struct ExecBase *SysBase);
void FreeMemHeader(APTR addr, struct ExecBase *SysBase);

APTR InternalAllocPooled(APTR poolHeader, IPTR memSize, ULONG flags, struct ExecBase *SysBase);
void InternalFreePooled(APTR memory, IPTR memSize, struct ExecBase *SysBase);

APTR nommu_AllocMem(IPTR byteSize, ULONG flags, struct ExecBase *SysBase);
APTR nommu_AllocAbs(APTR location, IPTR byteSize, struct ExecBase *SysBase);
void nommu_FreeMem(APTR memoryBlock, IPTR byteSize, struct ExecBase *SysBase);
IPTR nommu_AvailMem(ULONG attributes, struct ExecBase *SysBase);

#define BLOCK_TOTAL \
((sizeof(struct Block)+AROS_WORSTALIGN-1)&~(AROS_WORSTALIGN-1))

/* These bits describe physical properties of the memory */
#define MEMF_PHYSICAL_MASK (MEMF_PUBLIC|MEMF_CHIP|MEMF_FAST|MEMF_LOCAL|MEMF_24BITDMA)

#endif
