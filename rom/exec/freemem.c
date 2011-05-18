/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
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

#include <stdlib.h>

#include "exec_intern.h"
#include "memory.h"
#include "mungwall.h"

#undef FreeMem	/* If we're debugging, AROS Clib will try to remap this */

/*****************************************************************************

    NAME */

	AROS_LH2(void, FreeMem,

/*  SYNOPSIS */
	AROS_LHA(APTR,  memoryBlock, A1),
	AROS_LHA(ULONG, byteSize,    D0),

/*  LOCATION */
	struct ExecBase *, SysBase, 35, Exec)

/*  FUNCTION
	Give a block of memory back to the system pool.

    INPUTS
	memoryBlock - Pointer to the memory to be freed
	byteSize    - Size of the block

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	AllocMem()

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    D(bug("Call FreeMem (%08lx, %ld)\n", memoryBlock, byteSize));

    /* If there is no memory free nothing */
    if(!byteSize || !memoryBlock)
	ReturnVoid ("FreeMem");

    RT_Free (RTT_MEMORY, memoryBlock, byteSize);

    memoryBlock = MungWall_Check(memoryBlock, byteSize, "FreeMem", __builtin_return_address(0), __builtin_frame_address(1), SysBase);

    if (PrivExecBase(SysBase)->IntFlags & EXECF_MungWall)
        byteSize += MUNGWALL_TOTAL_SIZE;

    InternalFreeMem(memoryBlock, byteSize, SysBase);

    ReturnVoid ("FreeMem");

    AROS_LIBFUNC_EXIT
} /* FreeMem */

