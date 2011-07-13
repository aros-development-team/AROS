/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

#include <aros/libcall.h>
#include <aros/asmcall.h>
#include <exec/libraries.h>
#include <proto/debug.h>
#include <proto/exec.h>

#include "dos_intern.h"
#include <loadseg/loadseg.h>

/*****************************************************************************

    NAME */
#include <proto/dos.h>

        AROS_LH2(BOOL, InternalUnLoadSeg,

/*  SYNOPSIS */
        AROS_LHA(BPTR     , seglist , D1),
        AROS_LHA(VOID_FUNC, freefunc, A1),

/*  LOCATION */
        struct DosLibrary *, DOSBase, 127, Dos)

/*  FUNCTION
	Unloads a seglist loaded with InternalLoadSeg().

    INPUTS
	seglist  - Seglist
	freefunc - Function to be called to free memory

    RESULT
	DOSTRUE if everything wents O.K.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    return UnLoadSegment(seglist, freefunc, DOSBase);

    AROS_LIBFUNC_EXIT
} /* InternalUnLoadSeg */
