/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>
#include <asm/registers.h>

extern void _sys_dispatch(void);
/*
 * The functions that will move into AROS
 */
void Init_Hardware(void)
{
	/*
	 * Disable all interrupts.
	 */
	WREG_L(IMR) = 0xffffffff;

	/*
	 * Program real time clock, and make it work
	 */
	WREG_W(RTCCTL)  = (UWORD)0x0000; /* disable RTC */
	WREG_W(RTCISR)  = (UWORD)0xffff; /* clear interrupt status register */
	WREG_W(RTCIENR) = (UWORD)0x8010; /* 1Hz & 512Hz interrupt */
	WREG_W(RTCCTL)  = (UWORD)0x00a0; /* enable RTC */
	
	/*
	 * Reset the ISR (if this has any effect?)
	 */
	WREG_L(ISR) = (ULONG)0;
	
	/*
	 * Allow access to on chip registers from user mode
	 * This is absolutley necessary!
	 */
	WREG_B(SCR) = (UBYTE)0x0;
}

void Init_IRQVectors(void)
{
	/*
	 * For the Real Time clock! The task switcher.
	 */
/*
	WREG_L(IRQ_LEVEL6) = (ULONG)_sys_dispatch;
	WREG_L(IRQ_LEVEL5) = (ULONG)_sys_dispatch;
*/
	WREG_L(IRQ_LEVEL4) = (ULONG)_sys_dispatch;
/*
	WREG_L(IRQ_LEVEL3) = (ULONG)_sys_dispatch;
	WREG_L(IRQ_LEVEL2) = (ULONG)_sys_dispatch;
	WREG_L(IRQ_LEVEL1) = (ULONG)_sys_dispatch;
*/
	/*
	 * The task switcher also for the trap 1
	 * This is used when Switch() is executed!
	 */
	WREG_L(TRAP_0) = (ULONG)_sys_dispatch;
}
