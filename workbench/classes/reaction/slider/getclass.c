/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction slider - GetClass entry point
*/

#include <intuition/classes.h>
#include "slider_intern.h"

/*****************************************************************************

    NAME */
#include <proto/slider.h>

        AROS_LH0(Class *, SLIDER_GetClass,

/*  LOCATION */
        struct Library *, SliderBase, 5, Slider)

/*  FUNCTION
        Returns a pointer to the slider BOOPSI class.

    RESULT
        Pointer to the class, or NULL on failure.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    return (Class *)GM_CLASSPTR_FIELD(SliderBase);

    AROS_LIBFUNC_EXIT
}
