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
 * $FreeBSD: src/lib/msun/i387/fenv.c,v 1.3 2007/01/05 07:15:26 das Exp $
 */

#include "fenv.h"

const fenv_t __fe_dfl_env = {
#ifndef __AROS__
	__INITIAL_NPXCW__,
#else
        0x127F,
#endif
	0x0000,
	0x0000,
	0x1f80,
	0xffffffff,
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff }
};

int
fesetexceptflag(const fexcept_t *flagp, int excepts)
{
	fenv_t env;
	int mxcsr;

	__fnstenv(&env);
	env.__status &= ~excepts;
	env.__status |= *flagp & excepts;
	__fldenv(env);

	if (__HAS_SSE()) {
		__stmxcsr(&mxcsr);
		mxcsr &= ~excepts;
		mxcsr |= *flagp & excepts;
		__ldmxcsr(mxcsr);
	}

	return (0);
}

int
feraiseexcept(int excepts)
{
	fexcept_t ex = excepts;

	fesetexceptflag(&ex, excepts);
	__fwait();
	return (0);
}

int
fegetenv(fenv_t *envp)
{
	int mxcsr;

	__fnstenv(envp);
	/*
	 * fnstenv masks all exceptions, so we need to restore
	 * the old control word to avoid this side effect.
	 */
	__fldcw(envp->__control);
	if (__HAS_SSE()) {
		__stmxcsr(&mxcsr);
		__set_mxcsr(*envp, mxcsr);
	}
	return (0);
}

int
feholdexcept(fenv_t *envp)
{
	int mxcsr;

	__fnstenv(envp);
	__fnclex();
	if (__HAS_SSE()) {
		__stmxcsr(&mxcsr);
		__set_mxcsr(*envp, mxcsr);
		mxcsr &= ~FE_ALL_EXCEPT;
		mxcsr |= FE_ALL_EXCEPT << _SSE_EMASK_SHIFT;
		__ldmxcsr(mxcsr);
	}
	return (0);
}

int
feupdateenv(const fenv_t *envp)
{
	int mxcsr;
        short status;
        
	__fnstsw(&status);
	if (__HAS_SSE())
		__stmxcsr(&mxcsr);
	else
		mxcsr = 0;
	fesetenv(envp);
	feraiseexcept((mxcsr | status) & FE_ALL_EXCEPT);
	return (0);
}

int
feenableexcept(int mask)
{
	int mxcsr, control, omask;

	mask &= FE_ALL_EXCEPT;
	__fnstcw(&control);
	if (__HAS_SSE())
		__stmxcsr(&mxcsr);
	else
		mxcsr = 0;
	omask = (control | mxcsr >> _SSE_EMASK_SHIFT) & FE_ALL_EXCEPT;
	control &= ~mask;
	__fldcw(control);
	if (__HAS_SSE()) {
		mxcsr &= ~(mask << _SSE_EMASK_SHIFT);
		__ldmxcsr(mxcsr);
	}
	return (~omask);
}

int
fedisableexcept(int mask)
{
	int mxcsr, control, omask;

	mask &= FE_ALL_EXCEPT;
	__fnstcw(&control);
	if (__HAS_SSE())
		__stmxcsr(&mxcsr);
	else
		mxcsr = 0;
	omask = (control | mxcsr >> _SSE_EMASK_SHIFT) & FE_ALL_EXCEPT;
	control |= mask;
	__fldcw(control);
	if (__HAS_SSE()) {
		mxcsr |= mask << _SSE_EMASK_SHIFT;
		__ldmxcsr(mxcsr);
	}
	return (~omask);
}
