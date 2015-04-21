/*
    Copyright © 2015, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/kernel.h>
#include <aros/symbolsets.h>

#include "kernel_base.h"

#include <proto/kernel.h>
#include <proto/exec.h>

#include <inttypes.h>
#include <hardware/intbits.h>

#include <strings.h>

#include "kernel_intern.h"
#include "kernel_debug.h"
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

#define D(x)
#define DIRQ(x)

extern mpcore_trampoline();
extern uint32_t mpcore_end;
extern uint32_t mpcore_pde;

extern void cpu_Register(void);
extern void arm_flush_cache(uint32_t addr, uint32_t length);

static void bcm2708_init(APTR _kernelBase, APTR _sysBase)
{
    struct ExecBase *SysBase = (struct ExecBase *)_sysBase;
    struct KernelBase *KernelBase = (struct KernelBase *)_kernelBase;

    D(bug("[KRN:BCM2708] %s()\n", __PRETTY_FUNCTION__));

    if (__arm_arosintern.ARMI_PeripheralBase == (APTR)BCM2836_PERIPHYSBASE)
    {
        void *trampoline_src = mpcore_trampoline;
        void *trampoline_dst = (void *)0x2000;
        uint32_t trampoline_length = (uintptr_t)&mpcore_end - (uintptr_t)mpcore_trampoline;
        uint32_t trampoline_data_offset = (uintptr_t)&mpcore_pde - (uintptr_t)mpcore_trampoline;
        int core;
        uint32_t *core_stack;
        uint32_t tmp;

        bug("[KRN:BCM2708] Initialising Multicore System\n");
        D(bug("[KRN:BCM2708] %s: Copy SMP trampoline from %p to %p (%d bytes)\n", __PRETTY_FUNCTION__, trampoline_src, trampoline_dst, trampoline_length));

        bcopy(trampoline_src, trampoline_dst, trampoline_length);

        D(bug("[KRN:BCM2708] %s: Patching data for trampoline at offset %d\n", __PRETTY_FUNCTION__, trampoline_data_offset));

        asm volatile ("mrc p15, 0, %0, c2, c0, 0":"=r"(tmp));
        ((uint32_t *)(trampoline_dst + trampoline_data_offset))[0] = tmp; // pde
        ((uint32_t *)(trampoline_dst + trampoline_data_offset))[1] = (uint32_t)cpu_Register;

        for (core = 1; core < 4; core ++)
        {
                core_stack = (uint32_t *)AllocMem(1024, MEMF_CLEAR); /* MEMF_PRIVATE */
                ((uint32_t *)(trampoline_dst + trampoline_data_offset))[2] = &core_stack[1024-16];

                D(bug("[KRN:BCM2708] %s: Attempting to wake core #%d\n", __PRETTY_FUNCTION__, core));
                D(bug("[KRN:BCM2708] %s: core #%d stack @ 0x%p : 0x%p)\n", __PRETTY_FUNCTION__, core, core_stack, ((uint32_t *)(trampoline_dst + trampoline_data_offset))[2]));

                arm_flush_cache((uint32_t)trampoline_dst, 512);
                *((uint32_t *)(0x4000008c + (0x10 * core))) = trampoline_dst;

                if (__arm_arosintern.ARMI_Delay)
                    __arm_arosintern.ARMI_Delay(10000000);
        }
    }
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
    unsigned int reg;

    reg = (unsigned int)IRQBANK_POINTER(bank);

    (bug("[KRN:BCM2708] Enabling irq %d [bank %d, reg 0x%p]\n", irq, bank, reg));

    *((volatile unsigned int *)reg) = IRQ_MASK(irq);

    (bug("[KRN:BCM2708] irqmask=%08x\n", *((volatile unsigned int *)reg)));
}

static void bcm2807_irq_disable(int irq)
{
    int bank = IRQ_BANK(irq);
    unsigned int reg;

    reg = (unsigned int)IRQBANK_POINTER(bank) + 0x0c;

    (bug("[KRN:BCM2708] Disabling irq %d [bank %d, reg 0x%p]\n", irq, bank, reg));

    *((volatile unsigned int *)reg) = IRQ_MASK(irq);

    (bug("[KRN:BCM2708] irqmask=%08x\n", *((volatile unsigned int *)reg)));
}

static void bcm2807_irq_process()
{
    unsigned int pendingarm, pending0, pending1, irq;

    for(;;)
    {
        pendingarm = *((volatile unsigned int *)(ARMIRQ_PEND));
        pending0 = *((volatile unsigned int *)(GPUIRQ_PEND0));
        pending1 = *((volatile unsigned int *)(GPUIRQ_PEND1));

        if (!(pendingarm || pending0 || pending1))
            break;

        DIRQ(bug("[KRN] PendingARM %08x\n", pendingarm));
        DIRQ(bug("[KRN] Pending0 %08x\n", pending0));
        DIRQ(bug("[KRN] Pending1 %08x\n", pending1));

        if (pendingarm & ~(IRQ_BANK1 | IRQ_BANK2))
        {
            for (irq = (2 << 5); irq < ((2 << 5) + 8); irq++)
            {
                if (pendingarm & (1 << (irq - (2 << 5))))
                {
                    DIRQ(bug("[KRN] Handling IRQ %d ..\n", irq));
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
                    DIRQ(bug("[KRN] Handling IRQ %d ..\n", irq));
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
                    DIRQ(bug("[KRN] Handling IRQ %d ..\n", irq));
                    krnRunIRQHandlers(KernelBase, irq);
                }
            }
        }
    }
}


static void bcm2708_toggle_led(int LED, int state)
{
    if (__arm_arosintern.ARMI_PeripheralBase == (APTR)BCM2836_PERIPHYSBASE)
    {
        int pin = 35;
        APTR gpiofunc = GPCLR1;

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

/* Use system timer 3 for our scheduling heartbeat */
#define VBLANK_TIMER            3
#define VBLANK_INTERVAL         (1000000 / 50)

static void bcm2708_gputimer_handler(unsigned int timerno, void *unused1)
{
    unsigned int stc;

    D(bug("[KRN:BCM2708] %s(%d)\n", __PRETTY_FUNCTION__, timerno));

    /* Aknowledge our timer interrupt */
    *((volatile unsigned int *)(SYSTIMER_CS)) = 1 << timerno;

    /* Signal the Exec VBlankServer */
    if (SysBase && (SysBase->IDNestCnt < 0)) {
        core_Cause(INTB_VERTB, 1L << INTB_VERTB);
    }

    /* Refresh our timer interrupt */
    stc = *((volatile unsigned int *)(SYSTIMER_CLO));
    stc += VBLANK_INTERVAL;
    *((volatile unsigned int *)(SYSTIMER_C0 + (timerno * 4))) = stc;

    D(bug("[BCM2708] %s: Done..\n", __PRETTY_FUNCTION__));
}

static APTR bcm2708_init_gputimer(APTR _kernelBase)
{
    struct KernelBase *KernelBase = (struct KernelBase *)_kernelBase;
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
    BOOL bcm2708found = FALSE;
    void *bootPutC = NULL;

    while(msg->ti_Tag != TAG_DONE)
    {
        switch (msg->ti_Tag)
        {
        case KRN_Platform:
            if (msg->ti_Data == 0xc42)
                bcm2708found = TRUE;
            break;
        case KRN_FuncPutC:
            bootPutC = (void *)msg->ti_Data;
            break;
        }
        msg++;
    }

    if (!bcm2708found)
        return FALSE;

    if (krnARMImpl->ARMI_Family == 7) /*  bcm2836 uses armv7 */
        krnARMImpl->ARMI_PeripheralBase = (APTR)BCM2836_PERIPHYSBASE;
    else
        krnARMImpl->ARMI_PeripheralBase = (APTR)BCM2835_PERIPHYSBASE;

    krnARMImpl->ARMI_GetTime = &bcm2807_get_time;
    krnARMImpl->ARMI_InitTimer = &bcm2708_init_gputimer;
    krnARMImpl->ARMI_LED_Toggle = &bcm2708_toggle_led;

    krnARMImpl->ARMI_SerPutChar = &bcm2708_ser_putc;
    krnARMImpl->ARMI_SerGetChar = &bcm2708_ser_getc;
    if ((krnARMImpl->ARMI_PutChar = bootPutC) != NULL)
            krnARMImpl->ARMI_PutChar(0xFF); // Clear the display

    krnARMImpl->ARMI_IRQInit = &bcm2807_irq_init;
    krnARMImpl->ARMI_IRQEnable = &bcm2807_irq_enable;
    krnARMImpl->ARMI_IRQDisable = &bcm2807_irq_disable;
    krnARMImpl->ARMI_IRQProcess = &bcm2807_irq_process;

    krnARMImpl->ARMI_Init = &bcm2708_init;

    return TRUE;
}

ADD2SET(bcm2708_probe, ARMPLATFORMS, 0);
