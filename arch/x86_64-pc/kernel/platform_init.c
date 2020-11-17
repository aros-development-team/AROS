/*
    Copyright © 1995-2020, The AROS Development Team. All rights reserved.
    $Id$
*/

#define __KERNEL_NOLIBBASE__

#include <aros/multiboot.h>
#include <aros/symbolsets.h>
#include <asm/cpu.h>
#include <asm/io.h>
#include <exec/lists.h>
#include <proto/exec.h>
#include <proto/kernel.h>

#include <inttypes.h>

#include "kernel_base.h"
#include "kernel_intern.h"
#include "kernel_debug.h"
#include "kernel_intr.h"

#define D(x)
#define DAPIC(x)

/* 
    This file contains code that is run once Exec has been brought up - and is launched
    via the RomTag/Autoinit routines in Exec.
    
    Here we do the platform setup that requires Exec to have minimal configuration
    but still running inthe SINGLETASK environment.
*/

extern struct syscallx86_Handler x86_SCSupervisorHandler;

#if defined(EMULATE_SYSBASE)
/* CPU exceptions are processed here */
void core_IRQ0EHandle(struct ExceptionContext *regs, void *HandlerData, void *HandlerData2)
{
    uint64_t ptr = rdcr(cr2);
    unsigned char *ip = (unsigned char *)regs->rip;

    D(bug("[Kernel] Page fault exception\n"));

    if (ptr == EMULATE_SYSBASE)
    {
        D(bug("[Kernel] ** Code at 0x%p is trying to access the SysBase at 0x%p.\n", ip, ptr));

        if ((ip[0] & 0xfb) == 0x48 &&
             ip[1]         == 0x8b && 
            (ip[2] & 0xc7) == 0x04 &&
             ip[3]         == 0x25)
        {
            int reg = ((ip[2] >> 3) & 0x07) | ((ip[0] & 0x04) << 1);

            switch(reg)
            {
                case 0:
                    regs->rax = (UQUAD)SysBase;
                    break;
                case 1:
                    regs->rcx = (UQUAD)SysBase;
                    break;
                case 2:
                    regs->rdx = (UQUAD)SysBase;
                    break;
                case 3:
                    regs->rbx = (UQUAD)SysBase;
                    break;
//                    case 4:   /* Cannot put SysBase into rSP register */
//                        regs->rsp = (UQUAD)SysBase;
//                        break;
                case 5:
                    regs->rbp = (UQUAD)SysBase;
                    break;
                case 6:
                    regs->rsi = (UQUAD)SysBase;
                    break;
                case 7:
                    regs->rdi = (UQUAD)SysBase;
                    break;
                case 8:
                    regs->r8 = (UQUAD)SysBase;
                    break;
                case 9:
                    regs->r9 = (UQUAD)SysBase;
                    break;
                case 10:
                    regs->r10 = (UQUAD)SysBase;
                    break;
                case 11:
                    regs->r11 = (UQUAD)SysBase;
                    break;
                case 12:
                    regs->r12 = (UQUAD)SysBase;
                    break;
                case 13:
                    regs->r13 = (UQUAD)SysBase;
                    break;
                case 14:
                    regs->r14 = (UQUAD)SysBase;
                    break;
                case 15:
                    regs->r15 = (UQUAD)SysBase;
                    break;
            }

            regs->rip += 8;
            
            core_LeaveInterrupt(regs);
        }
        else if ((ip[0] & 0xfb) == 0x48 &&
                  ip[1]         == 0x8b && 
                 (ip[2] & 0xc7) == 0x05)
        {
            int reg = ((ip[2] >> 3) & 0x07) | ((ip[0] & 0x04) << 1);

            switch(reg)
            {
                case 0:
                    regs->rax = (UQUAD)SysBase;
                    break;
                case 1:
                    regs->rcx = (UQUAD)SysBase;
                    break;
                case 2:
                    regs->rdx = (UQUAD)SysBase;
                    break;
                case 3:
                    regs->rbx = (UQUAD)SysBase;
                    break;
//                    case 4:   /* Cannot put SysBase into rSP register */
//                        regs->rsp = (UQUAD)SysBase;
//                        break;
                case 5:
                    regs->rbp = (UQUAD)SysBase;
                    break;
                case 6:
                    regs->rsi = (UQUAD)SysBase;
                    break;
                case 7:
                    regs->rdi = (UQUAD)SysBase;
                    break;
                case 8:
                    regs->r8 = (UQUAD)SysBase;
                    break;
                case 9:
                    regs->r9 = (UQUAD)SysBase;
                    break;
                case 10:
                    regs->r10 = (UQUAD)SysBase;
                    break;
                case 11:
                    regs->r11 = (UQUAD)SysBase;
                    break;
                case 12:
                    regs->r12 = (UQUAD)SysBase;
                    break;
                case 13:
                    regs->r13 = (UQUAD)SysBase;
                    break;
                case 14:
                    regs->r14 = (UQUAD)SysBase;
                    break;
                case 15:
                    regs->r15 = (UQUAD)SysBase;
                    break;
            }
            
            regs->rip += 7;
            
            core_LeaveInterrupt(regs);
        }
            D(else bug("[Kernel] Instruction not recognized\n"));
    }

#ifdef DUMP_CONTEXT
    unsigned int i;

    bug("[Kernel] PAGE FAULT accessing 0x%p\n", ptr);
    bug("[Kernel] Insn: ");
    for (i = 0; i < 16; i++)
        bug("%02x ", ip[i]);
    bug("\n");
#endif

    return FALSE;
}
#endif

int core_SysCallHandler(struct ExceptionContext *regs, struct KernelBase *KernelBase, void *HandlerData2);

static int Platform_Init(struct KernelBase *LIBBASE)
{
    struct PlatformData *pdata;
    struct KernelBase *KernelBase = LIBBASE;
#if 1
    int i;
#endif

        D(
            bug("[Kernel:x86_64] %s: Performing Post-Exec initialization\n", __func__);
            bug("[Kernel:x86_64] %s: KernelBase @ %p\n", __func__, LIBBASE);)

        /*
     * Setup the Interrupt Controller Environment ...
     */
        NEWLIST(&LIBBASE->kb_ICList);
    NEWLIST(&LIBBASE->kb_InterruptMappings);
    LIBBASE->kb_ICTypeBase = KBL_INTERNAL + 1;

    D(bug("[Kernel:x86_64] %s: Interrupt Controller Base ID = %d\n", __func__, LIBBASE->kb_ICTypeBase));

    for (i = 0; i < HW_IRQ_COUNT; i++)
    {
        LIBBASE->kb_Interrupts[i].ki_Priv &= ~IRQINTF_ENABLED;
        LIBBASE->kb_Interrupts[i].ki_List.lh_Type = KBL_INTERNAL;
    }

    D(bug("[Kernel:x86_64] %s: Interrupt Lists initialised\n", __func__));

    /*
     * Setup our Platform Data ...
     */
    pdata = AllocMem(sizeof(struct PlatformData), MEMF_PUBLIC|MEMF_CLEAR);
    if (!pdata)
    	return FALSE;

    D(bug("[Kernel:x86_64] %s: Platform Data allocated @ 0x%p\n", __func__, pdata));

    LIBBASE->kb_PlatformData = pdata;

    /*
     * Setup the base syscall handler(s) ...
     */
    NEWLIST(&pdata->kb_SysCallHandlers);

    // we need to setup the BSP's syscall gate early..
    if (!core_SetIDTGate((apicidt_t *)__KernBootPrivate->BOOTIDT, APIC_CPU_EXCEPT_TO_VECTOR(APIC_EXCEPT_SYSCALL), (uintptr_t)IntrDefaultGates[APIC_CPU_EXCEPT_TO_VECTOR(APIC_EXCEPT_SYSCALL)], TRUE, FALSE))
    {
        krnPanic(NULL, "Failed to set BSP Syscall Vector\n"
                       "Vector #%02X\n",
                 APIC_CPU_EXCEPT_TO_VECTOR(APIC_EXCEPT_SYSCALL));
    }
    KrnAddExceptionHandler(APIC_EXCEPT_SYSCALL, core_SysCallHandler, LIBBASE, NULL);
    krnAddSysCallHandler(pdata, &x86_SCSupervisorHandler, FALSE, TRUE);

    D(bug("[Kernel:x86_64] %s: SysCall set up\n", __func__));

#if defined(EMULATE_SYSBASE)
//    KrnAddExceptionHandler(0x0E, core_IRQ0EHandle, void *handlerData, void *handlerData2);
#endif

    return TRUE;
}

ADD2INITLIB(Platform_Init, 10)
