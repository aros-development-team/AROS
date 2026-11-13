/*
    Copyright (C) 2002-2026, The AROS Development Team. All rights reserved.
*/

#include <proto/muimaster.h>
#include "muimaster_intern.h"

/*****************************************************************************

    NAME */
        AROS_LH1(LONG, MUI_SetError,

/*  SYNOPSIS */
        AROS_LHA(LONG, num, D0),

/*  LOCATION */
        struct Library *, MUIMasterBase, 12, MUIMaster)

/*  FUNCTION
        Return zero without storing an error value. This obsolete
        compatibility entrypoint does not call dos.library/SetIoErr().

    INPUTS
        num - Ignored.

    RESULT
        Always zero. This is not the previous error value.

    NOTES
        Calling this function does not change the result of MUI_Error()
        or the process's DOS error value.

    SEE ALSO
        MUI_Error(), dos.library/SetIoErr()

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    return 0;

    AROS_LIBFUNC_EXIT

} /* MUIA_SetError */
