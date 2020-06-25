/*
    Copyright © 2020, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
*/

#include <aros/kernel.h>
#include <aros/libcall.h>

#include <proto/timesource.h>

#include <kernel_base.h>

#include <kernel_debug.h>

/* We have own bug(), so don't use aros/debug.h to avoid conflicts */
#define D(x)

const static struct Node KernTSNode =
{
    .ln_Name = "kernel.resource"
};

/*****************************************************************************

    NAME */
#include <proto/kernel.h>

        AROS_LH2(void, KrnRegisterTimeSource,

/*  SYNOPSIS */
	AROS_LHA(APTR, TSBase, A0),
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
    if ((!KernelBase->kb_TimeSource) && TSBase)
    {
        IPTR TSUnit;
        D(
            bug("[KRN] KrnRegisterTimeSource: using TimeSource resource @ %p\n", TSBase);
          )

        KernelBase->kb_TimeSource = TSBase;
        TSUnit = AllocTSUnit(&KernTSNode);
        if (TSUnit != (IPTR)-1)
        {
            bug("[KRN] KrnRegisterTimeSource: allocated unit %p for timesource @ %p\n", TSUnit, TSBase);
        }
    }

    return;

    AROS_LIBFUNC_EXIT
}
