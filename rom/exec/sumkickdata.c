/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Build checksum for Kickstart.
    Lang: english
*/
#include <aros/debug.h>
#include "exec_intern.h"
#include <proto/exec.h>

/*****************************************************************************

    NAME */

	AROS_LH0(ULONG, SumKickData,

/*  SYNOPSIS */
	/* void */

/*  LOCATION */
	struct ExecBase *, SysBase, 102, Exec)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    ULONG chksum = 0;

    if (SysBase->KickTagPtr) {
    	IPTR *list = SysBase->KickTagPtr;
 	while(*list)
	{
   	    chksum += (ULONG)*list;
            /* on amiga, if bit 31 is set then this points to another list of
             * modules rather than pointing to a single module. bit 31 is
             * inconvenient on architectures where code may be loaded above
             * 2GB. on these platforms we assume aligned pointers and use bit
             * 0 instead */
#ifdef __mc68000__
	    if(*list & 0x80000000) { list = (IPTR *)(*list & 0x7fffffff); continue; }
#else
            if(*list & 0x1) { list = (IPTR *)(*list & ~(IPTR)0x1); continue; }
#endif
	    list++;
   	}
    }

    if (SysBase->KickMemPtr) {
	struct MemList *ml = (struct MemList*)SysBase->KickMemPtr;
	while (ml->ml_Node.ln_Succ) {
	    UBYTE i;
	    ULONG *p = (ULONG*)ml;
	    for (i = 0; i < sizeof(struct MemList) / sizeof(ULONG); i++)
	    	chksum += p[i];
	    ml = (struct MemList*)ml->ml_Node.ln_Succ;
	}
    }

    return chksum;

    AROS_LIBFUNC_EXIT
} /* SumKickData */
