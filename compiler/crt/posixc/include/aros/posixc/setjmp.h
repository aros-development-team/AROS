#ifndef _POSIXC_SETJMP_H_
#define _POSIXC_SETJMP_H_

/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    POSIX-2008.1 header file setjmp.h
*/

/* C99 */
#include <aros/stdc/setjmp.h>

typedef struct __sigjmp_buf
{
    unsigned long   retaddr;
    unsigned long   regs[_JMPLEN];
} sigjmp_buf[1];


__BEGIN_DECLS

/* NOTIMPL void	_longjmp(jmp_buf, int) __noreturn ; */
void   siglongjmp(sigjmp_buf, int) __noreturn ;

/* NOTIMPL int	_setjmp(jmp_buf); */
int    sigsetjmp(sigjmp_buf, int);

__END_DECLS

#endif /* _POSIXC_SETJMP_H_ */
