/*
    Copyright © 2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Initialize the debug interface
    Lang: english
*/

#include <aros/symbolsets.h>

#include "kernel_base.h"
#include "kernel_debug.h"
#include "kernel_memory.h"

#ifdef AROS_MODULES_DEBUG
/* Provided for GdbStub debugging */
struct MinList *Debug_ModList;
dbg_seg_t *Debug_KickList; /* Not unused */

typedef ULONG size_t;
/* 'malloc' and 'free' are needed for GDB's strcmp(), which is
 * used by the 'loadseg' method of the .gdbinit of AROS
 */
void *malloc(ULONG size)
{
    size_t *mem;

    size = (size + sizeof(size_t) - 1) & ~(sizeof(size_t) - 1);

    mem = krnAllocMem(size);
    *(mem++) = size;
    return mem;
}

void free(void *ptr)
{
    size_t *mem = ptr;

    mem--;
    krnFreeMem(mem, mem[0]);
}

/* Since GDB can't patch 'start' in the ROM with 
 * 'trap #1' instructions, we make a fake 'start'
 * in the .bss segment.
 */
UWORD start;

#endif

static int Kernel_DebugInit(struct KernelBase *KernelBase)
{
#ifdef AROS_MODULES_DEBUG
	/* Provision for gdbstub debugging */
	Debug_ModList = &KernelBase->kb_Modules;
#endif

	return TRUE;
}

ADD2INITLIB(Kernel_DebugInit, 10)
