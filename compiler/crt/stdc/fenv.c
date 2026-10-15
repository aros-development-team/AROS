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

/****************************************************************************
    NAME
        int fegetexceptflag(

    SYNOPSIS
        fexcept_t *flagp, int excepts)

    FUNCTION
        Save the selected floating-point exception flags without
        clearing them.

    INPUTS
        flagp - Non-NULL pointer to writable storage for a fexcept_t.
        excepts - Zero, a supported FE_* exception constant, or their
                  bitwise OR. FE_ALL_EXCEPT selects all supported flags.

    RESULT
        Zero on success; -1 in the generic fallback. The saved
        representation is written to *flagp. See BUGS for stub behavior.

    NOTES
        Include <fenv.h>. Pass the saved value to fesetexceptflag()
        with a mask containing only flags included in this call.
        The representation is architecture-dependent.

        On x86, combines x87 and available SSE flags. The saved value
        does not preserve which unit reported each flag.

    BUGS
        On AArch64, ARM soft-float and RISC-V soft-float, writes zero
        to *flagp and returns zero without reading exception state.

        On m68k, only a software environment is accessed; hardware
        floating-point exceptions are not tracked.

        The generic fallback returns -1 without accessing *flagp.

    SEE ALSO
        fesetexceptflag(), fetestexcept()

*****************************************************************************/
int fegetexceptflag(fexcept_t *flagp, int excepts) {
    AROS_FUNCTION_NOT_IMPLEMENTED("stdc");
    return -1;
}

/****************************************************************************
    NAME
        int fesetexceptflag(

    SYNOPSIS
        const fexcept_t *flagp, int excepts)

    FUNCTION
        Restore selected floating-point exception flags from a
        saved representation.

    INPUTS
        flagp - Non-NULL pointer to a value saved by fegetexceptflag().
        excepts - Zero, a supported FE_* exception constant, or their
                  bitwise OR. FE_ALL_EXCEPT selects all supported flags.
                  Select only flags included when the value was saved.

    RESULT
        Zero on success; -1 in the generic fallback. See BUGS for
        implementations that return zero without restoring flags.

    NOTES
        Include <fenv.h>. Replaces selected flags with their saved
        values. Unselected flags are preserved, except as noted below.
        This function restores flag state rather than performing the
        exceptional operations used by feraiseexcept().

        On x86, installs the selected saved flags in both x87 and
        available SSE state. It does not restore their original
        distribution between the two units.

    BUGS
        On PowerPC, selecting FE_INVALID also replaces every other
        flag in FE_ALL_EXCEPT. Flags outside the requested selection
        can therefore be cleared or changed.

        On AArch64, ARM soft-float and RISC-V soft-float, returns zero
        without reading *flagp or changing exception state.

        On m68k, only a software environment is accessed; hardware
        floating-point exceptions are not tracked.

        The generic fallback returns -1 without accessing *flagp.

    SEE ALSO
        fegetexceptflag(), feraiseexcept()

*****************************************************************************/
int fesetexceptflag(const fexcept_t *flagp, int excepts) {
    AROS_FUNCTION_NOT_IMPLEMENTED("stdc");
    return -1;
}

/****************************************************************************
    NAME
        int feraiseexcept(

    SYNOPSIS
        int excepts)

    FUNCTION
        Request the selected floating-point exceptions.

    INPUTS
        excepts - Zero, a supported FE_* exception constant, or their
                  bitwise OR. FE_ALL_EXCEPT selects all supported
                  exceptions.

    RESULT
        Zero on completion in the architecture-specific implementations;
        -1 in the generic fallback. See BUGS: zero does not establish
        that an exception was raised.

    NOTES
        Include <fenv.h>. Enabled floating-point traps may prevent
        normal completion. Do not rely on a particular delivery order
        when requesting several exceptions.

        On x86, sets the selected x87 and available SSE flags, then
        waits for pending x87 exceptions. Unselected flags are retained.

    BUGS
        On AArch64, ARM soft-float and RISC-V soft-float, returns zero
        without changing exception state.

        On m68k, does not set exception flags. A selection containing
        FE_OVERFLOW or FE_UNDERFLOW sets errno to ERANGE; any other
        nonzero selection sets errno to EDOM. Zero leaves errno unchanged.
        The function returns zero in each case.

        ARM VFP and RISC-V hardware-float paths only set exception
        status flags.

        The generic fallback is unimplemented and returns -1.

    SEE ALSO
        fetestexcept(), feclearexcept(), fesetexceptflag()

*****************************************************************************/
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

