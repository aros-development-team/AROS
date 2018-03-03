/*
    Copyright © 1995-2018, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
*/

#include <stdarg.h>
#include <stdio.h>

#include <kernel_base.h>
#include <kernel_debug.h>

#if defined(__AROSEXEC_SMP__)
#include <aros/atomic.h>
#include <asm/cpu.h>
extern volatile ULONG   safedebug;
#endif

#include <proto/exec.h>

static int UniPutC(int c, struct KernelBase *KernelBase);

/*
 * This is internal version of KrnBug(), to be called directly by bug() macro.
 * If you want to reimplement it, override this file.
 * However, generally you won't want to do this, because __vcformat() supports
 * AROS-specific extensions, like %b, which are unlikely supported by your host OS.
 */
int krnBug(const char *format, va_list args, APTR kernelBase)
{
    int retval = 0;

#if defined(__AROSEXEC_SMP__)
    if (safedebug & 1)
    {
        while (bit_test_and_set_long((ULONG*)&safedebug, 1)) { asm volatile("pause"); };
    }
#endif

    retval = __vcformat(kernelBase, (int (*)(int, void *))UniPutC, format, args);

#if defined(__AROSEXEC_SMP__)
    if (safedebug & 1)
    {
        __AROS_ATOMIC_AND_L(safedebug, ~(1 << 1));
    }
#endif

    return retval;
}

/*
 * This is yet another character stuffing callback for debug output. This one unifies the output
 * with debug output from outside the kernel where possible, allowing kernel debug output to be
 * redirected alongside that other output (e.g. with Sashimi or Bifteck).
 */
static int UniPutC(int c, struct KernelBase *KernelBase)
{
    int result;

    if (SysBase != NULL)
    {
        RawPutChar(c);
        result = 1;
    }
    else
        result = krnPutC(c, KernelBase);

    return result;
}
