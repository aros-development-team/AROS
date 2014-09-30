/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>
#include <asm/amcc440.h>
#include <asm/io.h>
#include <aros/libcall.h>
#include <stdarg.h>
#include <string.h>
#include <proto/exec.h>
#include <exec/lists.h>
#include <exec/nodes.h>
#include <exec/memory.h>

#include "kernel_intern.h"

struct PrivData {
    struct KernelBase *kbase;
    uint32_t tbu, tbl;
};

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
    while(!(inb(UART0_LSR) & UART_LSR_TEMT));
    outb(c, UART0_THR);

    return 1;
}
