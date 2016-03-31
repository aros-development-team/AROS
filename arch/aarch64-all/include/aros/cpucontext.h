#ifndef AROS_AARCH64_CPUCONTEXT_H
#define AROS_AARCH64_CPUCONTEXT_H

/*
    Copyright © 2016, The AROS Development Team. All rights reserved.
    $Id$

    Desc: CPU context definition for ARM AArch64 processors
    Lang: english
*/

struct ExceptionContext
{
    IPTR r[29]; /* General purpose registers	*/
    IPTR fp;	/*  x30    			*/
    IPTR sp;	/*      			*/
    IPTR pc;	/*      			*/
};

/* VFP context */
struct VFPContext
{
    IPTR fpr[64];
};

#endif
