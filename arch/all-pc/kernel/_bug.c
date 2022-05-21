/*
    Copyright (C) 1995-2022, The AROS Development Team. All rights reserved.

    Desc:
*/

#include <stdarg.h>
#include <stdio.h>

#include <kernel_base.h>
#include <kernel_debug.h>

#include <proto/exec.h>
#if defined(DEBUG_TIMESTAMP)
#define __KERNEL_NOLIBBASE__
#include <proto/kernel.h>
#endif

extern UQUAD Kernel_64_KrnTimeStamp(struct KernelBase *);
static int UniPutC(int, struct KernelBase *);

/*
 * This is internal version of KrnBug(), to be called directly by bug() macro.
 * If you want to reimplement it, override this file.
 * However, generally you won't want to do this, because __vcformat() supports
 * AROS-specific extensions, like %b, which are unlikely supported by your host OS.
 */
int krnBug(const char *format, va_list args, APTR kernelBase)
{
#if defined(DEBUG_TIMESTAMP)
    static BOOL newline = TRUE;
    struct Task *thisTask = NULL;
    if (SysBase != NULL)
        thisTask = FindTask(NULL);
    UQUAD timeCur = Kernel_64_KrnTimeStamp(kernelBase);
#endif
    int retval = 0;

    KRNDEBUGLOCK

#if defined(DEBUG_TIMESTAMP)
    if ((__KernBootPrivate->debug_buffer) && (__KernBootPrivate->debug_buffsize > 0))
    {
        int i;
        //TODO: replace use of snprintf/vsnprintf
        if (newline)
            retval = snprintf(__KernBootPrivate->debug_buffer, __KernBootPrivate->debug_buffsize,
                                     "%08x%08x 0x%p | %03u | ",
                                     timeCur >> 32, timeCur & 0xFFFFFFFF, thisTask, 0);
        newline = FALSE;
        retval = vsnprintf((char *)((IPTR)__KernBootPrivate->debug_buffer + retval), __KernBootPrivate->debug_buffsize - retval, format, args);
        for (i = 0; i < __KernBootPrivate->debug_buffsize && __KernBootPrivate->debug_buffer[i] != 0  ; i++)
        {
            if (__KernBootPrivate->debug_buffer[i] == '\n')
                newline = TRUE;
            UniPutC(__KernBootPrivate->debug_buffer[i], KernelBase);
        }
        KRNDEBUGUNLOCK

        return retval;
    }
#endif
    retval = __vcformat(kernelBase, (int (*)(int, void *))UniPutC, format, args);

    KRNDEBUGUNLOCK

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
