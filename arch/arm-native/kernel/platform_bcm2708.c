/*
    Copyright � 2015, The AROS Development Team. All rights reserved.
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

#include "exec_platform.h"

#define ARM_PERIIOBASE __arm_arosintern.ARMI_PeripheralBase
#include <hardware/bcm2708.h>
#include <hardware/bcm2708_boot.h>
#include <hardware/pl011uart.h>

#define IRQBANK_POINTER(bank)   ((bank == 0) ? GPUIRQ_ENBL0 : (bank == 1) ? GPUIRQ_ENBL1 : ARMIRQ_ENBL)

#define IRQ_BANK1	0x00000100
#define IRQ_BANK2	0x00000200

#define D(x) x
#define DIRQ(x)
#define DFIQ(x)
#define DTIMER(x)

extern void mpcore_trampoline();
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
        void *trampoline_dst = (void *)BOOTMEMADDR(bm_mctrampoline);
        uint32_t trampoline_length = (uintptr_t)&mpcore_end - (uintptr_t)mpcore_trampoline;
        uint32_t trampoline_data_offset = (uintptr_t)&mpcore_pde - (uintptr_t)mpcore_trampoline;
        int core;
        uint32_t *core_stack;
        uint32_t tmp;
        tls_t   *__tls;

        bug("[KRN:BCM2708] Initialising Multicore System\n");
        D(bug("[KRN:BCM2708] %s: Copy SMP trampoline from %p to %p (%d bytes)\n", __PRETTY_FUNCTION__, trampoline_src, trampoline_dst, trampoline_length));

        bcopy(trampoline_src, trampoline_dst, trampoline_length);

        D(bug("[KRN:BCM2708] %s: Patching data for trampoline at offset %d\n", __PRETTY_FUNCTION__, trampoline_data_offset));

        asm volatile ("mrc p15, 0, %0, c2, c0, 0":"=r"(tmp));
        ((uint32_t *)(trampoline_dst + trampoline_data_offset))[0] = tmp; // pde
        ((uint32_t *)(trampoline_dst + trampoline_data_offset))[1] = (uint32_t)cpu_Register;

        for (core = 1; core < 4; core ++)
        {
                core_stack = (uint32_t *)AllocMem(AROS_STACKSIZE*sizeof(uint32_t), MEMF_CLEAR); /* MEMF_PRIVATE */
                ((uint32_t *)(trampoline_dst + trampoline_data_offset))[2] = (uint32_t)&core_stack[AROS_STACKSIZE-sizeof(IPTR)];

                __tls =  AllocMem(sizeof(tls_t),  MEMF_CLEAR); /* MEMF_PRIVATE */
                __tls->SysBase = _sysBase;
                __tls->KernelBase = _kernelBase;
                __tls->ThisTask = NULL;
                arm_flush_cache(((uint32_t)__tls) & ~63, 512);
                ((uint32_t *)(trampoline_dst + trampoline_data_offset))[3] = __tls;

                D(bug("[KRN:BCM2708] %s: Attempting to wake core #%d\n", __PRETTY_FUNCTION__, core));
                D(bug("[KRN:BCM2708] %s: core #%d stack @ 0x%p (sp=0x%p)\n", __PRETTY_FUNCTION__, core, core_stack, ((uint32_t *)(trampoline_dst + trampoline_data_offset))[2]));
                D(bug("[KRN:BCM2708] %s: core #%d tls @ 0x%p\n", __PRETTY_FUNCTION__, core, ((uint32_t *)(trampoline_dst + trampoline_data_offset))[3]));

                arm_flush_cache((uint32_t)trampoline_dst, 512);
                *((uint32_t *)(BCM2836_MAILBOX3_SET0 + (0x10 * core))) = (uint32_t)trampoline_dst;

                if (__arm_arosintern.ARMI_Delay)
                    __arm_arosintern.ARMI_Delay(10000000);
        }
    }
}

static void bcm2708_init_core(APTR _kernelBase, APTR _sysBase)
{
    struct ExecBase *SysBase = (struct ExecBase *)_sysBase;
    struct KernelBase *KernelBase = (struct KernelBase *)_kernelBase;
    uint32_t tmp;

    D(bug("[KRN:BCM2708] %s()\n", __PRETTY_FUNCTION__));

    asm volatile (" mrc p15, 0, %0, c0, c0, 5 " : "=r" (tmp));

    // enable FIQ mailbox interupt
    *((uint32_t *)(BCM2836_MAILBOX_INT_CTRL0 + (0x4 * (tmp & 0x3)))) = (1 << 3);
}

static unsigned int bcm2807_get_time(void)
{
    return *((volatile unsigned int *)(SYSTIMER_CLO));
}

static void bcm2807_irq_init(void)
{
    // disable IRQ's
//    *(volatile unsigned int *)ARMFIQ_CTRL = 0;
    
    *(volatile unsigned int *)ARMIRQ_DIBL = ~0;
    *(volatile unsigned int *)GPUIRQ_DIBL0 = ~0; 
    *(volatile unsigned int *)GPUIRQ_DIBL1 = ~0;

    // aknowledge pending IRQ's
    *(volatile unsigned int *)ARMIRQ_PEND = *(volatile unsigned int *)ARMIRQ_PEND;
    *(volatile unsigned int *)GPUIRQ_PEND0 = *(volatile unsigned int *)GPUIRQ_PEND0;
    *(volatile unsigned int *)GPUIRQ_PEND1 = *(volatile unsigned int *)GPUIRQ_PEND1;
}

static void bcm2807_irq_enable(int irq)
{
    int bank = IRQ_BANK(irq);
    unsigned int reg;

    reg = (unsigned int)IRQBANK_POINTER(bank);

    DIRQ(bug("[KRN:BCM2708] Enabling irq %d [bank %d, reg 0x%p]\n", irq, bank, reg));

    *((volatile unsigned int *)reg) = IRQ_MASK(irq);

    DIRQ(bug("[KRN:BCM2708] irqmask=%08x\n", *((volatile unsigned int *)reg)));
}

static void bcm2807_irq_disable(int irq)
{
    int bank = IRQ_BANK(irq);
    unsigned int reg;

    reg = (unsigned int)IRQBANK_POINTER(bank) + 0x0c;

    DIRQ(bug("[KRN:BCM2708] Disabling irq %d [bank %d, reg 0x%p]\n", irq, bank, reg));

    *((volatile unsigned int *)reg) = IRQ_MASK(irq);

    DIRQ(bug("[KRN:BCM2708] irqmask=%08x\n", *((volatile unsigned int *)reg)));
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

        DIRQ(bug("[KRN:BCM2708] PendingARM %08x\n", pendingarm));
        DIRQ(bug("[KRN:BCM2708] Pending0 %08x\n", pending0));
        DIRQ(bug("[KRN:BCM2708] Pending1 %08x\n", pending1));

        if (pendingarm & ~(IRQ_BANK1 | IRQ_BANK2))
        {
            for (irq = (2 << 5); irq < ((2 << 5) + 8); irq++)
            {
                if (pendingarm & (1 << (irq - (2 << 5))))
                {
                    DIRQ(bug("[KRN:BCM2708] Handling IRQ %d ..\n", irq));
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
                    DIRQ(bug("[KRN:BCM2708] Handling IRQ %d ..\n", irq));
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
                    DIRQ(bug("[KRN:BCM2708] Handling IRQ %d ..\n", irq));
                    krnRunIRQHandlers(KernelBase, irq);
                }
            }
        }
    }
}

static void bcm2807_fiq_process()
{
    uint32_t tmp, fiq;

    asm volatile (" mrc p15, 0, %0, c0, c0, 5 " : "=r" (tmp));

    DFIQ(bug("[KRN:BCM2708] %s(%d)\n", __PRETTY_FUNCTION__, (tmp & 0x3)));

    fiq = *((uint32_t *)(BCM2836_FIQ_PEND0 + (0x4 * (tmp & 0x3))));

    DFIQ(bug("[KRN:BCM2708] %s: FIQ %x\n", __PRETTY_FUNCTION__, fiq));

    *((uint32_t *)(BCM2836_FIQ_PEND0 + (0x4 * (tmp & 0x3)))) = fiq;
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

    DTIMER(bug("[KRN:BCM2708] %s(%d)\n", __PRETTY_FUNCTION__, timerno));

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

    DTIMER(bug("[BCM2708] %s: Done..\n", __PRETTY_FUNCTION__));
}

static APTR bcm2708_init_gputimer(APTR _kernelBase)
{
    struct KernelBase *KernelBase = (struct KernelBase *)_kernelBase;
    struct IntrNode *GPUTimerHandle;
    unsigned int stc;

    DTIMER(bug("[KRN:BCM2708] %s(%012p)\n", __PRETTY_FUNCTION__, KernelBase));

    if ((GPUTimerHandle = AllocMem(sizeof(struct IntrNode), MEMF_PUBLIC|MEMF_CLEAR)) != NULL)
    {
        DTIMER(bug("[KRN:BCM2708] %s: IntrNode @ 0x%p:\n", __PRETTY_FUNCTION__, GPUTimerHandle));
        DTIMER(bug("[KRN:BCM2708] %s: Using GPUTimer %d for VBlank\n", __PRETTY_FUNCTION__, VBLANK_TIMER));

        GPUTimerHandle->in_Handler = bcm2708_gputimer_handler;
        GPUTimerHandle->in_HandlerData = (void *)VBLANK_TIMER;
        GPUTimerHandle->in_HandlerData2 = KernelBase;
        GPUTimerHandle->in_type = it_interrupt;
        GPUTimerHandle->in_nr = IRQ_TIMER0 + VBLANK_TIMER;

        ADDHEAD(&KernelBase->kb_Interrupts[IRQ_TIMER0 + VBLANK_TIMER], &GPUTimerHandle->in_Node);

        DTIMER(bug("[KRN:BCM2708] %s: Enabling Hardware IRQ.. \n", __PRETTY_FUNCTION__));

        stc = *((volatile unsigned int *)(SYSTIMER_CLO));
        stc += VBLANK_INTERVAL;
        *((volatile unsigned int *)(SYSTIMER_CS)) = (1 << VBLANK_TIMER);
        *((volatile unsigned int *)(SYSTIMER_C0 + (VBLANK_TIMER * 4))) = stc;

        ictl_enable_irq(IRQ_TIMER0 + VBLANK_TIMER, KernelBase);
    }

    DTIMER(bug("[KRN:BCM2708] %s: Done.. \n", __PRETTY_FUNCTION__));

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
    void *bootPutC = NULL;

    while(msg->ti_Tag != TAG_DONE)
    {
        switch (msg->ti_Tag)
        {
        case KRN_FuncPutC:
            bootPutC = (void *)msg->ti_Data;
            break;
        }
        msg++;
    }

    if (krnARMImpl->ARMI_Platform != 0xc42)
        return FALSE;

    if (krnARMImpl->ARMI_Family == 7)
    {
        /*  bcm2836 uses armv7 */
        krnARMImpl->ARMI_PeripheralBase = (APTR)BCM2836_PERIPHYSBASE;
        krnARMImpl->ARMI_InitCore = &bcm2708_init_core;
        krnARMImpl->ARMI_FIQProcess = &bcm2807_fiq_process;
    }
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
