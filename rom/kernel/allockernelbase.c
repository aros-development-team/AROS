/*
    Copyright © 2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Allocate kernel.resource base.
    Lang: english
*/

#include <exec/memory.h>
#include <proto/exec.h>

#include LC_LIBDEFS_FILE
#include <kernel_globals.h>

struct KernelBase *AllocKernelBase(struct ExecBase *SysBase)
{
    APTR mem;
    ULONG i = FUNCTIONS_COUNT * LIB_VECTSIZE;

    /* Align vector table size */
    i  = ((i - 1) / sizeof(IPTR) + 1) * sizeof(IPTR);    

    /* Allocate the memory */
    mem = AllocMem(i + sizeof(struct KernelBase), MEMF_PUBLIC|MEMF_CLEAR);
    if (!mem)
    	return NULL;

    /* Skip past the vector table */
    mem += i;

    /* Set global KernelBase storage and return */
    setKernelBase(mem);
    return mem;
}
