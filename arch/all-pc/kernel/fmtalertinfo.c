/*
    Copyright (C) 2023, The AROS Development Team. All rights reserved.

    Desc: x86_64 arch specific KrnFmtAlertInfo over-ride
*/

#include <aros/kernel.h>

#include <kernel_base.h>
#include <kernel_debug.h>

static const char irqstring[] =   "IRQ : #%U - 0x%P";

/*****************************************************************************

    NAME */
#include <proto/kernel.h>

        AROS_LH2(void, KrnFmtAlertInfo,

/*  SYNOPSIS */
        AROS_LHA(STRPTR *, TemplatePtr, A0),
        AROS_LHA(IPTR *, ParamPtr, A1),

/*  LOCATION */
        struct KernelBase *, KernelBase, 65, Kernel)

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

    if (TemplatePtr && ParamPtr) {
        struct PlatformData *pdata;
        IPTR                        *params = *ParamPtr;

        if ((pdata = (struct PlatformData *)KernelBase->kb_PlatformData) != NULL)
        {
            if (pdata->kb_LastState & (PLATFORMF_INIRQ << 16))
            {
                /* Crashed inside an interrupt handler ... */
                *TemplatePtr = (STRPTR)irqstring;
                params[0] = (IPTR)pdata->kb_LastState & 0xFF;
                params[1] = (IPTR)pdata->kb_LastIntr;
            }
        }
    }
    return;

    AROS_LIBFUNC_EXIT
}
