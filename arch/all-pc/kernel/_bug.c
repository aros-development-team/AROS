/*
    Copyright (C) 1995-2023, The AROS Development Team. All rights reserved.

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

static int UniPutC(int, struct KernelBase *);

#if defined(DEBUG_TIMESTAMP)
#include "apic.h"
extern UQUAD Kernel_64_KrnTimeStamp(struct KernelBase *);
#endif

/*
 * This is pc arch specific version of KrnBug(), to be called directly by the kernel bug() macro.
 * Note: In early stages of kernel startup, kernelBase will be NULL. Code needs to work with such
 * assumption
 */
int krnBug(const char *format, va_list args, APTR kernelBase)
{
    int retval = 0;

    KRNDEBUGLOCK

#if defined(DEBUG_TIMESTAMP)
    if ((__KernBootPrivate->debug_buffer) && (__KernBootPrivate->debug_buffsize > 0))
    {
        unsigned int i;
        static BOOL newline = TRUE;

        //TODO: replace use of snprintf/vsnprintf
        if (newline)
        {
            struct Task         *debugTask = NULL;
            struct PlatformData *kernPlatD;
            UQUAD               debugStamp = 0L;
            unsigned int        debugCPU = 0;

            if (kernelBase != NULL)
                debugStamp = Kernel_64_KrnTimeStamp(kernelBase);
            if (SysBase != NULL)
                debugTask = FindTask(NULL);
            if ((kernelBase != NULL) &&
                ((kernPlatD = (struct PlatformData *)(((struct KernelBase *)kernelBase)->kb_PlatformData)) != NULL) &&
                (kernPlatD->kb_APIC))
                debugCPU = core_APIC_GetNumber(kernPlatD->kb_APIC);

            retval = snprintf(__KernBootPrivate->debug_buffer, __KernBootPrivate->debug_buffsize,
                                     "%08x%08x 0x%p | %03u | ",
                                     debugStamp >> 32, debugStamp & 0xFFFFFFFF, debugTask, debugCPU);
        }
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

extern BOOL IsKernelBaseReady(struct ExecBase *SysBase);

/*
 * This is yet another character stuffing callback for debug output. This one unifies the output
 * with debug output from outside the kernel where possible, allowing kernel debug output to be
 * redirected alongside that other output (e.g. with Sashimi or Bifteck).
 */
/*
 * Default implementation of RawPutChar requires KernelBase to be setup. Check for this. Otherwise
 * since SysBase is setup first (PrepareExecBase), before SysBase->KernelBase is setup (Kernel_Init)
 * just checking for SysBase causes some debug early to be lost.
 */
static int UniPutC(int c, struct KernelBase *KernelBase)
{
    int result;

    if (IsKernelBaseReady(SysBase))
    {
        RawPutChar(c);
        result = 1;
    }
    else
        result = krnPutC(c, KernelBase);

    return result;
}
