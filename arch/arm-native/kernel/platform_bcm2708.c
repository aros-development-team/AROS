/*
    Copyright © 2015, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/kernel.h>
#include <aros/symbolsets.h>

#include <proto/exec.h>

#include <inttypes.h>
#include <hardware/intbits.h>

#include "kernel_intern.h"
#include "kernel_base.h"
#include "kernel_cpu.h"
#include "kernel_interrupts.h"
#include "kernel_intr.h"
#include "kernel_fb.h"

#define ARM_PERIIOBASE __arm_arosintern.ARMI_PeripheralBase
#include <hardware/bcm2708.h>
#include <hardware/pl011uart.h>

#define IRQBANK_POINTER(bank)   ((bank == 0) ? GPUIRQ_ENBL0 : (bank == 1) ? GPUIRQ_ENBL1 : ARMIRQ_ENBL)
#define IRQ_BANK1	0x00000100
#define IRQ_BANK2	0x00000200

#define DIRQ(x)

extern void cpu_Register(void);

static void bcm2708_init(void)
{

}

static unsigned int bcm2807_get_time(void)
{
    return *((volatile unsigned int *)(SYSTIMER_CLO));
}

static void bcm2807_irq_init(void)
{
    // disable IRQ's
    *(volatile unsigned int *)ARMIRQ_DIBL = ~0;
    *(volatile unsigned int *)GPUIRQ_DIBL0 = ~0; 
    *(volatile unsigned int *)GPUIRQ_DIBL1 = ~0;
}

static void bcm2807_irq_enable(int irq)
{
    int bank = IRQ_BANK(irq);
    unsigned int val, reg;

    reg = IRQBANK_POINTER(bank);

    DIRQ(bug("[KRN:BCM2708] Enabling irq %d [bank %d, reg 0x%p]\n", irq, bank, reg));

    val = *((volatile unsigned int *)reg);
    val |= IRQ_MASK(irq);
    *((volatile unsigned int *)reg) = val;
}

static void bcm2807_irq_disable(int irq)
{
    int bank = IRQ_BANK(irq);
    unsigned int val, reg;

    reg = IRQBANK_POINTER(bank);

    DIRQ(bug("[KRN:BCM2708] Dissabling irq %d [bank %d, reg 0x%p]\n", irq, bank, reg));

    val = *((volatile unsigned int *)reg);
    val |= IRQ_MASK(irq);
    *((volatile unsigned int *)reg) = val;
}

static void bcm2807_irq_process()
{
    unsigned int pending, processed, irq;

    pending = *((volatile unsigned int *)(ARMIRQ_PEND));
    DIRQ(bug("[KRN] PendingARM %08x\n", pending));
    if (!(pending & IRQ_BANK1))
    {
        processed = 0;
        for (irq = (2 << 5); irq < ((2 << 5) + 32); irq++)
        {
            if (pending & (1 << (irq - (2 << 5))))
            {
                DIRQ(bug("[KRN] Handling IRQ %d ..\n", irq));
                krnRunIRQHandlers(KernelBase, irq);
                processed |= (1 << (irq - (2 << 5)));
            }
        }
    }
    else
    {
        processed = IRQ_BANK1;
    }
    if (processed) *((volatile unsigned int *)(ARMIRQ_PEND)) = (pending & ~processed);

    pending = *((volatile unsigned int *)(GPUIRQ_PEND0));
    DIRQ(bug("[KRN] Pending0 %08x\n", pending));
    if (!(pending & IRQ_BANK2))
    {
        processed = 0;
        for (irq = (0 << 5); irq < ((0 << 5) + 32); irq++)
        {
            if (pending & (1 << (irq - (0 << 5))))
            {
                DIRQ(bug("[KRN] Handling IRQ %d ..\n", irq));
                krnRunIRQHandlers(KernelBase, irq);
                processed |= (1 << (irq - (0 << 5)));
            }
        }
    }
    else
    {
        processed = IRQ_BANK2;
    }
    if (processed) *((volatile unsigned int *)(GPUIRQ_PEND0)) = (pending & ~processed);

    pending = *((volatile unsigned int *)(GPUIRQ_PEND1));
    DIRQ(bug("[KRN] Pending1 %08x\n", pending));
    processed = 0;
    for (irq = (1 << 5); irq < ((1 << 5) + 32); irq++)
    {
            if (pending & (1 << (irq - (1 << 5))))
        {
            DIRQ(bug("[KRN] Handling IRQ %d ..\n", irq));
            krnRunIRQHandlers(KernelBase, irq);
            processed |= (1 << (irq - (1 << 5)));
        }
    }
    if (processed) *((volatile unsigned int *)(GPUIRQ_PEND1)) = (pending & ~processed);
}


static void bcm2708_toggle_led(int LED, int state)
{
    if (__arm_arosintern.ARMI_PeripheralBase == BCM2836_PERIPHYSBASE)
    {
        int pin = 35;
        IPTR gpiofunc = GPCLR1;

        if (LED == ARM_LED_ACTIVITY)
            pin = 47;

        if (state == ARM_LED_ON)
            gpiofunc = GPSET1;

        *(volatile unsigned int *)gpiofunc = (1 << (pin-32));
    }
    else
    {
        // RasPi 1 only allows us to toggle the activity LED
        if (state)
            *(volatile unsigned int *)GPCLR0 = (1 << 16);
        else
            *(volatile unsigned int *)GPSET0 = (1 << 16);
    }
}

static void bcm2708_gputimer_handler(unsigned int timerno, void *unused1)
{
    unsigned int stc, cs;

    D(bug("[KRN:BCM2708] %s(%d)\n", __PRETTY_FUNCTION__, timerno));

    /* Aknowledge our timer interrupt */
    cs = *((volatile unsigned int *)(SYSTIMER_CS));
    cs &= ~ (1 << timerno);
    *((volatile unsigned int *)(SYSTIMER_CS)) = cs;

    /* Signal the Exec VBlankServer */
    if (SysBase && (SysBase->IDNestCnt < 0)) {
        core_Cause(INTB_VERTB, 1L << INTB_VERTB);
    }

    /* Refresh our timer interrupt */
    stc = *((volatile unsigned int *)(SYSTIMER_CLO));
    stc += VBLANK_INTERVAL;
    *((volatile unsigned int *)(SYSTIMER_CS)) = cs | (1 << timerno);
    *((volatile unsigned int *)(SYSTIMER_C0 + (timerno * 4))) = stc;

    D(bug("[BCM2708] %s: Done..\n", __PRETTY_FUNCTION__));
}

static bcm2708_init_gputimer(struct KernelBase *KernelBase)
{
    struct IntrNode *GPUTimerHandle;
    unsigned int stc;

    D(bug("[KRN:BCM2708] %s(%012p)\n", __PRETTY_FUNCTION__, KernelBase));

    if ((GPUTimerHandle = AllocMem(sizeof(struct IntrNode), MEMF_PUBLIC|MEMF_CLEAR)) != NULL)
    {
        D(bug("[KRN:BCM2708] %s: IntrNode @ 0x%p:\n", __PRETTY_FUNCTION__, GPUTimerHandle));
        D(bug("[KRN:BCM2708] %s: Using GPUTimer %d for VBlank\n", __PRETTY_FUNCTION__, VBLANK_TIMER));

        GPUTimerHandle->in_Handler = bcm2708_gputimer_handler;
        GPUTimerHandle->in_HandlerData = VBLANK_TIMER;
        GPUTimerHandle->in_HandlerData2 = KernelBase;
        GPUTimerHandle->in_type = it_interrupt;
        GPUTimerHandle->in_nr = IRQ_TIMER0 + VBLANK_TIMER;

        ADDHEAD(&KernelBase->kb_Interrupts[IRQ_TIMER0 + VBLANK_TIMER], &GPUTimerHandle->in_Node);

        D(bug("[KRN:BCM2708] %s: Enabling Hardware IRQ.. \n", __PRETTY_FUNCTION__));

        stc = *((volatile unsigned int *)(SYSTIMER_CLO));
        stc += VBLANK_INTERVAL;
        *((volatile unsigned int *)(SYSTIMER_CS)) = (1 << VBLANK_TIMER);
        *((volatile unsigned int *)(SYSTIMER_C0 + (VBLANK_TIMER * 4))) = stc;

        ictl_enable_irq(IRQ_TIMER0 + VBLANK_TIMER, KernelBase);
    }

    D(bug("[KRN:BCM2708] %s: Done.. \n", __PRETTY_FUNCTION__));

    return GPUTimerHandle;
}

static inline void bcm2708_ser_waitout()
{
    while(1)
    {
       if ((*(volatile uint32_t *)(PL011_0_BASE + PL011_FR) & PL011_FR_TXFF) == 0) break;
    }
}

static void bcm2708_ser_putc(uint8_t chr)
{
    bcm2708_ser_waitout();

    if (chr == '\n')
    {
        *(volatile uint32_t *)(PL011_0_BASE + PL011_DR) = '\r';
        bcm2708_ser_waitout();
    }
    *(volatile uint32_t *)(PL011_0_BASE + PL011_DR) = chr;
}

static int bcm2708_ser_getc(void)
{
    if ((*(volatile uint32_t *)(PL011_0_BASE + PL011_FR) & PL011_FR_RXFE) == 0)
        return (int)*(volatile uint32_t *)(PL011_0_BASE + PL011_DR);

    return -1;
}

static IPTR bcm2708_probe(struct ARM_Implementation *krnARMImpl, struct TagItem *msg)
{
    //TODO: really detect if we are running on a broadcom 2835/2836
    if (krnARMImpl->ARMI_Family == 7) /*  bcm2836 uses armv7 */
        krnARMImpl->ARMI_PeripheralBase = BCM2836_PERIPHYSBASE;
    else
        krnARMImpl->ARMI_PeripheralBase = BCM2835_PERIPHYSBASE;

    krnARMImpl->ARMI_GetTime = &bcm2807_get_time;
    krnARMImpl->ARMI_InitTimer = &bcm2708_init_gputimer;
    krnARMImpl->ARMI_LED_Toggle = &bcm2708_toggle_led;

    krnARMImpl->ARMI_SerPutChar = &bcm2708_ser_putc;
    krnARMImpl->ARMI_SerGetChar = &bcm2708_ser_getc;

    while(msg->ti_Tag != TAG_DONE)
    {
        switch (msg->ti_Tag)
        {
        case KRN_FuncPutC:
            krnARMImpl->ARMI_PutChar = (void *)msg->ti_Data;
            krnARMImpl->ARMI_PutChar(0xFF); // Clear the display
            break;
        }
        msg++;
    }

    krnARMImpl->ARMI_IRQInit = &bcm2807_irq_init;
    krnARMImpl->ARMI_IRQEnable = &bcm2807_irq_enable;
    krnARMImpl->ARMI_IRQDisable = &bcm2807_irq_disable;
    krnARMImpl->ARMI_IRQProcess = &bcm2807_irq_process;

    if (krnARMImpl->ARMI_PeripheralBase == BCM2836_PERIPHYSBASE)
    {
        int core;
        for (core = 1; core < 3; core ++)
        {
             *((volatile unsigned int *)(0x4000008C + (0x10 * core))) = cpu_Register;
        }

        if (krnARMImpl->ARMI_Delay)
            krnARMImpl->ARMI_Delay(1500);
    }

    return TRUE;
}

ADD2SET(bcm2708_probe, ARMPLATFORMS, 0);
