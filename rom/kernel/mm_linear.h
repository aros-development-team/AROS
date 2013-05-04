/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
*/

#include <exec/memory.h>

/*
 * This structure describes usable space. It is pointed to by mh_First.
 * start and size specify address space which can actually be used for allocations.
 *
 * map is an array describing which pages are free and which are not. It contains as
 * many entries as many pages there are. Every entry is a single byte which
 * describes page state as follows:
 * - Most significant bit tells if the page is free (0) or allocated (1)
 * - Other bits tell size of pages block to which the page belongs.
 * 'block' is a contiguous space of one or more pages in the same state.
 * Status bit of all pages in the block will have the same value (allocated or free)
 * and count bits will decrease from 127 to 1. 0 is not a valid value for them.
 * For example, free block of three pages will be described by the sequence:
 * 0x03, 0x02, 0x01. Allocated block of four pages will be: 0x84, 0x83, 0x82, 0x81.
 *
 * With 4K pages it gives 256 KB map size for 1GB of RAM.
 * For 2 MB of RAM (A1200 example) it gives 512 bytes map size. This seems to be acceptable.
 *
 * Using counter allows faster navigation in the memory map. In order to get past
 * the block you just add counter to the current page number. Note that the block
 * can be longer than 127 pages, in this case all entries where count would be >127,
 * will have count = 127. In this case more than one step will be required to advance
 * past the block. It is possible to reduce number of such steps by changing map
 * entries from UBYTE to UWORD or ULONG, but this will also double memory usage. So it's a
 * memory vs speed tradeoff.
 * In order to change entry size, replace five macros and typedef below.
 *
 * TODOs:
 * 1. Implement reverse lookup order (for MEMF_REVERSE). Find some way to optimize
 *    it (with current implementation it is going to be slow because we can't skip
 *    more than one page in reverse direction).
 * 2. Allow administrative information (BlockHeader and memory map) to live outside of
 *    managed region. This would be a nice option for managing slow memory (like A1200
 *    chip RAM).
 */
 
/* Type of map entry */
typedef UBYTE page_t;

/* Parts of map entry */
#define P_COUNT(x)  (x & 0x7F)
#define P_STATUS(x) (x & 0x80)

/* Status bits */
#define P_ALLOC 0x80
#define P_FREE  0x00

/* Use this macro to increment pages count in the block */
#define INC_COUNT(x) if (x < 127) x++

struct BlockHeader
{
    struct MemChunk mc;		/* Backwards compatibility */
    APTR start;			/* Start address	   */
    ULONG size;			/* Total size in pages	   */
    ULONG pageSize;		/* Page size in bytes	   */
    struct SignalSemaphore sem;	/* Access semaphore	   */
    page_t map[1];		/* Allocations map	   */
};
