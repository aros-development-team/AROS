/*
    Copyright © 2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Global KernelBase access
    Lang: english
*/

static inline struct KernelBase *getKernelBase(void)
{
    return KernelBase;
}

static inline void setKernelBase(struct KernelBase *base)
{
    KernelBase = base;
}
