/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    C11 function at_quick_exit().
*/

#include <stdlib.h>

#include "__exitfunc.h"

/*****************************************************************************

    NAME */
#include <stdlib.h>

        int at_quick_exit (

/*  SYNOPSIS */
        void (*func)(void))

/*  FUNCTION
        Register a function to be called, without arguments, by quick_exit().
        Registered functions are called in the reverse order of registration.

    INPUTS
        func - the function to register.

    RESULT
        0 on success, non-zero on failure.

    NOTES
        Handlers registered with at_quick_exit() are independent of those
        registered with atexit(); only quick_exit() calls them.

    EXAMPLE

    BUGS

    SEE ALSO
        quick_exit(), atexit()

    INTERNALS

******************************************************************************/
{
    struct AtExitNode *aen = malloc(sizeof(*aen));

    if (!aen) return -1;

    aen->node.ln_Type = AEN_VOID;
    aen->func.fn = func;

    return __add_quick_exit_func(aen);
} /* at_quick_exit */
