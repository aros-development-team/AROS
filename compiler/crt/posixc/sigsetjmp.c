/*****************************************************************************
    NAME
        int sigsetjmp(

    SYNOPSIS
        sigjmp_buf env, int val)

    FUNCTION
        Save a calling environment for a later return with siglongjmp().

    INPUTS
        env - Writable sigjmp_buf object with its declared alignment. Keep it
            intact and alive while a later jump can use it.
        val - Signal-mask save request, conventionally named savemask.
            Currently ignored, including when nonzero.

    RESULT
        Zero initially. After siglongjmp(), the value supplied to that call,
        with zero replaced by 1.

    NOTES
        Include <setjmp.h>. Pair this call with siglongjmp(), not longjmp().
        The lifetime, thread, expression and local-variable restrictions of
        stdc/setjmp() also apply. This call saves CPU context, not a copy of
        program memory or resource ownership.

    BUGS
        The AROS implementations do not save a signal mask, and siglongjmp()
        does not restore one. A nonzero val therefore does not provide the
        POSIX signal-mask preservation contract. Do not use this pair where
        correctness depends on restoring the mask after a jump.

    SEE ALSO
        siglongjmp(), stdc/setjmp(), stdc/longjmp()

*****************************************************************************/

#error sigsetjmp has to be implemented for each cpu
