/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang:
*/
#ifndef _MEMORY_H_
#define _MEMORY_H_

//#include <aros/debug.h>
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

#if AROS_MUNGWALL_DEBUG

#define MUNGWALL_HEADER_ID 0x1ADEBCA1

/* This struct must not be bigger than MUNGWALLHEADER_SIZE!! */

struct MungwallHeader
{   
    union
    {
    	struct
	{
    	    struct  MinNode 	node;
    	    ULONG   	    	magicid;
    	    ULONG   	    	allocsize;
	} s;
	struct
	{
	    UBYTE   	    	blub[MUNGWALLHEADER_SIZE];
	} b;
    } u;    
};

#define mwh_node    	u.s.node
#define mwh_magicid 	u.s.magicid
#define mwh_allocsize 	u.s.allocsize

#endif


#define BLOCK_TOTAL \
((sizeof(struct Block)+AROS_WORSTALIGN-1)&~(AROS_WORSTALIGN-1))

#endif
