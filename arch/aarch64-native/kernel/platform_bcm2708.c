/*
    Copyright (C) 2015-2026, The AROS Development Team. All rights reserved.

    BCM2708/2836/2837 platform support for AArch64.

    SoC-common code (SMP bring-up, mailbox FIQ/IPI, system timer counter,
    LED, PL011 serial) lives in platform_bcm27xx.c; this file provides the
    legacy Broadcom interrupt controller and the GPU system timer heartbeat.
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
#include "io.h"

#include "exec_platform.h"

#include "bcm27xx.h"

#define ARM_PERIIOBASE ((IPTR)__arm_arosintern.ARMI_PeripheralBase)
#include <hardware/bcm2708.h>

#define IRQBANK_POINTER(bank)   ((bank == 0) ? GPUIRQ_ENBL0 : (bank == 1) ? GPUIRQ_ENBL1 : ARMIRQ_ENBL)

#define IRQ_BANK1       0x00000100
#define IRQ_BANK2       0x00000200

#define DIRQ(x)
#define DTIMER(x)

static void bcm2708_irq_init(void)
{
    /* Make sure no source is stolen by the FIQ path: a source selected in
     * ARMFIQ_CTRL is removed from the IRQ pending registers entirely. */
    wr32le(ARMFIQ_CTRL, 0);

    /*
     * Route the aggregated GPU interrupt (the whole legacy BCM2708
     * controller: system timer, SD, USB, ...) to core 0's IRQ input.
     * The reset value is 0 (= core 0 IRQ), but the firmware/armstub
     * state on real Pi 2/3 is not the reset state -- without this
     * write GPU IRQs never reach the ARM and the boot hangs at the
     * first timed DoIO (empirically confirmed on real hardware).
     */
    wr32le(BCM2836_GPU_INT_ROUTING, 0);

    wr32le(ARMIRQ_DIBL, ~0);
    wr32le(GPUIRQ_DIBL0, ~0);
    wr32le(GPUIRQ_DIBL1, ~0);
}

static void bcm2708_irq_enable(int irq)
{
    int bank = IRQ_BANK(irq);
    uintptr_t reg;

    reg = (uintptr_t)IRQBANK_POINTER(bank);

    DIRQ(bug("[Kernel:BCM2708] Enabling irq %d [bank %d, reg 0x%p]\n", irq, bank, reg));

    wr32le(reg, IRQ_MASK(irq));

    DIRQ(bug("[Kernel:BCM2708] irqmask=%08x\n", rd32le(reg)));
}

static void bcm2708_irq_disable(int irq)
{
    int bank = IRQ_BANK(irq);
    uintptr_t reg;

    reg = (uintptr_t)IRQBANK_POINTER(bank) + 0x0c;

    DIRQ(bug("[Kernel:BCM2708] Disabling irq %d [bank %d, reg 0x%p]\n", irq, bank, reg));

    wr32le(reg, IRQ_MASK(irq));

    DIRQ(bug("[Kernel:BCM2708] irqmask=%08x\n", rd32le(reg)));
}

static void bcm2708_irq_process()
{
    unsigned int pendingarm, pending0, pending1, irq;

    for (;;)
    {
        pendingarm = rd32le(ARMIRQ_PEND);
        pending0 = rd32le(GPUIRQ_PEND0);
        pending1 = rd32le(GPUIRQ_PEND1);

        if (!(pendingarm || pending0 || pending1))
            break;

        DIRQ(bug("[Kernel:BCM2708] PendingARM %08x\n", pendingarm));
        DIRQ(bug("[Kernel:BCM2708] Pending0 %08x\n", pending0));
        DIRQ(bug("[Kernel:BCM2708] Pending1 %08x\n", pending1));

        if (pendingarm & ~(IRQ_BANK1 | IRQ_BANK2))
        {
            for (irq = (2 << 5); irq < ((2 << 5) + 8); irq++)
            {
                if (pendingarm & (1 << (irq - (2 << 5))))
                {
                    DIRQ(bug("[Kernel:BCM2708] Handling IRQ %d ..\n", irq));
                    krnRunIRQHandlers(KernelBase, irq);
                }
            }
        }

        if (pending0)
        {
            for (irq = (0 << 5); irq < ((0 << 5) + 32); irq++)
            {
                if (pending0 & (1 << (irq - (0 << 5))))
                {
                    DIRQ(bug("[Kernel:BCM2708] Handling IRQ %d ..\n", irq));
                    krnRunIRQHandlers(KernelBase, irq);
                }
            }
        }

        if (pending1)
        {
            for (irq = (1 << 5); irq < ((1 << 5) + 32); irq++)
            {
                if (pending1 & (1 << (irq - (1 << 5))))
                {
                    DIRQ(bug("[Kernel:BCM2708] Handling IRQ %d ..\n", irq));
                    krnRunIRQHandlers(KernelBase, irq);
                }
            }
        }
    }
}

/* Use system timer 3 for scheduling heartbeat */
#define VBLANK_TIMER            3
#define VBLANK_INTERVAL         (1000000 / 50)

static void bcm2708_gputimer_handler(unsigned int timerno, void *unused1)
{
    unsigned int stc;

    DTIMER(bug("[Kernel:BCM2708] %s(%d)\n", __PRETTY_FUNCTION__, timerno));

    /* Acknowledge timer interrupt */
    wr32le(SYSTIMER_CS, 1 << timerno);

    /* Signal the Exec VBlankServer */
    if (SysBase && (IDNESTCOUNT_GET < 0)) {
        core_Cause(INTB_VERTB, 1L << INTB_VERTB);
    }

    /* Refresh timer */
    stc = rd32le(SYSTIMER_CLO);
    stc += VBLANK_INTERVAL;
    wr32le(SYSTIMER_C0 + (timerno * 4), stc);

    DTIMER(bug("[BCM2708] %s: Done..\n", __PRETTY_FUNCTION__));
}

static APTR bcm2708_init_gputimer(APTR _kernelBase)
{
    struct KernelBase *KernelBase = (struct KernelBase *)_kernelBase;
    struct IntrNode *GPUTimerHandle;
    unsigned int stc;

    DTIMER(bug("[Kernel:BCM2708] %s(%012p)\n", __PRETTY_FUNCTION__, KernelBase));

    if ((GPUTimerHandle = AllocMem(sizeof(struct IntrNode), MEMF_PUBLIC|MEMF_CLEAR)) != NULL)
    {
        DTIMER(bug("[Kernel:BCM2708] %s: IntrNode @ 0x%p:\n", __PRETTY_FUNCTION__, GPUTimerHandle));
        DTIMER(bug("[Kernel:BCM2708] %s: Using GPUTimer %d for VBlank\n", __PRETTY_FUNCTION__, VBLANK_TIMER));

        GPUTimerHandle->in_Handler = bcm2708_gputimer_handler;
        GPUTimerHandle->in_HandlerData = (void *)(uintptr_t)VBLANK_TIMER;
        GPUTimerHandle->in_HandlerData2 = KernelBase;
        GPUTimerHandle->in_type = it_interrupt;
        GPUTimerHandle->in_nr = IRQ_TIMER0 + VBLANK_TIMER;

        ADDHEAD(&KernelBase->kb_Interrupts[IRQ_TIMER0 + VBLANK_TIMER], &GPUTimerHandle->in_Node);

        DTIMER(bug("[Kernel:BCM2708] %s: Enabling Hardware IRQ.. \n", __PRETTY_FUNCTION__));

        stc = rd32le(SYSTIMER_CLO);
        stc += VBLANK_INTERVAL;
        wr32le(SYSTIMER_CS, 1 << VBLANK_TIMER);
        wr32le(SYSTIMER_C0 + (VBLANK_TIMER * 4), stc);

        ictl_enable_irq(IRQ_TIMER0 + VBLANK_TIMER, KernelBase);

        /*
         * One-shot diagnostic of the interrupt chain (real-Pi bring-up):
         * poll for two VBlank intervals and report each hop -- does the
         * SYSTIMER compare fire (CS), does it show as pending in the
         * legacy controller (GPUIRQ_PEND0), and does the aggregated GPU
         * interrupt reach core 0 (BCM2836_IRQ_PEND0 bit 8)? IRQs are
         * still handled normally afterwards; this only reads/reports.
         */
        {
            unsigned int t0 = rd32le(SYSTIMER_CLO);
            unsigned int cs = 0, pend0 = 0, core0 = 0;

            while ((rd32le(SYSTIMER_CLO) - t0) < (2 * VBLANK_INTERVAL))
            {
                cs    |= rd32le(SYSTIMER_CS);
                pend0 |= rd32le(GPUIRQ_PEND0);
                core0 |= rd32le(BCM2836_IRQ_PEND0);
            }
            bug("[Kernel:BCM2708] IRQ chain probe: CS=%08x PEND0=%08x CORE0=%08x ENBL0=%08x ROUTING=%08x\n",
                cs, pend0, core0, rd32le(GPUIRQ_ENBL0), rd32le(BCM2836_GPU_INT_ROUTING));
        }
    }

    DTIMER(bug("[Kernel:BCM2708] %s: Done.. \n", __PRETTY_FUNCTION__));

    return GPUTimerHandle;
}

static IPTR bcm2708_probe(struct ARM_Implementation *krnARMImpl, struct TagItem *msg)
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

    /* BCM2837 (RPi3 in AArch64 mode) uses platform ID 0xc43 */
    if (krnARMImpl->ARMI_Platform != 0xc43)
        return FALSE;

    /* BCM2837 is ARMv8 (family 8) */
    krnARMImpl->ARMI_PeripheralBase = (APTR)BCM2836_PERIPHYSBASE;
    krnARMImpl->ARMI_InitCore = &bcm27xx_init_cpu;
    krnARMImpl->ARMI_FIQProcess = &bcm27xx_fiq_process;
    krnARMImpl->ARMI_SendIPI = &bcm27xx_send_ipi;

    krnARMImpl->ARMI_GetTime = &bcm27xx_get_time;
    krnARMImpl->ARMI_InitTimer = &bcm2708_init_gputimer;
    krnARMImpl->ARMI_LED_Toggle = &bcm27xx_toggle_led;

    krnARMImpl->ARMI_SerPutChar = &bcm27xx_ser_putc;
    krnARMImpl->ARMI_SerGetChar = &bcm27xx_ser_getc;

    if ((krnARMImpl->ARMI_PutChar = bootPutC) != NULL)
    {
        krnARMImpl->ARMI_PutChar(0xFF); /* Clear the display */
    }

    krnARMImpl->ARMI_IRQInit = &bcm2708_irq_init;
    krnARMImpl->ARMI_IRQEnable = &bcm2708_irq_enable;
    krnARMImpl->ARMI_IRQDisable = &bcm2708_irq_disable;
    krnARMImpl->ARMI_IRQProcess = &bcm2708_irq_process;

    krnARMImpl->ARMI_Init = &bcm27xx_init;

    return TRUE;
}

ADD2SET(bcm2708_probe, ARMPLATFORMS, 0);
