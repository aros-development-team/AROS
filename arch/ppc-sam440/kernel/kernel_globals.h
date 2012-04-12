/*
    Copyright © 2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Global KernelBase access
    Lang: english
*/
#ifndef KERNEL_GLOBALS_H
#define KERNEL_GLOBALS_H

struct KernelBase;

static inline struct KernelBase *getKernelBase(void)
{
    return (struct KernelBase *)rdspr(SPRG4U);
}

static inline void setKernelBase(struct KernelBase *base)
{
    wrspr(SPRG4U, base);
}

#endif /* KERNEL_GLOBALS_H */
