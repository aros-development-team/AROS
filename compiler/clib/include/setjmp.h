#ifndef _SETJMP_H_
#define _SETJMP_H_

/*
    Copyright (C) 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: ANSI-C header file setjmp.h
    Lang: english
*/

#include <aros/system.h>

#ifdef __mc68000__
#   define _JMPLEN 15
#elif __i386__
#   define _JMPLEN 7
#elif __x86_64__
#   define _JMPLEN 15
#elif __powerpc__
#   define _JMPLEN 58
#elif __arm__
#   define _JMPLEN 63
#endif

typedef struct __jmp_buf
{
    unsigned long retaddr;
    unsigned long regs[_JMPLEN];
}  __attribute__ ((aligned (16))) jmp_buf[1];

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

/* Unix functions */
/* NOTIMPL void   siglongjmp(sigjmp_buf, int) __noreturn ; */
/* NOTIMPL int    sigsetjmp(sigjmp_buf, int); */

/* NOTIMPL void	_longjmp(jmp_buf, int) __noreturn ; */
/* NOTIMPL int	_setjmp(jmp_buf); */

__END_DECLS

#endif /* _SETJMP_H_ */
