/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction led - GetClass entry point
*/

#include <intuition/classes.h>
#include "led_intern.h"

/*****************************************************************************

    NAME */
#include <proto/led.h>

        AROS_LH0(Class *, LED_GetClass,

/*  LOCATION */
        struct Library *, LEDBase, 5, LED)

/*  FUNCTION
        Returns a pointer to the led BOOPSI class.

    RESULT
        Pointer to the class, or NULL on failure.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    return (Class *)GM_CLASSPTR_FIELD(LEDBase);

    AROS_LIBFUNC_EXIT
}
