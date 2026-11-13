/*
    Copyright 2025 Johan Gill. Licensed under AROS Public License 1.1.
    See workbench/libs/rexxsyslib/strcmpn.c for function documentation.
*/

#include "aros/m68k/asm.h"

    .text
    .balign 4

    .globl  AROS_SLIB_ENTRY(StrcmpN,RexxSys,42)
    .type   AROS_SLIB_ENTRY(StrcmpN,RexxSys,42),@function

AROS_SLIB_ENTRY(StrcmpN,RexxSys,42):
    movem.l %d2-%d3, -(%sp)
    move.l  %d0, %d3        /* D3 counts down to 0 */
    beq     return_equal

loop:
    move.b  (%a0)+, %d1
    move.b  (%a1)+, %d2

    /* Check if any string has ended */
    tst.b   %d1
    beq     string1_ends
    tst.b   %d2
    beq     string2_ends

    /* Compare the characters */
    cmp.b   %d1, %d2
    beq     next_char
    blt     string2_less
    bgt     string2_greater

next_char:
    subq.l  #1, %d3
    bne     loop

    /* All characters up to length are equal */
    bra     return_equal

string1_ends:
    tst.b   %d2
    beq     return_equal
    bra     string2_greater

string2_ends:
    moveq.l #-1, %d0
    bra     return

string2_greater:
    moveq.l #1, %d0
    bra     return

string2_less:
    moveq.l #-1, %d0
    bra     return

return_equal:
    clr.l   %d0

return:
    tst.l   %d0             /* set CCR flags based on D0 */
    movem.l (%sp)+, %d2-%d3
    rts
