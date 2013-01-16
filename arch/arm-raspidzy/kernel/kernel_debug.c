/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/kernel.h>
#include <kernel_base.h>
#include <kernel_debug.h>
#include <raspi/raspi.h>

#include "kernel_intern.h"

extern void ser_PutCMINIUART(uint32_t c);

/*
 * Character output function. All debug output ends up there.
 * This function needs to be implemented for every supported architecture.
 * KernelBase is an optional parameter here. During
 * very early startup it can be NULL.
 */

int krnPutC(int c, struct KernelBase *KernelBase)
{
    if (c == '\n')
    {
        krnPutC('\r', KernelBase);
    }

    ser_PutCMINIUART( (uint32_t)c );

    return 1;
}

