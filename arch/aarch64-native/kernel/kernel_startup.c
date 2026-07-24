/*
    Copyright (C) 2013-2026, The AROS Development Team. All rights reserved.

    AArch64 kernel startup.
*/

#define DEBUG 1

#include <aros/kernel.h>
#include <aros/symbolsets.h>

#include <aros/aarch64/cpucontext.h>

#include <aros/cpu.h>

#include <exec/memory.h>
#include <exec/memheaderext.h>
#include <exec/tasks.h>
#include <exec/alerts.h>
#include <exec/execbase.h>
#include <asm/io.h>
#include <proto/kernel.h>
#include <proto/exec.h>

#include <strings.h>
#include <string.h>

#include "exec_intern.h"
#include "etask.h"
#include "tlsf.h"

#include "kernel_intern.h"
#include "kernel_debug.h"
#include "kernel_romtags.h"

#include "exec_platform.h"

#undef KernelBase
#include "tls.h"

extern struct TagItem *BootMsg;

void __attribute__((used)) kernel_cstart(struct TagItem *msg);
static void __attribute__((used, noreturn, noinline)) kernel_cstart_user(void);

uint64_t stack[AROS_STACKSIZE] __attribute__((used,aligned(16)));
uint64_t stack_super[AROS_STACKSIZE] __attribute__((used,aligned(16)));

/*
 * AArch64 entry point.
 * The bootstrap passes the TagItem pointer in x0.
 * We arrive at EL1 with MMU enabled by the bootstrap.
 */
asm (
    ".section .aros.init,\"ax\"\n\t"
    ".globl start\n\t"
    ".type start,%function\n"
    "start:\n"
    "           mov     x20, x0                  \n" /* Save TagItem pointer */
    "           bl      __clear_bss              \n"
    "           mov     x0, x20                  \n" /* Restore TagItem pointer */

    /* Set up EL1 stack (SP_EL1 via SPSel) */
    "           msr     spsel, #1                \n" /* Use SP_EL1 */
    "           ldr     x1, =stack_super_end     \n"
    "           ldr     x1, [x1]                 \n"
    "           mov     sp, x1                   \n"

    /* Set up EL0 stack (SP_EL0 for user mode) */
    "           ldr     x1, =stack_end           \n"
    "           ldr     x1, [x1]                 \n"
    "           msr     sp_el0, x1               \n"

    "           b       kernel_cstart            \n"

    ".string \"Native/CORE AArch64 v1 (" __DATE__ ")\"" "\n\t\n\t"
);

#if defined(__clang__)
static uint64_t * const stack_end __attribute__((used, section(".aros.init"))) = &stack[AROS_STACKSIZE - sizeof(IPTR)];
static uint64_t * const stack_super_end __attribute__((used, section(".aros.init"))) = &stack_super[AROS_STACKSIZE - sizeof(IPTR)];
#else
static uint64_t * const stack_end __attribute__((used, section(".aros.init " TARGET_SECTION_COMMENT))) = &stack[AROS_STACKSIZE - sizeof(IPTR)];
static uint64_t * const stack_super_end __attribute__((used, section(".aros.init " TARGET_SECTION_COMMENT))) = &stack_super[AROS_STACKSIZE - sizeof(IPTR)];
#endif

struct ARM_Implementation __arm_arosintern  __attribute__((aligned(8), section(".data"))) = {0,0,NULL,0};
struct ExecBase *SysBase __attribute__((section(".data"))) = NULL;

static void __attribute__((used)) __clear_bss(struct TagItem *msg)
{
    struct KernelBSS *bss = (struct KernelBSS *)krnGetTagData(KRN_KernelBss, 0, msg);
    register uintptr_t dest;
    unsigned int length;

    if (bss)
    {
        while (bss->addr && bss->len)
        {
            dest = (uintptr_t)bss->addr;
            length = bss->len;

            /* Byte-align start */
            while ((dest & 7) && length)
            {
                *((unsigned char *)dest) = 0;
                dest++;
                length--;
            }

            /* 64-bit word-aligned fill */
            while (length >= 8)
            {
                *((uint64_t *)dest) = 0;
                dest += 8;
                length -= 8;
            }

            /* Remaining bytes */
            while (length)
            {
                *((unsigned char *)dest) = 0;
                dest++;
                length--;
            }
            bss++;
        }
    }
}

static inline void uart_putc(char c)
{
    volatile uint32_t *uart = (volatile uint32_t *)0x3f201000;
    while (uart[0x18/4] & (1 << 5)) ; /* wait for TXFF clear */
    if (c == '\n') { uart[0] = '\r'; while (uart[0x18/4] & (1 << 5)) ; }
    uart[0] = c;
}
static void uart_puts(const char *s) { while (*s) uart_putc(*s++); }

void __attribute__((used)) kernel_cstart(struct TagItem *msg)
{
    UWORD *ranges[3];
    struct MinList memList;
    struct MemHeader *mh;
    long unsigned int memlower = 0, memupper = 0, protlower = 0, protupper = 0;
    char *cmdline = NULL;

    uart_puts("[Kernel] kernel_cstart entered\n");
    BootMsg = msg;
    tls_t *__tls;

    /* First find the device tree */
    while (msg->ti_Tag != TAG_DONE)
    {
        if (msg->ti_Tag == KRN_OpenFirmwareTree)
        {
            dt_set_root((void *)msg->ti_Data);
            break;
        }
        msg++;
    }
    msg = BootMsg;

    uart_puts("[Kernel] calling cpu_Probe\n");
    /* Probe the CPU */
    cpu_Probe(&__arm_arosintern);

    uart_puts("[Kernel] calling platform_Init\n");
    /* Find platform identifier */
    __arm_arosintern.ARMI_Platform = 0;
    while (msg->ti_Tag != TAG_DONE)
    {
        if (msg->ti_Tag == KRN_Platform)
        {
            __arm_arosintern.ARMI_Platform = msg->ti_Data;
            break;
        }
        msg++;
    }
    msg = BootMsg;

    platform_Init(&__arm_arosintern, msg);
    uart_puts("[Kernel] platform_Init done\n");

    if (__arm_arosintern.ARMI_LED_Toggle)
    {
        if (__arm_arosintern.ARMI_Delay)
            __arm_arosintern.ARMI_Delay(100000);
        __arm_arosintern.ARMI_LED_Toggle(ARM_LED_POWER, ARM_LED_OFF);
    }

    cpu_Init(&__arm_arosintern, msg);

    if (__arm_arosintern.ARMI_LED_Toggle)
    {
        if (__arm_arosintern.ARMI_Delay)
            __arm_arosintern.ARMI_Delay(100000);
        __arm_arosintern.ARMI_LED_Toggle(ARM_LED_POWER, ARM_LED_ON);
    }

    while (msg->ti_Tag != TAG_DONE)
    {
        switch (msg->ti_Tag)
        {
        case KRN_CmdLine:
            cmdline = (char *)msg->ti_Data;
            break;
        case KRN_MEMLower:
            memlower = msg->ti_Data;
            break;
        case KRN_MEMUpper:
            memupper = msg->ti_Data;
            break;
        case KRN_ProtAreaStart:
            protlower = msg->ti_Data;
            break;
        case KRN_ProtAreaEnd:
            protupper = (msg->ti_Data + 4095) & ~4095;
            break;
        case KRN_KernelBase:
            break;
        }
        msg++;
    }
    msg = BootMsg;

    /* Allocate TLS from the protected area */
    __tls = (void *)protupper;
    protupper += (sizeof(tls_t) + 4095) & ~4095;

    /*
     * Zero the whole TLS block: QEMU hands us zeroed RAM, but real DRAM
     * after a warm reboot does not -- garbage ScheduleFlags/nest counts
     * would survive into steady state.
     */
    memset(__tls, 0, sizeof(tls_t));

    __tls->KernelBase = NULL;
    __tls->SysBase = NULL;
    __tls->ThisTask = NULL;

    /* Set TPIDR_EL1 for TLS access BEFORE any bug() calls */
    asm volatile("msr tpidr_el1, %0" : : "r"(__tls));

    /* Bring-up diagnostic: verify the relocated stack layout */
    bug("[Kernel] stack @ %p, stack_super @ %p..%p (SP_EL1 start %p)\n",
        stack, stack_super, &stack_super[AROS_STACKSIZE], stack_super_end);

    D(bug("[Kernel] AROS AArch64 Native Kernel built on %s\n", __DATE__));
    if (dt_find_node("/")) {
        D(bug("[Kernel] Device: %s\n", dt_get_prop_value(dt_find_property(dt_find_node("/"), "model"))));
    }

    D(bug("[Kernel] Entered kernel_cstart @ 0x%p, BootMsg @ 0x%p\n", kernel_cstart, BootMsg));

    D(
        if (__arm_arosintern.ARMI_PutChar)
        {
            bug("[Kernel] Using PutChar implementation @ %p\n", __arm_arosintern.ARMI_PutChar);
        }
        bug("[Kernel] Boot CPU TLS @ 0x%p\n", __tls);
    )

    uart_puts("[Kernel] calling core_SetupIntr\n");
    core_SetupIntr();

    if (__arm_arosintern.ARMI_LED_Toggle)
        __arm_arosintern.ARMI_LED_Toggle(ARM_LED_POWER, ARM_LED_OFF);

    D(bug("[Kernel] Platform initialised\n"));

    if (__arm_arosintern.ARMI_Delay)
        __arm_arosintern.ARMI_Delay(1500);

    if (__arm_arosintern.ARMI_LED_Toggle)
        __arm_arosintern.ARMI_LED_Toggle(ARM_LED_POWER, ARM_LED_ON);

    D(bug("[Kernel] Preparing memory 0x%p -> 0x%p\n", memlower, memupper));
    D(bug("[Kernel] - protected area 0x%p -> 0x%p)\n", protlower, protupper));

    NEWLIST(&memList);

    if (memlower >= protlower)
        memlower = protupper;

    mh = (struct MemHeader *)memlower;

    if (cmdline && strstr(cmdline, "notlsf"))
    {
        /* Plain memory header (for debugging) */
    }
    else
    {
        /* Initialize TLSF memory allocator */
        krnCreateTLSFMemHeader("System Memory", 0, mh, (memupper - memlower), MEMF_FAST | MEMF_PUBLIC | MEMF_KICK | MEMF_LOCAL);
        if (memlower < protlower)
        {
            ((struct MemHeaderExt *)mh)->mhe_AllocAbs((struct MemHeaderExt *)mh, protupper-protlower, (void *)protlower);
        }
    }

    ranges[0] = (UWORD *)krnGetTagData(KRN_KernelLowest, 0, msg);
    ranges[1] = (UWORD *)krnGetTagData(KRN_KernelHighest, 0, msg);
    ranges[2] = (UWORD *)-1;

    D(bug("[Kernel] Preparing ExecBase (memheader @ 0x%p)\n", mh));
    krnPrepareExecBase(ranges, mh, BootMsg);

    __tls->SysBase = SysBase;
    D(bug("[Kernel] SysBase @ 0x%p\n", SysBase));

    D(bug("[Kernel] InitCode(RTF_SINGLETASK) ... \n"));
    InitCode(RTF_SINGLETASK, 0);

    /*
     * Switch to the OS stack but stay privileged at EL1.
     *
     * AROS is a flat, single-address-space OS and runs privileged (like the
     * x86_64 port in ring 0); it cannot run at EL0, because AArch64 has no
     * ARMv7-style domains and treats any EL0-writable page as PXN, so EL1
     * could not execute and EL0 could not be granted access to kernel memory.
     *
     * We still want the OS to run on stack[] while stack_super (SP_EL1) stays
     * reserved for exception handlers, so ERET to EL1t (SPSel=0 -> SP_EL0,
     * which boot set to stack_end). Taking an exception then switches to
     * SP_EL1 = stack_super automatically.
     */
    D(bug("[Kernel] Continuing at EL1 on the OS stack ... \n"));
    asm volatile(
        "adr    x0, 1f                 \n" /* Address of kernel_cstart_user call */
        "msr    elr_el1, x0            \n" /* Set return address */
        "mov    x0, #0x4               \n" /* SPSR: EL1t (stay at EL1, use SP_EL0) */
        "msr    spsr_el1, x0           \n"
        "eret                          \n"
        "1:                            \n"
        "bl     kernel_cstart_user     \n"
        ::: "x0", "x30", "memory"
    );
    __builtin_unreachable();
}

/*
 * Continuation of kernel_cstart. Runs at EL1 on stack[] (SP_EL0), with
 * stack_super (SP_EL1) left for exception handlers.
 */
static void __attribute__((used, noreturn, noinline)) kernel_cstart_user(void)
{
    D(bug("[Kernel] InitCode(RTF_COLDSTART) ...\n"));
    InitCode(RTF_COLDSTART, 0);

    /* The above should not return */
    krnPanic(KernelBase, "System Boot Failed!");
    __builtin_unreachable();
}

DEFINESET(ARMPLATFORMS);
