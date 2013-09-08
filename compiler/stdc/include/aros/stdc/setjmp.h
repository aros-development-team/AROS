#ifndef _SETJMP_H_
#define _SETJMP_H_

/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    Desc: C99 & POSIX-2008.1 header file setjmp.h
*/

#include <aros/system.h>


/* C99 */
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

__BEGIN_DECLS

int	setjmp (jmp_buf env);
void	longjmp (jmp_buf env, int val) __noreturn ;

__END_DECLS


/* POSIX-2008.1 */
typedef struct __sigjmp_buf
{
    unsigned long   retaddr;
    unsigned long   regs[_JMPLEN];
} sigjmp_buf[1];

__BEGIN_DECLS

/* NOTIMPL void	_longjmp(jmp_buf, int) __noreturn ; */
/* NOTIMPL void   siglongjmp(sigjmp_buf, int) __noreturn ; */

/* NOTIMPL int	_setjmp(jmp_buf); */
/* NOTIMPL int    sigsetjmp(sigjmp_buf, int); */

__END_DECLS

#endif /* _SETJMP_H_ */
