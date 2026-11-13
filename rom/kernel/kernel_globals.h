#ifndef KERNEL_GLOBALS_H
#define KERNEL_GLOBALS_H
/*
    Copyright © 2011-2017, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Global KernelBase access
    Lang: english
*/

struct KernelBase;
extern struct KernelBase *KernelBase;

static inline struct KernelBase *getKernelBase(void)
{
    return KernelBase;
}

static inline void setKernelBase(struct KernelBase *base)
{
    KernelBase = base;
}

#endif /* KERNEL_GLOBALS_H */
