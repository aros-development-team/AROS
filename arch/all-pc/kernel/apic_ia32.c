/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Intel IA-32 APIC driver.
*/

#include <asm/cpu.h>
#include <asm/io.h>
#include <exec/types.h>

#include <inttypes.h>

#include "apic.h"
#include "apic_ia32.h"
#include "kernel_base.h"
#include "kernel_debug.h"
#include "kernel_syscall.h"
#include "kernel_timer.h"

#define D(x)
#define DWAKE(x) /* Badly interferes with AP startup */
/* #define DEBUG_WAIT */

/*
 * On i386 platform we need to support various quirks of old APICs.
 * x86-64 is free of that crap.
 */
#ifdef __i386__
#define CONFIG_LEGACY
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

    D(bug("[IPI] Waiting for IPI to complete "));

    for (ipisend_timeout = 1000; ipisend_timeout > 0; ipisend_timeout--)
    {
        pit_udelay(100);
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

void core_APIC_Init(struct APICData *apic, ULONG cpuNum)
{
    IPTR __APICBase = apic->lapicBase;
    ULONG apic_ver = APIC_REG(__APICBase, APIC_VERSION);
    ULONG maxlvt = APIC_LVT(apic_ver);
    ULONG lapic_initial, lapic_final;
    WORD pit_final;

#ifdef CONFIG_LEGACY
    /* 82489DX doesnt report no. of LVT entries. */
    if (!APIC_INTEGRATED(apic_ver))
    	maxlvt = 2;
#endif

    /* Use flat interrupt model with logical destination ID = 1 */
    APIC_REG(__APICBase, APIC_DFR) = DFR_FLAT;
    APIC_REG(__APICBase, APIC_LDR) = 1 << LDR_ID_SHIFT;

    /* Set Task Priority to 'accept all interrupts' */
    APIC_REG(__APICBase, APIC_TPR) = 0;

    /* Set spurious IRQ vector to 0xFF. APIC enabled, focus check disabled. */
    APIC_REG(__APICBase, APIC_SVR) = SVR_ASE|SVR_FCC|0xFF;

    /*
     * Set LINT0 to external and LINT1 to NMI.
     * These are common defaults and they are going to be overriden by ACPI tables.
     */
    APIC_REG(__APICBase, APIC_LINT0_VEC) = LVT_MT_EXT;
    APIC_REG(__APICBase, APIC_LINT1_VEC) = LVT_MT_NMI;

#ifdef CONFIG_LEGACY
    /* Due to the Pentium erratum 3AP. */
    if (maxlvt > 3)
       	 APIC_REG(__APICBase, APIC_ESR) = 0;
#endif

    D(bug("[APIC.%u] _APIC_IA32_init: APIC ESR before enabling vector: %08x\n", cpuNum, APIC_REG(__APICBase, APIC_ESR)));

    /* Set APIC error interrupt to fixed vector 0xFE interrupt on APIC error */
    APIC_REG(__APICBase, APIC_ERROR_VEC) = 0xfe;

    /* spec says clear errors after enabling vector. */
    if (maxlvt > 3)
       	 APIC_REG(__APICBase, APIC_ESR) = 0;

    D(bug("[APIC.%u] _APIC_IA32_init: APIC ESR after enabling vector: %08x\n", cpuNum, APIC_REG(__APICBase, APIC_ESR)));

    /*
     * Now the tricky thing - calibrate LAPIC timer frequency.
     * In fact we could simply query CPU's clock frequency, but... x86 sucks. There's no
     * unified way to get it on whatever CPU. Intel has own way, AMD has own way... Etc... Which, additionally,
     * varies between CPU generations.
     *
     * The idea behing the calibration is to run the timer once, and see how much ticks passes in some defined
     * period of time. Then calculate a proportion.
     * We use 8253 PIT as our reference.
     * This calibrarion algorighm is based on NetBSD one.
     */

    /* Set the timer to one-shot mode, no interrupt, 1:1 divisor */
    APIC_REG(__APICBase, APIC_TIMER_VEC) = LVT_MASK;
    APIC_REG(__APICBase, APIC_TIMER_DIV) = TIMER_DIV_1;
    APIC_REG(__APICBase, APIC_TIMER_ICR) = 0x80000000;	/* Just some very large value */

    /*
     * Now wait for 11931 PIT ticks, which is equal to 10 milliseconds.
     * We don't use pit_udelay() here, because for improved accuracy we need to sample LAPIC timer counter twice,
     * before and after our actual delay (PIT setup also takes up some time, so LAPIC will count away from its
     * initial value).
     */
    pit_start(11931);
    lapic_initial = APIC_REG(__APICBase, APIC_TIMER_CCR);

    pit_final   = pit_wait(11931);
    lapic_final = APIC_REG(__APICBase, APIC_TIMER_CCR);

    D(bug("[APIC.%u] LAPIC counted from %u to %u in 10ms (%u ticks)\n", cpuNum, lapic_initial, lapic_final, 11931 - pit_final));

    apic->cores[cpuNum].timerFreq = (lapic_initial - lapic_final) * 100;
    D(bug("[APIC.%u] LAPIC frequency should be %u Hz (%u mHz)\n", cpuNum, apic->cores[cpuNum].timerFreq, apic->cores[cpuNum].timerFreq / 1000000));
}

IPTR core_APIC_GetBase(void)
{
    IPTR _apic_base = 0;

    if (IN_USER_MODE)
    {
        D(bug("[APIC] _APIC_IA32_GetMSRAPICBase: Called in UserMode\n"));

	__asm__ __volatile__ ("int $0x80":"=a"(_apic_base):"a"(SC_RDMSR),"c"(MSR_LAPIC_BASE));
    }
    else
    {
        _apic_base = rdmsri(MSR_LAPIC_BASE);
    }

    _apic_base &= APIC_BASE_MASK;

    D(bug("[APIC] _APIC_IA32_GetMSRAPICBase: MSR APIC Base @ %p\n", _apic_base));
    return _apic_base;
}

UBYTE core_APIC_GetID(IPTR _APICBase)
{
    UBYTE _apic_id;

    /* The actual ID is in 8 most significant bits */
    _apic_id = APIC_REG(_APICBase, APIC_ID) >> APIC_ID_SHIFT;
    D(bug("[APIC] _APIC_IA32_GetID: APIC ID %d\n", _apic_id));

    return _apic_id;
}

void core_APIC_AckIntr(void)
{
    /* Write zero to EOI of current APIC */
    IPTR apic_base = rdmsri(MSR_LAPIC_BASE) & APIC_BASE_MASK;

    APIC_REG(apic_base, APIC_EOI) = 0;
}

ULONG core_APIC_Wake(APTR wake_apicstartrip, UBYTE wake_apicid, IPTR __APICBase)
{
    ULONG status_ipisend, status_ipirecv;
    ULONG start_count;
#ifdef CONFIG_LEGACY
    ULONG apic_ver = APIC_REG(__APICBase, APIC_VERSION);
#endif

    D(bug("[APIC] _APIC_IA32_wake(0x%02X @ %p)\n", wake_apicid, wake_apicstartrip));
    D(bug("[APIC] _APIC_IA32_wake: APIC ID 0x%02X Base @ %p\n", core_APIC_GetID(__APICBase), __APICBase));

#ifdef CONFIG_LEGACY
    /*
     * Check if we have old 82489DX discrete APIC (version & 0xF0 == 0).
     * This APIC needs different startup procedure. It doesn't support STARTUP IPI
     * because old CPUs didn't have INIT signal. They jump to BIOS ROM boot code
     * immediately after INIT IPI. In order to run the bootstrap, a BIOS warm reset
     * magic has to be used there.
     */
    if (!APIC_INTEGRATED(apic_ver))
    {
    	/*
    	 * BIOS warm reset magic, part one.
    	 * Write real-mode bootstrap routine address to 40:67 (real-mode address) location.
    	 * This is standard feature of IBM PC AT BIOS. If a warm reset condition is detected,
    	 * the BIOS jumps to the given address.
     	 */
	D(bug("[APIC] Setting BIOS vector for trampoline @ %p ..\n", wake_apicstartrip));
	*((volatile unsigned short *)0x469) = (IPTR)wake_apicstartrip >> 4;
	*((volatile unsigned short *)0x467) = 0; /* Actually wake_apicstartrip & 0x0F, it's page-aligned. */

	/*
	 * BIOS warm reset magic, part two.
	 * This writes 0x0A into CMOS RAM, location 0x0F. This signals a warm reset condition to BIOS,
	 * making part one work.
	 */
	D(bug("[APIC] Setting warm reset code ..\n"));
	outb(0xf, 0x70);
	outb(0xa, 0x71);
    }
#endif

    /* Flush TLB (we are supervisor here) */
    wrcr(cr3, rdcr(cr3));

    /* First we send the INIT command (reset the core). Vector must be zero for this. */
    status_ipisend = DoIPI(__APICBase, wake_apicid, ICR_INT_LEVELTRIG | ICR_INT_ASSERT | ICR_DM_INIT);    
    if (status_ipisend)
    {
    	D(bug("[APIC] Error asserting INIT\n"));
    	return status_ipisend;
    }

    /* Deassert INIT after a small delay */
    pit_udelay(10 * 1000);

    /* Deassert INIT */
    status_ipisend = DoIPI(__APICBase, wake_apicid, ICR_INT_LEVELTRIG | ICR_DM_INIT);
    if (status_ipisend)
    {
    	D(bug("[APIC] Error deasserting INIT\n"));
    	return status_ipisend;
    }

    /* memory barrier */
    asm volatile("mfence":::"memory");

#ifdef CONFIG_LEGACY
    /* If it's 82489DX, we are done. */
    if (!APIC_INTEGRATED(apic_ver))
    {
	DWAKE(bug("[APIC] _APIC_IA32_wake: 82489DX detected, wakeup done\n"));
    	return 0;
    }
#endif

    /*
     * Perform IPI STARTUP loop.
     * According to official Intel specification, this must be done twice.
     * It's not explained why. ;-)
     */
    for (start_count = 1; start_count <= 2; start_count++)
    {
        D(bug("[APIC] _APIC_IA32_wake: Attempting STARTUP .. %d\n", start_count));

	/* Clear any pending error condition */
        APIC_REG(__APICBase, APIC_ESR) = 0;

        /*
         * Send STARTUP IPI.
         * The processor starts up at CS = (vector << 16) and IP = 0.
         */
        status_ipisend = DoIPI(__APICBase, wake_apicid, ICR_DM_STARTUP | ((IPTR)wake_apicstartrip >> 12));

        /* Allow the target APIC to accept the IPI */
        pit_udelay(200);

#ifdef CONFIG_LEGACY
	/* Pentium erratum 3AP quirk */
        if (APIC_LVT(apic_ver) > 3)
            APIC_REG(__APICBase, APIC_ESR) = 0;
#endif

        status_ipirecv = APIC_REG(__APICBase, APIC_ESR) & 0xEF;

	/*
	 * EXPERIMENTAL:
	 * On my machine (macmini 3,1, as OS X system profiler says), the core starts up from first
	 * attempt. The second attempt ends up in error (according to the documentation, the STARTUP
	 * can be accepted only once, while the core in RESET or INIT state, and first STARTUP, if
	 * succesful, brings the core out of this state).
	 * Here we try to detect this condition. If the core accepted STARTUP, we suggest that it has
	 * started up, and break the loop.
	 * A topic at osdev.org forum (http://forum.osdev.org/viewtopic.php?f=1&t=23018)
	 * also tells about some problems with double STARTUP. According to it, the second STARTUP can
	 * manage to re-run the core from the given address, leaving it in 64-bit mode, causing it to crash.
	 *
	 * If startup problems pops up (the core doesn't respond and AROS halts at "Launching APIC no X" stage),
	 * the following two variations of this algorithm can be tried:
	 * a) Always send STARTUP twice, but signal error condition only if both attempts failed.
	 * b) Send first STARTUP, abort on error. Allow second attempt to fail and ignore its result.
	 *
	 *								Sonic <pavel_fedin@mail.ru>
	 */
	if (!status_ipisend && !status_ipirecv)
	    break;
    }

    DWAKE(bug("[APIC] _APIC_IA32_wake: STARTUP run status 0x%08X, error 0x%08X\n", status_ipisend, status_ipirecv));

    /*
     * We return nonzero on error.
     * Actually least significant byte of this value holds ESR value, and 12th bit
     * holds delivery status flag from DoIPI() routine. It will be '1' if we got
     * stuck at sending phase.
     */
    return (status_ipisend | status_ipirecv);
}
