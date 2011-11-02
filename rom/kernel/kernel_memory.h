/*
 * Kernel memory allocation functions
 *
 * In this default file they are just exec functions. However you may
 * reimplement them for a system with memory protection.
 *
 * void *krnAllocMem(unsigned int len);
 * void krnFreeMem(void *addr, unsigned int len);
 */

#include <exec/memory.h>
#include <proto/exec.h>

static inline struct KernelBase *AllocKernelBase(struct ExecBase *SysBase, int i)
{
    APTR mem;

    i *= LIB_VECTSIZE;
    i  = ((i - 1) / sizeof(IPTR) + 1) * sizeof(IPTR);
    
    mem = AllocMem(i + sizeof(struct KernelBase), MEMF_PUBLIC|MEMF_CLEAR);

    return mem ? mem + i : NULL;
}
