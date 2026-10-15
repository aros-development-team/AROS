/*
    Copyright (C) 2007-2013, The AROS Development Team. All rights reserved.
  
    C99 floating-point environment
*/

/*
 * The implementation of these functions are architecture-specific, and so are
 * merely stubs here, provided to allow linking to happen correctly.
 */

#include <aros/debug.h>

/* Include fenv_t.h directly as including <fenv.h> may cause conflicts
 * cpu specific static inline version of these functions.
 */
#include <aros/types/fenv_t.h>

const fenv_t __fe_dfl_env = 0;

/****************************************************************************
    NAME
        int feclearexcept(

    SYNOPSIS
        int excepts)

    FUNCTION
        Clear the selected floating-point exception flags.

    INPUTS
        excepts - Zero, a supported FE_* exception constant, or a
                  bitwise OR of those constants. Use FE_ALL_EXCEPT
                  to select every supported exception.

    RESULT
        Zero on success; -1 in the generic fallback. See BUGS for
        implementations that report success without clearing flags.

    NOTES
        Include <fenv.h>. Clearing flags does not disable exception
        traps.

        On x86, clears selected flags in x87 and, when available, SSE.
        On PowerPC, selecting FE_INVALID also clears its associated
        invalid-operation cause flags.

    BUGS
        On AArch64, ARM soft-float and RISC-V soft-float, the function
        returns zero without accessing exception flags.

        On m68k, only a software environment is accessed; hardware
        floating-point exceptions are not tracked.

        The generic fallback is unimplemented and returns -1.

    SEE ALSO
        fetestexcept(), feraiseexcept()

*****************************************************************************/
int feclearexcept(int excepts) {
    AROS_FUNCTION_NOT_IMPLEMENTED("stdc");
    return -1;
}

int fegetexceptflag(fexcept_t *flagp, int excepts) {
    AROS_FUNCTION_NOT_IMPLEMENTED("stdc");
    return -1;
}

int fesetexceptflag(const fexcept_t *flagp, int excepts) {
    AROS_FUNCTION_NOT_IMPLEMENTED("stdc");
    return -1;
}

int feraiseexcept(int excepts) {
    AROS_FUNCTION_NOT_IMPLEMENTED("stdc");
    return -1;
}

/****************************************************************************
    NAME
        int fetestexcept(

    SYNOPSIS
        int excepts)

    FUNCTION
        Test the selected floating-point exception flags without
        clearing them.

    INPUTS
        excepts - Zero, a supported FE_* exception constant, or a
                  bitwise OR of those constants. Use FE_ALL_EXCEPT
                  to select every supported exception.

    RESULT
        The bitwise OR of the selected flags that are set, or zero
        if none are set. The generic fallback returns -1. See BUGS
        before treating a zero result as evidence of clear flags.

    NOTES
        Include <fenv.h>. On x86, reports a flag if it is set in
        either x87 or, when available, SSE. The result does not
        identify which unit raised it.

    BUGS
        On AArch64, ARM soft-float and RISC-V soft-float, the function
        returns zero without accessing exception flags.

        On m68k, only a software environment is accessed; hardware
        floating-point exceptions are not tracked.

        The generic fallback is unimplemented and returns -1.

    SEE ALSO
        feclearexcept(), feraiseexcept()

*****************************************************************************/
int fetestexcept(int excepts) {
    AROS_FUNCTION_NOT_IMPLEMENTED("stdc");
    return -1;
}

int fegetround(void) {
    AROS_FUNCTION_NOT_IMPLEMENTED("stdc");
    return -1;
}

int fesetround(int round) {
    AROS_FUNCTION_NOT_IMPLEMENTED("stdc");
    return -1;
}

int fegetenv(fenv_t *envp) {
    AROS_FUNCTION_NOT_IMPLEMENTED("stdc");
    return -1;
}

int feholdexcept(fenv_t *envp) {
    AROS_FUNCTION_NOT_IMPLEMENTED("stdc");
    return -1;
}

int fesetenv(const fenv_t *envp) {
    AROS_FUNCTION_NOT_IMPLEMENTED("stdc");
    return -1;
}

int feupdateenv(const fenv_t *envp) {
    AROS_FUNCTION_NOT_IMPLEMENTED("stdc");
    return -1;
}

int feenableexcept(int mask) {
    AROS_FUNCTION_NOT_IMPLEMENTED("stdc");
    return -1;
}

int fedisableexcept(int mask) {
    AROS_FUNCTION_NOT_IMPLEMENTED("stdc");
    return -1;
}

int fegetexcept(void) {
    AROS_FUNCTION_NOT_IMPLEMENTED("stdc");
    return -1;
}

