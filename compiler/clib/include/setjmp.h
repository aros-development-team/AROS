#ifndef _SETJMP_H_
#define _SETJMP_H_

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: ANSI-C header file setjmp.h
    Lang: english
*/

#include <sys/cdefs.h>

#ifdef __mc68000__
#   define _JMPLEN 12
#elif __i386__
#   define _JMPLEN 7
#elif __powerpc__ || __ppc__
#   define _JMPLEN 58
#endif

typedef struct __jmp_buf
{
    unsigned long retaddr;
    unsigned long regs[_JMPLEN];
} jmp_buf[1];

#if !defined(_ANSI_SOURCE)
typedef struct __sigjmp_buf
{
    unsigned long   retaddr;
    unsigned long   regs[_JMPLEN];
} sigjmp_buf[1];
#endif /* !_ANSI_SOURCE */

/* Prototypes */
__BEGIN_DECLS
int	setjmp (jmp_buf env);
void	longjmp (jmp_buf env, int val) __noreturn ;

#if __BSD_VISIBLE || __POSIX_VISIBLE || __XSI_VISIBLE
/* Unix functions */
/* NOTIMPL void   siglongjmp(sigjmp_buf, int) __noreturn ; */
/* NOTIMPL int    sigsetjmp(sigjmp_buf, int); */

/* NOTIMPL void	_longjmp(jmp_buf, int) __noreturn ; */
/* NOTIMPL int	_setjmp(jmp_buf); */

#endif /* __BSD_VISIBLE || __POSIX_VISIBLE || __XSI_VISIBLE */

__END_DECLS

#endif /* _SETJMP_H_ */
