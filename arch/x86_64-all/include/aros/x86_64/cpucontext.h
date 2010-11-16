/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id: cpucontext.h 35535 2010-11-16 06:12:00Z sonic $

    Desc: CPU context definition for x86-64 processors
    Lang: english
*/

/* Black box for now */
struct FPXContext;

struct ExceptionContext
{
    ULONG Flags;	/* Context flags		*/
    UQUAD Reserved;	/* Padding			*/
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
    UQUAD eip;
    UQUAD cs;
    UQUAD eflags;
    UQUAD rsp;
    UQUAD ss;

    struct FPXContext *FXData;	/* Pointer to SSE context area */
};

enum enECFlags
{
    ECF_SEGMENTS = 1<<0, /* Segment registers are present */
    ECF_FPX      = 1<<1, /* SSE context is present	  */
};
