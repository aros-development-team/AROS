/*
    Copyright � 2007-2012, The AROS Development Team. All rights reserved.
    $Id$

    C99 floating-point environment
*/

/* Use architecture-specific implementation if available */
#if defined __i386__
#   include <aros/i386/fenv.h>
#elif defined __x86_64__
#   include <aros/x86_64/fenv.h>
#elif defined __powerpc__
#   include <aros/ppc/fenv.h>
#elif defined __arm__
#   include <aros/arm/fenv.h>
#elif defined __mc68000__
#   include <aros/m68k/fenv.h>

/* otherwise just use the stub implementation */
#elif !defined _STDC_FENV_H_
#define _STDC_FENV_H_

#include <aros/system.h>
#include <aros/types/fenv_t.h>

#define FE_ALL_EXCEPT   0

__BEGIN_DECLS

extern const fenv_t	__fe_dfl_env;
#define	FE_DFL_ENV	(&__fe_dfl_env)

/* Floating-point exceptions */
int feclearexcept(int excepts);
int fegetexceptflag(fexcept_t *flagp, int excepts);
int feraiseexcept(int excepts);
int fesetexceptflag(const fexcept_t *flagp, int excepts);
int fetestexcept(int excepts);

/* Rounding */
int fegetround(void);
int fesetround(int round);

/* Environment */
int fegetenv(fenv_t *envp);
int feholdexcept(fenv_t *envp);
int fesetenv(const fenv_t *envp);
int feupdateenv(const fenv_t *envp);

#if __BSD_VISIBLE
int feenableexcept(int mask);
int fedisableexcept(int mask);
int fegetexcept(void);
#endif

__END_DECLS

#endif
