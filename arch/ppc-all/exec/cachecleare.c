/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc: CacheClearE() - Clear the caches with extended control.
    Lang: english
*/

#include <aros/config.h>
#include <exec/types.h>
#include <exec/execbase.h>
#include <aros/libcall.h>

#include "kernel_syscall.h"

#include <proto/exec.h>

/* See rom/exec/cachecleare.c for documentation */

AROS_LH3(void, CacheClearE,
    AROS_LHA(APTR, address, A0),
    AROS_LHA(ULONG, length, D0),
    AROS_LHA(ULONG, caches, D1),
    struct ExecBase *, SysBase, 107, Exec)
{
    AROS_LIBFUNC_INIT

    char *start = (char*)((IPTR)address & 0xffffffe0);
    char *end = (char*)(((IPTR)address + length + 31) & 0xffffffe0);
    char *ptr;
    
    /* Flush data caches and mark cache lines invalid */
    if (caches & CACRF_ClearD)
    {
        for (ptr = start; ptr < end; ptr +=32)
        {
            asm volatile("dcbf 0,%0"::"r"(ptr));
        }
        asm volatile("sync");
    }

#if (AROS_FLAVOUR & AROS_FLAVOUR_STANDALONE)
    if (caches & CACRF_InvalidateD)
    {
        register APTR addr asm ("r4") = address;
        register ULONG len asm ("r5") = length;
        asm volatile("li %%r3,%0; sc"::"i"(SC_INVALIDATED),"r"(addr),"r"(len):"memory","r3");
    }
#endif

    if (caches & CACRF_ClearI) /* Clear ICache with DCache together */
    {
        for (ptr = start; ptr < end; ptr +=32)
        {
            asm volatile("icbi 0,%0"::"r"(ptr));
        }
    
        asm volatile("sync; isync; ");
    }

    AROS_LIBFUNC_EXIT
} /* CacheClearE */
