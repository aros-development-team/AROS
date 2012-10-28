/*-
 * Copyright (c) 2004 David Schultz <das@FreeBSD.ORG>
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
 * $FreeBSD: src/lib/msun/arm/fenv.c,v 1.1.20.1 2009/04/15 03:14:26 kensmith Exp $
 */

#include <errno.h>
#include <fenv.h>

/*
 * Hopefully the system ID byte is immutable, so it's valid to use
 * this as a default environment.
 */
const fenv_t __fe_dfl_env = { };
static fenv_t fe_env;

/* FIXME: These are just stubs for the moment for soft-float
 *        A new implemntation is needed for hard-float support
 */

/* Clear the supported exceptions represented by EXCEPTS.  */
int feclearexcept (int __excepts)
{
    fe_env.__status_register &= ~__excepts;
    return 0;
}

/* Store implementation-defined representation of the exception flags
   indicated by EXCEPTS in the object pointed to by FLAGP.  */
int fegetexceptflag (fexcept_t *__flagp, int __excepts)
{
    *__flagp = __excepts;
    return 0;
}

/* Raise the supported exceptions represented by EXCEPTS.  */
int feraiseexcept (int __excepts)
{
    if (__excepts & (FE_UNDERFLOW | FE_OVERFLOW)) {
        errno = ERANGE;
    } else if (__excepts) {
        errno = EDOM;
    }
    return 0;
}

/* Set complete status for exceptions indicated by EXCEPTS according to
   the representation in the object pointed to by FLAGP.  */
int fesetexceptflag(const fexcept_t *flagp, int excepts)
{
    fe_env.__status_register &= ~excepts;
    return 0;
}

/* Determine which of subset of the exceptions specified by EXCEPTS are
   currently set.  */
int fetestexcept (int __excepts)
{
    return (fe_env.__status_register & __excepts);
}

/* Store the current floating-point environment in the object pointed
   to by ENVP.  */
int fegetenv (fenv_t *envp)
{
    *envp = fe_env;
    return 0;
}

/* Save the current environment in the object pointed to by ENVP, clear
   exception flags and install a non-stop mode (if available) for all
   exceptions.  */
int feholdexcept (fenv_t *envp)
{
    *envp = fe_env;
    errno = 0;
    return 0;
}

/* Establish the floating-point environment represented by the object
   pointed to by ENVP.  */
int fesetenv(const fenv_t *envp)
{
    fe_env = *envp;
    return 0;
}

/* Save current exceptions in temporary storage, install environment
   represented by object pointed to by ENVP and raise exceptions
   according to saved exceptions.  */
int feupdateenv(const fenv_t *envp)
{
    int fexcept = fe_env.__status_register;
    fesetenv(envp);
    feraiseexcept(fexcept);
    return 0;
}
