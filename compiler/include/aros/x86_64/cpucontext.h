#ifndef AROS_X86_64_CPUCONTEXT_H
#define AROS_X86_64_CPUCONTEXT_H

/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: CPU context definition for x86-64 processors
    Lang: english
*/

typedef struct
{
    UBYTE data[10];
    UBYTE pad[6];
} MMReg;

typedef struct
{
    UBYTE data[16];
} XMMReg;

struct FPXContext
{
    UWORD  fcw;
    UWORD  fsw;
    UWORD  ftw;
    UWORD  fop;
    ULONG  ip;
    ULONG  cs;
    ULONG  dp;
    ULONG  ds;
    ULONG  mxcsr;
    ULONG  pad;
    MMReg  mm[8];
    XMMReg xmm[16];
    XMMReg reserved[6];
};

struct ExceptionContext
{
    ULONG Flags;	/* Context flags		*/
    ULONG Reserved;	/* Padding			*/
    UQUAD rax;
    UQUAD rbx;
    UQUAD rcx;
    UQUAD rdx;
    UQUAD rsi;
    UQUAD rdi;
    UQUAD r8;
    UQUAD r9;
    UQUAD r10;
    UQUAD r11;
    UQUAD r12;
    UQUAD r13;
    UQUAD r14;
    UQUAD r15;
    UQUAD rbp;
    UQUAD ds;		/* Segment registers are padded */
    UQUAD es;
    UQUAD fs;
    UQUAD gs;
    UQUAD rip;
    UQUAD cs;
    UQUAD rflags;
    UQUAD rsp;
    UQUAD ss;

    struct FPXContext *FXData;	/* Pointer to SSE context area */
};

enum enECFlags
{
    ECF_SEGMENTS = 1<<0, /* Segment registers are present */
    ECF_FPX      = 1<<1, /* SSE context is present	  */
};

#endif
