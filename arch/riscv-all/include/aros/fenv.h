/*
    Copyright (C) 2023, The AROS Development Team. All rights reserved.

    Desc: FPU-specific definitions for risc-v processors
*/

#ifndef	_FENV_H_
#define	_FENV_H_

#include <aros/system.h>
#include <aros/types/int_t.h>

#ifndef	__fenv_static
#define	__fenv_static	static
#endif

typedef	uint64_t	fenv_t;
typedef	uint64_t	fexcept_t;

/* Exception flags */
#define	FE_INVALID	0x0010
#define	FE_DIVBYZERO	0x0008
#define	FE_OVERFLOW	0x0004
#define	FE_UNDERFLOW	0x0002
#define	FE_INEXACT	0x0001
#define	FE_ALL_EXCEPT	(FE_DIVBYZERO | FE_INEXACT | \
			 FE_INVALID | FE_OVERFLOW | FE_UNDERFLOW)

/*
 * RISC-V Rounding modes
 */
#define	_ROUND_SHIFT	5
#define	FE_TONEAREST	(0x00 << _ROUND_SHIFT)
#define	FE_TOWARDZERO	(0x01 << _ROUND_SHIFT)
#define	FE_DOWNWARD	(0x02 << _ROUND_SHIFT)
#define	FE_UPWARD	(0x03 << _ROUND_SHIFT)
#define	_ROUND_MASK	(FE_TONEAREST | FE_DOWNWARD | \
			 FE_UPWARD | FE_TOWARDZERO)

__BEGIN_DECLS

/* Default floating-point environment */
extern const fenv_t	__fe_dfl_env;
#define	FE_DFL_ENV	(&__fe_dfl_env)

#if !defined(__riscv_float_abi_soft) && !defined(__riscv_float_abi_double)
#if defined(__riscv_float_abi_single)
#error single precision floating point ABI not supported
#else
#error compiler did not set soft/hard float macros
#endif
#endif

#ifndef __riscv_float_abi_soft
#define	__rfs(__fcsr)	__asm __volatile("csrr %0, fcsr" : "=r" (__fcsr))
#define	__wfs(__fcsr)	__asm __volatile("csrw fcsr, %0" :: "r" (__fcsr))
#endif

#ifdef __riscv_float_abi_soft

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
#else
__fenv_static inline int
feclearexcept(int __excepts)
{

	__asm __volatile("csrc fflags, %0" :: "r"(__excepts));

	return (0);
}

__fenv_static inline int
fegetexceptflag(fexcept_t *__flagp, int __excepts)
{
	fexcept_t __fcsr;

	__rfs(__fcsr);
	*__flagp = __fcsr & __excepts;

	return (0);
}

__fenv_static inline int
fesetexceptflag(const fexcept_t *__flagp, int __excepts)
{
	fexcept_t __fcsr;

	__fcsr = *__flagp;
	__asm __volatile("csrc fflags, %0" :: "r"(__excepts));
	__asm __volatile("csrs fflags, %0" :: "r"(__fcsr & __excepts));

	return (0);
}

__fenv_static inline int
feraiseexcept(int __excepts)
{

	__asm __volatile("csrs fflags, %0" :: "r"(__excepts));

	return (0);
}

__fenv_static inline int
fetestexcept(int __excepts)
{
	fexcept_t __fcsr;

	__rfs(__fcsr);

	return (__fcsr & __excepts);
}

__fenv_static inline int
fegetround(void)
{
	fexcept_t __fcsr;

	__rfs(__fcsr);

	return (__fcsr & _ROUND_MASK);
}

__fenv_static inline int
fesetround(int __round)
{
	fexcept_t __fcsr;

	if (__round & ~_ROUND_MASK)
		return (-1);

	__rfs(__fcsr);
	__fcsr &= ~_ROUND_MASK;
	__fcsr |= __round;
	__wfs(__fcsr);

	return (0);
}

__fenv_static inline int
fegetenv(fenv_t *__envp)
{

	__rfs(*__envp);

	return (0);
}

__fenv_static inline int
feholdexcept(fenv_t *__envp __unused)
{

	/* No exception traps. */

	return (-1);
}

__fenv_static inline int
fesetenv(const fenv_t *__envp)
{

	__wfs(*__envp);

	return (0);
}

__fenv_static inline int
feupdateenv(const fenv_t *__envp)
{
	fexcept_t __fcsr;

	__rfs(__fcsr);
	__wfs(*__envp);
	feraiseexcept(__fcsr & FE_ALL_EXCEPT);

	return (0);
}
#endif /* !__riscv_float_abi_soft */

#if __BSD_VISIBLE

/* We currently provide no external definitions of the functions below. */

#ifdef __riscv_float_abi_soft
int feenableexcept(int __mask);
int fedisableexcept(int __mask);
int fegetexcept(void);
#else
static inline int
feenableexcept(int __mask __unused)
{

	/* No exception traps. */

	return (0);
}

static inline int
fedisableexcept(int __mask __unused)
{

	/* No exception traps. */

	return (0);
}

static inline int
fegetexcept(void)
{

	/* No exception traps. */

	return (0);
}
#endif /* !__riscv_float_abi_soft */

#endif /* __BSD_VISIBLE */

__END_DECLS

#endif	/* !_FENV_H_ */
