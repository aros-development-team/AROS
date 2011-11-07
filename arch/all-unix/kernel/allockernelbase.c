#include <exec/execbase.h>
#include <exec/memory.h>
#include <proto/exec.h>

#include "kernel_libdefs.h"	/* Actually LC_LIBDEFS_FILE, we want FUNCTIONS_COUNT */

#include <kernel_debug.h>
#include <kernel_globals.h>
#include "kernel_intern.h"
#include "kernel_unix.h"

#define D(x) x

struct KernelBase *AllocKernelBase(struct ExecBase *SysBase)
{
    APTR mem;
    struct KernelBase *KernelBase;
    ULONG i = FUNCTIONS_COUNT * LIB_VECTSIZE;

    /* Align vector table size */
    i  = ((i - 1) / sizeof(IPTR) + 1) * sizeof(IPTR);    

    /* Allocate the memory. Note that we have platform-specific portion in KernelBase. */
    mem = AllocMem(i + sizeof(struct UnixKernelBase), MEMF_PUBLIC|MEMF_CLEAR);
    if (!mem)
    	return NULL;

    /* Skip past the vector table */
    KernelBase = mem + i;

    KernelBase->kb_PlatformData = AllocMem(sizeof(struct PlatformData), MEMF_ANY);
    D(bug("[KRN] PlatformData %p\n", KernelBase->kb_PlatformData));

    if (!KernelBase->kb_PlatformData)
    	return NULL;

    /* Set global KernelBase storage and return */
    setKernelBase(KernelBase);
    return KernelBase;
}
