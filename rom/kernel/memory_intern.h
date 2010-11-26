#include <exec/memory.h>

/*
 * This structure describes usable space. It is pointed to by mh_First.
 * start and size specify address space which can actually be used for allocations.
 *
 * map is an array describing which pages are free and which are not. It contains as
 * many entries as many pages there are. Every entry contains:
 * - a number of subsequent free pages starting from this particular address, if
 *   the corresponding page is free
 * - zero if the corresponding page is allocated
 * 
 * With 4K pages it gives one megabyte map size for 1GB of RAM.
 * For 2 MB of RAM (A1200 example) it gives 2KB map size. This seems to be acceptable.
 */
struct BlockHeader
{
    struct MemChunk mc;		/* Backwards compatibility */
    APTR start;			/* Start address	   */
    ULONG size;			/* Total size in pages	   */
    struct SignalSemaphore sem;	/* Access semaphore	   */
    ULONG map[1];		/* Allocations map	   */
};

APTR krnAllocate(struct MemHeader *mh, IPTR size, struct KernelBase *KernelBase);
void krnFree(struct MemHeader *mh, APTR addr, IPTR size, struct KernelBase *KernelBase);
