/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
    Author: Fabian Schmieder (@metaneutrons)

    BCM2712 (Raspberry Pi 5) experimental platform support for AArch64.

    The Raspberry Pi 5 uses the Broadcom BCM2712 SoC with quad Cortex-A76
    cores and an ARM GIC-400 (GICv2) interrupt controller. Most I/O
    peripherals reside behind the RP1 southbridge chip connected via PCIe.
    The BCM2712 itself provides the core PL011 UART, GIC-400, and ARM
    generic timer.
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

/* SoC-common bits shared with Raspberry Pi platforms */
#include "bcm27xx.h"

#define DTIMER(x)

/* BCM2712 Peripheral Base and PL011 UART */
#define BCM2712_PERIBASE        0x107C000000UL
#define BCM2712_UART_BASE       (BCM2712_PERIBASE + 0x201000UL)

#define PL011_DR                0x00
#define PL011_FR                0x18
#define PL011_FR_TXFF           (1 << 5)
#define PL011_FR_RXFE           (1 << 4)

static inline void bcm2712_mmio_wr(unsigned long addr, uint32_t val)
{
    *(volatile uint32_t *)addr = val;
}

static inline uint32_t bcm2712_mmio_rd(unsigned long addr)
{
    return *(volatile uint32_t *)addr;
}

static void bcm2712_ser_putc(uint8_t chr)
{
    int timeout = 100000;
    while (timeout-- > 0 && (bcm2712_mmio_rd(BCM2712_UART_BASE + PL011_FR) & PL011_FR_TXFF))
        ;
    if (chr == '\n')
    {
        bcm2712_mmio_wr(BCM2712_UART_BASE + PL011_DR, '\r');
        timeout = 100000;
        while (timeout-- > 0 && (bcm2712_mmio_rd(BCM2712_UART_BASE + PL011_FR) & PL011_FR_TXFF))
            ;
    }
    bcm2712_mmio_wr(BCM2712_UART_BASE + PL011_DR, chr);
}

static int bcm2712_ser_getc(void)
{
    if (!(bcm2712_mmio_rd(BCM2712_UART_BASE + PL011_FR) & PL011_FR_RXFE))
        return (int)bcm2712_mmio_rd(BCM2712_UART_BASE + PL011_DR);
    return -1;
}

/* ---- GIC-400 (GICv2), Pi 5 physical addresses ---- */
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

#define GIC_FIRST_SPI   32

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

static void bcm2712_irq_init(void)
{
    /* Distributor + CPU interface: mask everything, then enable, PMR open. */
    GICD(GICD_CTLR) = 0;
    GICC(GICC_CTLR) = 0;
    GICC(GICC_PMR) = 0xF0;
    GICD(GICD_CTLR) = 1;
    GICC(GICC_CTLR) = 1;
}

static void bcm2712_irq_enable(int irq)
{
    *((volatile uint8_t *)(GICD_BASE + GICD_IPRIORITYR + irq)) = 0xA0;

    if (irq >= GIC_FIRST_SPI)
    {
        uint32_t cfg;

        *((volatile uint8_t *)(GICD_BASE + GICD_ITARGETSR + irq)) = 0x01;

        cfg = GICD(GICD_ICFGR + 4 * (irq / 16));
        cfg &= ~(2u << ((irq % 16) * 2));
        GICD(GICD_ICFGR + 4 * (irq / 16)) = cfg;
    }

    GICD(GICD_ISENABLER + 4 * (irq / 32)) = 1u << (irq % 32);
}

static void bcm2712_irq_disable(int irq)
{
    GICD(GICD_ICENABLER + 4 * (irq / 32)) = 1u << (irq % 32);
}

static uint32_t irq_last = GIC_SPURIOUS;
static unsigned int irq_repeats;

static void bcm2712_irq_process(void)
{
    for (;;)
    {
        uint32_t iar = GICC(GICC_IAR);
        uint32_t intid = iar & 0x3FF;

        if (intid >= GIC_SPURIOUS)
            break;

        krnRunIRQHandlers(KernelBase, intid);

        GICC(GICC_EOIR) = iar;

        if (intid == irq_last)
        {
            if (++irq_repeats > 10000)
            {
                bcm2712_irq_disable(intid);
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

static void bcm2712_gentimer_handler(unsigned int irq, void *unused)
{
    (void)irq; (void)unused;

    /* Reload compare for next quantum */
    __asm__ volatile("msr cntp_tval_el0, %0" :: "r"(gentimer_interval));

    /* Cause scheduler quantum */
    core_Cause(INTB_VERTB, 1L << INTB_VERTB);
}

static APTR bcm2712_init_gentimer(APTR _kernelBase)
{
    struct KernelBase *KernelBase = (struct KernelBase *)_kernelBase;
    struct IntrNode *TimerHandle;
    uint64_t freq;

    DTIMER(bug("[Kernel:BCM2712] %s\n", __func__));

    __asm__ volatile("mrs %0, cntfrq_el0" : "=r"(freq));
    gentimer_interval = freq / GENTIMER_HZ;

    TimerHandle = AllocMem(sizeof(struct IntrNode), MEMF_PUBLIC | MEMF_CLEAR);
    if (!TimerHandle)
        return NULL;

    TimerHandle->in_Handler = bcm2712_gentimer_handler;
    TimerHandle->in_HandlerData = (void *)(uintptr_t)GENTIMER_PPI;
    TimerHandle->in_HandlerData2 = KernelBase;
    TimerHandle->in_type = it_interrupt;
    TimerHandle->in_nr = GENTIMER_PPI;

    ADDHEAD(&KernelBase->kb_Interrupts[GENTIMER_PPI], &TimerHandle->in_Node);

    /* Program initial interval, enable timer, unmask PPI in GIC */
    __asm__ volatile("msr cntp_tval_el0, %0" :: "r"(gentimer_interval));
    __asm__ volatile("msr cntp_ctl_el0, %0" :: "r"((uint64_t)1));
    ictl_enable_irq(GENTIMER_PPI, KernelBase);

    return TimerHandle;
}

/* ------------------------------- probe ------------------------------- */

static IPTR bcm2712_probe(struct ARM_Implementation *krnARMImpl, struct TagItem *msg)
{
    void *bootPutC = NULL;
    uint64_t midr;

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

    __asm__ volatile("mrs %0, midr_el1" : "=r"(midr));

    /* Detect BCM2712 platform ID (0x2712) or Cortex-A76 core (MIDR part 0xD0B) */
    if (krnARMImpl->ARMI_Platform != 0x2712 && ((midr >> 4) & 0xFFF) != 0xD0B)
        return FALSE;

    krnARMImpl->ARMI_Family = 8;
    krnARMImpl->ARMI_Platform = 0x2712;
    krnARMImpl->ARMI_PeripheralBase = (APTR)BCM2712_PERIBASE;
    krnARMImpl->ARMI_InitCore = &bcm27xx_init_cpu;
    krnARMImpl->ARMI_FIQProcess = &bcm27xx_fiq_process;
    krnARMImpl->ARMI_SendIPI = &bcm27xx_send_ipi;

    krnARMImpl->ARMI_GetTime = &bcm27xx_get_time;
    krnARMImpl->ARMI_InitTimer = &bcm2712_init_gentimer;
    krnARMImpl->ARMI_LED_Toggle = &bcm27xx_toggle_led;

    krnARMImpl->ARMI_SerPutChar = &bcm2712_ser_putc;
    krnARMImpl->ARMI_SerGetChar = &bcm2712_ser_getc;

    if ((krnARMImpl->ARMI_PutChar = bootPutC) != NULL)
    {
        krnARMImpl->ARMI_PutChar(0xFF); /* Clear the display */
    }

    krnARMImpl->ARMI_IRQInit = &bcm2712_irq_init;
    krnARMImpl->ARMI_IRQEnable = &bcm2712_irq_enable;
    krnARMImpl->ARMI_IRQDisable = &bcm2712_irq_disable;
    krnARMImpl->ARMI_IRQProcess = &bcm2712_irq_process;

    krnARMImpl->ARMI_Init = &bcm27xx_init;

    return TRUE;
}

ADD2SET(bcm2712_probe, ARMPLATFORMS, 0);
