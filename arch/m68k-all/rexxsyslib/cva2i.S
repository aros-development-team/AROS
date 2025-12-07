/*
    Copyright 2025 Johan Gill. Licensed under AROS Public License 1.1.
    See workbench/libs/rexxsyslib/cva2i.c for function documentation.
*/

#include "aros/m68k/asm.h"

    .text
    .balign 4

    .globl  AROS_SLIB_ENTRY(CVa2i,RexxSys,50)
    .type   AROS_SLIB_ENTRY(CVa2i,RexxSys,50),@function

AROS_SLIB_ENTRY(CVa2i,RexxSys,50):
    movem.l  %d2-%d5, -(%sp)
    clr.l    %d0       /* absolute value (will be value on exit) */
    clr.l    %d1       /* digit count */
    moveq.l  #1, %d2   /* sign (1 for positive, -1 for negative) */
    clr.l    %d3

    /* Check for sign character */
    move.b  (%a0)+, %d3
    beq     cva2i_apply_sign_and_return /* End of string */
    cmpi.b  #'+', %d3
    beq     cva2i_count_sign
    cmpi.b  #'-', %d3
    bne     cva2i_process_d3_char
    moveq.l #-1, %d2
cva2i_count_sign:
    addq    #1, %d1

cva2i_next_char:
    /* Process digits starting from where A0 points */
    move.b  (%a0)+, %d3
    beq     cva2i_apply_sign_and_return /* End of string */
cva2i_process_d3_char:
    cmpi.b  #'0', %d3
    blt     cva2i_apply_sign_and_return /* ASCII < '0' is not a digit */
    cmpi.b  #'9', %d3
    bgt     cva2i_apply_sign_and_return /* ASCII > '9' is not a digit */

    /* Convert ASCII digit to integer value */
    subi.b  #'0', %d3

    /* Compute new absolute value: D0 * 10 + digit */
    move.l  %d0, %d4
    move.l  %d0, %d5
    lsl.l   #3, %d4
    lsl.l   #1, %d5
    add.l   %d5, %d4
    add.l   %d3, %d4

cva2i_check_overflow:
    tst.l   %d4
    blt     cva2i_apply_sign_and_return /* Sign bit set means overflow here */

    /* Update value and continue processing */
    move.l  %d4, %d0
    addq    #1, %d1
    bra     cva2i_next_char

cva2i_apply_sign_and_return:
    cmpi    #1, %d2
    beq     cva2i_end
    neg.l   %d0
cva2i_end:
    subq.l  #1, %a0  /* Set A0 to the end of scan */
    movem.l (%sp)+, %d2-%d5
    rts
