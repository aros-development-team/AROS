/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    C11 function quick_exit().
*/

#include "__exitfunc.h"

/*****************************************************************************

    NAME */
#include <stdlib.h>

        void quick_exit (

/*  SYNOPSIS */
        int status)

/*  FUNCTION
        Cause normal-ish program termination: the functions registered with
        at_quick_exit() are called (in reverse order of registration), then
        control is handed to _Exit(status).

    INPUTS
        status - the exit status reported to the host.

    RESULT
        This function does not return.

    NOTES
        Unlike exit(), quick_exit() does not call atexit() handlers and does
        not flush or close C streams (it terminates via _Exit()).

    EXAMPLE

    BUGS

    SEE ALSO
        at_quick_exit(), exit(), _Exit()

    INTERNALS

******************************************************************************/
{
    __call_quick_exit_funcs();

    _Exit(status);
} /* quick_exit */
