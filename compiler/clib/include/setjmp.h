#ifndef _SETJMP_H
#define _SETJMP_H

/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: ANSI-C header file unistd.h
    Lang: english
*/

typedef struct _jmp_buf
{
    unsigned long retaddr;
    unsigned long regs[7];
} jmp_buf[1];

/* Prototypes */
extern int setjmp (jmp_buf env);
extern void longjmp (jmp_buf env, int val);

#endif /* _SETJMP_H */
