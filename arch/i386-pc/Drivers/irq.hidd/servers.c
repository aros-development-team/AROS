/*
    (C) 1999 AROS - The Amiga Research OS
    $Id$

    Desc: IRQ servers for standalone i386 AROS
    Lang: english
*/

#define AROS_ALMOST_COMPATIBLE
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

#include <proto/exec.h>
#include <proto/oop.h>

#include "irq.h"

#ifdef SysBase
#undef SysBase
#endif /* SysBase */

void global_server(int cpl, void *dev_id, struct pt_regs *regs, struct irq_staticdata *isd);

struct irqaction timer_int = { global_server, "timer", NULL, NULL};
struct irqaction kbd_int = { global_server, "keyboard", NULL, NULL};
struct irqaction com1_int = { global_server, "serial 1", NULL, NULL};
struct irqaction com2_int = { global_server, "serial 2", NULL, NULL};
struct irqaction floppy_int = { global_server, "floppy", NULL, NULL};
struct irqaction rtc_int = { global_server, "rtc", NULL, NULL};
struct irqaction ide0_int = { global_server, "ide0", NULL, NULL};
struct irqaction ide1_int = { global_server, "ide1", NULL, NULL};

void timer_interrupt(HIDDT_IRQ_Handler *irq, HIDDT_IRQ_HwInfo *hw);

#define SysBase (isd->sysbase)

void init_Servers(struct irq_staticdata *isd)
{
    HIDDT_IRQ_Handler	*timer;

    setup_x86_irq(0, &timer_int);
    setup_x86_irq(1, &kbd_int);
    setup_x86_irq(3, &com2_int);
    setup_x86_irq(4, &com1_int);
    setup_x86_irq(6, &floppy_int);
    setup_x86_irq(8, &rtc_int);
    setup_x86_irq(14, &ide0_int);
    setup_x86_irq(15, &ide1_int);
    
    /* Install timer interrupt */
    timer = AllocMem(sizeof(HIDDT_IRQ_Handler), MEMF_CLEAR|MEMF_PUBLIC);
    if (timer)
    {
	timer->h_Node.ln_Name = "INT_VERTB emulator";
	timer->h_Node.ln_Type = NT_INTERRUPT;
	timer->h_Node.ln_Pri = 0;
	timer->h_Data = &SysBase->IntVects[INTB_VERTB];
	timer->h_Code = timer_interrupt;
	
	Enqueue((struct List *)&isd->irqlist[vHidd_IRQ_Timer], (struct Node *)timer);
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
}

/*******************************************************************************
    Global Interrupt Handler
    
    This pice of code translates real irq number to AROS specific irq id. This
    allows us to map system-dependent irqs (audio, ethernet and so on) into well
    defined ids.
*******************************************************************************/

void global_server(int cpl, void *dev_id, struct pt_regs *regs, struct irq_staticdata *isd)
{
    HIDDT_IRQ_Id	id;
    HIDDT_IRQ_HwInfo	hwinfo;
    HIDDT_IRQ_Handler	*handler;
    
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
    
    hwinfo.Error = 0;	/* No errorcode */
    hwinfo.sysBase = isd->sysbase;
    
    /* Execute all installed handlers */
    ForeachNode(&isd->irqlist[id], handler)
    {
	handler->h_Code(handler, &hwinfo);
    }
}
