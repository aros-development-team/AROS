
#include "aros/m68k/asm.h"

	.text
	.balign 4
	.globl	AROS_SLIB_ENTRY(WaitBlit,Graphics,38)

AROS_SLIB_ENTRY(WaitBlit,Graphics,38):
    tst.w 0xdff002 /* original DIP Agnus bug workaround */
w2: btst #6,0xdff002
    bne.s w1
    rts
w1: tst.b 0xbfe001 /* keep CPU out of the chipbus for few cycles */
    bra.s w2
