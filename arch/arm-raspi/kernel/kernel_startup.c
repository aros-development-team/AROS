/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 0

#include <aros/kernel.h>
#include <aros/symbolsets.h>
#include <exec/memory.h>
#include <exec/tasks.h>
#include <exec/alerts.h>
#include <exec/execbase.h>
#include <asm/io.h>
#include <proto/kernel.h>
#include <proto/exec.h>
#include <strings.h>

#include "exec_intern.h"
#include "etask.h"

#include "kernel_intern.h"
#include "kernel_debug.h"
#include "kernel_fb.h"
#include "kernel_romtags.h"

extern void krnCreateMemHeader(CONST_STRPTR name, BYTE pri, APTR start, IPTR size, ULONG flags);

static void __attribute__((used)) kernel_cstart(struct TagItem *msg);

uint32_t stack[STACK_SIZE] __attribute__((used,aligned(16)));
static uint32_t stack_super[STACK_SIZE] __attribute__((used,aligned(16)));
static uint32_t stack_abort[STACK_SIZE] __attribute__((used,aligned(16)));
static uint32_t stack_irq[STACK_SIZE] __attribute__((used,aligned(16)));

asm(".section .aros.init,\"ax\"\n\t"
    ".globl start\n\t"
    ".type start,%function\n"
    "start:\n"
    "           push {r0}                    \n"
    "           bl      __clear_bss          \n" 
    "           pop {r0}                     \n"
    "           cps     #0x1f                \n" /* system mode */
    "           ldr     sp, stack_end        \n"
    "           cps     #0x17                \n" /* abort mode */
    "           ldr     sp, stack_abort_end  \n"
    "           cps     #0x12                \n" /* IRQ mode */
    "           ldr     sp, stack_irq_end    \n"
    "           cps     #0x13                \n" /* SVC (supervisor) mode */
    "           ldr     sp, stack_super_end  \n"
    "		b       kernel_cstart	     \n"

    ".string \"Native/CORE v3 (" __DATE__ ")\"" "\n\t\n\t"
);

static const uint32_t *stack_end __attribute__((used, section(".aros.init"))) = &stack[STACK_SIZE-4];
static const uint32_t *stack_super_end __attribute__((used, section(".aros.init"))) = &stack_super[STACK_SIZE-4];
static const uint32_t *stack_abort_end __attribute__((used, section(".aros.init"))) = &stack_abort[STACK_SIZE-4];
static const uint32_t *stack_irq_end __attribute__((used, section(".aros.init"))) = &stack_irq[STACK_SIZE-4];

__attribute__((section(".data"))) struct ExecBase *SysBase = NULL;

extern struct TagItem *BootMsg;

static void __attribute__((used)) __clear_bss(struct TagItem *msg)
{
    struct KernelBSS *bss = (struct KernelBSS *)krnGetTagData(KRN_KernelBss, 0, msg);
    register unsigned int dest;
    unsigned int length;

    if (bss)
    {
        while (bss->addr && bss->len)
        {
            dest = bss->addr;
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

static void __attribute__((used)) kernel_cstart(struct TagItem *msg)
{
    UWORD *ranges[3];
    struct MinList memList;
    struct MemHeader *mh;
    struct MemChunk *mc;
    long unsigned int memlower = 0, memupper = 0, protlower = 0, protupper = 0;
    unsigned int delay;
    BootMsg = msg;

    /* NB: the bootstrap has conveniently setup the framebuffer
            and initialised the serial port and led for us */

    core_SetupMMU();

    *gpioGPCLR0 = 1<<16; // LED ON
    delay = 10000;
    while(delay--)
        asm ("mov r0, r0");
    *gpioGPSET0 = 1<<16; // LED OFF
    delay = 10000;
    while(delay--)
        asm ("mov r0, r0");
    *gpioGPCLR0 = 1<<16; // LED ON

    while(msg->ti_Tag != TAG_DONE)
    {
        switch (msg->ti_Tag)
        {
        case KRN_FuncPutC:
            _KrnPutC = msg->ti_Data;
            _KrnPutC(0xFF); // Clear the display
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
            protupper = msg->ti_Data;
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

    D(bug("[KRN] AROS Raspberry Pi Kernel built on %s\n", __DATE__));

    D(bug("[KRN] Entered kernel_cstart @ 0x%p, BootMsg @ %p\n", kernel_cstart, BootMsg));

    D(
        if (_KrnPutC)
        {
            bug("[KRN] Using boostrap PutC implementation @ %p\n", _KrnPutC);
        }
    )

    core_SetupIntr();

    *gpioGPSET0 = 1<<16; // LED OFF
    delay = 1500;
    while(delay--)
        asm ("mov r0, r0");
    *gpioGPCLR0 = 1<<16; // LED ON

    NEWLIST(&memList);

    mh = (struct MemHeader *)memlower;
    krnCreateMemHeader("System Memory", 0, mh, protlower - (long unsigned int)mh, MEMF_FAST | MEMF_PUBLIC | MEMF_KICK | MEMF_LOCAL);

    mc = (struct MemChunk *)((protupper + MEMCHUNK_TOTAL-1) & ~(MEMCHUNK_TOTAL-1));
    mc->mc_Bytes = memupper - (long unsigned int)mc;
    mc->mc_Next = NULL;

    if (mh->mh_First->mc_Next == NULL)
    {
        mh->mh_First->mc_Next = mc;
        mh->mh_Upper = memupper;
        mh->mh_Free += mc->mc_Bytes;
    }

    ranges[0] = (UWORD *)krnGetTagData(KRN_KernelLowest, 0, msg);
    ranges[1] = (UWORD *)krnGetTagData(KRN_KernelHighest, 0, msg);
    ranges[2] = (UWORD *)-1;

    D(bug("[KRN] Preparing ExecBase (memheader @ 0x%p)\n", mh));
    krnPrepareExecBase(ranges, mh, BootMsg);

    /* 
     * Make kickstart code area read-only.
     * We do it only after ExecBase creation because SysBase pointer is put
     * into .rodata. This way we prevent it from ocassional modification by buggy software.
     */
//    core_ProtKernelArea(addr, kick_highest - addr, 1, 0, 1);

    D(bug("[KRN] InitCode(RTF_SINGLETASK) ... \n"));
    InitCode(RTF_SINGLETASK, 0);

    D(bug("[KRN] InitCode(RTF_COLDSTART) ...\n"));
    asm("cps #0x1f\n");	/* switch to system mode */
    InitCode(RTF_COLDSTART, 0);

    /* The above should not return */
    krnPanic(KernelBase, "System Boot Failed!");
}
