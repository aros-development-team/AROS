/*
    Copyright (C) 1995-2001 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang: English
*/
#include <proto/exec.h>
#include "dos_intern.h"

#include <aros/debug.h>

/*****************************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH1(LONG, SetIoErr,

/*  SYNOPSIS */
	AROS_LHA(LONG, result, D1),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 77, Dos)

/*  FUNCTION
	Sets to dos error code for the current process.

    INPUTS
	result -- new error code

    RESULT
	Old error code.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	29-10-95    digulla automatically created from
			    dos_lib.fd and clib/dos_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct DosLibrary *,DOSBase)

    /* old contents */
    LONG old;

    /* Get pointer to process structure */
    struct Process *me = (struct Process *)FindTask(NULL);

    ASSERT(__is_process(me));
    
    /* Nothing spectacular */
    old = me->pr_Result2;
    me->pr_Result2 = result;

    return old;

    AROS_LIBFUNC_EXIT
} /* SetIoErr */
