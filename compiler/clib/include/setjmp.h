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
#elif ppc || __powerpc__
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

#if !defined(_ANSI_SOURCE)
/* Unix functions */
void   siglongjmp(sigjmp_buf, int) __noreturn ;
int    sigsetjmp(sigjmp_buf, int);

#if !defined(_POSIX_SOURCE)
void	_longjmp(jmp_buf, int) __noreturn ;
int	_setjmp(jmp_buf);
#endif

#endif /* !_ANSI_SOURCE */

__END_DECLS

#endif /* _SETJMP_H_ */
