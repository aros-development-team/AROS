/*
    Copyright © 2007, The AROS Development Team. All rights reserved.
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

/* otherwise just use the stub implementation */
#elif !defined _FENV_H_
#define _FENV_H_

#include <sys/cdefs.h>
#include <sys/_types.h>

typedef	__uint32_t	fenv_t;
typedef	__uint32_t	fexcept_t;

#define FE_ALL_EXCEPT   0

__BEGIN_DECLS

extern const fenv_t	__fe_dfl_env;
#define	FE_DFL_ENV	(&__fe_dfl_env)

int feclearexcept(int excepts);
int fegetexceptflag(fexcept_t *flagp, int excepts);
int fesetexceptflag(const fexcept_t *flagp, int excepts);
int feraiseexcept(int excepts);
int fetestexcept(int excepts);
int fegetround(void);
int fesetround(int round);
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
