/*
    Copyright © 2008-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 1

#include <aros/debug.h>
#include <inttypes.h>
#include <aros/kernel.h>
#include <aros/libcall.h>
#include <asm/mpc5200b.h>
#include <asm/io.h>
#include <stddef.h>
#include <string.h>

#include <proto/exec.h>
#include <proto/kernel.h>

#include "kernel_intern.h"
#include "syscall.h"


static volatile mpc5200b_ictl_t *ictl;

void ictl_init(void *MBAR)
{
	uint32_t tmp;
	D(bug("[KRN] Entering ictl_init.\n"));
	ictl = (mpc5200b_ictl_t *)((intptr_t)MBAR + 0x500);

	D(bug("[KRN] Stopping all interrupt activities\n"));

	/* Disable all peripheral interrupts */
	outl(0xffffff00, &ictl->ictl_pim);

	/* Disable all master interrupts */
	outl(0x00010fff, &ictl->ictl_cpmim);

	/* Critical interrupts should generate EE. Keep the old IRQ[0..3] config! */
	tmp = inl(&ictl->ictl_ee);
	tmp &= 0x00ff0000;	/* Leave the IRQ configuration intact */
	tmp |= 0x0f000000;	/* Clear any pending IRQ's */
	outl(ICTL_EE_MEE | ICTL_EE_CEB,&ictl->ictl_ee);

	/* Set all Main priorities to 0 */
	outl(0, &ictl->ictl_mip[0]);
	outl(0, &ictl->ictl_mip[1]);

	/* Set all Peripheral priorities to 0 */
	outl(0, &ictl->ictl_ppri[0]);
	outl(0, &ictl->ictl_ppri[1]);
	outl(0, &ictl->ictl_ppri[2]);
}

void ictl_enable_irq(uint8_t irqnum)
{
	if (irqnum <= MPC5200B_WAKEUP)
		D(bug("[KRN] Enabling critical irq %d? Cannot. It's already enabled.\n", irqnum));
	else if (irqnum <= MPC5200B_TMR7)
	{
		D(bug("[KRN] Enabling main irq %d.\n", irqnum));

		outl(inl(&ictl->ictl_cpmim) & ~(0x00010000 >> (irqnum - MPC5200B_ST1)), &ictl->ictl_cpmim);
	}
	else if (irqnum <= MPC5200B_BESTCOMMLP)
	{
		D(bug("[KRN] Enabling peripheral irq %d.\n", irqnum));

		outl(inl(&ictl->ictl_pim) & ~(0x80000000 >> (irqnum - MPC5200B_BESTCOMM)), &ictl->ictl_pim);
	}
	else
		D(bug("[KRN] Uhh?! Someone tried to enable non-existing irq %d\n", irqnum));

	D(bug("[KRN] CPMIM=%08x PIM=%08x\n", inl(&ictl->ictl_cpmim), inl(&ictl->ictl_pim)));
	D(bug("[KRN] PMCE=%08x  PIS=%08x\n", inl(&ictl->ictl_pmce), inl(&ictl->ictl_pis)));
}

void ictl_disable_irq(uint8_t irqnum)
{
	if (irqnum <= MPC5200B_WAKEUP)
		D(bug("[KRN] Disabling critical irq %d? Cannot.\n", irqnum));
	else if (irqnum <= MPC5200B_TMR7)
	{
		D(bug("[KRN] Disabling main irq %d.\n", irqnum));

		outl(inl(&ictl->ictl_cpmim) | (0x00010000 > irqnum - MPC5200B_ST1), &ictl->ictl_cpmim);
	}
	else if (irqnum <= MPC5200B_BESTCOMMLP)
	{
		D(bug("[KRN] Disabling peripheral irq %d.\n", irqnum));

		outl(inl(&ictl->ictl_pim) | (0x80000000 > irqnum - MPC5200B_BESTCOMM), &ictl->ictl_pim);
	}
	else
		D(bug("[KRN] Uhh?! Someone tried to disable non-existing irq %d\n", irqnum));
}


void __attribute__((noreturn)) ictl_handler(regs_t *ctx, uint8_t exception, void *self)
{
	//D(bug("[KRN] ictl_handler\n"));
	struct KernelBase *KernelBase = getKernelBase();

	int irqnum = 0;
	int i;
	uint32_t bit;
	uint32_t irqstate;

	/* KernelBase set? */
	if (KernelBase)
	{
		/* First, check the critical interrupts */
		irqstate = inl(&ictl->ictl_cis);
		for (i=0, bit=0x08000000; i<4; i++, bit>>=1, irqnum++)
		{
			/* Interrupt occurred? */
			if (irqstate & bit)
			{
				/* Any handlers available? */
				if (!IsListEmpty(&KernelBase->kb_Interrupts[irqnum]))
				{
					struct IntrNode *in, *in2;

					/* Call all handlers */
					ForeachNodeSafe(&KernelBase->kb_Interrupts[irqnum], in, in2)
					{
						if (in->in_Handler)
							in->in_Handler(in->in_HandlerData, in->in_HandlerData2);
					}
				}
			}
		}

		/* Now the main interrupts */
		irqstate = inl(&ictl->ictl_mis);
		for (i=0, bit=0x00010000; i < 17; i++, bit>>=1, irqnum++)
		{
			if (irqstate & bit)
			{
				/* Any handlers available? */
				if (!IsListEmpty(&KernelBase->kb_Interrupts[irqnum]))
				{
					struct IntrNode *in, *in2;

					/* Call all handlers */
					ForeachNodeSafe(&KernelBase->kb_Interrupts[irqnum], in, in2)
					{
						if (in->in_Handler)
							in->in_Handler(in->in_HandlerData, in->in_HandlerData2);
					}
				}
			}
		}

		/* Finally the peripheral interrupts */
		irqstate = inl(&ictl->ictl_pis);
		for (i=0, bit=0x00200000; i < 22; i++, bit>>=1, irqnum++)
		{
			if (irqstate & bit)
			{
				/* Any handlers available? */
				if (!IsListEmpty(&KernelBase->kb_Interrupts[irqnum]))
				{
					struct IntrNode *in, *in2;

					/* Call all handlers */
					ForeachNodeSafe(&KernelBase->kb_Interrupts[irqnum], in, in2)
					{
						if (in->in_Handler)
							in->in_Handler(in->in_HandlerData, in->in_HandlerData2);
					}
				}
				else
				{
//					D(bug("[KRN] Orphan peripheral interrupt %d! Disabling\n", i));
					outl(inl(&ictl->ictl_pim) | __BV32(i), &ictl->ictl_pim);
				}
			}
		}

		if (irqstate & __BV32(9))
		{
			/* Any handlers available? */
			if (!IsListEmpty(&KernelBase->kb_Interrupts[irqnum]))
			{
				struct IntrNode *in, *in2;

				/* Call all handlers */
				ForeachNodeSafe(&KernelBase->kb_Interrupts[irqnum], in, in2)
				{
					if (in->in_Handler)
						in->in_Handler(in->in_HandlerData, in->in_HandlerData2);
				}
			}
			else
			{
				D(bug("[KRN] Orphan peripheral interrupt %d! Disabling\n", 22));
				outl(inl(&ictl->ictl_pim) | __BV32(22), &ictl->ictl_pim);
			}
		}
		irqnum++;

		if (irqstate & __BV32(8))
		{
			/* Any handlers available? */
			if (!IsListEmpty(&KernelBase->kb_Interrupts[irqnum]))
			{
				struct IntrNode *in, *in2;

				/* Call all handlers */
				ForeachNodeSafe(&KernelBase->kb_Interrupts[irqnum], in, in2)
				{
					if (in->in_Handler)
						in->in_Handler(in->in_HandlerData, in->in_HandlerData2);
				}
			}
		}
		irqnum++;
	}

	outl(inl(&ictl->ictl_pmce), &ictl->ictl_pmce);
	core_ExitInterrupt(ctx);
}
