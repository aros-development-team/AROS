/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Intel IA-32 APIC driver.
*/

#include <asm/io.h>
#include <exec/types.h>

#include <inttypes.h>

#include "kernel_base.h"
#include "kernel_debug.h"
#include "kernel_intern.h"
#include "kernel_syscall.h"
#include "apic.h"
#include "apic_ia32.h"

#define D(x)
#define DEBUG_WAIT

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

static ULONG DoIPI(IPTR __APICBase, ULONG target, ULONG cmd)
{
    ULONG ipisend_timeout, status_ipisend;

    D(bug("[IPI] Command 0x%08X to target %u\n", cmd, target));

    /*
     * Send the IPI.
     * First we write target APIC ID into high command register.
     * Writing to the low register triggers the IPI itself.
     */
    APIC_REG(__APICBase, APIC_ICRH) = target << 24;
    APIC_REG(__APICBase, APIC_ICRL) = cmd;

    D(bug("[IPI] Waiting for IPI to complete ", __thisAPICNo));

    for (ipisend_timeout = 1000; ipisend_timeout > 0; ipisend_timeout--)
    {
        udelay(100);
#ifdef DEBUG_WAIT
        if ((ipisend_timeout % 100) == 0)
        {
            bug(".");
        }
#endif
        status_ipisend = APIC_REG(__APICBase, APIC_ICRL) & ICR_DS;
        /* Delivery status resets to 0 when delivery is done */
        if (status_ipisend == 0)
            break;
    }
    D(bug("\n"));
    D(bug("[IPI] ... left wait loop (status = 0x%08X)\n", status_ipisend));

    return status_ipisend;
}

/**********************************************************
                        Driver functions
 **********************************************************/

static IPTR _APIC_IA32_probe(void)
{
    return 1; /* should be called last. */
} 

static inline ULONG apic_GetMaxLVT(IPTR __APICBase)
{
    ULONG apic_ver = APIC_REG(__APICBase, APIC_VERSION);
     /* 82489DXs doesnt report no. of LVT entries. */
    ULONG maxlvt = (apic_ver & 0xF0) ? (apic_ver & APIC_LVT_MASK) >> APIC_LVT_SHIFT : 2;
    
    return maxlvt;
}

static IPTR _APIC_IA32_init(IPTR __APICBase)
{
    ULONG APIC_VAL, maxlvt;

    APIC_REG(__APICBase, APIC_DFR) = 0xFFFFFFFF; /* Put the APIC into flat delivery mode */

    /* Set up the logical destination ID.  */
    APIC_VAL = APIC_REG(__APICBase, APIC_LDR) & ~(0xFF<<24);
    APIC_VAL |= (1 << 24);
    APIC_REG(__APICBase, APIC_LDR) = APIC_VAL;
    D(bug("[Kernel] _APIC_IA32_init: APIC Logical Destination ID: %lx\n", APIC_VAL));

    /* Set Task Priority to 'accept all' */
    APIC_VAL = APIC_REG(__APICBase, APIC_TPR) & ~0xFF;
    APIC_REG(__APICBase, APIC_TPR) = APIC_VAL;

    D(bug("[Kernel] _APIC_IA32_init: APIC TPR=%08x\n", APIC_REG(__APICBase, APIC_TPR)));
    /* ??? What's 0x314 ??? */
    D(bug("[Kernel] _APIC_IA32_init: APIC ICR=%08x%08x\n", APIC_REG(__APICBase, 0x314), APIC_REG(__APICBase, APIC_ICRH)));

    APIC_VAL = APIC_REG(__APICBase, APIC_SVR) & ~0xFF;
    APIC_VAL |= (1 << 8); /* Enable APIC */
    APIC_VAL |= (1 << 9); /* Disable focus processor (bit==1) */
    APIC_VAL |= 0xFF; /* Set spurious IRQ vector */
    APIC_REG(__APICBase, APIC_SVR) = APIC_VAL;
    D(bug("[Kernel] _APIC_IA32_init: APIC SVR=%08x\n", APIC_REG(__APICBase, APIC_SVR)));

    D(bug("[Kernel] _APIC_IA32_init: APIC Timer divide=%08x\n", APIC_REG(__APICBase, APIC_TIMER_DIV)));
    D(bug("[Kernel] _APIC_IA32_init: APIC Timer config=%08x\n", APIC_REG(__APICBase, APIC_TIMER_VEC)));

    APIC_REG(__APICBase, APIC_LINT0_VEC) = 0x700;
    /* only the BSP should see the LINT1 NMI signal.  */
    APIC_REG(__APICBase, APIC_LINT1_VEC) = 0x400;

    D(bug("[Kernel] _APIC_IA32_init: APIC LVT0=%08x\n", APIC_REG(__APICBase, APIC_LINT0_VEC)));
    D(bug("[Kernel] _APIC_IA32_init: APIC LVT1=%08x\n", APIC_REG(__APICBase, APIC_LINT1_VEC)));

    /* Due to the Pentium erratum 3AP. */
    maxlvt = apic_GetMaxLVT(__APICBase);
    if (maxlvt > 3)
       APIC_REG(__APICBase, APIC_ESR) = 0;

    D(bug("[Kernel] _APIC_IA32_init: APIC ESR before enabling vector: %08x\n", APIC_REG(__APICBase, APIC_ESR)));
 
    APIC_REG(__APICBase, APIC_ERROR_VEC) = 0xfe; /* Enable error sending, interrupt 0xFE */

     /* spec says clear errors after enabling vector. */
    if (maxlvt > 3)
	APIC_REG(__APICBase, APIC_ESR) = 0;

    D(bug("[Kernel] _APIC_IA32_init: APIC ESR after enabling vector: %08x\n", APIC_REG(__APICBase, APIC_ESR)));

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

#define MAX_STARTS 2

static IPTR _APIC_IA32_wake(APTR wake_apicstartrip, UBYTE wake_apicid, IPTR __APICBase)
{
    ULONG status_ipisend, status_ipirecv;
    ULONG maxlvt, start_count;
    D(UBYTE __thisAPICNo = core_APIC_GetID(__APICBase));

    D(bug("[Kernel] _APIC_IA32_wake[%d](%d @ %p)\n", __thisAPICNo, wake_apicid, wake_apicstartrip));
    D(bug("[Kernel] _APIC_IA32_wake[%d] KernelBase @ %p, APIC No %d Base @ %p\n", __thisAPICNo, KernelBase, __thisAPICNo, __APICBase));

    /* First we send the INIT command (reset the core). Vector must be zero for this. */
    DoIPI(__APICBase, wake_apicid, ICR_INT_LEVELTRIG | ICR_INT_ASSERT | ICR_DM_INIT);

    /* Deassert INIT after a small delay */
    udelay(10 * 1000);
    DoIPI(__APICBase, wake_apicid, ICR_INT_LEVELTRIG | ICR_DM_INIT);

    /* memory barrier */
    do { asm volatile("mfence":::"memory"); }while(0);

    /* check for Pentium erratum 3AP .. */
    maxlvt = apic_GetMaxLVT(__APICBase);

    /* Perform IPI STARTUP loop */
    for (start_count = 1; start_count <= MAX_STARTS; start_count++)
    {
        D(bug("[Kernel] _APIC_IA32_wake[%d]: Attempting STARTUP .. %d\n", __thisAPICNo, start_count));

	/* Clear any pending error condition */
        APIC_REG(__APICBase, APIC_ESR) = 0;

        /* Send STARTUP IPI.  */
        status_ipisend = DoIPI(__APICBase, wake_apicid, ICR_DM_STARTUP | ((IPTR)wake_apicstartrip >> 12));

        /* Allow the target APIC to accept the IPI */
        udelay(200);

        if (maxlvt > 3)
            APIC_REG(__APICBase, APIC_ESR) = 0;

        status_ipirecv = APIC_REG(__APICBase, APIC_ESR) & 0xEF;
        
        /* Is everything ok? No need to retry then. */
        if ((!status_ipisend) && (!status_ipirecv))
            break;

        D(bug("[Kernel] _APIC_IA32_wake[%d]: STARTUP run status 0x%08X, error 0x%08X\n", __thisAPICNo, status_ipisend, status_ipirecv));
    }

    D(bug("[Kernel] _APIC_IA32_wake[%d]: STARTUP run finished...\n", __thisAPICNo));

    return (status_ipisend | status_ipirecv);
}

static IPTR _APIC_IA32_GetMSRAPICBase(void)
{
    IPTR _apic_base = 0;

    if (!(IN_USER_MODE))
    {
        _apic_base = rdmsrq(MSR_LAPIC_BASE);
    }
    else
    {
        D(bug("[Kernel] _APIC_IA32_GetMSRAPICBase: Called in UserMode\n"));

	__asm__ __volatile__ ("int $0x80":"=a"(_apic_base):"a"(SC_RDMSR),"c"(MSR_LAPIC_BASE));
    }
    _apic_base &= APIC_BASE_MASK;

    D(bug("[Kernel] _APIC_IA32_GetMSRAPICBase: MSR APIC Base @ %p\n", _apic_base));
    return _apic_base;
}

static IPTR _APIC_IA32_GetID(IPTR _APICBase)
{
    UBYTE _apic_id;

    /* The actual ID is in 8 most significant bits */
    _apic_id = APIC_REG(_APICBase, APIC_ID) >> 24;
    D(bug("[Kernel] _APIC_IA32_GetID: APIC ID %d\n", _apic_id));

    return _apic_id;
}

static void _APIC_IA32_Ack(UBYTE intnum)
{
    /* Write zero to EOI of current APIC */
    IPTR apic_base = rdmsrq(MSR_LAPIC_BASE) & APIC_BASE_MASK;

    APIC_REG(apic_base, APIC_EOI) = 0;
}

/**********************************************************/

const struct GenericAPIC apic_ia32_default =
{
    name        : "IA32 default",
    probe       : (APTR)_APIC_IA32_probe,
    getbase     : (APTR)_APIC_IA32_GetMSRAPICBase,
    getid       : (APTR)_APIC_IA32_GetID,
    wake        : (APTR)_APIC_IA32_wake,
    init        : (APTR)_APIC_IA32_init,
    ack		: (APTR)_APIC_IA32_Ack
};
