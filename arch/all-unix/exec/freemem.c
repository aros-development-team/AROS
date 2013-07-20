/*
    Copyright � 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Free memory allocated by AllocMem()
    Lang: english
*/
#include <exec/alerts.h>
#include <exec/execbase.h>
#include <aros/libcall.h>
#include <aros/config.h>
#include <aros/macros.h>
#include <aros/rt.h>
#include <exec/memory.h>
#include <exec/memheaderext.h>
#include <proto/exec.h>

#include "exec_debug.h"

#ifndef DEBUG_FreeMem
#   define DEBUG_FreeMem 0
#endif
#undef DEBUG
#if DEBUG_FreeMem
#   define DEBUG 1
#endif
#define MDEBUG 1

#include <aros/debug.h>

#include <stdlib.h>

#include "exec_intern.h"
#include "memory.h"

#undef FreeMem	/* If we're debugging, AROS Clib will try to remap this */

/* See rom/exec/freemem.c for documentation */

AROS_LH2(void, FreeMem,
    AROS_LHA(APTR, memoryBlock, A1),
    AROS_LHA(IPTR, byteSize,    D0),
    struct ExecBase *, SysBase, 35, Exec)
{
    AROS_LIBFUNC_INIT

    ULONG origsize = byteSize;

    D(bug("Call FreeMem (%08lx, %ld)\n", memoryBlock, byteSize));

    /* If there is no memory free nothing */
    if(!byteSize || !memoryBlock)
	ReturnVoid ("FreeMem");

#if ENABLE_RT
    RT_Free(RTT_MEMORY, memoryBlock, byteSize);
#endif

    /* In early boot mode we can't free any memory */
    if (!PrivExecBase(SysBase)->defaultPool)
    	return;

    FreePooled(PrivExecBase(SysBase)->defaultPool, memoryBlock, byteSize);

    ReturnVoid ("FreeMem");
    
    AROS_LIBFUNC_EXIT
} /* FreeMem */

