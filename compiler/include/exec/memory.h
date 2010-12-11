#ifndef EXEC_MEMORY_H
#define EXEC_MEMORY_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
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
    ULONG             mh_Free;
};

struct MemChunk
{
    struct MemChunk * mc_Next;
    ULONG             mc_Bytes;
};

struct MemEntry
{
    union
    {
        ULONG meu_Reqs;
        APTR  meu_Addr;
    } me_Un;
    ULONG me_Length;
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

/* AllocMem() Flags */
#define MEMF_ANY        0L
#define MEMF_PUBLIC     (1L<<0)
#define MEMF_CHIP       (1L<<1)
#define MEMF_FAST       (1L<<2)
#define MEMF_EXECUTABLE (1L<<4)	 /* AmigaOS v4 compatible */
#define MEMF_LOCAL      (1L<<8)
#define MEMF_24BITDMA   (1L<<9)
#define MEMF_KICK       (1L<<10)
#define MEMF_31BIT      (1L<<12) /* MorphOS compatible */
#define MEMF_CLEAR      (1L<<16)
#define MEMF_LARGEST    (1L<<17)
#define MEMF_REVERSE    (1L<<18)
#define MEMF_TOTAL      (1L<<19)
#define MEMF_HWALIGNED  (1L<<20) /* For AllocMem() only! */
#define MEMF_NO_EXPUNGE (1L<<31)

/* New in AROS/MorphOS. Flag for CreatePool to get automatic
   semaphore protection */
#define MEMF_SEM_PROTECTED (1L << 20)

struct MemHandlerData
{
    ULONG memh_RequestSize;
    ULONG memh_RequestFlags;
    ULONG memh_Flags;
};

#define MEMHF_RECYCLE (1L<<0)

#define MEM_ALL_DONE    (-1)
#define MEM_DID_NOTHING 0
#define MEM_TRY_AGAIN   1

#endif /* EXEC_MEMORY_H */
