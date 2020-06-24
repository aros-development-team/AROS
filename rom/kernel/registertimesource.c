/*
    Copyright © 2020, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
*/

#include <aros/kernel.h>
#include <aros/libcall.h>

#include <kernel_base.h>

/*****************************************************************************

    NAME */
#include <proto/kernel.h>

        AROS_LH2(void, KrnRegisterTimeSource,

/*  SYNOPSIS */
	AROS_LHA(APTR, tsbase, A0),
	AROS_LHA(struct TagItem *, tstags, A1),

/*  LOCATION */
	struct KernelBase *, KernelBase, 57, Kernel)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    /* HACK: for now we just use the given timesource if we dont have one yet... */
    if (!KernelBase->kb_TimeSource)
        KernelBase->kb_TimeSource = tsbase;

    return;

    AROS_LIBFUNC_EXIT
}
