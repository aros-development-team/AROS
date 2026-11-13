/*
    Copyright (C) 2002-2026, The AROS Development Team. All rights reserved.
*/

#include <proto/muimaster.h>
#include "muimaster_intern.h"

/*****************************************************************************

    NAME */
        AROS_LH0(LONG, MUI_Error,

/*  SYNOPSIS */

/*  LOCATION */
        struct Library *, MUIMasterBase, 11, MUIMaster)

/*  FUNCTION
        Return zero. This obsolete compatibility entrypoint does not read
        an error variable or call dos.library/IoErr().

    INPUTS
        None.

    RESULT
        Always zero, regardless of preceding MUI calls.

    NOTES
        Do not use this return value to determine whether another call
        succeeded. Check that call's documented return value and error
        interface. IoErr() is relevant only when that interface uses it.

    SEE ALSO
        MUI_SetError(), dos.library/IoErr()

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    return 0;

    AROS_LIBFUNC_EXIT

} /* MUIA_Error */
