#include <exec/execbase.h>
#include <exec/memory.h>
#include <proto/exec.h>

#include LC_LIBDEFS_FILE

/*
 * This code allocates KernelBase via exec.library, assuming its memory management is up and running.
 * On non-MMU systems this is true. On MMU-aware systems we must create KernelBase on our own, exec's
 * allocator won't work without it, because it needs to request memory pages for us. On such systems
 * this code needs to be replaced.
 */
struct KernelBase *AllocKernelBase(struct ExecBase *SysBase)
{
    int vecsize = FUNCTIONS_COUNT * LIB_VECTSIZE;
    APTR mem;

    vecsize = ((vecsize - 1) / sizeof(IPTR) + 1) * sizeof(IPTR);
    mem = AllocMem(vecsize + sizeof(struct KernelBase), MEMF_PUBLIC|MEMF_CLEAR);

    return mem ? mem + vecsize : NULL;
}
