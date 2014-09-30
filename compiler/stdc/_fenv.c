/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

/* The functions of fenv.h may be static inline functions which may not be
 * available for the shared library. Which of the functions may depend
 * on cpu arch.
 * Add some stub function to make them available. In stdc.conf the
 * function name with _ prepended will be used.
 */ 

#include <fenv.h>

int _feclearexcept(int excepts)
{
    return feclearexcept(excepts);
}

int _fegetexceptflag(fexcept_t *flagp, int excepts) {
    return fegetexceptflag(flagp, excepts);
}

int _fesetexceptflag(const fexcept_t *flagp, int excepts) {
    return fesetexceptflag(flagp, excepts);
}

int _feraiseexcept(int excepts) {
    return feraiseexcept(excepts);
}

int _fetestexcept(int excepts) {
    return fetestexcept(excepts);
}

int _fegetround(void) {
    return fegetround();
}

int _fesetround(int round) {
    return fesetround(round);
}

int _fegetenv(fenv_t *envp) {
    return fegetenv(envp);
}

int _feholdexcept(fenv_t *envp) {
    return feholdexcept(envp);
}

int _fesetenv(const fenv_t *envp) {
    return fesetenv(envp);
}

int _feupdateenv(const fenv_t *envp) {
    return feupdateenv(envp);
}


