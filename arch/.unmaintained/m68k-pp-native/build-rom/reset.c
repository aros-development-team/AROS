#include <asm/registers.h>
#include <gfx.h>

extern void main_init(void * memory, ULONG memSize);

void aros_reset(void)
{
	/*
	 * Turn the LCD controller on
	 */
	WREG_B(PFDATA) = 0x010;

	/*
	 * Initialize the LCD Controller
	 */
	WREG_L(LSSA)  = 0x90000;
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
	 * Allow the interrupt for timer 1 & 2 to go through!
	 * Disable interrupts by manipulating SR.
	 */
	__asm__ __volatile__("oriw #0x0700,%%sr" :: );
	WREG_L(IMR) = ~((1 << 1) | (1 << 5));
	
	/*
	 * Set the Interrupt vector register
	 */
	WREG_B(IVR) = 0x40;

	/*
	 * First parameter is memory start, 2nd is size of memory.
	 */
	main_init((void *)0x400,0x90000-0x400);
}
