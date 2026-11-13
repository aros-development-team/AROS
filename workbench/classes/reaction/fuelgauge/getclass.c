/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction fuelgauge - GetClass entry point
*/

#include <intuition/classes.h>
#include "fuelgauge_intern.h"

/*****************************************************************************

    NAME */
#include <proto/fuelgauge.h>

        AROS_LH0(Class *, FUELGAUGE_GetClass,

/*  LOCATION */
        struct Library *, FuelGaugeBase, 5, FuelGauge)

/*  FUNCTION
        Returns a pointer to the fuelgauge BOOPSI class.

    RESULT
        Pointer to the class, or NULL on failure.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    return (Class *)GM_CLASSPTR_FIELD(FuelGaugeBase);

    AROS_LIBFUNC_EXIT
}
