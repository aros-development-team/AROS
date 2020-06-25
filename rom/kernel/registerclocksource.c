/*
    Copyright © 2020, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
*/

#include <aros/kernel.h>
#include <aros/libcall.h>

#include <proto/clocksource.h>

#include <kernel_base.h>

#include <kernel_debug.h>

/* We have own bug(), so don't use aros/debug.h to avoid conflicts */
#define D(x)

const static struct Node KernCSNode =
{
    .ln_Name = "kernel.resource"
};

/*****************************************************************************

    NAME */
#include <proto/kernel.h>

        AROS_LH2(void, KrnRegisterClockSource,

/*  SYNOPSIS */
	AROS_LHA(APTR, CSBase, A0),
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

    /* HACK: for now we just use the given clocksource if we dont have one yet... */
    if ((!KernelBase->kb_ClockSource) && CSBase)
    {
        D(
            bug("[KRN] KrnRegisterClockSource: using ClockSource resource @ %p\n", CSBase);
          )

        KernelBase->kb_ClockUnit = AllocCSUnit(&KernCSNode);
        if (KernelBase->kb_ClockUnit != (IPTR)-1)
        {
            KernelBase->kb_ClockSource = CSBase;
            D(bug("[KRN] KrnRegisterClockSource: allocated unit %p for clocksource @ %p\n", KernelBase->kb_ClockUnit, KernelBase->kb_ClockSource);)
        }
    }

    return;

    AROS_LIBFUNC_EXIT
}
