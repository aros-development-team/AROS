#include <asm/cpu.h>
#include <asm/io.h>
#include <aros/libcall.h>
#include <aros/asmcall.h>
#include <exec/execbase.h>
#include <hardware/intbits.h>
#include <proto/exec.h>

#include <inttypes.h>

#include "kernel_base.h"
#include "kernel_bootmem.h"
#include "kernel_debug.h"
#include "kernel_interrupts.h"
#include "kernel_intern.h"
#include "kernel_intr.h"
#include "kernel_scheduler.h"
#include "kernel_syscall.h"
#include "apic.h"
#include "xtpic.h"

#define D(x)
#define DSYSCALL(x)
#define DTRAP(x) x
#define DUMP_CONTEXT

/*
 * Simulate SysBase access at address 8.
 * Disabled because global SysBase is moved away from zeropage.
 *
#define EMULATE_SYSBASE 8 */

#define IRQ(x,y) \
    IRQ##x##y##_intr

#define IRQPROTO(x, y) \
    void IRQ(x, y)(void)

#define IRQPROTO_16(x) \
    IRQPROTO(x,0); IRQPROTO(x,1); IRQPROTO(x,2); IRQPROTO(x,3); \
    IRQPROTO(x,4); IRQPROTO(x,5); IRQPROTO(x,6); IRQPROTO(x,7); \
    IRQPROTO(x,8); IRQPROTO(x,9); IRQPROTO(x,a); IRQPROTO(x,b); \
    IRQPROTO(x,c); IRQPROTO(x,d); IRQPROTO(x,e); IRQPROTO(x,f)

#define IRQLIST_16(x) \
    IRQ(x,0), IRQ(x,1), IRQ(x,2), IRQ(x,3), \
    IRQ(x,4), IRQ(x,5), IRQ(x,6), IRQ(x,7), \
    IRQ(x,8), IRQ(x,9), IRQ(x,a), IRQ(x,b), \
    IRQ(x,c), IRQ(x,d), IRQ(x,e), IRQ(x,f)

/* This generates prototypes for entry points */
IRQPROTO_16(0x0);
IRQPROTO_16(0x1);
IRQPROTO_16(0x2);
IRQPROTO(0x8, 0);
IRQPROTO(0xf, e);
extern void core_DefaultIRETQ(void);

const void *interrupt[256] =
{
    IRQLIST_16(0x0),
    IRQLIST_16(0x1),
    IRQLIST_16(0x2)
};

#ifdef DUMP_CONTEXT

static void PrintContext(struct ExceptionContext *regs, unsigned long error_code)
{
    int i;
    uint64_t *ptr;
    struct Task *t = SysBase->ThisTask;

    if (t)
    {
        bug("[Kernel]  %s %p '%s'\n", t->tc_Node.ln_Type == NT_TASK?"task":"process", t, t->tc_Node.ln_Name);
        bug("[Kernel] SPLower=%016lx SPUpper=%016lx\n", t->tc_SPLower, t->tc_SPUpper);

        if (((void *)regs->rsp < t->tc_SPLower) || ((void *)regs->rsp > t->tc_SPUpper))
            bug("[Kernel] Stack out of Bounds!\n");
    }

    bug("[Kernel] Error 0x%016lX\n", error_code);
    PRINT_CPUCONTEXT(regs);

    bug("[Kernel] Stack:\n");
    ptr = (uint64_t *)regs->rsp;
    for (i=0; i < 10; i++)
        bug("[Kernel] %02x: %016p\n", i * sizeof(uint64_t), ptr[i]);
}

#else

#define PrintContext(regs, err)

#endif

void core_SetupIDT(struct KernBootPrivate *__KernBootPrivate)
{
    IPTR _APICBase;
    UBYTE _APICID;
    int i;
    uintptr_t off;
    struct segment_selector IDT_sel;

    _APICBase = boot_APIC_GetBase(__KernBootPrivate);
    _APICID   = boot_APIC_GetID(__KernBootPrivate, _APICBase);

    if (_APICID == __KernBootPrivate->kbp_APIC_BSPID)
    {
    	struct int_gate_64bit *IGATES;

    	if (!__KernBootPrivate->IDT)
    	{
    	    __KernBootPrivate->IDT = krnAllocBootMemAligned(sizeof(struct int_gate_64bit) * 256, 256);

    	    D(bug("[Kernel] Allocated IDT at 0x%p\n", __KernBootPrivate->IDT));
    	}

        bug("[Kernel] core_SetupIDT[%d] Setting all interrupt handlers to default value\n", _APICID);

        IGATES = __KernBootPrivate->IDT;
        for (i=0; i < 256; i++)
        {
            if (interrupt[i])
                off = (uintptr_t)interrupt[i];
            else if (i == 0x80)
                off = (uintptr_t)IRQ0x80_intr;
            else if (i == 0xfe)
                off = (uintptr_t)IRQ0xfe_intr;
            else
                off = (uintptr_t)core_DefaultIRETQ;

            IGATES[i].offset_low = off & 0xffff;
            IGATES[i].offset_mid = (off >> 16) & 0xffff;
            IGATES[i].offset_high = (off >> 32) & 0xffffffff;
            IGATES[i].type = 0x0e;
            IGATES[i].dpl = 3;
            IGATES[i].p = 1;
            IGATES[i].selector = KERNEL_CS;
            IGATES[i].ist = 0;
        }
    }

    bug("[Kernel] core_SetupIDT[%d] Registering interrupt handlers ..\n", _APICID);

    IDT_sel.size = sizeof(struct int_gate_64bit) * 256 - 1;
    IDT_sel.base = (unsigned long)__KernBootPrivate->IDT;    
    asm volatile ("lidt %0"::"m"(IDT_sel));
}

/*
 * This table is used to translate x86 trap number
 * to AmigaOS trap number to be passed to exec exception handler.
 */
static const char AmigaTraps[] =
{
     5,  9, -1,  4, 11, 2,
     4,  0,  8, 11,  3, 3,
     2,  8,  3, -1, 11, 3,
    -1
};

/* CPU exceptions are processed here */
void core_IRQHandle(struct ExceptionContext *regs, unsigned long error_code, unsigned long irq_number)
{
#ifdef EMULATE_SYSBASE
    if (irq_number == 0x0e)
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

	/* The exception will now be passed on to handling code below */
    }
#endif

    /* These exceptions are CPU traps */
    if (irq_number < sizeof(AmigaTraps))
    {
        D(bug("[Kernel] Trap exception %u\n", irq_number));

	if (krnRunExceptionHandlers(irq_number, regs))
	    core_LeaveInterrupt(regs);

	DTRAP(bug("[Kernel] Passing on to exec, Amiga trap %d\n", AmigaTraps[irq_number]));

	if (AmigaTraps[irq_number] != -1)
	{
	    if (core_Trap(AmigaTraps[irq_number], regs))
	    {
		/* If the trap handler returned, we can continue */
		DTRAP(bug("[Kernel] Trap handler returned\n"));
		core_LeaveInterrupt(regs);
	    }
	}

	bug("[Kernel] UNHANDLED EXCEPTION %lu\n", irq_number);
	PrintContext(regs, error_code);

	while (1) asm volatile ("hlt");
    }
    else if (irq_number == 0x80)  /* Syscall? */
    {
	/* Syscall number is actually ULONG (we use only eax) */
    	ULONG sc = regs->rax;

        DSYSCALL(bug("[Kernel] Syscall %u\n", sc));

	/* The following syscalls can be run in both supervisor and user mode */
	switch (sc)
	{
	case SC_REBOOT:
	    D(bug("[Kernel] Warm restart...\n"));

	    __asm__ __volatile__("cli; cld;");
	    /*
	     * Restart the kernel from kernel_cstart() function reusing already relocated BootMsg.
	     * Debug output, including split-screen vesahack mode, survives the restart.
	     * This doesn't return.
	     */
	    core_Kick(BootMsg, kernel_cstart);

	case SC_SUPERVISOR:
	    /* This doesn't return */
	    core_Supervisor(regs);
	}

	/* Scheduler can be called only from within user mode */
        if (regs->ds != KERNEL_DS)
        {
            DSYSCALL(bug("[Kernel] User-mode syscall\n"));

	    /* Disable interrupts for a while */
	    __asm__ __volatile__("cli; cld;");

            switch (sc)
            {
            case SC_CAUSE:
	    	core_ExitInterrupt(regs);
            	break;

            case SC_SCHEDULE:
            	DSYSCALL(bug("[Kernel] Schedule...\n"));
            	if (!core_Schedule())
                    break;
            	/* Fallthrough */

            case SC_SWITCH:
            	DSYSCALL(bug("[Kernel] Switch...\n"));
            	cpu_Switch(regs);
            	/* Fallthrough */

            case SC_DISPATCH:
            	DSYSCALL(bug("[Kernel] Dispatch...\n"));
                cpu_Dispatch(regs);
            	break;
            }
        }

	DSYSCALL(bug("[Kernel] Returning from syscall...\n"));
    }
    else if (irq_number >= 0x20) /* Hardware IRQ */
    {
	if (KernelBase)
    	{
	    /* From CPU's point of view, IRQs are exceptions starting from 0x20. */
    	    irq_number -= 0x20;

    	    switch (KernelBase->kb_Interrupts[irq_number].lh_Type)
    	    {
    	    case KBL_APIC:
            	core_APIC_AckIntr(irq_number, KernelBase->kb_PlatformData);
            	krnRunIRQHandlers(irq_number);
            	break;

            case KBL_XTPIC:
            	core_XTPIC_AckIntr(irq_number, KernelBase->kb_PlatformData);
            	krnRunIRQHandlers(irq_number);

            	if (!IsListEmpty(&KernelBase->kb_Interrupts[irq_number]))
                    core_XTPIC_EnableIRQ(irq_number, KernelBase->kb_PlatformData);

                break;

            default:
            	krnRunIRQHandlers(irq_number);
            	break;
	    }
	}

	/* Upon exit from the lowest-level hardware IRQ we run the task scheduler */
	if (SysBase && (regs->ds != KERNEL_DS))
	{
	    /* Disable interrupts for a while */
	    __asm__ __volatile__("cli; cld;");

	    core_ExitInterrupt(regs);
	}
    }

    core_LeaveInterrupt(regs);
}

void ictl_enable_irq(unsigned char irq, struct KernelBase *KernelBase)
{
    if (KernelBase->kb_Interrupts[irq].lh_Type == KBL_XTPIC)
            core_XTPIC_EnableIRQ(irq, KernelBase->kb_PlatformData);
}
