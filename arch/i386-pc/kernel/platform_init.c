/*
    Copyright © 1995-2017, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/symbolsets.h>
#include <asm/cpu.h>
#include <exec/execbase.h>
#include <proto/exec.h>

#include "kernel_base.h"
#include "kernel_debug.h"
#include "kernel_intern.h"

#include "traps.h"
#include "utils.h"

#define D(x)

extern struct syscallx86_Handler x86_SCSupervisorHandler;

static int PlatformInit(struct KernelBase *KernelBase)
{
    struct PlatformData *data;
    struct table_desc idtr;
    struct tss	    *tss;
    long long *idt;

    NEWLIST(&KernelBase->kb_ICList);
    KernelBase->kb_ICTypeBase = KBL_INTERNAL + 1;

    data = AllocMem(sizeof(struct PlatformData), MEMF_PUBLIC|MEMF_CLEAR);
    if (!data)
	return FALSE;
	
    D(bug("[Kernel:i386] %s: Allocated platform data at 0x%p\n", __func__, data));
    KernelBase->kb_PlatformData = data;

    // Setup the base syscall handler(s) ...
    NEWLIST(&data->kb_SysCallHandlers);
    krnAddSysCallHandler(data, &x86_SCSupervisorHandler, FALSE, TRUE);

    /*
     * Now we have a complete memory list and working AllocMem().
     * We can allocate space for IDT and TSS now and build them to make
     * interrupts working.
     */
    tss            = krnAllocMemAligned(sizeof(struct tss), 64);
    idt            = krnAllocMemAligned(sizeof(long long) * 256, 256);
    SysBase->SysStkLower = AllocMem(0x10000, MEMF_PUBLIC);  /* 64KB of system stack */

    if ((!tss) || (!idt) || (!SysBase->SysStkLower))
	return FALSE;

    tss->ssp_seg = KERNEL_DS; /* SSP segment descriptor */
    tss->cs      = USER_CS;
    tss->ds      = USER_DS;
    tss->es      = USER_DS;
    tss->ss      = USER_DS;
    tss->iomap   = 104;

    /* Set up system stack */
    SysBase->SysStkUpper = SysBase->SysStkLower + 0x10000;
    tss->ssp       = (IPTR)SysBase->SysStkUpper;

    /* Restore IDT structure */
    Init_Traps(data, idt);

    /* Set correct TSS address in the GDT */
    GDT[6].base_low  = ((unsigned long)tss) & 0xffff;
    GDT[6].base_mid  = (((unsigned long)tss) >> 16) & 0xff;
    GDT[6].base_high = (((unsigned long)tss) >> 24) & 0xff;

    /*
     * As we prepared all necessary stuff, we can hopefully load IDT
     * into CPU. We may also play a bit with TSS
     */
    idtr.size = 0x07FF;
    idtr.base = (unsigned long)idt;
    asm
    (
	"lidt %0\n\t"
	"ltr %%ax\n\t"
	::"m"(idtr),"ax"(0x30)
    );

    D(bug("[Kernel:i386] %s: System restored\n", __func__));

    return TRUE;
}

ADD2INITLIB(PlatformInit, 10);
