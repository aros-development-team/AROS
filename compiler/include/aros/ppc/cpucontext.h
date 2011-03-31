#ifndef AROS_PPC_CPUCONTEXT_H
#define AROS_PPC_CPUCONTEXT_H

/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: CPU context definition for PowerPC processors
    Lang: english
*/

/* The following definitions are source-compatible with AmigaOS 4 */

struct ExceptionContext
{
    ULONG  Flags;
    ULONG  Traptype;
    ULONG  msr;		/* Machine state		*/
    ULONG  ip;		/* Return instruction pointer	*/
    ULONG  gpr[32];
    ULONG  cr;		/* Condition codes		*/
    ULONG  xer;		/* Extended exception register	*/
    ULONG  ctr;		/* Count register	     	*/
    ULONG  lr;		/* Link register	      	*/
    ULONG  dsisr;	/* DSI status			*/
    ULONG  dar;		/* Data address			*/
    /* FPU context */
    double fpr[32];
    UQUAD  fpscr;	/* Status and control		*/
    /* AltiVec context */
    UBYTE  vscr[16];	/* Status and control		*/
    UBYTE  vr[512];
    ULONG  vrsave;
};

enum enECFlags
{
    ECF_FULL_GPRS = 1<<0, /* GPR 14-31 are present	   */
    ECF_FPU       = 1<<1, /* FPU reisters are present	   */
    ECF_FULL_FPU  = 1<<2, /* FPSCR is present		   */
    ECF_VECTOR    = 1<<3, /* AltiVec registers are present */
    ECF_VRSAVE    = 1<<4  /* VRSAVE is present		   */
};

#endif
