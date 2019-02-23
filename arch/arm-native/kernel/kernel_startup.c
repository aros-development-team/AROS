/*
    Copyright © 2013-2015, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 1

#include <aros/kernel.h>
#include <aros/symbolsets.h>

#include <aros/arm/cpucontext.h>

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

uint32_t stack[AROS_STACKSIZE] __attribute__((used,aligned(16)));
static uint32_t stack_super[AROS_STACKSIZE] __attribute__((used,aligned(16)));

static uint32_t stack_fiq[1024] __attribute__((used,aligned(16)));

asm (
    ".section .aros.init,\"ax\"\n\t"
    ".globl start\n\t"
    ".type start,%function\n"
    "start:\n"
    "           push {r0}                    \n"
    "           bl      __clear_bss          \n"
    "           pop {r0}                     \n"
    "           cps     #0x1f                \n" /* system mode */
    "           ldr     sp, stack_end        \n"
    "           cps     #0x11                \n" /* fiq mode */
    "           ldr     sp, stack_fiq_end    \n"
    "           cps     #0x13                \n" /* SVC (supervisor) mode */
    "           ldr     sp, stack_super_end  \n"
    "           b       kernel_cstart	     \n"

    ".string \"Native/CORE v3 (" __DATE__ ")\"" "\n\t\n\t"
);

static uint32_t * const stack_end __attribute__((used, section(".aros.init"))) = &stack[AROS_STACKSIZE - sizeof(IPTR)];
static uint32_t * const stack_super_end __attribute__((used, section(".aros.init"))) = &stack_super[AROS_STACKSIZE - sizeof(IPTR)];
static uint32_t * const stack_fiq_end __attribute__((used, section(".aros.init"))) = &stack_fiq[1024 - sizeof(IPTR)];


struct ARM_Implementation __arm_arosintern  __attribute__((aligned(4), section(".data"))) = {0,0,NULL,0};
struct ExecBase *SysBase __attribute__((section(".data"))) = NULL;

static void __attribute__((used)) __clear_bss(struct TagItem *msg)
{
    struct KernelBSS *bss = (struct KernelBSS *)krnGetTagData(KRN_KernelBss, 0, msg);
    register unsigned int dest;
    unsigned int length;

    if (bss)
    {
        while (bss->addr && bss->len)
        {
            dest = (unsigned int)bss->addr;
            length = bss->len;

            // If the start address is unaligned, fill in the first 1-3 bytes until it is
            while((dest & 3) && length)
            {
                *((unsigned char *)dest) = 0;
                dest++;
                length--;
            }

            // Fill in the remaining 32-bit word-aligned memory locations
            while(length & 0xfffffffc)
            {
                *((unsigned int *)dest) = 0;
                dest += 4;
                length -= 4;
            }

            // Deal with the remaining 1-3 bytes, if any
            while(length)
            {
                dest++;
                length--;
                *((unsigned char *)dest) = 0;
            }
            bss++;
        }
    }
}

void __attribute__((used)) kernel_cstart(struct TagItem *msg)
{
    UWORD *ranges[3];
    struct MinList memList;
    struct MemHeader *mh;
    long unsigned int memlower = 0, memupper = 0, protlower = 0, protupper = 0;
    char *cmdline = NULL;
    BootMsg = msg;
    tls_t *__tls;

    // First find out if device tree is present
    while(msg->ti_Tag != TAG_DONE)
    {
        int found = 0;

        switch (msg->ti_Tag)
        {
        case KRN_OpenFirmwareTree:
            dt_set_root((void *)msg->ti_Data);
            found = 1;
            break;
        }

        if (found)
            break;

        msg++;
    }
    msg = BootMsg;

    // Probe the ARM core
    cpu_Probe(&__arm_arosintern);

    // Probe the ARM Implementation/Platform
    __arm_arosintern.ARMI_Platform = 0;
    while(msg->ti_Tag != TAG_DONE)
    {
        switch (msg->ti_Tag)
        {
        case KRN_Platform:
            __arm_arosintern.ARMI_Platform = msg->ti_Data;
            break;
        }
        msg++;
    }
    msg = BootMsg;

    platform_Init(&__arm_arosintern, msg);

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

    /* NB: the bootstrap has conveniently setup the framebuffer
            and initialised the serial port and led for us */

    while(msg->ti_Tag != TAG_DONE)
    {
        switch (msg->ti_Tag)
        {
        case KRN_CmdLine:
//            RelocateStringData(tag);
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
            // Page align
            protupper = (msg->ti_Data + 4095) & ~4095;
            break;
        case KRN_KernelBase:
            /*
             * KRN_KernelBase is actually a border between read-only
             * (code) and read-write (data) sections of the kickstart.
             * read-write section goes to lower addresses from this one,
             * so we align it upwards in order not to make part of RW data
             * read-only.
             */
//            addr = AROS_ROUNDUP2(msg->ti_Data, PAGE_SIZE);
            break;
        }
        msg++;
    }
    msg = BootMsg;

    __tls = (void *)protupper;
    protupper += (sizeof(tls_t) + 4095) & ~4095;

    __tls->KernelBase = NULL;
    __tls->SysBase = NULL;
    __tls->ThisTask = NULL;

    D(bug("[Kernel] AROS ARM Native Kernel built on %s\n", __DATE__));
    if (dt_find_node("/")) {
        D(bug("[Kernel] Device: %s\n", dt_get_prop_value(dt_find_property(dt_find_node("/"), "model"))));
    }

    D(bug("[Kernel] Entered kernel_cstart @ 0x%p, BootMsg @ 0x%p\n", kernel_cstart, BootMsg));

    asm volatile("mcr p15, 0, %0, c13, c0, 3" : : "r"(__tls));

    D(
        if (__arm_arosintern.ARMI_PutChar)
        {
            bug("[Kernel] Using PutChar implementation @ %p\n", __arm_arosintern.ARMI_PutChar);
        }
        bug("[Kernel] Boot CPU TLS @ 0x%p\n", __tls);
    )

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
#if (0)
        krnCreateMemHeader("System Memory", 0, mh, (memupper - memlower), MEMF_FAST | MEMF_PUBLIC | MEMF_KICK | MEMF_LOCAL);

        if (memlower < protlower)
        {
//            AllocAbs(protupper-protlower, (void *)protlower);
        }
#endif
    }
    else
    {
        /* Initialize TLSF memory allocator */
        krnCreateTLSFMemHeader("System Memory", 0, mh, (memupper - memlower), MEMF_FAST | MEMF_PUBLIC | MEMF_KICK | MEMF_LOCAL);
        if (memlower < protlower)
        {
            /* Protect the bootstrap area from further use. AllocAbs will do the trick */
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

    /*
     * Make kickstart code area read-only.
     * We do it only after ExecBase creation because SysBase pointer is put
     * into .rodata. This way we prevent it from ocassional modification by buggy software.
     */
//    core_ProtKernelArea(addr, kick_highest - addr, 1, 0, 1);

    D(bug("[Kernel] InitCode(RTF_SINGLETASK) ... \n"));
    InitCode(RTF_SINGLETASK, 0);

    D(bug("[Kernel] Dropping into USER mode ... \n"));
    asm("cps %[mode_user]\n"
#if AROS_BIG_ENDIAN
        "setend be\n"
#endif
     : : [mode_user] "I" (CPUMODE_USER)); /* switch to user mode */

    D(bug("[Kernel] InitCode(RTF_COLDSTART) ...\n"));
    InitCode(RTF_COLDSTART, 0);

    /* The above should not return */
    krnPanic(KernelBase, "System Boot Failed!");
}

DEFINESET(ARMPLATFORMS);
