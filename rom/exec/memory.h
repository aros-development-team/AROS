/*
    Copyright (C) 1995-2001 AROS - The Amiga Research OS
    $Id$
    $Log$
    Revision 1.6  2001/10/18 12:11:05  stegerg
    support for automatic semaphore protection in pools
    (use MEMF_SEM_PROTECTED in CreatePool).

    Revision 1.5  1998/10/20 16:45:13  hkiel
    Amiga Research OS

    Revision 1.4  1996/10/23 14:26:58  aros
    Renamed AROS macros from XYZ to AROS_XYZ, so we know what they are

    Use only roundup macro for 256 bytes (not for 2^32)

    Use AROS_WORSTALIGN instead of DOUBLEALIGN

    Revision 1.3  1996/10/19 17:11:07  aros
    Moved ALLOCVEC_TOTAL to machine.h because it's used many times.

    Revision 1.2  1996/08/01 17:41:27  digulla
    Added standard header for all files

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

/* Private Pool structure */
struct Pool 
{
    struct MinList PuddleList;
    struct MinList BlockList;
    ULONG Requirements;
    ULONG PuddleSize;
    ULONG ThreshSize;
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

#define BLOCK_TOTAL \
((sizeof(struct Block)+AROS_WORSTALIGN-1)&~(AROS_WORSTALIGN-1))

#endif
