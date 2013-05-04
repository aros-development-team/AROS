/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
*/

#include <aros/kernel.h>

#include <kernel_base.h>
#include <kernel_debug.h>

/*
 * Character output function. All debug output ends up there.
 * This function needs to be implemented for every supported architecture.
 * KernelBase is an optional parameter here. During
 * very early startup it can be NULL.
 */

int krnPutC(int chr, struct KernelBase *KernelBase)
{
    /* The implementation is architecture-specific */
    return 1;
}
