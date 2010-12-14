/*
    Copyright © 1995-2009, The AROS Development Team. All rights reserved.
    $Id$

    Desc: BCPL stubs that call into Exec and Dos libraries.
    Lang: english
*/
#define DEBUG 1
#include <aros/m68k/asm.h>

#include "bcpl.h"

#define BCPL(id,name)	.equ GV_##name, id
#include "bcpl.inc"
#undef BCPL

#define MAXARGS	(1 + 26)	/* Up to 26 arguments to the format */

	/* BCPL writef format strings:
	 * (case insensitive!)
	 *
	 * %S   - Send argument       to writes(BSTR)
	 * %Tn  - Send argument and n to writet(BSTR, n)
	 * %C   - Send argument       to wrch(LONG)
	 * %Bn  - Send argument and n to writebin(LONG, n)
	 * %On  - Send argument and n to writeoct(LONG, n)
	 * %Xn  - Send argument and n to writehex(LONG, n)
	 * %In  - Send argument and n to writei(LONG, n)
	 * %N   - Send argument       to writei(LONG, 0)
	 * %%   - Call wrch('%')
	 */
	.text
	.balign	4
	.globl BCPL_writef
BCPL_writef:
	/* Entry:
	 *   A0      - 0x00000000
	 *   A1[0]   - BSTR Format (also in D1)
	 *   A1[1..] - LONG Arguments
	 *   A2      - GlobVec
	 *   A5      - BCPL JSR
	 *   A6      - BCPL RTS
	 *   A7      - C stack pointer
	 */
	movem.l	%d2-%d7,%sp@-

	move.l	%d1, %d6	/* D6 = Format pointer - 1 */

#if 0
	/* Dump format */
	move.l	#(3*4 + MAXARGS*4),%d0
	move.l	%a2@(GV_writes),%a4
	jsr	(%a5)
	move.l	#(3*4 + MAXARGS*4),%d0
	move.l	%a2@(GV_newline),%a4
	jsr	(%a5)
#endif
     
	lsl.l	#2, %d6
	clr.l	%d5		/* D5 = Format Length */
	move.b	%a0@(%d6), %d5

	tst	%d2		/* Done already? */
	beq	.Lformat_exit

	clr.l	%d4		/* D4 = Arg Index */
.Lformat_loop:
	addq.l	#1, %d6		/* D6 = Format Pointer */
	clr.l	%d7		/* D7 = Current char to test */
	move.b	%a0@(%d6),%d7
	cmp.b	#'%',%d7
	bne	.Lformat_loop_wrch

.Lformat_loop_perc:	/* Seen a '%' sign */
	addq.l	#1, %d6		/* D6 = Address of char after the '%' */
	subq.l	#1, %d5		/* D5 = length - 1 */
	move.b	%a0@(%d6),%d7	/* D7 = char after the '%' */

	cmp.b	#'%',%d7	/* Handle '%%' */
	beq	.Lformat_loop_wrch

	bclr	#5,%d7		/* To Upper */
.Lformat_loop_switch:

.Lformat_loop_string:
	cmp.b	#'S', %d7	/* Handle '%S' */
	bne	0f
	addq.l	#4,%d4
	move.l	%a1@(%d4), %d1
	move.l	#(3*4 + MAXARGS*4),%d0
	move.l	%a2@(GV_writes),%a4
	jsr	(%a5)
	jmp	.Lformat_loop_next
0:

.Lformat_loop_text:
	cmp.b	#'T', %d7	/* Handle '%Tn' */
	bne	0f
	addq.l	#1, %d6		/* D6 = Address of char after the 'T' */
	subq.l	#1, %d5		/* D5 = length - 1 */
	addq.l	#4,%d4		/* D4 = Next argument */
	move.l	%a1@(%d4), %d1	/* D1 = BSTR to print */

	move.b	%a0@(%d6),%d7	/* D7 = char after the 'T' */
	jsr	BCPL_writef_de36/* D2 = Base36 of D7 */

	move.l	#(3*4 + MAXARGS*4),%d0
	move.l	%a2@(GV_writet),%a4
	jsr	(%a5)
	jmp	.Lformat_loop_next
0:

.Lformat_loop_char:
	cmp.b	#'C', %d7	/* Handle '%C' */
	bne	0f
	addq.l	#4,%d4
	move.l	%a1@(%d4), %d1
	move.l	#(3*4 + MAXARGS*4),%d0
	move.l	%a2@(GV_wrch),%a4
	jsr	(%a5)
	jmp	.Lformat_loop_next
0:

.Lformat_loop_oct:
	cmp.b	#'O', %d7	/* Handle '%On' */
	bne	0f
	addq.l	#1, %d6		/* D6 = Address of char after the '%' */
	subq.l	#1, %d5		/* D5 = length - 1 */
	move.b	%a0@(%d6),%d7	/* D7 = char after the 'O' */
	jsr	BCPL_writef_de36/* D2 = Field width */

	addq.l	#4,%d4		/* D4 = Next argument */
	move.l	%a1@(%d4), %d1	/* D1 = LONG to print */

	move.l	#(3*4 + MAXARGS*4),%d0
	move.l	%a2@(GV_writeoct),%a4
	jsr	(%a5)
	jmp	.Lformat_loop_next
0:

.Lformat_loop_hex:
	cmp.b	#'X', %d7	/* Handle '%Xn' */
	bne	0f
	addq.l	#1, %d6		/* D6 = Address of char after the '%' */
	subq.l	#1, %d5		/* D5 = length - 1 */
	move.b	%a0@(%d6),%d7	/* D7 = char after the 'X' */
	jsr	BCPL_writef_de36/* D2 = Field width */

	addq.l	#4,%d4		/* D4 = Next argument */
	move.l	%a1@(%d4), %d1	/* D1 = LONG to print */

	move.l	#(3*4 + MAXARGS*4),%d0
	move.l	%a2@(GV_writehex),%a4
	jsr	(%a5)
	jmp	.Lformat_loop_next
0:

.Lformat_loop_integer:
	cmp.b	#'I', %d7	/* Handle '%In' */
	bne	0f
	addq.l	#1, %d6		/* D6 = Address of char after the '%' */
	subq.l	#1, %d5		/* D5 = length - 1 */
	move.b	%a0@(%d6),%d7	/* D7 = char after the 'I' */
	jsr	BCPL_writef_de36/* D2 = Field width */

	addq.l	#4,%d4		/* D4 = Next argument */
	move.l	%a1@(%d4), %d1	/* D1 = LONG to print */

	move.l	#(3*4 + MAXARGS*4),%d0
	move.l	%a2@(GV_writei),%a4
	jsr	(%a5)
	jmp	.Lformat_loop_next
0:

.Lformat_loop_natural:
	cmp.b	#'N', %d7	/* Handle '%N' */
	bne	0f
	addq.l	#4,%d4
	move.l	%a1@(%d4), %d1
	move.l	#(3*4 + MAXARGS*4),%d0
	clr.l	%d2
	move.l	%a2@(GV_writei),%a4
	jsr	(%a5)
	jmp	.Lformat_loop_next
0:

	/* Default: Write D7 via wrch */
.Lformat_loop_wrch:
	move.l	%d7, %d1
	move.l	#(3*4 + MAXARGS*4),%d0
	move.l	%a2@(GV_wrch),%a4
	jsr	(%a5)
	jmp	.Lformat_loop_next

.Lformat_loop_next:
	dbf	%d5, .Lformat_loop

.Lformat_exit:
	movem.l	%sp@+,%d2-%d7
	jmp	(%a6)


/* Helper functions */

/* In:  D7 '0'..'9','A'..'Z','a'..'z'
   Out: D2 0..35
 */
BCPL_writef_de36:
	clr.l	%d2
	move.b	%d7, %d2
	subi.b	#'0',%d2
	blt	.LBCPL_writef_de36_invalid
	cmp.b	#9,%d2
	ble	.LBCPL_writef_de36_exit
	subi.b	#(('A'-'0')-10),%d2
	blt	.LBCPL_writef_de36_invalid
	cmp.b	#35,%d2
	ble	.LBCPL_writef_de36_exit
.LBCPL_writef_de36_invalid:
	clr.l	%d2
.LBCPL_writef_de36_exit:
	rts
