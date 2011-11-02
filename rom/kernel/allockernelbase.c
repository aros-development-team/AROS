#include <exec/execbase.h>
#include <exec/memory.h>
#include <proto/exec.h>

#include <kernel_base.h>
#include <kernel_debug.h>
#include <kernel_mm.h>

#define D(x)

/*
 * This code allocates KernelBase via exec.library, assuming its memory management is up and running.
 * On non-MMU systems this is true. On MMU-aware systems we must create KernelBase on our own, exec's
 * allocator won't work without it, because it needs to request memory pages for us. On such systems
 * this code needs to be replaced.
 */
struct KernelBase *AllocKernelBase(struct ExecBase *SysBase, int vecsize)
{
    APTR mem = AllocMem(vecsize + sizeof(struct KernelBase), MEMF_PUBLIC|MEMF_CLEAR);

    return mem ? mem + vecsize : NULL;
}

/* 
 * Allocate memory space for boot-time usage. Returns address and size of the usable area.
 * It's strongly adviced to return enough space to store resident list of sane length.
 */
APTR krnGetSysMem(struct MemHeader *mh, IPTR *size)
{
    /* Just dequeue the first MemChunk. It's assumed that it has the required space for sure. */
    struct MemChunk *mc = mh->mh_First;

    mh->mh_First = mc->mc_Next;
    mh->mh_Free -= mc->mc_Bytes;

    D(bug("[SysMem] Using chunk 0x%p of %lu bytes\n", mc, mc->mc_Bytes));
 
    *size = mc->mc_Bytes;
    return mc;
}

/* Release unused boot-time memory */
void krnReleaseSysMem(struct MemHeader *mh, APTR addr, IPTR chunkSize, IPTR allocSize)
{
    struct MemChunk *mc;

    allocSize = AROS_ROUNDUP2(allocSize, MEMCHUNK_TOTAL);
    chunkSize -= allocSize;

    D(bug("[SysMem] Chunk 0x%p, %lu of %lu bytes used\n", addr, allocSize, chunkSize));

    if (chunkSize < MEMCHUNK_TOTAL)
    	return;

    mc = addr + allocSize;

    mc->mc_Next  = mh->mh_First;
    mc->mc_Bytes = chunkSize - allocSize;

    mh->mh_First = mc;
    mh->mh_Free += mc->mc_Bytes;
}
