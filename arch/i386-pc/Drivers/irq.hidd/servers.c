/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: IRQ servers for standalone i386 AROS
    Lang: english
*/

#include <oop/oop.h>
#include <aros/asmcall.h>
#include <exec/types.h>
#include <exec/lists.h>
#include <exec/interrupts.h>
#include <exec/execbase.h>
#include <exec/memory.h>
#include <hidd/irq.h>
#include <utility/utility.h>
#include <hardware/intbits.h>

#include <asm/ptrace.h>
#include <asm/irq.h>
#include <asm/io.h>

#include <proto/exec.h>
#include <proto/oop.h>

#include "irq.h"

#ifdef SysBase
#undef SysBase
#endif /* SysBase */

void global_server(int cpl, struct irq_staticdata *isd, struct pt_regs *);

struct irqServer timer_int  = { global_server, "timer"	    , NULL};
struct irqServer kbd_int    = { global_server, "keyboard"   , NULL};
struct irqServer com1_int   = { global_server, "serial 1"   , NULL};
struct irqServer com2_int   = { global_server, "serial 2"   , NULL};
struct irqServer floppy_int = { global_server, "floppy"     , NULL};
struct irqServer rtc_int    = { global_server, "rtc"	    , NULL};
struct irqServer mouse_int  = { global_server, "ps/2 mouse" , NULL};
struct irqServer ide0_int   = { global_server, "ide0"	    , NULL};
struct irqServer ide1_int   = { global_server, "ide1"	    , NULL};

void timer_interrupt(HIDDT_IRQ_Handler *irq, HIDDT_IRQ_HwInfo *hw);

/*******************************************************************************
    Two special irq handlers. As we don't need these two interrupts we define
    here dummy handlers.
*******************************************************************************/

void no_action(int cpl, struct irq_staticdata *isd, struct pt_regs *regs)
{
}

static void math_error_irq(int cpl, struct irq_staticdata *isd, struct pt_regs *regs)
{
	outb(0,0xF0);
}

static struct irqServer irq13 = { math_error_irq, "fpu", NULL};

/*
 * IRQ2 is cascade interrupt to second interrupt controller
 */

static struct irqServer irq2  = { no_action, "cascade", NULL};

#define SysBase (isd->sysbase)

void irqSet(int, struct irqServer *);

void init_Servers(struct irq_staticdata *isd)
{
    HIDDT_IRQ_Handler	*timer;

    timer_int.is_UserData = isd;
    irq2.is_UserData = isd;
    kbd_int.is_UserData = isd;
    com1_int.is_UserData = isd;
    com2_int.is_UserData = isd;
    floppy_int.is_UserData = isd;
    rtc_int.is_UserData = isd;
    mouse_int.is_UserData = isd;
    irq13.is_UserData = isd;
    ide0_int.is_UserData = isd;
    ide1_int.is_UserData = isd;

    irqSet(0, &timer_int);
    irqSet(1, &kbd_int);
    irqSet(2, &irq2);
    irqSet(3, &com2_int);
    irqSet(4, &com1_int);
    irqSet(6, &floppy_int);
    irqSet(8, &rtc_int);
    irqSet(12, &mouse_int);
    irqSet(13, &irq13);
    irqSet(14, &ide0_int);
    irqSet(15, &ide1_int);
    
    /* Install timer interrupt */
    timer = AllocMem(sizeof(HIDDT_IRQ_Handler), MEMF_CLEAR|MEMF_PUBLIC);
    if (timer)
    {
	timer->h_Node.ln_Name = "INT_VERTB emulator";
	timer->h_Node.ln_Type = NT_INTERRUPT;
	timer->h_Node.ln_Pri = 0;
	timer->h_Data = &SysBase->IntVects[INTB_VERTB];
	timer->h_Code = timer_interrupt;
	
	Enqueue((struct List *)&isd->irqlist[0], (struct Node *)timer);
    }
}

#undef SysBase

/*******************************************************************************
    This timer interrupt is used to keep compatibility with old Amiga software
*******************************************************************************/

void timer_interrupt(HIDDT_IRQ_Handler *irq, HIDDT_IRQ_HwInfo *hw)
{
    struct IntVector *iv = irq->h_Data;

    if (iv->iv_Code)
    {
	/*  Call it. I call with all these parameters for a reason.

	    In my `Amiga ROM Kernel Reference Manual: Libraries and
	    Devices' (the 1.3 version), interrupt servers are called
	    with the following 5 parameters.

	    D1 - Mask of INTENAR and INTREQR
	    A0 - 0xDFF000 (base of custom chips)
	    A1 - Interrupt Data
	    A5 - Interrupt Code vector
	    A6 - SysBase

	    It is quite possible that some code uses all of these, so
	    I must supply them here. Obviously I will dummy some of these
	    though.
	*/
	AROS_UFC5(void, iv->iv_Code,
	    AROS_UFCA(ULONG, 0, D1),
	    AROS_UFCA(ULONG, 0, A0),
	    AROS_UFCA(APTR, iv->iv_Data, A1),
	    AROS_UFCA(APTR, iv->iv_Code, A5),
	    AROS_UFCA(struct ExecBase *, hw->sysBase, A6)
	);
    }

    if (--SysBase->Elapsed == 0)
    {
	SysBase->SysFlags |= 0x2000;
	SysBase->AttnResched |= 0x80;
    }
}

/*******************************************************************************
    Global Interrupt Handler
    
    This pice of code translates real irq number to AROS specific irq id. This
    allows us to map system-dependent irqs (audio, ethernet and so on) into well
    defined ids.
*******************************************************************************/

#undef SysBase
#define SysBase (isd->sysbase)

void global_server(int cpl, struct irq_staticdata *isd, struct pt_regs *regs)
{
    HIDDT_IRQ_Id	id;
    HIDDT_IRQ_HwInfo	hwinfo;
    HIDDT_IRQ_Handler	*handler;
    
#if 0
    switch (cpl)
    {
	case 0:	/* Timer */
	    id = vHidd_IRQ_Timer;
	    break;
	case 1: /* Keyboard */
	    id = vHidd_IRQ_Keyboard;
	    break;
	case 3: /* COM2 */
	    id = vHidd_IRQ_Serial2;
	    break;
	case 4: /* COM1 */
	    id = vHidd_IRQ_Serial1;
	    break;
	case 6: /* Floppy */
	    id = vHidd_IRQ_Floppy;
	    break;
	case 8: /* RTC */
	    id = vHidd_IRQ_RTC;
	    break;
	case 12: /* PS/2 mouse */
	    id = vHidd_IRQ_Mouse;
	    break;
	case 14: /* HDD1 */
	    id = vHidd_IRQ_HDD1;
	    break;
	case 15: /* HDD2 */
	    id = vHidd_IRQ_HDD2;
	    break;
	default:
#warning TODO: Write mapping for Global Interrupt Server
	    /*
		We should implement some kind of translation table built
		while autodetecting devices
	    */
	    return;
    }
#endif
    
    hwinfo.Error = 0;	/* No errorcode */
    hwinfo.sysBase = isd->sysbase;
    
    /* Execute all installed handlers */
    ForeachNode(&isd->irqlist[cpl], handler)
    {
	handler->h_Code(handler, &hwinfo);
    }

    /* Leave the interrupt. */
}

