/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id: $

    Desc: CPU context definition for x86 processors
    Lang: english
*/

/* Black boxes for now */
struct FPUContext;
struct FPXContext;

struct ExceptionContext
{
    ULONG Flags;	/* Context flags		*/
    ULONG eax;
    ULONG ebx;
    ULONG ecx;
    ULONG edx;
    ULONG esi;
    ULONG edi;
    ULONG ebp;
    ULONG ds;
    ULONG es;
    ULONG fs;
    ULONG gs;
    ULONG eip;
    ULONG cs;
    ULONG eflags;
    ULONG esp;
    ULONG ss;

    struct FPUContext *FPData;	/* Pointer to 8087 FPU context area	*/
    struct FPXContext *FXData;	/* Pointer to SSE context area		*/
};

enum enECFlags
{
    ECF_SEGMENTS = 1<<0, /* Segment registers are present */
    ECF_FPU      = 1<<1, /* 8087 FPU context is present   */
    ECF_FPX      = 1<<2, /* SSE context is present	  */
};
