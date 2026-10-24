/*****************************************************************************
    NAME
        void longjmp(

    SYNOPSIS
        jmp_buf env, int val)

    FUNCTION
        Resume execution at the environment most recently saved in env.

    INPUTS
        env - Intact jmp_buf initialized by setjmp() in the current thread.
            Its saving function and the relevant variable-length scopes must
            still be active.
        val - Value returned by the resumed setjmp(). Zero is converted to 1.

    RESULT
        Does not return to its caller.

    NOTES
        Restores the saved stack pointer, return address and CPU-specific
        register state. It does not run cleanup code in skipped functions,
        release locks, close files or free heap allocations. Arrange cleanup
        before jumping or in the resumed path.

        Use only an environment saved by setjmp(), with the same target ABI.
        Invalid, expired or cross-thread environments are not diagnosed before
        restoring the stack and transferring control. See setjmp() for the
        rules governing local variables and permitted save expressions.

        No signal mask is restored. Do not rely on this operation to restore
        floating-point control or exception state.

    SEE ALSO
        setjmp(), posixc/siglongjmp()

*****************************************************************************/

#error longjmp has to be implemented for each cpu
