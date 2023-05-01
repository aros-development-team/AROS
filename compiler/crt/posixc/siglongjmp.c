/*
    Copyright (C) 2015, The AROS Development Team. All rights reserved.

    Desc: POSIX.1-2008 function siglongjmp()
*/

/******************************************************************************

    NAME
#include <setjmp.h>

        void siglongjmp (

/*  SYNOPSIS
        jmp_buf env,
        int val)

    FUNCTION
        Save the current context so that you can return to it later.

    INPUTS
        env - The context/environment to restore
        val - This value is returned by setjmp() when you return to the
                saved context. You cannot return 0. If val is 0, then
                setjmp() returns with 1.

    RESULT
        This function doesn't return.

    NOTES

    EXAMPLE
        jmp_buf env;

        ... some code ...

        if (!setjmp (env))
        {
            ... this code is executed after setjmp() returns ...

            // This is no good example on how to use this function
            // You should not do that
            if (error)
                siglongjmp (env, 5);

            ... some code ...
        }
        else
        {
            ... this code is executed if you call siglongjmp(env) ...
        }

    BUGS

    SEE ALSO
        stdc/setjmp()

    INTERNALS

******************************************************************************/

#error siglongjmp has to be implemented for each cpu
