/*
    Copyright © 1995-2018, The AROS Development Team. All rights reserved.
    $Id$
*/

/* The functions of fenv.h may be static inline functions which may not be
 * available for the shared library. Which of the functions may depend
 * on cpu arch.
 * Add some stub function to make them available. In stdc.conf the
 * function name with _ prepended will be used.
 */ 

#include "__stdc_intbase.h"
#include <fenv.h>

void *__stdc_get_fe_dfl(void)
{
    struct StdCIntBase *_StdCBase = (struct StdCIntBase *)__aros_getbase_StdCBase();
    return (void *)_StdCBase->__fe_dfl_env;
}

void *__stdc_get_fe_nom(void)
{
    struct StdCIntBase *_StdCBase = (struct StdCIntBase *)__aros_getbase_StdCBase();
    return (void *)_StdCBase->__fe_nomask_env;
}

int __stdc_get_fe_round(void)
{
    struct StdCIntBase *_StdCBase = (struct StdCIntBase *)__aros_getbase_StdCBase();
    return _StdCBase->__fe_round;
}

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


