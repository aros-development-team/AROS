#ifndef _STDC_SETJMP_H_
#define _STDC_SETJMP_H_

/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    C99 header file setjmp.h
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


__BEGIN_DECLS

int	setjmp (jmp_buf env);
void	longjmp (jmp_buf env, int val) __noreturn ;

__END_DECLS

#endif /* _STDC_SETJMP_H_ */
