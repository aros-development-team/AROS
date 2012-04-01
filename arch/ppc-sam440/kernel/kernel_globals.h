/*
    Copyright © 2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Global KernelBase access
    Lang: english
*/

static inline struct KernelBase *getKernelBase(void)
{
    return (struct KernelBase *)rdspr(SPRG4U);
}

static inline void setKernelBase(struct KernelBase *base)
{
    wrspr(SPRG4U, base);
}
