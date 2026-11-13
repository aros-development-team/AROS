#ifndef _STDC_SETJMP_H_
#define _STDC_SETJMP_H_

/*
    Copyright � 1995-2023, The AROS Development Team. All rights reserved.
    $Id$

    C99 header file setjmp.h
*/

#include <aros/system.h>

#ifdef __mc68000__
// A0-A7/D0-D7 /
#   define _JMPLEN 15
#elif __i386__
#   define _JMPLEN 7
#elif __x86_64__
#   define _JMPLEN 15
#elif __powerpc__
#   define _JMPLEN 58
#elif __arm__
#   define _JMPLEN 63
#elif __aarch64__
    /* AAPCS64 callee-saved: x19-x30, sp, NEON v8-v15, plus headroom */
#   define _JMPLEN 31
#elif defined(__riscv) && (__riscv_xlen == 64)
    /* LP64D callee-saved: ra, sp, s0-s11, fs0-fs11 */
#   define _JMPLEN 25
#elif __riscv
    /* ILP32D callee-saved: ra, sp, s0-s11 (13 slots + pad),
       then fs0-fs11 as 12 8-byte pairs at an 8-aligned offset */
#   define _JMPLEN 37
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
