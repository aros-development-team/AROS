/*
    Copyright (C) 1995-2018, The AROS Development Team. All rights reserved.
*/

/* The functions of fenv.h may be static inline functions which may not be
 * available for the shared library. Which of the functions may depend
 * on cpu arch.
 * Add some stub function to make them available. In stdc.conf the
 * function name with _ prepended will be used.
 */

#include "__stdc_intbase.h"
#include <fenv.h>

/*****i**********************************************************************
    NAME
        void * __stdc_get_fe_dfl(

    SYNOPSIS
        void)

    FUNCTION
        Read the stored default environment reference from
        the current stdc.library base.

    INPUTS
        None. Requires a valid current stdc.library base.

    RESULT
        The stored opaque pointer, which may be NULL. No copy
        is created and no ownership is transferred.

    NOTES
        Internal softfloat/fenv support accessor, not an
        application API. The value is returned without validation
        or initialization.

        Interpret the reference only according to the platform
        fenv implementation. This call does not install an
        environment or establish that the reference is usable.

    SEE ALSO
        fesetenv()

*****************************************************************************/
void *__stdc_get_fe_dfl(void)
{
    struct StdCIntBase *_StdCBase = (struct StdCIntBase *)__aros_getbase_StdCBase();
    return (void *)_StdCBase->__fe_dfl_env;
}

/*****i**********************************************************************
    NAME
        void * __stdc_get_fe_nom(

    SYNOPSIS
        void)

    FUNCTION
        Read the stored no-mask environment reference from
        the current stdc.library base.

    INPUTS
        None. Requires a valid current stdc.library base.

    RESULT
        The stored opaque pointer, which may be NULL. No copy
        is created and no ownership is transferred.

    NOTES
        Internal softfloat/fenv support accessor, not an
        application API. The value is returned without validation
        or initialization.

        Interpret the reference only according to the platform
        fenv implementation. This call does not install an
        environment or establish that the reference is usable.

    SEE ALSO
        fesetenv()

*****************************************************************************/
void *__stdc_get_fe_nom(void)
{
    struct StdCIntBase *_StdCBase = (struct StdCIntBase *)__aros_getbase_StdCBase();
    return (void *)_StdCBase->__fe_nomask_env;
}

/*****i**********************************************************************
    NAME
        int __stdc_get_fe_round(

    SYNOPSIS
        void)

    FUNCTION
        Read the stored rounding-mode value from the current
        stdc.library base.

    INPUTS
        None. Requires a valid current stdc.library base.

    RESULT
        The stored integer value. This is not a query of the
        processor rounding mode.

    NOTES
        Internal softfloat/fenv support accessor, not an
        application API. The value is returned without validation
        or initialization.

        The accessor does not synchronize the stored value with
        fegetround() or fesetround().

    SEE ALSO
        fegetround(), fesetround()

*****************************************************************************/
int __stdc_get_fe_round(void)
{
    struct StdCIntBase *_StdCBase = (struct StdCIntBase *)__aros_getbase_StdCBase();
    return _StdCBase->__fe_round;
}

/* Library-vector adapter for feclearexcept().
 * Passes arguments unchanged and returns the callee result.
 * The target feclearexcept() contract governs argument validity and effects.
 */
int _feclearexcept(int excepts)
{
    return feclearexcept(excepts);
}

/* Library-vector adapter for fegetexceptflag().
 * Passes arguments unchanged and returns the callee result.
 * The target fegetexceptflag() contract governs argument validity and effects.
 */
int _fegetexceptflag(fexcept_t *flagp, int excepts) {
    return fegetexceptflag(flagp, excepts);
}

/* Library-vector adapter for fesetexceptflag().
 * Passes arguments unchanged and returns the callee result.
 * The target fesetexceptflag() contract governs argument validity and effects.
 */
int _fesetexceptflag(const fexcept_t *flagp, int excepts) {
    return fesetexceptflag(flagp, excepts);
}

/* Library-vector adapter for feraiseexcept().
 * Passes arguments unchanged and returns the callee result.
 * The target feraiseexcept() contract governs argument validity and effects.
 */
int _feraiseexcept(int excepts) {
    return feraiseexcept(excepts);
}

/* Library-vector adapter for fetestexcept().
 * Passes arguments unchanged and returns the callee result.
 * The target fetestexcept() contract governs argument validity and effects.
 */
int _fetestexcept(int excepts) {
    return fetestexcept(excepts);
}

/* Library-vector adapter for fegetround().
 * Returns the callee result unchanged.
 * The target fegetround() contract governs rounding-mode reporting.
 */
int _fegetround(void) {
    return fegetround();
}

/* Library-vector adapter for fesetround().
 * Passes arguments unchanged and returns the callee result.
 * The target fesetround() contract governs argument validity and effects.
 */
int _fesetround(int round) {
    return fesetround(round);
}

/* Library-vector adapter for fegetenv().
 * Passes arguments unchanged and returns the callee result.
 * The target fegetenv() contract governs argument validity and effects.
 */
int _fegetenv(fenv_t *envp) {
    return fegetenv(envp);
}

/* Library-vector adapter for feholdexcept().
 * Passes arguments unchanged and returns the callee result.
 * The target feholdexcept() contract governs argument validity and effects.
 */
int _feholdexcept(fenv_t *envp) {
    return feholdexcept(envp);
}

/* Library-vector adapter for fesetenv().
 * Passes arguments unchanged and returns the callee result.
 * The target fesetenv() contract governs argument validity and effects.
 */
int _fesetenv(const fenv_t *envp) {
    return fesetenv(envp);
}

/* Library-vector adapter for feupdateenv().
 * Passes arguments unchanged and returns the callee result.
 * The target feupdateenv() contract governs argument validity and effects.
 */
int _feupdateenv(const fenv_t *envp) {
    return feupdateenv(envp);
}


