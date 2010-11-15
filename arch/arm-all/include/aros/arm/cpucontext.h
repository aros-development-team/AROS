/*
    Copyright � 1995-2010, The AROS Development Team. All rights reserved.
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

/* Flags */
enum enECFlags
{
    ECF_FPU = 1<<0	/* FPU data is present */
};

/* FPU types */
#define FPU_NONE 0
#define FPU_AFP  1
#define FPU_VFP  2

/* VFP context */
struct VFPContext
{
    ULONG fpr[64];
    ULONG fpscr;
};
