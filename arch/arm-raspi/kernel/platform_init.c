/*
    Copyright � 2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 0

#include <aros/kernel.h>
#include <aros/symbolsets.h>
#include <exec/memory.h>
#include <exec/tasks.h>
#include <exec/alerts.h>
#include <exec/execbase.h>
#include <asm/io.h>
#include <proto/kernel.h>
#include <proto/exec.h>
#include <strings.h>

#include <hardware/bcm283x.h>

#include "etask.h"

#include "kernel_intern.h"
#include "kernel_arch.h"
#include "kernel_romtags.h"

#undef ARM_PERIIOBASE
extern uint32_t __arm_periiobase;
#define ARM_PERIIOBASE (__arm_periiobase)

void *(*__AllocMem)();

#define ExecAllocMem(bytesize, requirements)        \
AROS_CALL2(void *, __AllocMem,                      \
		AROS_LCA(ULONG, byteSize,     D0),			\
		AROS_LCA(ULONG, requirements, D1),			\
		struct ExecBase *, SysBase)

AROS_LH2(APTR, AllocMem,
	AROS_LHA(ULONG, byteSize,     D0),
	AROS_LHA(ULONG, requirements, D1),
	struct ExecBase *, SysBase, 33, Kernel)
{
    AROS_LIBFUNC_INIT

    if (requirements & MEMF_CHIP)
    {
        DALLOCMEM(bug("[Kernel] AllocMem: Ignoring MEMF_CHIP flag\n"));
        requirements &= ~MEMF_CHIP;
    }
    return ExecAllocMem(bytesize, requirements);

    AROS_LIBFUNC_EXIT
}

static int PlatformInit(struct KernelBase *KernelBase)
{
    UBYTE *ptr;
    D(bug("[Kernel] PlatformInit()\n"));

#if (1)
    // TODO:
    // How to identify broadcom IP's?
    // Expose this as a seprate subsystem (like PCI?)
    D(bug("[Kernel] Integrated Peripherals -:\n"));
    for (ptr = ARM_PERIIOBASE; ptr < (ARM_PERIIOBASE + ARM_PERIIOSIZE); ptr += ARM_PRIMECELLPERISIZE)
    {
        unsigned int perihreg = (*(volatile unsigned int *)(ptr + 0xFF0) & 0xFF) | (*(volatile unsigned int *)(ptr + 0xFF4) & 0xFF) << 8 | (*(volatile unsigned int *)(ptr + 0xFF8) & 0xFF) << 16 | (*(volatile unsigned int *)(ptr + 0xFFC) & 0xFF) << 24;
        if (perihreg == ARM_PRIMECELLID)
        {
            perihreg = (*(volatile unsigned int *)(ptr + 0xFE0) & 0xFF) | (*(volatile unsigned int *)(ptr + 0xFE4) & 0xFF) << 8 | (*(volatile unsigned int *)(ptr + 0xFE8) & 0xFF) << 16 | (*(volatile unsigned int *)(ptr + 0xFEC) & 0xFF) << 24;
            unsigned int manu = (perihreg & (0x7F << 12)) >> 12;
            unsigned int prod = (perihreg & 0xFFF);
            unsigned int rev = (perihreg & (0xF << 20)) >> 20;
            unsigned int config = (perihreg & (0x7F << 24)) >> 24;
            D(bug("[Kernel]           0x%p: manu %x, prod %x, rev %d, config %d\n", ptr, manu, prod, rev, config));
        }
/*        else
        {
            if (perihreg)
            {
                D(bug("[Kernel] PlatformInit:           0x%p: PrimeCellID != %08x\n", ptr, perihreg));
            }
        }*/
    }
#endif

    D(bug("[Kernel] PlatformInit: Patching in our AllocMem to ignore MEMF_CHIP..\n"));
    
    __AllocMem = SetFunction(SysBase, -33*LIB_VECTSIZE, AROS_SLIB_ENTRY(AllocMem, Kernel, 33));

    D(bug("[Kernel] PlatformInit: Registering Heartbeat timer..\n"));

    KrnAddSysTimerHandler(KernelBase);

    D(bug("[Kernel] PlatformInit: Done..\n"));

    return TRUE;
}

ADD2INITLIB(PlatformInit, 0)

struct KernelBase *getKernelBase()
{
    return (struct KernelBase *)KernelBase;
}
