/*
    copyright © 1995-2010, the aros development team. all rights reserved.
    $id$

    desc: m68k-amiga IRQ handling
    lang: english
 */

#include <aros/kernel.h>
#include <aros/asmcall.h>

#include <exec/resident.h>
#include <exec/execbase.h>
#include <defines/kernel.h>

#include <hardware/intbits.h>

#include "kernel_base.h"
#include "kernel_intr.h"
#include "kernel_syscall.h"
#include "kernel_scheduler.h"

#include "m68k_exception.h"
#include "amiga_irq.h"

#include "exec_intern.h"

/** Interrupts */
#define INTENAR			0x1c
#define INTREQR			0x1e
#define INTENA			0x9a
#define INTREQ			0x9c

/** DMA **/
#define DMACON			0x96

/** Display Control **/
#define DIWSTRT	0x8e
#define DIWSTOP	0x90

#define DIWSTRT_NTSC	0x2c81
#define DIWSTOP_NTSC	0xf4c1

#define DIWSTRT_PAL	0x2c81
#define DIWSTOP_PAL	0x2cc1

#define DDFSTRT	0x92
#define DDFSTOP	0x94

#define DDFSTRT_LOW	0x0038
#define DDFSTOP_LOW	0x00d0

#define DDFSTRT_HIGH	0x003c
#define DDFSTOP_HIGH	0x00d4

static inline void custom_w(ULONG reg, UWORD val)
{
	volatile UWORD *r = (void *)(0xdff000 + reg);

	*r = val;
}

static inline UWORD custom_r(ULONG reg)
{
	volatile UWORD *r = (void *)(0xdff000 + reg);

	return *r;
}


/* Here's how it's all laid out on the Amiga
 *    M68K Exception
 *      0	Reset: Initial SP
 *      1	Reset: Initial PC (NOTE: Really is SysBase!)
 *      2	Bus Error
 *      3	Address Error
 *      4	Illegal Instruction
 *      5	Divide by Zero
 *      6	CHK Instruction
 *      7	TRAPV Instruction
 *      8	Privileged Instruction
 *      9	Trace
 *      10	Line 1010 Emulator
 *      11	Line 1111 Emulator
 *      12	-
 *      13	-
 *      14	Format Error
 *      15	Uninitilaized Interrupt Vector
 *      16	-
 *      ..
 *      23	-
 *      24	Spurious Interrupt
 *      25	Level 1 Interrupt
 *      		Paula 0: Serial TX
 *      		Paula 1: Disk DMA done
 *      		Paula 2: Software Int
 *      26	Level 2 Interrupt
 *      		Paula 3: CIAA & IRQ2
 *      27	Level 3 Interrupt
 *      		Paula 4: Copper
 *      		Paula 5: Vert Blank
 *      		Paula 6: Blitter
 *      28	Level 4 Interrupt
 *      		Paula 7: Audio 0
 *      		Paula 8: Audio 1
 *      		Paula 9: Audio 2
 *      		Paula 10: Audio 3
 *      29	Level 5 Interrupt
 *      		Paula 11: Serial RX
 *      		Paula 12: Disk Sync
 *      30	Level 6 Interrupt
 *      		Paula 13: External
 *      		Paula 14: CIAB & IRQ6
 *      31	Level 7 Interrupt
 *      		Paula 15: NMI
 *      32	TRAP #0
 *      ..
 *      47	TRAP #15
 *      48	-
 *      ..
 *      63	-
 *      64	User 1
 *      ..
 *      255	User 191
 */

#define PAULA_IRQ_CHECK(valid_mask) \
    const UWORD irq_mask = valid_mask; \
    UWORD intenar = custom_r(INTENAR); \
    if (!(intenar & INTF_INTEN)) \
    	return TRUE; \
    UWORD mask = intenar & custom_r(INTREQR) & (irq_mask); \
    do {

#define PAULA_IRQ_ACK(clear_mask) \
    custom_w(INTREQ, mask & (clear_mask));

#define PAULA_IRQ_HANDLE(irq) \
    	if ((mask) & (1 << (irq))) { \
    	    core_Cause(irq, mask); \
    	}

#define PAULA_IRQ_EXIT()	\
	/* mask = custom_r(INTENAR) & custom_r(INTREQR) & (irq_mask); */ \
    } while (0); \
    /* Call Exec/ExitIntr */	\
    return TRUE;

#define DECLARE_TrapCode(handler) \
    BOOL handler(VOID); \
    VOID handler##_TrapCode(ULONG Id); \
    asm ( \
            "   .global " #handler "_TrapCode\n" \
            "   .func " #handler "_TrapCode\n" \
            #handler "_TrapCode:\n" \
            "   addq.l  #4,%sp\n"       /* Drop the ID */   \
            "   movem.l %d0/%d1/%a0/%a1/%a5/%a6,%sp@-\n"    \
            "   jsr     " #handler "\n" \
            "   tst.w   %d0\n"          \
            "   beq     0f\n"           \
            "   jmp     Exec_6_ExitIntr\n" \
            "0:\n" \
            "   movem.l %sp@+,%d0/%d1/%a0/%a1/%a5/%a6\n" \
            "   rte\n" \
            "   .endfunc\n" \
        ); \

/* AOS interrupt handlers will clear INTREQ before executing interrupt code,
 * servers will clear INTREQ after whole server chain has been executed.
 */

DECLARE_TrapCode(Amiga_Level_1);
DECLARE_TrapCode(Amiga_Level_2);
DECLARE_TrapCode(Amiga_Level_3);
DECLARE_TrapCode(Amiga_Level_4);
DECLARE_TrapCode(Amiga_Level_5);
DECLARE_TrapCode(Amiga_Level_6);
DECLARE_TrapCode(Amiga_Level_7);

BOOL Amiga_Level_1(VOID)
{
    /* Paula IRQs 0 - Serial port TX done
     *            1 - Disk DMA finished
     *            2 - SoftInt
     */

    PAULA_IRQ_CHECK(INTF_SOFTINT | INTF_DSKBLK | INTF_TBE);

    /* SOFTINT is cleared by SOFTINT handler, we can't clear it
     * here anymore because SOFTINT handler may call Cause() internally
     */
    PAULA_IRQ_ACK(INTF_DSKBLK | INTF_TBE);

    PAULA_IRQ_HANDLE(INTB_TBE);
    PAULA_IRQ_HANDLE(INTB_DSKBLK);
    PAULA_IRQ_HANDLE(INTB_SOFTINT);

    PAULA_IRQ_EXIT();
}

BOOL Amiga_Level_2(VOID)
{
    /* Paula IRQs 3 - CIA-A
     */
    PAULA_IRQ_CHECK(INTF_PORTS);

    PAULA_IRQ_HANDLE(INTB_PORTS);

    PAULA_IRQ_ACK(INTF_PORTS);

    PAULA_IRQ_EXIT();
}

BOOL Amiga_Level_3(VOID)
{
    /* Paula IRQs 4 - Copper
     *            5 - Vert Blank
     *            6 - Blitter
     */
    PAULA_IRQ_CHECK(INTF_COPER | INTF_VERTB | INTF_BLIT);

    PAULA_IRQ_HANDLE(INTB_COPER);
    PAULA_IRQ_HANDLE(INTB_VERTB);
    
    PAULA_IRQ_ACK(INTF_COPER | INTF_VERTB | INTF_BLIT);

    PAULA_IRQ_HANDLE(INTB_BLIT);

    PAULA_IRQ_EXIT();
}

BOOL Amiga_Level_4(VOID)
{
    /* Paula IRQs  7 - Audio 0
     *             8 - Audio 1
     *             9 - Audio 2
     *            10 - Audio 3
     */
    PAULA_IRQ_CHECK(INTF_AUD0 | INTF_AUD1 | INTF_AUD2 | INTF_AUD3);

    PAULA_IRQ_ACK(INTF_AUD0 | INTF_AUD1 | INTF_AUD2 | INTF_AUD3);

    PAULA_IRQ_HANDLE(INTB_AUD0);
    PAULA_IRQ_HANDLE(INTB_AUD1);
    PAULA_IRQ_HANDLE(INTB_AUD2);
    PAULA_IRQ_HANDLE(INTB_AUD3);
    
    PAULA_IRQ_EXIT();
}

BOOL Amiga_Level_5(VOID)
{
    /* Paula IRQs  11 - Serial RX
     *             12 - Disk Sync
     */
    PAULA_IRQ_CHECK(INTF_RBF | INTF_DSKSYNC);

    PAULA_IRQ_ACK(INTF_RBF | INTF_DSKSYNC);

    PAULA_IRQ_HANDLE(INTB_RBF);
    PAULA_IRQ_HANDLE(INTB_DSKSYNC);

    PAULA_IRQ_EXIT();
}

BOOL Amiga_Level_6(VOID)
{
    /* Paula IRQ  13 - CIA-B & IRQ6
     *            14 - INTEN (manually setting INTEN bit in INTREQ triggers it)
     */
    PAULA_IRQ_CHECK(INTF_EXTER | INTF_INTEN);

    PAULA_IRQ_HANDLE(INTB_EXTER);
    PAULA_IRQ_HANDLE(INTB_INTEN);

    PAULA_IRQ_ACK(INTF_EXTER);

    PAULA_IRQ_EXIT();
}

BOOL Amiga_Level_7(VOID)
{
    /* NMI - no way around it.
     */
    const UWORD mask = (1 << 15);

    PAULA_IRQ_HANDLE(15);

    /* Don't reschedule on the way out - so don't
     * call PAULA_IRQ_EXIT()
     */
    return FALSE;
}

const struct M68KException AmigaExceptionTable[] = {
	{ .Id =  25, .Handler = Amiga_Level_1_TrapCode },
	{ .Id =  26, .Handler = Amiga_Level_2_TrapCode },
	{ .Id =  27, .Handler = Amiga_Level_3_TrapCode },
	{ .Id =  28, .Handler = Amiga_Level_4_TrapCode },
	{ .Id =  29, .Handler = Amiga_Level_5_TrapCode },
	{ .Id =  30, .Handler = Amiga_Level_6_TrapCode },
	{ .Id =  31, .Handler = Amiga_Level_7_TrapCode },
	{ .Id =   0, }
};

void AmigaIRQInit(struct ExecBase *SysBase)
{
	/* Disable all interrupts */
	custom_w(INTENA, 0x7fff);
	/* Clear any requests */
	custom_w(INTREQ, 0x7fff);

	M68KExceptionInit(AmigaExceptionTable, SysBase);

	/* Enable DMA */
	custom_w(DMACON, 0x8240);

	/* Set up Vert. & Horiz. interval
	 * PAL 320x200x4
	 */
	custom_w(DIWSTRT, DIWSTRT_PAL);
	custom_w(DIWSTOP, DIWSTOP_PAL);
	custom_w(DDFSTRT, DDFSTRT_LOW);
	custom_w(DDFSTOP, DDFSTOP_LOW);

	/* Enable Vertical Blank and SoftInt */
	custom_w(INTENA, INTF_SETCLR | INTF_VERTB | INTF_SOFTINT);

	/* IRQs will be enabled by the first Enable() in Exec's init */
}
