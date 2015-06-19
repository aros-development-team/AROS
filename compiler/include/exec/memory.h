#ifndef EXEC_MEMORY_H
#define EXEC_MEMORY_H

/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Memory handling
    Lang: english
*/

#ifndef EXEC_NODES_H
#   include <exec/nodes.h>
#endif

struct MemHeader
{
    struct Node       mh_Node;
    UWORD             mh_Attributes;
    struct MemChunk * mh_First;
    APTR              mh_Lower;
    APTR              mh_Upper;
    IPTR              mh_Free;
};

struct MemChunk
{
    struct MemChunk *mc_Next;
    IPTR             mc_Bytes;
};

/* Total size of struct MemChunk, including padding */
#define MEMCHUNK_TOTAL  (AROS_WORSTALIGN > sizeof(struct MemChunk) ? AROS_WORSTALIGN : sizeof(struct MemChunk))
/* Total size of struct MemHeader, including padding */
#define MEMHEADER_TOTAL (AROS_ROUNDUP2(sizeof(struct MemHeader), MEMCHUNK_TOTAL))

struct MemEntry
{
    union
    {
        ULONG meu_Reqs;
        APTR  meu_Addr;
    } me_Un;
    IPTR me_Length;
};
#define me_Reqs me_Un.meu_Reqs
#define me_Addr me_Un.meu_Addr

struct MemList
{
    struct Node     ml_Node;
    UWORD           ml_NumEntries;
    struct MemEntry ml_ME[1];
};

#define MEM_BLOCKSIZE 8L
#define MEM_BLOCKMASK (MEM_BLOCKSIZE - 1)

/* Memory allocation flags */
#define MEMF_ANY           0L
#define MEMB_PUBLIC        0
#define MEMF_PUBLIC        (1L << MEMB_PUBLIC)
#define MEMB_CHIP          1
#define MEMF_CHIP          (1L << MEMB_CHIP)
#define MEMB_FAST          2
#define MEMF_FAST          (1L << MEMB_FAST)
#define MEMB_VIRTUAL       3            /* AmigaOS v4 compatible */
#define MEMF_VIRTUAL       (1L << MEMB_VIRTUAL)
#define MEMB_EXECUTABLE    4            /* AmigaOS v4 compatible */
#define MEMF_EXECUTABLE    (1L << MEMB_EXECUTABLE)
#define MEMB_LOCAL         8
#define MEMF_LOCAL         (1L << MEMB_LOCAL)
#define MEMB_24BITDMA      9
#define MEMF_24BITDMA      (1L << MEMB_24BITDMA)
#define MEMB_KICK          10
#define MEMF_KICK          (1L << MEMB_KICK)
#define MEMB_PRIVATE       11
#define MEMF_PRIVATE       (1L << MEMB_PRIVATE)
#define MEMB_31BIT         12           /* Low address space (<2GB). Effective only on 64 bit machines. */
#define MEMF_31BIT         (1L << MEMB_31BIT)

#define MEMB_CLEAR         16           /* Explicitly clear memory after allocation */
#define MEMF_CLEAR         (1L << MEMB_CLEAR)
#define MEMB_LARGEST       17
#define MEMF_LARGEST       (1L << MEMB_LARGEST)
#define MEMB_REVERSE       18
#define MEMF_REVERSE       (1L << MEMB_REVERSE)
#define MEMB_TOTAL         19
#define MEMF_TOTAL         (1L << MEMB_TOTAL)
#define MEMB_HWALIGNED     20           /* For AllocMem() - align address and size to physical page boundary */
#define MEMF_HWALIGNED     (1L << MEMB_HWALIGNED)
#define MEMB_SEM_PROTECTED 20           /* For CreatePool() - add semaphore protection to the pool */
#define MEMF_SEM_PROTECTED (1L << MEMB_SEM_PROTECTED)
#define MEMB_NO_EXPUNGE    31
#define MEMF_NO_EXPUNGE    (1L << MEMB_NO_EXPUNGE)

/*
 * Mask for flags that describe physical properties of the memory.
 * MEMF_31BIT is effective only on 64-bit machines.
 * This is actually AROS internal definition, it may change. Do not rely on it.
 */
#if (__WORDSIZE == 64)
#define MEMF_PHYSICAL_MASK (MEMF_PUBLIC|MEMF_CHIP|MEMF_FAST|MEMF_LOCAL|MEMF_24BITDMA|MEMF_KICK|MEMF_31BIT)
#else
#define MEMF_PHYSICAL_MASK (MEMF_PUBLIC|MEMF_CHIP|MEMF_FAST|MEMF_LOCAL|MEMF_24BITDMA|MEMF_KICK)
#endif

struct MemHandlerData
{
    IPTR  memh_RequestSize;
    ULONG memh_RequestFlags;
    ULONG memh_Flags;
};

#define MEMHF_RECYCLE (1L<<0)

#define MEM_ALL_DONE    (-1)
#define MEM_DID_NOTHING 0
#define MEM_TRY_AGAIN   1

#endif /* EXEC_MEMORY_H */
