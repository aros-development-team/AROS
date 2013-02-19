#ifndef AROS_ARM_CPUCONTEXT_H
#define AROS_ARM_CPUCONTEXT_H

/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: CPU context definition for ARM processors
    Lang: english
*/

struct ExceptionContext
{
    UWORD Flags;	/* Currently reserved		*/
    UBYTE FPUType;	/* FPU type (see below)		*/
    UBYTE Reserved;	/* Unused			*/
    ULONG r[12];	/* General purpose registers	*/
    ULONG ip;		/* r12				*/
    ULONG sp;		/* r13				*/
    ULONG lr;		/* r14				*/
    ULONG pc;		/* r15				*/
    ULONG cpsr;
    APTR  fpuContext;	/* Pointer to FPU context area	*/
};

/* CPU modes */
#define CPUMODE_USER            0x10
#define CPUMODE_FIQ             0x11
#define CPUMODE_IRQ             0x12
#define CPUMODE_SUPERVISOR      0x13
#define CPUMODE_ABORT           0x17
#define CPUMODE_UNDEF           0x1B
#define CPUMODE_SYSTEM          0x1F

#define CPUMODE_MASK            ~0x1F

/* Flags */
enum enECFlags
{
    ECF_FPU = 1<<0	        /* FPU data is present */
};

/* FPU types */
#define FPU_NONE                0
#define FPU_AFP                 1
#define FPU_VFP                 2

/* VFP context */
struct VFPContext
{
    ULONG fpr[64];
    ULONG fpscr;
};

#endif
