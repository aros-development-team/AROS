/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.2  1996/08/01 17:41:27  digulla
    Added standard header for all files

    Desc:
    Lang:
*/
#ifndef _MEMORY_H_
#define _MEMORY_H_
#include <exec/lists.h>
#include <stddef.h>

#define RTPOTx4(a)      ((a)>2?4:(a)>1?2:1)

#define RTPOTx10(a)     ((a)>=4?RTPOTx4(((a)+3)/4)*4:RTPOTx4(a))

#define RTPOTx100(a)    \
((a)>=0x10?RTPOTx10(((a)+0xf)/0x10)*0x10:RTPOTx10(a))

#define RTPOTx10000(a)  \
((a)>=0x100?RTPOTx100(((a)+0xff)/0x100)*0x100:RTPOTx100(a))

#define RTPOTx100000000(a)      \
((a)>=0x10000?RTPOTx10000(((a)+0xffff)/0x10000)*0x10000:RTPOTx10000(a))

#define ROUNDUP_TO_POWER_OF_TWO(a)      RTPOTx100000000(a)

/* Some defines for the memory handling functions. */

/* This is for the alignment of memchunk structures. */
#define MEMCHUNK_TOTAL	\
ROUNDUP_TO_POWER_OF_TWO(DOUBLEALIGN>sizeof(struct MemChunk)? \
DOUBLEALIGN:sizeof(struct MemChunk))

/* This allows to take the end of the MemHeader as the first MemChunk. */
#define MEMHEADER_TOTAL \
((sizeof(struct MemHeader)+MEMCHUNK_TOTAL-1)&~(MEMCHUNK_TOTAL-1))

/* This is the extra memory needed by AllocVec() */
#define ALLOCVEC_TOTAL \
(DOUBLEALIGN>sizeof(ULONG)?DOUBLEALIGN:sizeof(ULONG))

struct Pool /* Private Pool structure */
{
    struct MinList PuddleList;
    struct MinList BlockList;
    ULONG Requirements;
    ULONG PuddleSize;
    ULONG ThreshSize;
};

struct Block
{
    struct MinNode Node;
    ULONG Size;
};

#define BLOCK_TOTAL \
((sizeof(struct Block)+DOUBLEALIGN-1)&~(DOUBLEALIGN-1))

#endif
