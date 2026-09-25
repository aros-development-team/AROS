/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    BCM2711 (Raspberry Pi 4) platform support for AArch64.

    The Pi 4 differs from the Pi 2/3 (bcm2708/2836/2837) in two ways that
    matter to the kernel: the peripheral base moves to 0xFE000000 (low-
    peripheral mode) and interrupts are delivered by a real ARM GIC-400
    (GICv2) instead of the legacy Broadcom controller. The mailbox,
    framebuffer and PL011 are the same peripherals at the new base, so this
    file reuses bcm2708's SoC-common code and only replaces the interrupt
    controller (GIC-400) and the timer (ARM generic timer / CNTP, PPI 30).
*/

#include <aros/kernel.h>
#include <aros/symbolsets.h>

#include "kernel_base.h"

#include <proto/kernel.h>
#include <proto/exec.h>

#include <inttypes.h>
#include <hardware/intbits.h>

#include "kernel_intern.h"
#include "kernel_debug.h"
#include "kernel_cpu.h"
#include "kernel_interrupts.h"
#include "kernel_intr.h"
#include "kernel_fb.h"
#include "tls.h"
#include "io.h"

#include "exec_platform.h"

/* SoC-common bits shared with the Pi 2/3 (platform_bcm27xx.c) */
#include "bcm27xx.h"
#include "gic400.h"

#define DTIMER(x)

/* ---- GIC-400 (GICv2), Pi 4 fixed physical addresses ---- */
#define GICD_BASE   0xFF841000UL
#define GICC_BASE   0xFF842000UL
#define GICD(o)     (*(volatile uint32_t *)(GICD_BASE + (o)))
#define GICC(o)     (*(volatile uint32_t *)(GICC_BASE + (o)))

#define GICD_CTLR       0x000
#define GICD_ISENABLER  0x100
#define GICD_ICENABLER  0x180
#define GICD_IPRIORITYR 0x400
#define GICD_ITARGETSR  0x800
#define GICD_ICFGR      0xC00

/* INTIDs below this are private to a core (SGIs and PPIs); at and above
   it they are shared and need explicit routing and trigger configuration. */
#define GIC_FIRST_SPI   32
/* INTIDs below this are SGIs (IPIs) */
#define GIC_FIRST_PPI   16

#define GICC_CTLR   0x000
#define GICC_PMR    0x004
#define GICC_IAR    0x00C
#define GICC_EOIR   0x010

#define GIC_SPURIOUS 1023

/* ARM generic timer (EL1 physical, CNTP) -> scheduling heartbeat. PPI 30. */
#define GENTIMER_PPI    30
#define GENTIMER_HZ     50

static uint64_t gentimer_interval;

/* -------------------------- GIC-400 driver -------------------------- */

static void bcm2711_irq_init(void)
{
    /* Distributor + CPU interface: mask everything, then enable, PMR open. */
    GICD(GICD_CTLR) = 0;
    GICC(GICC_CTLR) = 0;
    GICC(GICC_PMR) = 0xF0;
    GICD(GICD_CTLR) = 1;
    GICC(GICC_CTLR) = 1;
}

static void bcm2711_irq_enable(int irq)
{
    *((volatile uint8_t *)(GICD_BASE + GICD_IPRIORITYR + irq)) = 0xA0;

    if (irq >= GIC_FIRST_SPI)
    {
        /*
         * A shared interrupt reaches no core until it is targeted at one,
         * and peripheral lines on this SoC are level triggered.
         */
        uint32_t cfg;

        *((volatile uint8_t *)(GICD_BASE + GICD_ITARGETSR + irq)) = 0x01;

        cfg = GICD(GICD_ICFGR + 4 * (irq / 16));
        cfg &= ~(2u << ((irq % 16) * 2));
        GICD(GICD_ICFGR + 4 * (irq / 16)) = cfg;
    }

    GICD(GICD_ISENABLER + 4 * (irq / 32)) = 1u << (irq % 32);
}

static void bcm2711_irq_disable(int irq)
{
    GICD(GICD_ICENABLER + 4 * (irq / 32)) = 1u << (irq % 32);
}

/*
 * A source that is never acknowledged is re-presented as a fresh exception
 * rather than looping inside one dispatch, so the run length has to be
 * tracked across calls to be seen at all.
 */
static uint32_t irq_last = GIC_SPURIOUS;
static unsigned int irq_repeats;

static void bcm2711_gentimer_tick(void);

static void bcm2711_irq_process(void)
{
    for (;;)
    {
        uint32_t iar = GICC(GICC_IAR);
        uint32_t intid = iar & 0x3FF;

        if (intid >= GIC_SPURIOUS)
            break;

        /* SGIs and the tick bypass krnRunIRQHandlers: the global
         * KernelBase is still NULL while the secondaries already tick. */
        if (intid < GIC_FIRST_PPI)
            gic400_handle_ipi();
        else if (intid == GENTIMER_PPI)
            bcm2711_gentimer_tick();
        else if (KernelBase)
            krnRunIRQHandlers(KernelBase, intid);

        GICC(GICC_EOIR) = iar;

        /* Self-clearing, and the tick repeats by design; only SPIs (all on CPU 0) count */
        if (intid < GIC_FIRST_PPI || intid == GENTIMER_PPI)
            continue;

        /*
         * A level-triggered source that nobody acknowledges would be
         * re-presented forever and wedge the machine. Mask it once it has
         * proved it is not being cleared. Any other interrupt arriving in
         * between clears the count.
         */
        if (intid == irq_last)
        {
            if (++irq_repeats > 10000)
            {
                bcm2711_irq_disable(intid);
                bug("[Kernel] IRQ %u not cleared by its handler, masked\n", intid);
                irq_repeats = 0;
                break;
            }
        }
        else
        {
            irq_last = intid;
            irq_repeats = 0;
        }
    }
}

/* --------------- ARM generic timer (CNTP) scheduler tick --------------- */

static void bcm2711_gentimer_tick(void)
{
    /* Reload the compare for the next quantum. */
    __asm__ volatile("msr cntp_tval_el0, %0" :: "r"(gentimer_interval));

#if defined(__AROSEXEC_SMP__)
    /* PPI 30 is banked: each core expires its own quantum, VBlank stays on core 0 */
    bcm27xx_sched_tick();

    if (GetCPUNumber() != 0)
        return;
#endif

    /* Drive the exec scheduler quantum (same mechanism as bcm2708). */
    if (SysBase && (IDNESTCOUNT_GET < 0))
        core_Cause(INTB_VERTB, 1L << INTB_VERTB);

    /* Without this TaskTag_CPUUsage never advances */
    core_TaskCPUUsage();
}

/* CNTP and its PPI are per core */
static void bcm2711_init_timer_core(void)
{
    if (!gentimer_interval)
    {
        uint64_t freq;

        __asm__ volatile("mrs %0, cntfrq_el0" : "=r"(freq));
        gentimer_interval = freq / GENTIMER_HZ;
    }

    bcm2711_irq_enable(GENTIMER_PPI);

    __asm__ volatile("msr cntp_tval_el0, %0" :: "r"(gentimer_interval));
    __asm__ volatile("msr cntp_ctl_el0, %0" :: "r"((uint64_t)1));
}

static APTR bcm2711_init_gentimer(APTR _kernelBase)
{
    DTIMER(bug("[Kernel:BCM2711] %s\n", __func__));

    bcm2711_init_timer_core();

    /* No IntrNode: bcm2711_irq_process dispatches the tick directly */
    return _kernelBase;
}

/* --------------------------- per-core setup --------------------------- */

static void bcm2711_init_cpu(APTR _kernelBase, APTR _sysBase)
{
    struct ExecBase *SysBase = (struct ExecBase *)_sysBase;
    struct KernelBase *KernelBase = (struct KernelBase *)_kernelBase;
    (void)SysBase; (void)KernelBase;

    /* Per-CPU TLS; PrepareExecBase only sets the boot core's */
    SCHEDQUANTUM_SET(SCHEDQUANTUM_VALUE);
    SCHEDELAPSED_SET(SCHEDQUANTUM_VALUE);

    gic400_init_core();
}

/* ------------------------------- probe ------------------------------- */

static IPTR bcm2711_probe(struct ARM_Implementation *krnARMImpl, struct TagItem *msg)
{
    void *bootPutC = NULL;

    while (msg->ti_Tag != TAG_DONE)
    {
        switch (msg->ti_Tag)
        {
        case KRN_FuncPutC:
            bootPutC = (void *)msg->ti_Data;
            break;
        }
        msg++;
    }

    /* BCM2711 (Raspberry Pi 4) uses platform ID 0xc44 (set by the boot when
       the device tree reports a bcm2711). */
    if (krnARMImpl->ARMI_Platform != 0xc44)
        return FALSE;

    krnARMImpl->ARMI_PeripheralBase = (APTR)0xFE000000;
    krnARMImpl->ARMI_InitCore = &bcm2711_init_cpu;
    krnARMImpl->ARMI_SendIPI = &gic400_send_ipi;

    krnARMImpl->ARMI_GetTime = &bcm27xx_get_time;
    krnARMImpl->ARMI_InitTimer = &bcm2711_init_gentimer;
#if defined(__AROSEXEC_SMP__)
    krnARMImpl->ARMI_InitTimerCore = &bcm2711_init_timer_core;
#endif
    krnARMImpl->ARMI_LED_Toggle = &bcm27xx_toggle_led;

    krnARMImpl->ARMI_SerPutChar = &bcm27xx_ser_putc;
    krnARMImpl->ARMI_SerGetChar = &bcm27xx_ser_getc;

    if ((krnARMImpl->ARMI_PutChar = bootPutC) != NULL)
    {
        krnARMImpl->ARMI_PutChar(0xFF); /* Clear the display */
    }

    gic400_setbase(GICD_BASE, GICC_BASE);

    krnARMImpl->ARMI_IRQInit = &bcm2711_irq_init;
    krnARMImpl->ARMI_IRQEnable = &bcm2711_irq_enable;
    krnARMImpl->ARMI_IRQDisable = &bcm2711_irq_disable;
    krnARMImpl->ARMI_IRQProcess = &bcm2711_irq_process;

    krnARMImpl->ARMI_Init = &bcm27xx_init;

    return TRUE;
}

ADD2SET(bcm2711_probe, ARMPLATFORMS, 0);
