#ifndef _SETJMP_H
#define _SETJMP_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: ANSI-C header file setjmp.h
    Lang: english
*/

#ifdef __mc68000__
#define _JMPLEN 12

#elif i386
#define _JMPLEN 7

#elif ppc
#define _JMPLEN 58

#endif

typedef struct jmp_buf
{
    unsigned long retaddr;
    unsigned long regs[_JMPLEN];
} jmp_buf[1];

/* Prototypes */
extern int setjmp (jmp_buf env);
extern void longjmp (jmp_buf env, int val);

#endif /* _SETJMP_H */
