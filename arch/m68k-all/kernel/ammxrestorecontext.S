/*****************************************************************************

    NAME
#include <proto/kernel.h>

AROS_UFH1(void, AMMXRestoreContext,
	AROS_UFHA(struct AMMXContext *, ammx, A0))

    SYNOPSIS

    LOCATION

    FUNCTION
	Restore AMMX Context

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
    .globl	AMMXRestoreContext

AMMXRestoreContext:
    movem.l %d2/%d4/%d6, -(%sp)

    .short    0x1058		//move.l (a0)+,b0
    .short    0x1258		//move.l (a0)+,b1
    .short    0x1458		//move.l (a0)+,b2
    .short    0x1658		//move.l (a0)+,b3
    .short    0x1858		//move.l (a0)+,b4
    .short    0x1A58		//move.l (a0)+,b5
    .short    0x1C58		//move.l (a0)+,b6
    .short    0x1E58		//move.l (a0)+,b7

    .short    0xFE58,0x0001		//Load (a0)+,B0
    .short    0xFE58,0x0101		//Load (a0)+,B1
    .short    0xFE58,0x0201		//Load (a0)+,B2
    .short    0xFE58,0x0301		//Load (a0)+,B3
    .short    0xFE58,0x0401		//Load (a0)+,B4
    .short    0xFE58,0x0501		//Load (a0)+,B5
    .short    0xFE58,0x0601		//Load (a0)+,B6
    .short    0xFE58,0x0701		//Load (a0)+,B7
    .short    0xFE58,0x0801		//Load (a0)+,B8
    .short    0xFE58,0x0901		//Load (a0)+,B9
    .short    0xFE58,0x0a01		//Load (a0)+,B10
    .short    0xFE58,0x0b01		//Load (a0)+,B11
    .short    0xFE58,0x0c01		//Load (a0)+,B12
    .short    0xFE58,0x0d01		//Load (a0)+,B13
    .short    0xFE58,0x0e01		//Load (a0)+,B14
    .short    0xFE58,0x0f01		//Load (a0)+,B15

    .short    0xFE18,0x0801		//Load (a0)+,e0
    .short    0xFE18,0x0901		//Load (a0)+,e1
    .short    0xFE18,0x0A01		//Load (a0)+,e2
    .short    0xFE18,0x0B01		//Load (a0)+,e3
    .short    0xFE18,0x0C01		//Load (a0)+,e4
    .short    0xFE18,0x0D01		//Load (a0)+,e5
    .short    0xFE18,0x0E01		//Load (a0)+,e6
    .short    0xFE18,0x0F01		//Load (a0)+,e7

    .short    0xFE18,0x0001		//Load (a0)+,d0	//D0 D1 high
    .short    0xFE18,0x0201		//Load (a0)+,d2	//D2 D3 high
    .short    0xFE18,0x0401		//Load (a0)+,d4	//D4 D5 high
    .short    0xFE18,0x0601		//Load (a0)+,d6	//D6 D7 high
    .short    0xFE3F,0x1100,0x4567,0xcdef	//vperm	#0x...,d0,d1,d1
    .short    0xFE3F,0x3302,0x4567,0xcdef	//vperm	#0x...,d2,d3,d3
    .short    0xFE3F,0x5504,0x4567,0xcdef	//vperm	#0x...,d4,d5,d5
    .short    0xFE3F,0x7706,0x4567,0xcdef	//vperm	#0x...,d6,d7,d7

    movem.l (%sp)+, %d2/%d4/%d6
    rts
