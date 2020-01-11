/*****************************************************************************

    NAME
#include <proto/kernel.h>

AROS_UFH1(void, AMMXSaveContext,
	AROS_UFHA(struct AMMXContext *, ammx, A0))

    SYNOPSIS

    LOCATION

    FUNCTION
	Save AMMX Context

    INPUTS
	None

    RESULT
	None

    NOTES
	Must be called in supervisor mode.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
    #include "aros/m68k/asm.h"

    .text
    .balign 4
    .globl	AMMXSaveContext

AMMXSaveContext:
    movem.l %d2/%d4/%d6, -(%sp)
    .short	0xFE3F,0x7606,0x0123,0x89AB	//vperm	#0x012389ab,d6,d7,d6
    .short	0xFE3F,0x5404,0x0123,0x89AB	//vperm	#0x012389ab,d4,d5,d4
    .short	0xFE3F,0x3202,0x0123,0x89AB	//vperm	#0x012389ab,d2,d3,d2
    .short	0xFE3F,0x1000,0x0123,0x89AB	//vperm	#0x012389ab,d0,d1,d0
    .short	0xFE20,0x6004		//store	d6,-(a0)
    .short	0xFE20,0x4004		//store	d4,-(a0)
    .short	0xFE20,0x2004		//store	d2,-(a0)
    .short	0xFE20,0x0004		//store	d0,-(a0)

    .short	0xFE20,0xF004		//store	e7,-(a0)
    .short	0xFE20,0xE004		//store	e6,-(a0)
    .short	0xFE20,0xD004		//store	e5,-(a0)
    .short	0xFE20,0xC004		//store	e4,-(a0)
    .short	0xFE20,0xB004		//store	e3,-(a0)
    .short	0xFE20,0xA004		//store	e2,-(a0)
    .short	0xFE20,0x9004		//store	e1,-(a0)
    .short	0xFE20,0x8004		//store	e0,-(a0)

    .short	0xFEA0,0xf004		//store   e23,-(a0)
    .short	0xFEA0,0xe004		//store   e22,-(a0)
    .short	0xFEA0,0xd004		//store   e21,-(a0)
    .short	0xFEA0,0xc004		//store   e20,-(a0)
    .short	0xFEA0,0xb004		//store   e19,-(a0)
    .short	0xFEA0,0xa004		//store   e18,-(a0)
    .short	0xFEA0,0x9004		//store   e17,-(a0)
    .short	0xFEA0,0x8004		//store   e16,-(a0)
    .short	0xFEA0,0x7004		//store   e15,-(a0)
    .short	0xFEA0,0x6004		//store   e14,-(a0)
    .short	0xFEA0,0x5004		//store   e13,-(a0)
    .short	0xFEA0,0x4004		//store   e12,-(a0)
    .short	0xFEA0,0x3004		//store   e11,-(a0)
    .short	0xFEA0,0x2004		//store   e10,-(a0)
    .short	0xFEA0,0x1004		//store   e9 ,-(a0)
    .short	0xFEA0,0x0004		//store   e8 ,-(a0)

    .short	0x110F			//move.l  b7,-(a0)
    .short	0x110E			//move.l  b6,-(a0)
    .short	0x110D			//move.l  b5,-(a0)
    .short	0x110C			//move.l  b4,-(a0)
    .short	0x110B			//move.l  b3,-(a0)
    .short	0x110A			//move.l  b2,-(a0)
    .short	0x1109			//move.l  b1,-(a0)
    .short	0x1108			//move.l  b0,-(a0)

    movem.l (%sp)+, %d2/%d4/%d6
    rts
