/*
    Copyright (C) 2012-2023, The AROS Development Team. All rights reserved.

    Desc: AROS specific function for environ emulation handling
*/

#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <dos/dos.h>

#include "__stdcio_intbase.h"
#include "__env.h"

#include <stdio.h>

/*****************************************************************************

    NAME */
#include <stdlib.h>

        int __stdcio_set_environptr (

/*  SYNOPSIS */
        char ***environptr)

/*  FUNCTION
        This function is called to enable environ emulation mode.

    INPUTS
        environptr - ptr to the child environ variable (== &environ).

    RESULT
        0 on fail, other value on success

    NOTES
        This function will enable environ emulation. This means that
        all current DOS local variables are converted to the 'var=value'
        format and be accessible through char **environ.

    EXAMPLE

    BUGS
        At the moment only a static list is supported. getenv() and setenv()
        don't use this yet so changes done with these functions are not
        reflected in environ.
        This is still TODO.

    SEE ALSO
        __stdcio_get_environptr(), __stdcio_getenv(), setenv()

    INTERNALS

******************************************************************************/
{
    struct StdCIOIntBase *StdCIOBase =
        (struct StdCIOIntBase *)__aros_getbase_StdCIOBase();
    int len;

    D(
        bug("[stdcio] %s(0x%p)\n", __func__, environptr);
        bug("[stdcio] %s: Initializing environ emulation\n", __func__);
    )

    StdCIOBase->environptr = environptr;

    len = __env_get_environ(NULL, 0);
    *environptr = malloc(len);
    return (__env_get_environ(*environptr, len) >= 0);
}

/*****************************************************************************

    NAME */
#include <stdlib.h>

        char ***__stdcio_get_environptr (

/*  SYNOPSIS */
        void)

/*  FUNCTION
        This function the get pointer to the child environ global variable
        currently used by posixc.library.

    INPUTS
        -

    RESULT
        environptr - ptr to the child environ variable (== &environ).
                     NULL is return if envirion emulation is disabled.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        __stdcio_set_environptr()

    INTERNALS

******************************************************************************/
{
    struct StdCIOIntBase *StdCIOBase =
        (struct StdCIOIntBase *)__aros_getbase_StdCIOBase();

    return StdCIOBase->environptr;
}
