#include <registers.h>

extern void main_init(void * memory, ULONG memSize);
extern void forever(void);

extern void _sys_dispatch(void);
extern void _sys_trap1_handler(void);

void _Init_IRQVectors(void);


#include "../gfx.h"

void aros_reset(void)
{
	/*
	 * Turn the LCD controller on
	 */
	WREG_B(PFDATA) = 0x010;

	/*
	 * Initialize the LCD Controller
	 */
	WREG_L(LSSA)  = 0x60000;
	WREG_B(LVPW)  = 160/16;
	WREG_W(LXMAX) = 160-1;
	WREG_W(LYMAX) = 160-1;
	WREG_W(LCXP)  = 80;
	WREG_W(LCYP)  = 80;
	WREG_W(LCWCH) = (10 << 8) | 10;
	WREG_B(LBLKC) = 0x80 | 0x10;
	WREG_B(LPICF) = 0x4;
	clearscreen(0);

#if 0 
	// DO NOT ACTIVATE THESE! IT STOPS OUTPUT ON XCOPILOT!!
	WREG_B(LPOLCF)= 0x0;
	WREG_B(LACDRC)= 0x0;
	WREG_B(LPXCD) = 0x0;
	WREG_B(LCKCON)= 0x0;
	WREG_W(LRRA)  = 0x0;
#endif
	WREG_B(LPOSR) = 0x0;
	WREG_B(LOTCR) = 0x4e; // 0xfffffa2b

	/*
	 * Enable timer 2
	 */
	WREG_W(TCTL2)  = 0x11;  // enable timer + interrupt request on compare
	WREG_W(TCMP2)  = 0x0f00;

	/*
	 * Allow the interrupt for timer 1 & 2 to go through!
	 * Disable interrupts by manipulating SR.
	 */
	__asm__ __volatile__("oriw #0x0700,%%sr" :: );
	WREG_L(IMR) = ~((1 << 1) | (1 << 5));
	
	/*
	 * Set the Interrupt vector register
	 */
	WREG_B(IVR) = 0x40;

	_Init_IRQVectors();

	/*
	 * First parameter is memory start, 2nd is size of memory.
	 */
	main_init((void *)0x400,0x60000-0x400);
}

void _Init_IRQVectors(void)
{
	WREG_L(IRQ_LEVEL6) = (ULONG)_sys_dispatch;
	WREG_L(IRQ_LEVEL5) = (ULONG)_sys_dispatch;
	WREG_L(IRQ_LEVEL4) = (ULONG)_sys_dispatch;
	WREG_L(IRQ_LEVEL3) = (ULONG)_sys_dispatch;
	WREG_L(IRQ_LEVEL2) = (ULONG)_sys_dispatch;
	WREG_L(IRQ_LEVEL1) = (ULONG)_sys_dispatch;

	/*
	 * Initialize the trap #1 handler. It will 
	 * handle all traps use in AROS.
	 * Cannot use the other traps since POSE
	 * intercepts them.
	 */
	WREG_L(TRAP_1) = (ULONG)_sys_trap1_handler;

	/*
	 * Set the IRQ 4 (etc.) handler
	 */
	WREG_L(0x100) = (ULONG)_sys_dispatch;
	WREG_L(0x104) = (ULONG)_sys_dispatch;
	WREG_L(0x108) = (ULONG)_sys_dispatch;
	WREG_L(0x10c) = (ULONG)_sys_dispatch;
	WREG_L(0x110) = (ULONG)_sys_dispatch; //; this seems to be IRQ 4 according to the emulator
	WREG_L(0x114) = (ULONG)_sys_dispatch;
	WREG_L(0x118) = (ULONG)_sys_dispatch;
}
