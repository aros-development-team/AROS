
#include "aros/m68k/asm.h"

	.text
	.balign 4
	.globl	AROS_SLIB_ENTRY(WaitBlit,Graphics,38)

AROS_SLIB_ENTRY(WaitBlit,Graphics,38):
    /* Blitter nasty set? Also simultaneous original DIP Agnus bug workaround. */
    btst #2,0xdff002
    bne.s w1

    /* Already finished? Exit immediately. */
    btst #6,0xdff002
    beq.s w0

    /* Set blitter nasty temporarily. */
    move.w #0x8400,0xdff096
w2: /* Keep CPU out of the chipbus for few cycles. */
    tst.b 0xbfe001
    btst #6,0xdff002
    bne.s w2
    /* Clear blitter nasty. */
    move.w #0x0400,0xdff096

w0: rts

    /* Blitter nasty was already set, normal wait loop. */
w1: btst #6,0xdff002
    beq.s w0
    /* Keep CPU out of the chipbus for few cycles. */
    tst.b 0xbfe001
    bra.s w1
