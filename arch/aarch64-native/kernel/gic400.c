/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: GIC-400 (GICv2) per-core setup and SGI IPIs for the Pi 4 and Pi 5,
    which lack the Pi 3's BCM2836 IPI mailboxes. SGIs are IRQs here: group 0
    is secure-only, and Disable() masks I and F alike anyway.
*/

#include <aros/kernel.h>

#include "kernel_base.h"

#include <proto/kernel.h>
#include <proto/exec.h>

#include <inttypes.h>

#include "kernel_intern.h"
#include "kernel_debug.h"
#include "kernel_cpu.h"
#include "io.h"

#include "exec_platform.h"

#include "gic400.h"

#define GICD(o)     (*(volatile uint32_t *)(gic_dist + (o)))
#define GICC(o)     (*(volatile uint32_t *)(gic_cpuif + (o)))

#define GICD_ISENABLER  0x100
#define GICD_IPRIORITYR 0x400
#define GICD_SGIR       0xF00

#define GICC_CTLR   0x000
#define GICC_PMR    0x004

#define GIC400_MAXCPUS  4

static uintptr_t gic_dist;
static uintptr_t gic_cpuif;

void gic400_setbase(uintptr_t dist, uintptr_t cpuif)
{
    gic_dist = dist;
    gic_cpuif = cpuif;
}

#if defined(__AROSEXEC_SMP__)
extern void handle_ipi(uint32_t, uint32_t);

/* SGIs carry no payload: one slot per (target, source) so two senders
 * can't clobber each other's data word. */
struct gic_ipislot
{
    volatile uint32_t   is_Msg;
    volatile uint32_t   is_Data;
};

static struct gic_ipislot gic_ipi_slot[GIC400_MAXCPUS][GIC400_MAXCPUS];
#endif

/* PMR, GICC_CTLR and the first ISENABLER are banked per core */
void gic400_init_core(void)
{
    GICC(GICC_PMR) = 0xF0;
    GICC(GICC_CTLR) = 1;

#if defined(__AROSEXEC_SMP__)
    *((volatile uint8_t *)(gic_dist + GICD_IPRIORITYR + GIC400_IPI_SGI)) = 0xA0;
    GICD(GICD_ISENABLER) = 1u << GIC400_IPI_SGI;
#endif
}

void gic400_send_ipi(uint32_t ipi, uint32_t ipi_data, uint32_t cpumask)
{
#if defined(__AROSEXEC_SMP__)
    int src = GetCPUNumber();
    int cpu;

    /* Disable() keeps a Signal from re-entering. */
    Disable();

    for (cpu = 0; cpu < GIC400_MAXCPUS; cpu++)
    {
        if (!(cpumask & (1 << cpu)))
            continue;

        gic_ipi_slot[cpu][src].is_Data = ipi_data;
        __atomic_fetch_or(&gic_ipi_slot[cpu][src].is_Msg, ipi, __ATOMIC_RELEASE);
    }

    /* Order the Normal-memory slots before the Device SGIR write */
    dsb();

    /* Bits 23:16 are the CPU target list */
    GICD(GICD_SGIR) = ((cpumask & 0xff) << 16) | GIC400_IPI_SGI;

    Enable();
#else
    (void)ipi; (void)ipi_data; (void)cpumask;
#endif
}

void gic400_handle_ipi(void)
{
#if defined(__AROSEXEC_SMP__)
    int cpu = GetCPUNumber();
    int src;

    for (src = 0; src < GIC400_MAXCPUS; src++)
    {
        /* Bits set after the exchange arrive with a fresh SGI */
        uint32_t msg = __atomic_exchange_n(&gic_ipi_slot[cpu][src].is_Msg, 0,
                                           __ATOMIC_ACQUIRE);

        if (msg)
            handle_ipi(msg, gic_ipi_slot[cpu][src].is_Data);
    }
#endif
}
