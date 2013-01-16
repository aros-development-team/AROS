/*
    Copyright © 2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Global KernelBase access, Amiga hardware specific
    Lang: english
*/

/*
 * getKernelBase() is intentionally not defined in order to produce
 * build errors when referenced. Helps cleaning up the code.
 */

static inline void setKernelBase(struct KernelBase *base)
{
    /* Called from within AllocKernelBase(). Do nothing for now... */
}
