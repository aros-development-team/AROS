#include <registers.h>

extern void main_init(void * memory, ULONG memSize);
extern void forever(void);
extern void _sys_dispatch(void);

void _Init_IRQVectors(void);


#include "../palmgfx.h"

void aros_on_palm_reset(void)
{
	/*
	 * Initialize the LCD Controller
	 */
	WREG_L(LSSA)  = 0xc000;
	WREG_B(LVPW)  = 160/16;
	WREG_W(LXMAX) = 160-1;
	WREG_W(LYMAX) = 160-1;
	WREG_W(LCXP)  = 80;
	WREG_W(LCYP)  = 80;
	WREG_W(LCWCH) = (10 << 8) | 10;
	WREG_B(LBLKC) = 0x80 | 0x10;
	WREG_B(LPICF) = 0x4;

#if 0
	// DO NOT ACTIVATE THESE! IT STOPS OUTPUT ON XCOPILOT!!
	WREG_B(LPOLCF)= 0x0;
	WREG_B(LACDRC)= 0x0;
	WREG_B(LPXCD) = 0x0;
	WREG_B(LCKCON)= 0x58;
	WREG_W(LRRA)  = 0x9;
#endif
	WREG_B(LPOSR) = 0x0;
	WREG_B(LOTCR) = 0x4e; // 0xfffffa2b

#if 0
	/*
	 * Now the Real time clock
	 * Disable all interrupts.
	 */
	WREG_L(IMR) = 0xffffffff;

	/*
	 * Program real time clock, and make it work
	 */
	WREG_W(RTCCTL)  = (UWORD)0x0000; /* disable RTC */
	WREG_W(RTCISR)  = (UWORD)0xffff; /* clear interrupt status register */
	WREG_W(RTCIENR) = (UWORD)0x0018; /* 1Hz & 512Hz interrupt */
	WREG_W(RTCISR)  = (UWORD)0x0018; /* clear interrupt status register */
	WREG_W(RTCCTL)  = (UWORD)0x0080; /* enable RTC */
	
	/*
	 * Reset the ISR (if this has any effect?)
	 */
	WREG_L(ISR) = (ULONG)0;
	
	/*
	 * Allow access to on chip registers from user mode
	 * This is absolutely necessary!
	 */
	WREG_B(SCR) = (UBYTE)0x0;


	/*
	 * Now the intrerrupt controller
	 */
	WREG_W(ICR) = 0x0;
	WREG_W(IMR) = 0xffef;
	WREG_W(IWR) = 0x94;
	WREG_B(IVR) = 0x18;

	_Init_IRQVectors();
#endif
forever();
	main_init((void *)0x2000,0xc000-0x2000);
}

void _Init_IRQVectors(void)
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
