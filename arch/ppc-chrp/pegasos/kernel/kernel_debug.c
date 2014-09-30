/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <bootconsole.h>

#include "kernel_base.h"
#include "kernel_debug.h"

int krnPutC(int c, struct KernelBase *KernelBase)
{
    serial_Putc(c);
    return 1;
}
