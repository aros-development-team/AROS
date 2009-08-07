/*-
 * Copyright (c) 2004-2005 David Schultz <das@FreeBSD.ORG>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD: src/lib/msun/arm/fenv.h,v 1.5.20.1 2009/04/15 03:14:26 kensmith Exp $
 */

#ifndef	_FENV_H_
#define	_FENV_H_

#include <sys/cdefs.h>
#include <sys/_types.h>

typedef	__uint32_t	fenv_t;
typedef	__uint32_t	fexcept_t;

/* Exception flags */
#define	FE_INVALID	0x0001
#define	FE_DIVBYZERO	0x0002
#define	FE_OVERFLOW	0x0004
#define	FE_UNDERFLOW	0x0008
#define	FE_INEXACT	0x0010
#define FE_DENORMAL 0x0080
#define	FE_ALL_EXCEPT	(FE_DIVBYZERO | FE_INEXACT | \
			 FE_INVALID | FE_OVERFLOW | FE_UNDERFLOW | FE_DENORMAL)

/* Rounding modes */
#define	FE_TONEAREST	0x00000000
#define	FE_TOWARDZERO	0x00c00000
#define	FE_UPWARD		0x00400000
#define	FE_DOWNWARD		0x00800000
#define	_ROUND_MASK	(FE_TONEAREST | FE_DOWNWARD | \
			 FE_UPWARD | FE_TOWARDZERO)
__BEGIN_DECLS

/* Default floating-point environment */
extern const fenv_t	__fe_dfl_env;
#define	FE_DFL_ENV	(&__fe_dfl_env)

/* We need to be able to map status flag positions to mask flag positions */
#define _FPUSW_SHIFT	8
#define	_ENABLE_MASK	(FE_ALL_EXCEPT << _FPUSW_SHIFT)

#define __rfs(__fpscr)	asm volatile("fmrx  %0, fpscr" : "=r"(*(__fpscr)))
#define __wfs(__fpscr)	asm volatile("fmxr  fpscr, %0" :: "r"(__fpscr))

static __inline int
feclearexcept(int __excepts)
{
	fexcept_t __fpsr;

	__rfs(&__fpsr);
	__fpsr &= ~__excepts;
	__wfs(__fpsr);
	return (0);
}

static __inline int
fegetexceptflag(fexcept_t *__flagp, int __excepts)
{
	fexcept_t __fpsr;

	__rfs(&__fpsr);
	*__flagp = __fpsr & __excepts;
	return (0);
}

static __inline int
fesetexceptflag(const fexcept_t *__flagp, int __excepts)
{
	fexcept_t __fpsr;

	__rfs(&__fpsr);
	__fpsr &= ~__excepts;
	__fpsr |= *__flagp & __excepts;
	__wfs(__fpsr);
	return (0);
}

static __inline int
feraiseexcept(int __excepts)
{
	fexcept_t __ex = __excepts;

	fesetexceptflag(&__ex, __excepts);	/* XXX */
	return (0);
}

static __inline int
fetestexcept(int __excepts)
{
	fexcept_t __fpsr;

	__rfs(&__fpsr);
	return (__fpsr & __excepts);
}

static __inline int
fegetround(void)
{
	fexcept_t __fpsr;

	__rfs(&__fpsr);
	return (__fpsr & _ROUND_MASK);
}

static __inline int
fesetround(int __round)
{
	fexcept_t __fpsr;

	if (__round & ~_ROUND_MASK)
			return (-1);

	__rfs(&__fpsr);
	__fpsr &= ~_ROUND_MASK;
	__fpsr |= __round;
	__wfs(__fpsr);

	return (0);
}

static __inline int
fegetenv(fenv_t *__envp)
{

	__rfs(__envp);
	return (0);
}

static __inline int
feholdexcept(fenv_t *__envp)
{
	fenv_t __env;

	__rfs(&__env);
	*__envp = __env;
	__env &= ~(FE_ALL_EXCEPT | _ENABLE_MASK);
	__wfs(__env);
	return (0);
}

static __inline int
fesetenv(const fenv_t *__envp)
{

	__wfs(*__envp);
	return (0);
}

static __inline int
feupdateenv(const fenv_t *__envp)
{
	fexcept_t __fpsr;

	__rfs(&__fpsr);
	__wfs(*__envp);
	feraiseexcept(__fpsr & FE_ALL_EXCEPT);
	return (0);
}

#if __BSD_VISIBLE

static __inline int
feenableexcept(int __mask)
{
	fenv_t __old_fpsr, __new_fpsr;

	__rfs(&__old_fpsr);
	__new_fpsr = __old_fpsr | (__mask & FE_ALL_EXCEPT) << _FPUSW_SHIFT;
	__wfs(__new_fpsr);
	return ((__old_fpsr >> _FPUSW_SHIFT) & FE_ALL_EXCEPT);
}

static __inline int
fedisableexcept(int __mask)
{
	fenv_t __old_fpsr, __new_fpsr;

	__rfs(&__old_fpsr);
	__new_fpsr = __old_fpsr & ~((__mask & FE_ALL_EXCEPT) << _FPUSW_SHIFT);
	__wfs(__new_fpsr);
	return ((__old_fpsr >> _FPUSW_SHIFT) & FE_ALL_EXCEPT);
}

static __inline int
fegetexcept(void)
{
	fenv_t __fpsr;

	__rfs(&__fpsr);
	return ((__fpsr & _ENABLE_MASK) >> _FPUSW_SHIFT);
}

#endif /* __BSD_VISIBLE */

__END_DECLS

#endif	/* !_FENV_H_ */
