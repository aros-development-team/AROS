/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$

    Desc: console.device function RawKeyConvert()
    Lang: english
*/

#include <exec/libraries.h>
#include <devices/inputevent.h>
#include <devices/keymap.h>
#include <proto/console.h>


/*****************************************************************************

    NAME */
#include <devices/keymap.h>
#include <proto/console.h>
#include <proto/keymap.h>
#include "console_gcc.h"

        AROS_LH4(LONG, RawKeyConvert,

/*  SYNOPSIS */
        AROS_LHA(struct InputEvent *, events, A0),
        AROS_LHA(STRPTR, buffer, A1),
        AROS_LHA(LONG, length, D1), AROS_LHA(struct KeyMap *, keyMap, A2),

/*  LOCATION */
        struct Library *, ConsoleDevice, 8, Console)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    return MapRawKey(events, buffer, length, keyMap);

    AROS_LIBFUNC_EXIT
} /* RawKeyConvert */
