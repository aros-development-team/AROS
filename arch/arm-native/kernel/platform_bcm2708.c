/*
    Copyright (C) 2015-2016, The AROS Development Team. All rights reserved.
*/

#include <aros/types/spinlock_s.h>
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
#include "of_intern.h"
#include "kernel_cpu.h"
#include "kernel_interrupts.h"
#include "kernel_intr.h"
#include "kernel_fb.h"
#if defined(__AROSEXEC_SMP__)
#include "kernel_ipi.h"
#endif
#include "tls.h"
#include "io.h"

#include "exec_platform.h"

#define ARM_PERIIOBASE ((IPTR)__arm_arosintern.ARMI_PeripheralBase)
#include <hardware/bcm2708.h>
#include <hardware/bcm2708_boot.h>
#include <hardware/pl011uart.h>

#define IRQBANK_POINTER(bank)   ((bank == 0) ? GPUIRQ_ENBL0 : (bank == 1) ? GPUIRQ_ENBL1 : ARMIRQ_ENBL)

#define IRQ_BANK1       0x00000100
#define IRQ_BANK2       0x00000200

#undef D
#define D(x) x
#define DIRQ(x)
#define DFIQ(x)
#define DTIMER(x)

extern void mpcore_trampoline();
extern uint32_t mpcore_end;
extern uint32_t mpcore_pde;
extern spinlock_t startup_lock;

extern void cpu_Register(void);
extern void arm_flush_cache(uint32_t, uint32_t);
#if defined(__AROSEXEC_SMP__)
extern void handle_ipi(uint32_t, uint32_t);

struct cpu_ipidata
{
    uint32_t    ipi_data[4];
};

struct cpu_ipidata *bcm2708_cpuipid[4] = { 0, 0, 0, 0};
#endif

static void bcm2708_init(APTR _kernelBase, APTR _sysBase)
{
    struct ExecBase *SysBase = (struct ExecBase *)_sysBase;
    struct KernelBase *KernelBase = (struct KernelBase *)_kernelBase;

    KrnSpinInit(&startup_lock);

#if defined(__AROSEXEC_SMP__)
    core_IPIInit();
#endif

    D(bug("[Kernel:BCM2708] %s()\n", __PRETTY_FUNCTION__));

    if (__arm_arosintern.ARMI_PeripheralBase == (APTR)BCM2836_PERIPHYSBASE)
    {
        void *trampoline_src = mpcore_trampoline;
        void *trampoline_dst = (void *)BOOTMEMADDR(bm_mctrampoline);
        uint32_t trampoline_length = (uintptr_t)&mpcore_end - (uintptr_t)mpcore_trampoline;
        uint32_t trampoline_data_offset = (uintptr_t)&mpcore_pde - (uintptr_t)mpcore_trampoline;
        int cpu;
        uint32_t *cpu_stack;
        uint32_t *cpu_fiq_stack;
        uint32_t tmp;
        tls_t   *__tls;

        /* Firmware leaves the prescaler at its 1MHz default; run the
         * generic timer off the crystal so CNTPCT matches CNTFRQ. */
        wr32le(BCM2836_CTRL, 0);
        wr32le(BCM2836_PRESCALER, 0x80000000);

        /* Register the boot CPU as an IPI receiver before waking the
         * secondaries (they do it themselves in cpu_Register), or
         * bcm2708_cpuipid[0] stays NULL and IPIs to it are dropped. */
        if (__arm_arosintern.ARMI_InitCore)
            __arm_arosintern.ARMI_InitCore(_kernelBase, _sysBase);

        bug("[Kernel:BCM2708] Initialising Multicore System\n");
        D(bug("[Kernel:BCM2708] %s: Copy SMP trampoline from %p to %p (%d bytes)\n", __PRETTY_FUNCTION__, trampoline_src, trampoline_dst, trampoline_length));

        bcopy(trampoline_src, trampoline_dst, trampoline_length);

        D(bug("[Kernel:BCM2708] %s: Patching data for trampoline at offset %d\n", __PRETTY_FUNCTION__, trampoline_data_offset));

        asm volatile ("mrc p15, 0, %0, c2, c0, 0":"=r"(tmp));
        ((uint32_t *)(trampoline_dst + trampoline_data_offset))[0] = tmp; // pde
        ((uint32_t *)(trampoline_dst + trampoline_data_offset))[1] = (uint32_t)cpu_Register;

        for (cpu = 1; cpu < 4; cpu ++)
        {
            cpu_stack = (uint32_t *)AllocMem(AROS_STACKSIZE*sizeof(uint32_t), MEMF_CLEAR); /* MEMF_PRIVATE */
            ((uint32_t *)(trampoline_dst + trampoline_data_offset))[2] = (uint32_t)&cpu_stack[AROS_STACKSIZE-sizeof(IPTR)];

            cpu_fiq_stack = (uint32_t *)AllocMem(1024*sizeof(uint32_t), MEMF_CLEAR); /* MEMF_PRIVATE */
            ((uint32_t *)(trampoline_dst + trampoline_data_offset))[4] = (uint32_t)&cpu_fiq_stack[1024-sizeof(IPTR)];


#if defined(__AROSEXEC_SMP__)
            __tls =  (tls_t *)AllocMem(sizeof(tls_t) + sizeof(struct cpu_ipidata),  MEMF_CLEAR); /* MEMF_PRIVATE */
#else
            __tls =  (tls_t *)AllocMem(sizeof(tls_t),  MEMF_CLEAR); /* MEMF_PRIVATE */
#endif
            __tls->SysBase = _sysBase;
            __tls->KernelBase = _kernelBase;
            __tls->ThisTask = NULL;
            __tls->CPUNumber = cpu;     /* logical id - GetCPUNumber reads this */
            arm_flush_cache(((uint32_t)__tls) & ~63, 512);
            ((uint32_t *)(trampoline_dst + trampoline_data_offset))[3] = (uint32_t)__tls;

            D(bug("[Kernel:BCM2708] %s: Attempting to wake CPU #%02d\n", __PRETTY_FUNCTION__, cpu));
            D(bug("[Kernel:BCM2708] %s: CPU #%02d Stack @ 0x%p (sp=0x%p)\n", __PRETTY_FUNCTION__, cpu, cpu_stack, ((uint32_t *)(trampoline_dst + trampoline_data_offset))[2]));
            D(bug("[Kernel:BCM2708] %s: CPU #%02d FIQ Stack @ 0x%p (sp=0x%p)\n", __PRETTY_FUNCTION__, cpu, cpu_fiq_stack, ((uint32_t *)(trampoline_dst + trampoline_data_offset))[4]));
            D(bug("[Kernel:BCM2708] %s: CPU #%02d TLS @ 0x%p\n", __PRETTY_FUNCTION__, cpu, ((uint32_t *)(trampoline_dst + trampoline_data_offset))[3]));

            arm_flush_cache((uint32_t)trampoline_dst, 512);

            /* Lock the startup spinlock */
            KrnSpinLock(&startup_lock, NULL, SPINLOCK_MODE_WRITE);

            /* Wake up the cpu */
            wr32le(BCM2836_MAILBOX3_SET0 + (0x10 * cpu), (uint32_t)trampoline_dst);

            dsb();

            sev();

            /*
             * Try to obtain spinlock again.
             * This should put this cpu to sleep since the lock was already obtained. Once the cpu startup
             * is ready, it will call KrnSpinUnLock() too
             */
            KrnSpinLock(&startup_lock, NULL, SPINLOCK_MODE_WRITE);
            KrnSpinUnLock(&startup_lock);
        }
    }
}

static void bcm2708_init_cpu(APTR _kernelBase, APTR _sysBase)
{
    struct ExecBase *SysBase = (struct ExecBase *)_sysBase;
    struct KernelBase *KernelBase = (struct KernelBase *)_kernelBase;
    (void)SysBase;
#if defined(__AROSEXEC_SMP__)
    tls_t   *__tls = TLS_PTR_GET();
#endif
    int cpunum = GetCPUNumber();

    D(bug("[Kernel:BCM2708] %s(#%02d)\n", __PRETTY_FUNCTION__, cpunum));

    /* Clear all pending FIQ sources on mailboxes */
    wr32le(BCM2836_MAILBOX0_CLR0 + (16 * cpunum), 0xffffffff);
    wr32le(BCM2836_MAILBOX1_CLR0 + (16 * cpunum), 0xffffffff);
    wr32le(BCM2836_MAILBOX2_CLR0 + (16 * cpunum), 0xffffffff);
    wr32le(BCM2836_MAILBOX3_CLR0 + (16 * cpunum), 0xffffffff);

#if defined(__AROSEXEC_SMP__)
    bcm2708_cpuipid[cpunum] = (struct cpu_ipidata *)(__tls + 1);
    D(bug("[Kernel:BCM2708] %s: CPU #%02d IPI data @ 0x%p\n", __PRETTY_FUNCTION__, cpunum, bcm2708_cpuipid[cpunum]));

    /* FIQ on all 4 mailboxes - senders index by source CPU number, so
     * they never OR-collide in the SET register. */
    wr32le(BCM2836_MAILBOX_INT_CTRL0 + (0x4 * cpunum), 0xf0);
#endif
}

static uint64_t bcm2708_get_time(void)
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

#if defined(__AROSEXEC_SMP__)
/*
 * Per-core scheduler heartbeat on the generic timer (CNTP). The BCM2708
 * system timer only reaches core 0, so every core arms its own CNTP and
 * takes the tick as an FIQ. CNTP_* are PL1-banked, so arming has to run
 * on the target core (ARMI_InitTimerCore, from cpu_Register).
 */
#define CNTP_CTL_ENABLE         (1 << 0)
#define CNTP_CTL_IMASK          (1 << 1)
#define CNTP_CTL_ISTATUS        (1 << 2)

#define BCM2836_TIMER_CNTPNS_FIQ (1 << 5)       /* TIMER_INT_CTRLx: route CNTPNS as FIQ */
#define BCM2836_FIQ_CNTPNS       (1 << 1)       /* FIQ_PENDx: CNTPNS pending */

/* CNTP reload value (counter ticks per heartbeat). Same on every core. */
static uint32_t bcm2708_cntp_interval = 0;

static inline uint32_t bcm2708_cntfrq_get(void)
{
    uint32_t v;
    asm volatile ("mrc p15, 0, %0, c14, c0, 0" : "=r"(v));
    return v;
}

static inline uint32_t bcm2708_cntpct_get(void)
{
    uint32_t lo, hi;
    asm volatile ("mrrc p15, 0, %0, %1, c14" : "=r"(lo), "=r"(hi));
    (void)hi;
    return lo;
}

/* CNTFRQ is firmware-programmed and not always true - QEMU reports
 * 19.2MHz while counting at 1MHz - so measure against the system timer
 * and keep CNTFRQ as the fallback. */
static uint32_t bcm2708_cntp_measure(void)
{
    const uint32_t window = 10000;      /* microseconds */
    uint32_t s0, c0, c1;

    s0 = rd32le(SYSTIMER_CLO);
    c0 = bcm2708_cntpct_get();
    while ((rd32le(SYSTIMER_CLO) - s0) < window)
        ;
    c1 = bcm2708_cntpct_get();

    return (c1 - c0) * (1000000 / window);
}

static inline void bcm2708_cntp_tval_set(uint32_t v)
{
    asm volatile ("mcr p15, 0, %0, c14, c2, 0" :: "r"(v));
}

static inline void bcm2708_cntp_ctl_set(uint32_t v)
{
    asm volatile ("mcr p15, 0, %0, c14, c2, 1" :: "r"(v));
}

static void bcm2708_cntp_tick(void)
{
    tls_t *__tls;

    /* Rearm for the next tick - writing TVAL clears the pending condition */
    bcm2708_cntp_tval_set(bcm2708_cntp_interval);

    /* The first tick can land before Exec is up on this core; keep
     * rearming regardless. Deliberately not gated on IDNESTCOUNT - a task
     * busy-looping in short Disable windows must still expire. */
    if (!SysBase || !KernelBase)
        return;

    /* Mirror of VBlankServer, minus the INTB_VERTB chain. Per-CPU TLS,
     * touched only by the owning core, so no atomics. */
    asm volatile ("mrc p15, 0, %0, c13, c0, 3" : "=r"(__tls));
    if (__tls->Elapsed)
        __tls->Elapsed--;
    if (__tls->Elapsed == 0)
        __tls->ScheduleFlags |= (TLSSF_Quantum | TLSSF_Switch);
}

static void bcm2708_init_cntp_timer(void)
{
    int cpunum = GetCPUNumber();

    if (!bcm2708_cntp_interval)
    {
        uint32_t hz = bcm2708_cntp_measure();

        /* Sanity-bound the measurement before trusting it over CNTFRQ. */
        if (hz < 100000 || hz > 100000000)
            hz = bcm2708_cntfrq_get();

        bcm2708_cntp_interval = hz / 50;
    }

    DTIMER(bug("[Kernel:BCM2708] %s: CPU #%02d CNTP interval %u\n", __PRETTY_FUNCTION__, cpunum, bcm2708_cntp_interval));

    /* PrepareExecBase calls SCHEDQUANTUM_SET once, but on arm that macro
     * writes per-CPU TLS, not the SysBase field the generic code assumes,
     * so no core ends up with a usable quantum. Set it per core, before
     * the heartbeat starts consuming it. */
    SCHEDQUANTUM_SET(SCHEDQUANTUM_VALUE);
    SCHEDELAPSED_SET(SCHEDQUANTUM_VALUE);

    /* Route this core's non-secure physical timer interrupt as an FIQ */
    wr32le(BCM2836_TIMER_INT_CTRL0 + (0x4 * cpunum), BCM2836_TIMER_CNTPNS_FIQ);

    /* Arm: fire one interval from now, enabled and unmasked */
    bcm2708_cntp_tval_set(bcm2708_cntp_interval);
    bcm2708_cntp_ctl_set(CNTP_CTL_ENABLE);
}
#endif

static void bcm2708_irq_init(void)
{
    // disable IRQ's
//    wr32le(ARMFIQ_CTRL, 0);

    wr32le(ARMIRQ_DIBL, ~0);
    wr32le(GPUIRQ_DIBL0, ~0);
    wr32le(GPUIRQ_DIBL1, ~0);

    /* BCM2836/2837 puts a local interrupt controller in front of the
     * legacy GPU IRQ controller. Route the shared GPU IRQ to core 0
     * ([1:0] IRQ target, [3:2] FIQ target), or enabled sources go
     * pending without ever reaching the ARM dispatcher. */
    if (__arm_arosintern.ARMI_PeripheralBase == (APTR)BCM2836_PERIPHYSBASE)
        wr32le(BCM2836_GPU_INT_ROUTING, 0);
}

static void bcm2708_send_ipi(uint32_t ipi, uint32_t ipi_data, uint32_t cpumask)
{
    int cpu;
#if defined(__AROSEXEC_SMP__)
    /*
     * Index the mailbox by source CPU, so every (sender, target) pair
     * gets its own slot and never OR-collides in the SET register.
     * Back-to-back IPIs from the same sender coalesce by OR, which is
     * lossless: ipi_msg values are bit flags, and the only payload
     * carrier (IPI_CALL_HOOK) keeps its data on the ipi_call_queue.
     * Disable() keeps an interrupt-driven Signal from re-entering and
     * overwriting the slot mid-write.
     */
    int mbno = GetCPUNumber();
    Disable();
#endif

    for (cpu = 0; cpu < 4; cpu++)
    {
#if defined(__AROSEXEC_SMP__)
        if ((cpumask & (1 << cpu)) && bcm2708_cpuipid[cpu])
        {
            bcm2708_cpuipid[cpu]->ipi_data[mbno] = ipi_data;
            dsb();
            wr32le(BCM2836_MAILBOX0_SET0 + 4 * mbno + (0x10 * cpu), ipi);
        }
#endif
    }

#if defined(__AROSEXEC_SMP__)
    Enable();
#endif
}

static void bcm2708_irq_enable(int irq)
{
    int bank = IRQ_BANK(irq);
    unsigned int reg;

    reg = (unsigned int)IRQBANK_POINTER(bank);

    DIRQ(bug("[Kernel:BCM2708] Enabling irq %d [bank %d, reg 0x%p]\n", irq, bank, reg));

    wr32le(reg, IRQ_MASK(irq));

    DIRQ(bug("[Kernel:BCM2708] irqmask=%08x\n", rd32le(reg)));
}

static void bcm2708_irq_disable(int irq)
{
    int bank = IRQ_BANK(irq);
    unsigned int reg;

    reg = (unsigned int)IRQBANK_POINTER(bank) + 0x0c;

    DIRQ(bug("[Kernel:BCM2708] Disabling irq %d [bank %d, reg 0x%p]\n", irq, bank, reg));

    wr32le(reg, IRQ_MASK(irq));

    DIRQ(bug("[Kernel:BCM2708] irqmask=%08x\n", rd32le(reg)));
}

static void bcm2708_irq_process()
{
    unsigned int pendingarm, pending0, pending1, irq;

    for(;;)
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

static void bcm2708_fiq_process()
{
    int cpunum = GetCPUNumber();
    uint32_t fiq, fiq_data;
    int mbno;

    DFIQ(bug("[Kernel:BCM2708] %s(%d)\n", __PRETTY_FUNCTION__, cpunum));

    fiq = rd32le(BCM2836_FIQ_PEND0 + (0x4 * cpunum));

    DFIQ(bug("[Kernel:BCM2708] %s: CPU #%02d FIQ %x\n", __PRETTY_FUNCTION__, cpunum, fiq));

    if (fiq)
    {
#if defined(__AROSEXEC_SMP__)
        /* Per-core scheduler heartbeat (CNTP) - expire the local quantum */
        if (fiq & BCM2836_FIQ_CNTPNS)
            bcm2708_cntp_tick();
#endif
        for (mbno=0; mbno < 4; mbno++)
        {
            if (fiq & (0x10 << mbno))
            {
                fiq_data = rd32le(BCM2836_MAILBOX0_CLR0 + 4 * mbno + (16 * cpunum));
                (void)fiq_data;
                DFIQ(bug("[Kernel:BCM2708] %s: Mailbox%d: FIQ Data %08x\n", __PRETTY_FUNCTION__, mbno, fiq_data));
#if defined(__AROSEXEC_SMP__)
                /* Pairs with the sender's dsb; ipi_data is indexed by
                 * mailbox because senders index by source CPU. */
                dsb();
                if (bcm2708_cpuipid[cpunum])
                    handle_ipi(fiq_data, bcm2708_cpuipid[cpunum]->ipi_data[mbno]);
#endif
                /* Clear only the bits read - a bit set between the read
                 * and this write must stay pending, or the IPI is lost. */
                wr32le(BCM2836_MAILBOX0_CLR0 + 4 * mbno + (16 * cpunum), fiq_data);
            }
        }
    }
}

/*
 * A Pi 2 puts PWR on GPIO 35, a Pi 400 on 42. The Pi 3B and 4B hand theirs to 
 * firmware controllers (expgpio, virtgpio) this code cannot reach - those keep 
 * the state firmware left, which for the power light is lit.
 */
static struct
{
    int pin;                    /* < 0: not ours to drive */
    int active_low;
} bcm2708_led[2] = { { -2, 0 }, { -2, 0 } };    /* -2: not looked up yet */

static int bcm2708_led_on_soc(const char *name)
{
    return name && name[0] == 'g' && name[1] == 'p' && name[2] == 'i' &&
           name[3] == 'o' && (name[4] == '@' || name[4] == 0);
}

static void bcm2708_led_query(int LED)
{
    void *node = dt_find_node((LED == ARM_LED_ACTIVITY) ? "/leds/led-act"
                                                        : "/leds/led-pwr");
    void *gprop = node ? dt_find_property(node, "gpios") : NULL;
    void *sprop = node ? dt_find_property(node, "status") : NULL;
    uint32_t *cells;
    of_node_t *ctrl;

    bcm2708_led[LED].pin = -1;

    if (!gprop || dt_get_prop_len(gprop) < 8)
        return;

    /* A node for a light the board does not have is marked disabled */
    if (sprop && ((char *)dt_get_prop_value(sprop))[0] == 'd')
        return;

    cells = dt_get_prop_value(gprop);
    ctrl = dt_find_node_by_phandle(AROS_BE2LONG(cells[0]));
    if (!ctrl || !bcm2708_led_on_soc(ctrl->on_name))
        return;

    bcm2708_led[LED].pin = AROS_BE2LONG(cells[1]);
    bcm2708_led[LED].active_low = (dt_get_prop_len(gprop) >= 12) &&
                                  (AROS_BE2LONG(cells[2]) & 1);
}

static void bcm2708_toggle_led(int LED, int state)
{
    int pin, lit;

    if (LED != ARM_LED_POWER && LED != ARM_LED_ACTIVITY)
        return;

    if (bcm2708_led[LED].pin == -2)
        bcm2708_led_query(LED);

    if ((pin = bcm2708_led[LED].pin) < 0)
        return;

    lit = (state == ARM_LED_ON) != (bcm2708_led[LED].active_low != 0);

    wr32le((lit ? GPSET0 : GPCLR0) + 4 * (pin / 32), 1 << (pin % 32));
}

/* Use system timer 3 for our scheduling heartbeat */
#define VBLANK_TIMER            3
#define VBLANK_INTERVAL         (1000000 / 50)

static void bcm2708_gputimer_handler(unsigned int timerno, void *unused1)
{
    unsigned int stc;

    DTIMER(bug("[Kernel:BCM2708] %s(%d)\n", __PRETTY_FUNCTION__, timerno));

    /* Acknowledge our timer interrupt */
    wr32le(SYSTIMER_CS, 1 << timerno);

    /* Signal the Exec VBlankServer */
    if (SysBase && (IDNESTCOUNT_GET /*SysBase->IDNestCnt*/ < 0)) {
        core_Cause(INTB_VERTB, 1L << INTB_VERTB);
    }

    /* Close a per-task CPU-usage window if one is due */
    core_TaskCPUUsage();

    /* Refresh our timer interrupt */
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
        GPUTimerHandle->in_HandlerData = (void *)VBLANK_TIMER;
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
    }

    DTIMER(bug("[Kernel:BCM2708] %s: Done.. \n", __PRETTY_FUNCTION__));

    return GPUTimerHandle;
}

static inline void bcm2708_ser_waitout()
{
    unsigned int spins = 0;
    while(1)
    {
       if ((rd32le(PL011_0_BASE + PL011_FR) & PL011_FR_TXFF) == 0) break;
       if (++spins > 1000000) return;
    }
}

static void bcm2708_ser_putc(uint8_t chr)
{
    bcm2708_ser_waitout();

    if (chr == '\n')
    {
        wr32le(PL011_0_BASE + PL011_DR, '\r');
        bcm2708_ser_waitout();
    }
    wr32le(PL011_0_BASE + PL011_DR, chr);
}

static int bcm2708_ser_getc(void)
{
    if ((rd32le(PL011_0_BASE + PL011_FR) & PL011_FR_RXFE) == 0)
        return (int)rd32le(PL011_0_BASE + PL011_DR);

    return -1;
}

static IPTR bcm2708_probe(struct ARM_Implementation *krnARMImpl, struct TagItem *msg)
{
    void *bootPutC = NULL;
    APTR bootPeriBase = NULL;

    while(msg->ti_Tag != TAG_DONE)
    {
        switch (msg->ti_Tag)
        {
        case KRN_FuncPutC:
            bootPutC = (void *)msg->ti_Data;
            break;
        case KRN_PeripheralBase:
            bootPeriBase = (APTR)msg->ti_Data;
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
        krnARMImpl->ARMI_InitCore = &bcm2708_init_cpu;
        krnARMImpl->ARMI_FIQProcess = &bcm2708_fiq_process;
        krnARMImpl->ARMI_SendIPI = &bcm2708_send_ipi;
#if defined(__AROSEXEC_SMP__)
        krnARMImpl->ARMI_InitTimerCore = &bcm2708_init_cntp_timer;
#endif
    }
    else
        krnARMImpl->ARMI_PeripheralBase = (APTR)BCM2835_PERIPHYSBASE;

    if (bootPeriBase)
        krnARMImpl->ARMI_PeripheralBase = bootPeriBase;

    krnARMImpl->ARMI_GetTime = &bcm2708_get_time;
    krnARMImpl->ARMI_InitTimer = &bcm2708_init_gputimer;
    krnARMImpl->ARMI_LED_Toggle = &bcm2708_toggle_led;

    krnARMImpl->ARMI_SerPutChar = &bcm2708_ser_putc;
    krnARMImpl->ARMI_SerGetChar = &bcm2708_ser_getc;
    if ((krnARMImpl->ARMI_PutChar = bootPutC) != NULL)
            krnARMImpl->ARMI_PutChar(0xFF); // Clear the display

    krnARMImpl->ARMI_IRQInit = &bcm2708_irq_init;
    krnARMImpl->ARMI_IRQEnable = &bcm2708_irq_enable;
    krnARMImpl->ARMI_IRQDisable = &bcm2708_irq_disable;
    krnARMImpl->ARMI_IRQProcess = &bcm2708_irq_process;

    krnARMImpl->ARMI_Init = &bcm2708_init;

    return TRUE;
}

ADD2SET(bcm2708_probe, ARMPLATFORMS, 0);
