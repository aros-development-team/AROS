/*****************************************************************************
    NAME
        int setjmp(

    SYNOPSIS
        jmp_buf env)

    FUNCTION
        Save the calling environment for a later nonlocal return with longjmp().

    INPUTS
        env - Writable jmp_buf object. Keep it intact and alive until no further
            jump will use it. A new save replaces the previous environment.

    RESULT
        Zero on the initial return. After longjmp(), its nonzero val argument,
        or 1 when val was zero.

    NOTES
        Include <setjmp.h> and use jmp_buf with its declared alignment.
        The saved register and stack layout depends on the target CPU.

        Use setjmp() as a complete statement, a complete if/while condition,
        or in such a condition with unary ! or comparison against an integer
        constant expression. Do not assign its result to a variable or embed
        it in another expression.

        Only jump back while the saving function is still active in the same
        thread. Do not leave and then re-enter the scope of a variably modified
        object this way. Changed non-volatile automatic locals in the saving
        function have indeterminate values after the jump.

        This is not a snapshot of memory, files or acquired resources. Floating-
        point state saved by the CPU implementation varies; do not use this
        interface to save or restore a floating-point environment. No signal
        mask is saved.

    SEE ALSO
        longjmp(), posixc/sigsetjmp()

*****************************************************************************/

#error setjmp has to be implemented for each cpu
