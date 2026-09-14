/*
    Copyright (C) 2015-2026, The AROS Development Team. All rights reserved.

    BCM27xx SoC-common platform support for AArch64.

    Code shared between the legacy interrupt controller Pis
    (bcm2708/2836/2837) and the GIC based Pi 4 (bcm2711): SMP bring-up via
    the BCM2836 mailboxes, mailbox FIQ/IPI handling, the system timer
    counter, the activity LED and the PL011 debug serial port.
*/

#include <aros/types/spinlock_s.h>
#include <aros/kernel.h>

#include "kernel_base.h"

#include <proto/kernel.h>
#include <proto/exec.h>

#include <inttypes.h>

#include <strings.h>

#include "kernel_intern.h"
#include "kernel_debug.h"
#include "of_intern.h"
#include "kernel_cpu.h"
#include "tls.h"
#include "io.h"

#include "exec_platform.h"

#include "bcm27xx.h"

#define ARM_PERIIOBASE ((IPTR)__arm_arosintern.ARMI_PeripheralBase)
#include <hardware/bcm2708.h>
#include <hardware/bcm2708_boot.h>
#include <hardware/pl011uart.h>

#undef D
#define D(x) x
#define DFIQ(x)

extern void mpcore_trampoline();
extern uint64_t mpcore_end;
extern uint64_t mpcore_pde;
extern uint64_t mpcore_tcr;
extern uint64_t mpcore_mair;
extern spinlock_t startup_lock;

extern void cpu_Register(void);
extern void aarch64_flush_cache(uintptr_t, uint32_t);
#if defined(__AROSEXEC_SMP__)
extern void handle_ipi(uint32_t, uint32_t);
extern void core_IPIInit(void);

struct cpu_ipidata
{
    uint32_t    ipi_data[4];
};

static struct cpu_ipidata *bcm27xx_cpuipid[4] = { 0, 0, 0, 0 };

/* Per-core scheduler heartbeat: the system timer only reaches core 0, so
 * every core arms its own CNTP as an FIQ, from the target core. */
#define CNTP_CTL_ENABLE         (1 << 0)
#define CNTP_CTL_IMASK          (1 << 1)

#define BCM2836_TIMER_CNTPNS_FIQ (1 << 5)       /* TIMER_INT_CTRLx: route CNTPNS as FIQ */
#define BCM2836_FIQ_CNTPNS       (1 << 1)       /* FIQ_PENDx: CNTPNS pending */

/* CNTP reload value (counter ticks per heartbeat). Same on every core. */
static uint32_t bcm27xx_cntp_interval = 0;

static inline uint32_t bcm27xx_cntfrq_get(void)
{
    uint64_t v;
    asm volatile ("mrs %0, cntfrq_el0" : "=r"(v));
    return (uint32_t)v;
}

static inline uint32_t bcm27xx_cntpct_get(void)
{
    uint64_t v;
    asm volatile ("mrs %0, cntpct_el0" : "=r"(v));
    return (uint32_t)v;
}

/* CNTFRQ is not always true (QEMU reports 19.2MHz while counting at
 * 1MHz), so measure against the system timer and fall back to it. */
static uint32_t bcm27xx_cntp_measure(void)
{
    const uint32_t window = 10000;      /* microseconds */
    uint32_t s0, c0, c1;

    s0 = rd32le(SYSTIMER_CLO);
    c0 = bcm27xx_cntpct_get();
    while ((rd32le(SYSTIMER_CLO) - s0) < window)
        ;
    c1 = bcm27xx_cntpct_get();

    return (c1 - c0) * (1000000 / window);
}

static inline void bcm27xx_cntp_tval_set(uint32_t v)
{
    asm volatile ("msr cntp_tval_el0, %0" :: "r"((uint64_t)v));
}

static inline void bcm27xx_cntp_ctl_set(uint32_t v)
{
    asm volatile ("msr cntp_ctl_el0, %0" :: "r"((uint64_t)v));
}

static void bcm27xx_cntp_tick(void)
{
    tls_t *__tls;

    /* Rearm for the next tick - writing TVAL clears the pending condition */
    bcm27xx_cntp_tval_set(bcm27xx_cntp_interval);

    /* Not gated on IDNESTCOUNT: a task busy-looping in short Disable
     * windows must still expire its quantum. */
    if (!SysBase || !KernelBase)
        return;

    /* Per-CPU TLS, owning core only, so no atomics. */
    __tls = TLS_PTR_GET();
    if (__tls->Elapsed)
        __tls->Elapsed--;
    if (__tls->Elapsed == 0)
        __tls->ScheduleFlags |= (TLSSF_Quantum | TLSSF_Switch);
}

void bcm27xx_init_cntp_timer(void)
{
    int cpunum = GetCPUNumber();

    if (!bcm27xx_cntp_interval)
    {
        uint32_t hz = bcm27xx_cntp_measure();

        /* Sanity-bound the measurement before trusting it over CNTFRQ. */
        if (hz < 100000 || hz > 100000000)
            hz = bcm27xx_cntfrq_get();

        bcm27xx_cntp_interval = hz / 50;
    }

    /* Route this core's non-secure physical timer interrupt as an FIQ */
    wr32le(BCM2836_TIMER_INT_CTRL0 + (0x4 * cpunum), BCM2836_TIMER_CNTPNS_FIQ);

    /* Arm: fire one interval from now, enabled and unmasked */
    bcm27xx_cntp_tval_set(bcm27xx_cntp_interval);
    bcm27xx_cntp_ctl_set(CNTP_CTL_ENABLE);
}
#endif

#if defined(__AROSEXEC_SMP__)
/* AArch64 firmware parks secondaries on the spin-table, not the BCM2836
 * mailbox the 32-bit stub answers to. Address is board data:
 * /cpus/cpu@N/cpu-release-addr; 0xd8 + 8*cpu is the fallback. */
static volatile uint64_t *bcm27xx_cpu_release_addr(int cpu)
{
    char nodename[16] = "/cpus/cpu@0";
    void *node, *prop;
    uint32_t *cells;

    nodename[10] = '0' + cpu;
    node = dt_find_node(nodename);
    prop = node ? dt_find_property(node, "cpu-release-addr") : NULL;

    if (prop && dt_get_prop_len(prop) >= 8)
    {
        cells = dt_get_prop_value(prop);
        return (volatile uint64_t *)(uintptr_t)
            (((uint64_t)AROS_BE2LONG(cells[0]) << 32) | AROS_BE2LONG(cells[1]));
    }

    return (volatile uint64_t *)(uintptr_t)(0xd8 + 8 * cpu);
}
#endif

void bcm27xx_init(APTR _kernelBase, APTR _sysBase)
{
    struct ExecBase *SysBase = (struct ExecBase *)_sysBase;
    struct KernelBase *KernelBase = (struct KernelBase *)_kernelBase;

    KrnSpinInit(&startup_lock);

#if defined(__AROSEXEC_SMP__)
    core_IPIInit();
#endif

    D(bug("[Kernel:BCM27xx] %s()\n", __PRETTY_FUNCTION__));

    if (__arm_arosintern.ARMI_PeripheralBase == (APTR)BCM2836_PERIPHYSBASE)
    {
#if !defined(__AROSEXEC_SMP__)
        /* Uniprocessor: leave the secondaries parked. Releasing them would
         * run a core into cpu_Register with no VBAR, no scheduler and
         * no-op spinlocks. */
        bug("[Kernel:BCM27xx] Uniprocessor build - secondary cores left parked\n");
#else
        void *trampoline_src = mpcore_trampoline;
        void *trampoline_dst = (void *)BOOTMEMADDR(bm_mctrampoline);
        uint32_t trampoline_length = (uintptr_t)&mpcore_end - (uintptr_t)mpcore_trampoline;
        uint32_t trampoline_data_offset = (uintptr_t)&mpcore_pde - (uintptr_t)mpcore_trampoline;
        int cpu;
        uint64_t *cpu_stack;
        uint64_t tmp;
        tls_t   *__tls;

        /* Run the generic timer off the crystal so CNTPCT matches
         * CNTFRQ; firmware leaves the prescaler at 1MHz. */
        wr32le(BCM2836_CTRL, 0);
        wr32le(BCM2836_PRESCALER, 0x80000000);

        /* Register the boot CPU as an IPI receiver first, or IPIs aimed
         * at it are dropped. Secondaries do it in cpu_Register. */
        if (__arm_arosintern.ARMI_InitCore)
            __arm_arosintern.ARMI_InitCore(_kernelBase, _sysBase);

        bug("[Kernel:BCM27xx] Initialising Multicore System\n");
        D(bug("[Kernel:BCM27xx] %s: Copy SMP trampoline from %p to %p (%d bytes)\n", __PRETTY_FUNCTION__, trampoline_src, trampoline_dst, trampoline_length));

        bcopy(trampoline_src, trampoline_dst, trampoline_length);

        D(bug("[Kernel:BCM27xx] %s: Patching data for trampoline at offset %d\n", __PRETTY_FUNCTION__, trampoline_data_offset));

        /* Read TTBR0_EL1, TCR_EL1, MAIR_EL1 for secondary cores */
        asm volatile("mrs %0, ttbr0_el1" : "=r"(tmp));
        ((uint64_t *)(trampoline_dst + trampoline_data_offset))[0] = tmp; /* pde / TTBR0 */
        ((uint64_t *)(trampoline_dst + trampoline_data_offset))[1] = (uint64_t)cpu_Register;

        /* Store TCR_EL1 and MAIR_EL1 for trampoline */
        asm volatile("mrs %0, tcr_el1" : "=r"(tmp));
        ((uint64_t *)(trampoline_dst + trampoline_data_offset))[4] = tmp; /* TCR */
        asm volatile("mrs %0, mair_el1" : "=r"(tmp));
        ((uint64_t *)(trampoline_dst + trampoline_data_offset))[5] = tmp; /* MAIR */

        for (cpu = 1; cpu < 4; cpu++)
        {
            cpu_stack = (uint64_t *)AllocMem(AROS_STACKSIZE * sizeof(uint64_t), MEMF_CLEAR);
            ((uint64_t *)(trampoline_dst + trampoline_data_offset))[2] = (uint64_t)&cpu_stack[AROS_STACKSIZE - sizeof(IPTR)];

#if defined(__AROSEXEC_SMP__)
            __tls = (tls_t *)AllocMem(sizeof(tls_t) + sizeof(struct cpu_ipidata), MEMF_CLEAR);
#else
            __tls = (tls_t *)AllocMem(sizeof(tls_t), MEMF_CLEAR);
#endif
            __tls->SysBase = _sysBase;
            __tls->KernelBase = _kernelBase;
            __tls->ThisTask = NULL;
            __tls->CPUNumber = cpu;     /* logical id - GetCPUNumber reads this */
            aarch64_flush_cache(((uintptr_t)__tls) & ~63, 512);
            ((uint64_t *)(trampoline_dst + trampoline_data_offset))[3] = (uint64_t)__tls;

            D(bug("[Kernel:BCM27xx] %s: Attempting to wake CPU #%02d (release @ 0x%p)\n", __PRETTY_FUNCTION__, cpu, bcm27xx_cpu_release_addr(cpu)));
            D(bug("[Kernel:BCM27xx] %s: CPU #%02d Stack @ 0x%p (sp=0x%p)\n", __PRETTY_FUNCTION__, cpu, cpu_stack, ((uint64_t *)(trampoline_dst + trampoline_data_offset))[2]));
            D(bug("[Kernel:BCM27xx] %s: CPU #%02d TLS @ 0x%p\n", __PRETTY_FUNCTION__, cpu, ((uint64_t *)(trampoline_dst + trampoline_data_offset))[3]));

            aarch64_flush_cache((uintptr_t)trampoline_dst, 512);

            /* Lock the startup spinlock */
            KrnSpinLock(&startup_lock, NULL, SPINLOCK_MODE_WRITE);

            /* The parked core polls with the MMU off, so the entry
             * address has to reach memory, not just our cache. */
            {
                volatile uint64_t *release = bcm27xx_cpu_release_addr(cpu);

                *release = (uint64_t)(uintptr_t)trampoline_dst;
                aarch64_flush_cache(((uintptr_t)release) & ~63, 64);
            }

            dsb();
            sev();

            /* Wait for secondary core to be ready */
            KrnSpinLock(&startup_lock, NULL, SPINLOCK_MODE_WRITE);
            KrnSpinUnLock(&startup_lock);
        }
#endif /* __AROSEXEC_SMP__ */
    }
}

void bcm27xx_init_cpu(APTR _kernelBase, APTR _sysBase)
{
    struct ExecBase *SysBase = (struct ExecBase *)_sysBase;
    struct KernelBase *KernelBase = (struct KernelBase *)_kernelBase;
    (void)SysBase;
#if defined(__AROSEXEC_SMP__)
    tls_t   *__tls = TLS_PTR_GET();
#endif
    int cpunum = GetCPUNumber();

    D(bug("[Kernel:BCM27xx] %s(#%02d)\n", __PRETTY_FUNCTION__, cpunum));

    /* SCHEDQUANTUM_SET writes per-CPU TLS, so PrepareExecBase's single
     * call leaves the secondaries without a quantum. */
    SCHEDQUANTUM_SET(SCHEDQUANTUM_VALUE);
    SCHEDELAPSED_SET(SCHEDQUANTUM_VALUE);

    /* Clear all pending FIQ sources on mailboxes */
    wr32le(BCM2836_MAILBOX0_CLR0 + (16 * cpunum), 0xffffffff);
    wr32le(BCM2836_MAILBOX1_CLR0 + (16 * cpunum), 0xffffffff);
    wr32le(BCM2836_MAILBOX2_CLR0 + (16 * cpunum), 0xffffffff);
    wr32le(BCM2836_MAILBOX3_CLR0 + (16 * cpunum), 0xffffffff);

#if defined(__AROSEXEC_SMP__)
    bcm27xx_cpuipid[cpunum] = (struct cpu_ipidata *)((uintptr_t)__tls + sizeof(tls_t));
    D(bug("[Kernel:BCM27xx] %s: CPU #%02d IPI data @ 0x%p\n", __PRETTY_FUNCTION__, cpunum, bcm27xx_cpuipid[cpunum]));

    /* FIQ on all 4 mailboxes: senders index by source CPU, so enabling
     * only mailbox 0 drops every IPI from CPU 1-3. */
    wr32le(BCM2836_MAILBOX_INT_CTRL0 + (0x4 * cpunum), 0xf0);
#endif
}

uint64_t bcm27xx_get_time(void)
{
    uint32_t hi, lo;

    /* 64-bit 1MHz counter split over CHI:CLO - re-read CHI to close the
     * carry window (CLO alone wraps every ~71min). */
    do
    {
        hi = rd32le(SYSTIMER_CHI);
        lo = rd32le(SYSTIMER_CLO);
    } while (rd32le(SYSTIMER_CHI) != hi);

    return ((uint64_t)hi << 32) | lo;
}

void bcm27xx_send_ipi(uint32_t ipi, uint32_t ipi_data, uint32_t cpumask)
{
    int cpu;
#if defined(__AROSEXEC_SMP__)
    /* Index by source CPU so (sender, target) pairs never OR-collide in
     * the SET register. Same-sender coalescing is lossless: the messages
     * are bit flags. Disable() keeps a Signal from re-entering. */
    int mbno = GetCPUNumber();
    Disable();
#endif

    for (cpu = 0; cpu < 4; cpu++)
    {
#if defined(__AROSEXEC_SMP__)
        if ((cpumask & (1 << cpu)) && bcm27xx_cpuipid[cpu])
        {
            bcm27xx_cpuipid[cpu]->ipi_data[mbno] = ipi_data;
            dsb();
            wr32le(BCM2836_MAILBOX0_SET0 + 4 * mbno + (0x10 * cpu), ipi);
        }
#endif
    }

#if defined(__AROSEXEC_SMP__)
    Enable();
#endif
}

void bcm27xx_fiq_process()
{
    int cpunum = GetCPUNumber();
    uint32_t fiq, fiq_data;
    int mbno;

    DFIQ(bug("[Kernel:BCM27xx] %s(%d)\n", __PRETTY_FUNCTION__, cpunum));

    fiq = rd32le(BCM2836_FIQ_PEND0 + (0x4 * cpunum));

    DFIQ(bug("[Kernel:BCM27xx] %s: CPU #%02d FIQ %x\n", __PRETTY_FUNCTION__, cpunum, fiq));

    if (fiq)
    {
#if defined(__AROSEXEC_SMP__)
        /* Per-core scheduler heartbeat (CNTP) - expire the local quantum */
        if (fiq & BCM2836_FIQ_CNTPNS)
            bcm27xx_cntp_tick();
#endif
        for (mbno = 0; mbno < 4; mbno++)
        {
            if (fiq & (0x10 << mbno))
            {
                fiq_data = rd32le(BCM2836_MAILBOX0_CLR0 + 4 * mbno + (16 * cpunum));
                (void)fiq_data;
                DFIQ(bug("[Kernel:BCM27xx] %s: Mailbox%d: FIQ Data %08x\n", __PRETTY_FUNCTION__, mbno, fiq_data));
#if defined(__AROSEXEC_SMP__)
                /* Pairs with the sender's dsb. */
                dsb();
                if (bcm27xx_cpuipid[cpunum])
                    handle_ipi(fiq_data, bcm27xx_cpuipid[cpunum]->ipi_data[mbno]);
#endif
                /* Only the bits read: a bit set in between must stay
                 * pending, or the IPI is lost. */
                wr32le(BCM2836_MAILBOX0_CLR0 + 4 * mbno + (16 * cpunum), fiq_data);
            }
        }
    }
}

/*
 * Which pin carries which light is board wiring, and the tree spells it out:
 * /leds/led-pwr and /leds/led-act each hold a gpios triple of (controller
 * phandle, pin, flags). A Pi 2 puts PWR on GPIO 35, a Pi 400 on 42. The
 * Pi 3B and 4B hand theirs to firmware controllers (expgpio, virtgpio) this
 * code cannot reach - those keep the state firmware left, which for the
 * power light is lit. Fixed pin numbers used to be right for one board and
 * a stray write to somebody else's GPIO on the rest.
 */
static struct
{
    int pin;                    /* < 0: not ours to drive */
    int active_low;
} bcm27xx_led[2] = { { -2, 0 }, { -2, 0 } };    /* -2: not looked up yet */

static int bcm27xx_led_on_soc(const char *name)
{
    return name && name[0] == 'g' && name[1] == 'p' && name[2] == 'i' &&
           name[3] == 'o' && (name[4] == '@' || name[4] == 0);
}

static void bcm27xx_led_query(int LED)
{
    void *node = dt_find_node((LED == ARM_LED_ACTIVITY) ? "/leds/led-act"
                                                        : "/leds/led-pwr");
    void *gprop = node ? dt_find_property(node, "gpios") : NULL;
    void *sprop = node ? dt_find_property(node, "status") : NULL;
    uint32_t *cells;
    of_node_t *ctrl;

    bcm27xx_led[LED].pin = -1;

    if (!gprop || dt_get_prop_len(gprop) < 8)
        return;

    /* A node for a light the board does not have is marked disabled */
    if (sprop && ((char *)dt_get_prop_value(sprop))[0] == 'd')
        return;

    cells = dt_get_prop_value(gprop);
    ctrl = dt_find_node_by_phandle(AROS_BE2LONG(cells[0]));
    if (!ctrl || !bcm27xx_led_on_soc(ctrl->on_name))
        return;

    bcm27xx_led[LED].pin = AROS_BE2LONG(cells[1]);
    bcm27xx_led[LED].active_low = (dt_get_prop_len(gprop) >= 12) &&
                                  (AROS_BE2LONG(cells[2]) & 1);
}

void bcm27xx_toggle_led(int LED, int state)
{
    int pin, lit;

    if (LED != ARM_LED_POWER && LED != ARM_LED_ACTIVITY)
        return;

    if (bcm27xx_led[LED].pin == -2)
        bcm27xx_led_query(LED);

    if ((pin = bcm27xx_led[LED].pin) < 0)
        return;

    lit = (state == ARM_LED_ON) != (bcm27xx_led[LED].active_low != 0);

    wr32le((lit ? GPSET0 : GPCLR0) + 4 * (pin / 32), 1 << (pin % 32));
}

static inline void bcm27xx_ser_waitout()
{
    while (1)
    {
        if ((rd32le(PL011_0_BASE + PL011_FR) & PL011_FR_TXFF) == 0) break;
    }
}

void bcm27xx_ser_putc(uint8_t chr)
{
    bcm27xx_ser_waitout();

    if (chr == '\n')
    {
        wr32le(PL011_0_BASE + PL011_DR, '\r');
        bcm27xx_ser_waitout();
    }
    wr32le(PL011_0_BASE + PL011_DR, chr);
}

int bcm27xx_ser_getc(void)
{
    if ((rd32le(PL011_0_BASE + PL011_FR) & PL011_FR_RXFE) == 0)
        return (int)rd32le(PL011_0_BASE + PL011_DR);

    return -1;
}
