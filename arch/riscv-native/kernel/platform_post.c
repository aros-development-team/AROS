/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Platform configuration that needs a working exec (from all-pc).
*/

#include <aros/config.h>

#if defined(__AROSEXEC_SMP__)

#include <aros/asmcall.h>
#include <exec/execbase.h>
#include <exec/resident.h>

#include <proto/exec.h>

#include <kernel_base.h>
#include <kernel_debug.h>

#include "kernel_riscv.h"

#define D(x)

/*
 * This file contains code that is run once Exec has been brought up - and
 * is launched via the RomTag/Autoinit routines in Exec.
 *
 * At this point exec is fully configured, and multitasking is enabled.
 */

extern void kernelpost_end(void);

static AROS_UFP3 (APTR, KernelPost,
                  AROS_UFPA(struct Library *, lh, D0),
                  AROS_UFPA(BPTR, segList, A0),
                  AROS_UFPA(struct ExecBase *, sysBase, A6));

static const TEXT kernelpost_namestring[] = "kernel.post";
static const TEXT kernelpost_versionstring[] = "kernel.post 1.0\n";

const struct Resident kernelpost_romtag =
{
   RTC_MATCHWORD,
   (struct Resident *)&kernelpost_romtag,
   (APTR)&kernelpost_end,
   RTF_COLDSTART,
   1,
   NT_UNKNOWN,
   115,
   (STRPTR)kernelpost_namestring,
   (STRPTR)kernelpost_versionstring,
   (APTR)KernelPost
};

static AROS_UFH3 (APTR, KernelPost,
                  AROS_UFHA(struct Library *, lh, D0),
                  AROS_UFHA(BPTR, segList, A0),
                  AROS_UFHA(struct ExecBase *, SysBase, A6)
)
{
    AROS_USERFUNC_INIT

    struct KernelBase *KernelBase;

    KernelBase = (struct KernelBase *)OpenResource("kernel.resource");
    if (!KernelBase)
            return NULL;

    D(bug("[Kernel] %s: Attempting to start additional cores...\n", __func__));
    Disable();
    krnRVStartCPUs(KernelBase, SysBase);
    Enable();

    D(bug("[Kernel] %s: Platform Initialization complete\n", __func__));

    AROS_USERFUNC_EXIT

    return NULL;
}

void kernelpost_end(void) { };

#endif /* __AROSEXEC_SMP__ */
