/*
    Copyright © 2007, The AROS Development Team. All rights reserved.
    $Id$
  
    C99 floating-point environment
*/

/* 
 * The implementation of these functions are architecture-specific, and so are
 * merely stubs here, provided to allow linking to happen correctly.
 */

#include <aros/debug.h>

#include <fenv.h>

const fenv_t __fe_dfl_env = 0;

int feclearexcept(int excepts) {
    AROS_FUNCTION_NOT_IMPLEMENTED("arosm");
    return -1;
}

int fegetexceptflag(fexcept_t *flagp, int excepts) {
    AROS_FUNCTION_NOT_IMPLEMENTED("arosm");
    return -1;
}

int fesetexceptflag(const fexcept_t *flagp, int excepts) {
    AROS_FUNCTION_NOT_IMPLEMENTED("arosm");
    return -1;
}

int feraiseexcept(int excepts) {
    AROS_FUNCTION_NOT_IMPLEMENTED("arosm");
    return -1;
}

int fetestexcept(int excepts) {
    AROS_FUNCTION_NOT_IMPLEMENTED("arosm");
    return -1;
}

int fegetround(void) {
    AROS_FUNCTION_NOT_IMPLEMENTED("arosm");
    return -1;
}

int fesetround(int round) {
    AROS_FUNCTION_NOT_IMPLEMENTED("arosm");
    return -1;
}

int fegetenv(fenv_t *envp) {
    AROS_FUNCTION_NOT_IMPLEMENTED("arosm");
    return -1;
}

int feholdexcept(fenv_t *envp) {
    AROS_FUNCTION_NOT_IMPLEMENTED("arosm");
    return -1;
}

int fesetenv(const fenv_t *envp) {
    AROS_FUNCTION_NOT_IMPLEMENTED("arosm");
    return -1;
}

int feupdateenv(const fenv_t *envp) {
    AROS_FUNCTION_NOT_IMPLEMENTED("arosm");
    return -1;
}

int feenableexcept(int mask) {
    AROS_FUNCTION_NOT_IMPLEMENTED("arosm");
    return -1;
}

int fedisableexcept(int mask) {
    AROS_FUNCTION_NOT_IMPLEMENTED("arosm");
    return -1;
}

int fegetexcept(void) {
    AROS_FUNCTION_NOT_IMPLEMENTED("arosm");
    return -1;
}

