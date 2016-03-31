#ifndef	AROS_AARCH64_FENV_H
#define	AROS_AARCH64_FENV_H
/*
    Copyright © 2016, The AROS Development Team. All rights reserved.
    $Id$
 */

#include <aros/system.h>
#include <aros/types/int_t.h>

typedef	uint32_t	fenv_t;
typedef	uint32_t	fexcept_t;

/* Exception flags */
#define	FE_INVALID	0x0001
#define	FE_DIVBYZERO	0x0002
#define	FE_OVERFLOW	0x0004
#define	FE_UNDERFLOW	0x0008
#define	FE_INEXACT	0x0010
#define FE_DENORMAL     0x0080
#define	FE_ALL_EXCEPT	(FE_DIVBYZERO | FE_INEXACT | FE_INVALID | FE_OVERFLOW | FE_UNDERFLOW | FE_DENORMAL)

/* Rounding modes */
#define	FE_TONEAREST	0x00000000
#define	FE_TOWARDZERO	0x00c00000
#define	FE_UPWARD       0x00400000
#define	FE_DOWNWARD     0x00800000
#define	_ROUND_MASK	(FE_TONEAREST | FE_DOWNWARD | FE_UPWARD | FE_TOWARDZERO)

__BEGIN_DECLS

/* Default floating-point environment */
extern const fenv_t	__fe_dfl_env;
#define	FE_DFL_ENV	(&__fe_dfl_env)

#ifndef STDC_NOINLINE
static __inline int feclearexcept(int __excepts)
{
    return 0;
}

static __inline int fegetexceptflag(fexcept_t *__flagp, int __excepts)
{
    *__flagp = 0;
    return 0;
}

static __inline int fesetexceptflag(const fexcept_t *__flagp, int __excepts)
{
    return 0;
}

static __inline int feraiseexcept(int __excepts)
{
    return 0;
}

static __inline int fetestexcept(int __excepts)
{
    return 0;
}

static __inline int fegetround(void)
{
    return -1;
}

static __inline int fesetround(int __round)
{
    return -1;
}

static __inline int fegetenv(fenv_t *__envp)
{
    return 0;
}

static __inline int feholdexcept(fenv_t *__envp)
{
    *__envp = 0;
    return 0;
}

static __inline int fesetenv(const fenv_t *__envp)
{
    return 0;
}

static __inline int feupdateenv(const fenv_t *__envp)
{
    return 0;
}
#endif /* !STDC_NOINLINE */

#if __BSD_VISIBLE
#ifndef STDC_NOINLINE
static __inline int feenableexcept(int __mask)
{
    return 0;
}

static __inline int fedisableexcept(int __mask)
{
    return 0;
}

static __inline int fegetexcept(void)
{
    return 0;
}
#endif /* !STDC_NOINLINE */

#endif /* __BSD_VISIBLE */

__END_DECLS

#endif	/* AROS_AARCH64_FENV_H */
