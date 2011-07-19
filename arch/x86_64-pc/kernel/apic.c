/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <inttypes.h>

#include <exec/lists.h>
#include <exec/types.h>
#include <exec/tasks.h>
#include <exec/execbase.h>
#include <aros/libcall.h>
#include <asm/io.h>
#include <proto/exec.h>

#include "kernel_base.h"
#include "kernel_debug.h"
#include "kernel_intern.h"
#include "acpi.h"
#include "apic.h"

#define D(x) x

#define CONFIG_LAPICS

#define APICICR_INT_LEVELTRIG 0x8000
#define APICICR_INT_ASSERT    0x4000
#define APICICR_DM_INIT       0x500
#define APICICR_DM_STARTUP    0x600

#if 1
/*  FIXME: udelay doesn't work - fix! */
void udelay(LONG usec)
{
}
#else
static ULONG usec2tick(ULONG usec)
{
    ULONG ret, timer_rpr = 3599597124UL;
    asm volatile("movl $0,%%eax; divl %2":"=a"(ret):"d"(usec),"m"(timer_rpr));
    return ret;
}
 
void udelay(LONG usec)
{

    int tick_start, tick;
    usec = usec2tick(usec);

    outb(0x80, 0x43);
    tick_start = inb(0x42);
    tick_start += inb(0x42) << 8;

    while (usec > 0) 
    { 
        outb(0x80, 0x43);
        tick = inb(0x42);
        tick += inb(0x42) << 8;

        usec -= (tick_start - tick);
        if (tick > tick_start) usec -= 0x10000;
        tick_start = tick;
    }
}
#endif

/**********************************************************
                            HOOKS
 **********************************************************/
static const char str_IA32APIC[] = "IA32 default";

static IPTR _APIC_IA32_probe(const struct GenericAPIC *hook, struct KernBootPrivate *__KernBootPrivate)
{
    /*  Default to PIC(8259) interrupt routing model.  This gets overriden later if IOAPICs are enumerated */
    __KernBootPrivate->kbp_APIC_IRQ_Model = ACPI_IRQ_MODEL_PIC;

    return 1; /* should be called last. */
} 

static IPTR _APIC_IA32_init(IPTR __APICBase)
{
    ULONG APIC_VAL, apic_ver, maxlvt;

    *(volatile ULONG *)(__APICBase + 0xE0) = 0xFFFFFFFF; /* Put the APIC into flat delivery mode */

    /* Set up the logical destination ID.  */
    APIC_VAL = *(volatile ULONG *)(__APICBase + 0xD0) & ~(0xFF<<24);
    APIC_VAL |= (1 << 24);
    *(volatile ULONG *)(__APICBase + 0xD0) = APIC_VAL;
    D(bug("[Kernel] _APIC_IA32_init: APIC Logical Destination ID: %lx\n", APIC_VAL));

    /* Set Task Priority to 'accept all' */
    APIC_VAL = *(volatile ULONG *)(__APICBase +  0x80) & ~0xFF;
    *(volatile ULONG *)(__APICBase + 0x80) = APIC_VAL;

    D(bug("[Kernel] _APIC_IA32_init: APIC TPR=%08x\n", *(volatile ULONG *)(__APICBase + 0x80)));
    D(bug("[Kernel] _APIC_IA32_init: APIC ICR=%08x%08x\n", *(volatile ULONG *)(__APICBase + 0x314), *(volatile ULONG *)(__APICBase + 0x310)));
    
    APIC_VAL = *(volatile ULONG *)(__APICBase + 0xF0) & ~0xFF;
    APIC_VAL |= (1 << 8); /* Enable APIC */
    APIC_VAL |= (1 << 9); /* Disable focus processor (bit==1) */
    APIC_VAL |= 0xFF; /* Set spurious IRQ vector */
    *(volatile ULONG *)(__APICBase + 0xF0) = APIC_VAL;
    D(bug("[Kernel] _APIC_IA32_init: APIC SVR=%08x\n", *(volatile ULONG *)(__APICBase + 0xf0)));

    D(bug("[Kernel] _APIC_IA32_init: APIC Timer divide=%08x\n", *(volatile ULONG *)(__APICBase + 0x3e0)));
    D(bug("[Kernel] _APIC_IA32_init: APIC Timer config=%08x\n", *(volatile ULONG *)(__APICBase + 0x320)));

    APIC_VAL = *(volatile ULONG *)(__APICBase + 0x350) & (1<<16);
    APIC_VAL = 0x700;
    *(volatile ULONG *)(__APICBase + 0x350) = APIC_VAL;

    /* only the BSP should see the LINT1 NMI signal.  */
    APIC_VAL = 0x400;
    *(volatile ULONG *)(__APICBase + 0x360) = APIC_VAL;

    D(bug("[Kernel] _APIC_IA32_init: APIC LVT0=%08x\n", *(volatile ULONG *)(__APICBase + 0x350)));
    D(bug("[Kernel] _APIC_IA32_init: APIC LVT1=%08x\n", *(volatile ULONG *)(__APICBase + 0x360)));

    /* Due to the Pentium erratum 3AP. */
    apic_ver = (*((volatile ULONG *)(__APICBase + 0x30)) & 0xFF);
    maxlvt = (apic_ver & 0xF0) ? ((*((volatile ULONG *)(__APICBase + 0x30)) >> 16) & 0xFF) : 2; /* 82489DXs doesnt report no. of LVT entries. */
    if (maxlvt > 3)
       *(volatile ULONG *)(__APICBase + 0x280) = 0;

    D(bug("[Kernel] _APIC_IA32_init: APIC ESR before enabling vector: %08lx\n", *(volatile ULONG *)(__APICBase + 0x280)));
 
    *(volatile ULONG *)(__APICBase + 0x370) = 0xfe; /* Enable error sending */

     /* spec says clear errors after enabling vector.  */
     if (maxlvt > 3)
       *(volatile ULONG *)(__APICBase + 0x280) = 0;

    D(bug("[Kernel] _APIC_IA32_init: APIC ESR after enabling vector: %08lx\n", *(volatile ULONG *)(__APICBase + 0x280)));
     
/*
    ULONG *localAPIC = (ULONG*)__APICBase + 0x320;

    asm volatile ("movl %0,(%1)"::"r"(0),"r"((volatile ULONG *)(__APICBase + 0xb0)));
    
    asm volatile ("movl %0,(%1)"::"r"(0x000000fe),"r"((volatile ULONG *)(__APICBase + 0x320)));
    // *(volatile ULONG *)localAPIC = 0x000000fe;
    D(bug("[Kernel] _APIC_IA32_init: APIC Timer config=%08x\n", *(volatile ULONG *)(__APICBase + 0x320)));
    
    D(bug("[Kernel] _APIC_IA32_init: APIC Initial count=%08x\n", *(volatile ULONG *)(__APICBase + 0x380)));
    D(bug("[Kernel] _APIC_IA32_init: APIC Current count=%08x\n", *(volatile ULONG *)(__APICBase + 0x390)));
    *(volatile ULONG *)(__APICBase + 0x380) = 0x11111111;
    asm volatile ("movl %0,(%1)"::"r"(0x000200fe),"r"((volatile ULONG *)(__APICBase + 0x320)));
    D(bug("[Kernel] _APIC_IA32_init: APIC Timer config=%08x\n", *(volatile ULONG *)(__APICBase + 0x320)));
    
    for (i=0; i < 0x10000000; i++) asm volatile("nop;");
    
    D(bug("[Kernel] _APIC_IA32_init: APIC Initial count=%08x\n", *(volatile ULONG *)(__APICBase + 0x380)));
    D(bug("[Kernel] _APIC_IA32_init: APIC Current count=%08x\n", *(volatile ULONG *)(__APICBase + 0x390)));
    for (i=0; i < 0x1000000; i++) asm volatile("nop;");
    D(bug("[Kernel] _APIC_IA32_init: APIC Initial count=%08x\n", *(volatile ULONG *)(__APICBase + 0x380)));
    D(bug("[Kernel] _APIC_IA32_init: APIC Current count=%08x\n", *(volatile ULONG *)(__APICBase + 0x390)));

    for (i=0; i < 0x1000000; i++) asm volatile("nop;"); */

    return TRUE;
} 

static IPTR _APIC_IA32_wake(APTR wake_apicstartrip, UBYTE wake_apicid, struct PlatformData *pdata)
{
    IPTR ipisend_timeout, status_ipisend = 0, status_ipirecv = 0;
    ULONG apic_ver, maxlvt, start_count, max_starts = 2;
    IPTR __APICBase;
    APTR _APICStackBase;

    UBYTE __thisAPICNo = core_APICGetNumber(pdata);

    __APICBase = pdata->kb_APIC_BaseMap[__thisAPICNo];

    D(bug("[Kernel] _APIC_IA32_wake[%d](%d @ %p)\n", __thisAPICNo, wake_apicid, wake_apicstartrip));
    D(bug("[Kernel] _APIC_IA32_wake[%d] KernelBase @ %p, APIC No %d Base @ %p\n", __thisAPICNo, KernelBase, __thisAPICNo, __APICBase));

    /* Setup stack for the new APIC */
    _APICStackBase = AllocMem(STACK_SIZE, MEMF_CLEAR);
    D(bug("[Kernel] _APIC_IA32_wake[%d]: stack allocated for APIC ID %d @ %p ..\n", __thisAPICNo, wake_apicid, _APICStackBase));

    *(APTR *)(wake_apicstartrip + 0x0018) = _APICStackBase + STACK_SIZE - SP_OFFSET;
    *(APTR *)(wake_apicstartrip + 0x0020) = kernel_cstart;

    D(bug("[Kernel] _APIC_IA32_wake[%d]:  ... and set\n", __thisAPICNo));

    /* Send the IPI by setting APIC_ICR : Set INIT on target APIC
       by writing the apicid to the destfield of APIC_ICR2 */

    *((volatile ULONG *)(__APICBase + 0x310)) = ((wake_apicid) << 24);
    *((volatile ULONG *)(__APICBase + 0x300)) = (APICICR_INT_LEVELTRIG | APICICR_INT_ASSERT | APICICR_DM_INIT);

    D(bug("[Kernel] _APIC_IA32_wake[%d]: Waiting for IPI INIT to complete ", __thisAPICNo));
    status_ipisend = 0;
    for (ipisend_timeout = 1000; ((ipisend_timeout > 0) && (status_ipisend == 0)); ipisend_timeout--)
    {
        udelay(100);
        D(
            if ((ipisend_timeout % 100) == 0)
            {
                bug(".");
            }
         );
        status_ipisend = *((volatile ULONG *)(__APICBase + 0x300)) & 0x1000;
    }
    D(bug("\n"));

    D(bug("[Kernel] _APIC_IA32_wake[%d]: ... left IPI INIT loop (status = %lx)\n", __thisAPICNo, status_ipisend));

    udelay(10 * 1000);

    D(bug("[Kernel] _APIC_IA32_wake[%d]: Sending IPI...\n", __thisAPICNo));

    /* Send the IPI by setting APIC_ICR */
    *((volatile ULONG *)(__APICBase + 0x310)) = ((wake_apicid)<<24); /* Set the target APIC */
    *((volatile ULONG *)(__APICBase + 0x300)) = APICICR_INT_LEVELTRIG | APICICR_DM_INIT;

    D(bug("[Kernel] _APIC_IA32_wake[%d]: Waiting for IPI INIT to deassert ", __thisAPICNo));
    status_ipisend = 0;
    for (ipisend_timeout = 1000; ((ipisend_timeout > 0) && (status_ipisend == 0)); ipisend_timeout--)
    {
        udelay(100);
        D(
            if ((ipisend_timeout % 100) == 0)
            {
                bug(".");
            }
        );
        status_ipisend = *((volatile ULONG *)(__APICBase + 0x300)) & 0x1000;
    }
    D(bug("\n"));

    D(bug("[Kernel] _APIC_IA32_wake[%d]: ... left IPI INIT deassert loop (status = %lx)\n", __thisAPICNo, status_ipisend));

    /* memory barrier */
    do { asm volatile("mfence":::"memory"); }while(0);

    /* check for Pentium erratum 3AP .. */
    apic_ver = (*((volatile ULONG *)(__APICBase + 0x30)) & 0xFF);
    maxlvt = (apic_ver & 0xF0) ? ((*((volatile ULONG *)(__APICBase + 0x30)) >> 16) & 0xFF) : 2; /* 82489DXs doesnt report no. of LVT entries. */

    /* Perform IPI STARTUP loop */
    for (start_count = 1; start_count<=max_starts; start_count++)
    {
        D(bug("[Kernel] _APIC_IA32_wake[%d]: Attempting STARTUP .. %d\n", __thisAPICNo, start_count));
        *((volatile ULONG *)(__APICBase + 0x280)) = 0;
        status_ipisend = *(volatile ULONG *)(__APICBase + 0x280);
        D(bug("[Kernel] _APIC_IA32_wake[%d]: IPI STARTUP sent\n", __thisAPICNo));

        /* STARTUP IPI */
        *((volatile ULONG *)(__APICBase + 0x310)) = ((wake_apicid)<<24); /* Set the target APIC */
        *((volatile ULONG *)(__APICBase + 0x300)) = APICICR_DM_STARTUP | ((IPTR)wake_apicstartrip >> 12);

        /* Allow the target APIC to accept the IPI */
        udelay(300);

        D(bug("[Kernel] _APIC_IA32_wake[%d]: Waiting for IPI STARTUP to complete...\n", __thisAPICNo));
        status_ipisend = 0;
        for (ipisend_timeout = 1000; ((ipisend_timeout > 0) && (status_ipisend == 0)); ipisend_timeout--)
        {
            udelay(100);
            D(
                if ((ipisend_timeout % 100) == 0)
                {
                    bug(".");
                }
             );
            status_ipisend = *((volatile ULONG *)(__APICBase + 0x300)) & 0x1000;
        }
        D(bug("\n"));

        D(bug("[Kernel] _APIC_IA32_wake[%d]: ... left IPI STARTUP loop (status = %lx)\n", __thisAPICNo, status_ipisend));

        /* Allow the target APIC to accept the IPI */
        udelay(200);

        if (maxlvt > 3)
            *((volatile ULONG *)(__APICBase + 0x280)) = 0;

        status_ipirecv = *((volatile ULONG *)(__APICBase + 0x280)) & 0xEF;
        if (status_ipisend || status_ipirecv) break;
    }
    D(bug("[Kernel] _APIC_IA32_wake[%d]: STARTUP run finished...\n", __thisAPICNo));

    if (status_ipisend)
    {
        bug("[Kernel] _APIC_IA32_wake[%d]: APIC delivery failed\n", __thisAPICNo);
    }
    if (status_ipirecv)
    {
        bug("[Kernel] _APIC_IA32_wake[%d]: APIC delivery error (%lx)\n", __thisAPICNo, status_ipirecv);
    }

    return (status_ipisend | status_ipirecv);
}

static IPTR _APIC_IA32_GetMSRAPICBase(void)
{
    IPTR _apic_base = 0;

    if (!(IN_USER_MODE))
    {
        _apic_base = rdmsrq(27) & ~0x900;
    }
    else
    {
        D(bug("[Kernel] _APIC_IA32_GetMSRAPICBase: Called in UserMode\n"));
        _apic_base = 0xfee00000;
    }

    D(bug("[Kernel] _APIC_IA32_GetMSRAPICBase: MSR APIC Base @ %p\n", _apic_base));

    return _apic_base;
}

static IPTR _APIC_IA32_GetID(IPTR _APICBase)
{
    UBYTE _apic_id;
    
    _apic_id = (*(volatile ULONG *)(_APICBase + 0x20) & 0xFF000000) >> 24;
    D(bug("[Kernel] _APIC_IA32_GetID: APIC ID %d\n", _apic_id));

    return _apic_id;
}

/**********************************************************/

static const struct GenericAPIC apic_ia32_default =
{
    name        : str_IA32APIC,
    probe       : (APTR)_APIC_IA32_probe,
    getbase     : (APTR)_APIC_IA32_GetMSRAPICBase,
    getid       : (APTR)_APIC_IA32_GetID,
    wake        : (APTR)_APIC_IA32_wake,
    init        : (APTR)_APIC_IA32_init
};

/************************************************************************************************/
/************************************************************************************************
                                    APIC Functions used by kernel.resource from outside this file ..
 ************************************************************************************************/
/************************************************************************************************/

/**********************************************************/

static const struct GenericAPIC *probe_APIC[] =
{
        &apic_ia32_default, /* must be last */
        NULL,
};

IPTR core_APICProbe(struct KernBootPrivate *__KernBootPrivate)
{
    int driver_count, changed = 0;

    __KernBootPrivate->kbp_APIC_Drivers = probe_APIC;

    for (driver_count = 0; !changed && probe_APIC[driver_count]; driver_count++ )
    {
    	IPTR retval = probe_APIC[driver_count]->probe(probe_APIC[driver_count], __KernBootPrivate);

    	if (retval)
        {
            changed = 1;
            __KernBootPrivate->kbp_APIC_DriverID = driver_count;
        }
    }

    if (!changed)
    {
        bug("[Kernel] core_APICProbe: No suitable APIC driver found.\n");
    }
    else
    {
        bug("[Kernel] core_APICProbe: Using APIC driver '%s'\n", ((struct GenericAPIC *)probe_APIC[__KernBootPrivate->kbp_APIC_DriverID])->name);
    }

    return changed;
}

UBYTE core_APICGetNumber(struct PlatformData *pdata)
{
    IPTR  __APICBase;
    UBYTE __APICLogicalID;
    UBYTE __APICNo;

    __APICBase = pdata->kb_APIC_Drivers[pdata->kb_APIC_DriverID]->getbase();
    __APICLogicalID = pdata->kb_APIC_Drivers[pdata->kb_APIC_DriverID]->getid(__APICBase);

    for (__APICNo = 0; __APICNo < pdata->kb_APIC_Count; __APICNo++)
    {
        if ((pdata->kb_APIC_IDMap[__APICNo] & 0xFF) == __APICLogicalID)
            return __APICNo;
    }

    return -1;
}
