/*
    Copyright 2025 Johan Gill. Licensed under AROS Public License 1.1.
    See workbench/libs/rexxsyslib/cvi2a.c for function documentation.
*/

#include "aros/m68k/asm.h"

    .text
    .balign 4

    .globl  AROS_SLIB_ENTRY(CVi2a,RexxSys,51)
    .type   AROS_SLIB_ENTRY(CVi2a,RexxSys,51),@function

    /*
     * a0 = buffer start (in), end of written digits (out)
     * a1 = 10^x table for dividing with 10
     * a2 = Pointer to current power of 10
     * a3 = Pointer to next power of 10
     * a4 = Stack pointer before digits are pushed
     * d0 = value (in), work register to calculate quotients, digits (out)
     * d1 = digits (in) (locked)
     * d2 = digits still available
     * d3 = current 10^x value
     * d4 = next 10^x value (used for building the quotients)
     * d5 = work register to calculate remainders
     */
AROS_SLIB_ENTRY(CVi2a,RexxSys,51):
    movem.l %d2-%d5/%a2-%a4, -(%a7)
    tst.w   %d1
    bne     copy_d1
    /* If d1 is 0 we can use as many digits as we want. Eleven are enough. */
    moveq.l #11, %d1
copy_d1:
    move.w  %d1, %d2                 /* Keep track of digits still available */
    lea     powers_of_10, %a1
    move.l  %a7, %a4                 /* Remember stack for later. Digits will be pushed. */

    /* Negate value and output '-' if value is negative */
    tst.l   %d0
    bpl     extract_digits
    neg.l   %d0
    subq.w  #1, %d2
    beq     write_nul
    move.b  #'-', (%a0)+

extract_digits:
    /* Divide by 10. The remainder is the least significant digit.
     * Repeat to get all digits.
     */
    move.l  %d0, %d5
    tst.w   %d2                      /* Are we allowed to get more digits? */
    beq     write_digits
    cmpi.l  #10, %d0
    blo     quotient_is_last_digit   /* We got to the most significant digit */

    /* DIVS and DIVU do not have enough range on 68000, so subtract 10 a lot.
     * Be clever and subtract as large a power of 10 as possible.
     */
    move.l  %a1, %a2
    lea     4(%a2), %a3
    clr.l   %d0

subtract_power:
    /* We don't want unneccessary memory accesses in the hot path power_loop. */
    move.l  (%a2), %d3
    move.l  (%a3), %d4

power_loop:
    cmp.l   %d3, %d5                 /* Compare current power with remaining value */
    blo     next_power               /* If value < power, move to next power */
    sub.l   %d3, %d5                 /* Subtract power from value */
    add.l   %d4, %d0                 /* Build the quotient */
    bra     power_loop

next_power:
    cmpi.l  #10, %d5
    blo     got_digit
    addq.l  #4, %a2
    addq.l  #4, %a3
    bra     subtract_power

got_digit:
    add.b   #'0', %d5                /* Convert to ASCII representation */
    move.b  %d5, -(%a7)              /* Store digit on stack */
    subq.w  #1, %d2
    beq     write_digits
    addq.l  #4,  %a1                 /* No need to check the highest power */
    bra     extract_digits

quotient_is_last_digit:
    subq.w  #1, %d2
    add.b   #'0', %d0
    move.b  %d0, (%a0)+

write_digits:
    cmpa.l  %a4, %a7
    beq     write_nul
    move.b  (%a7)+, (%a0)+           /* Write digit to buffer */
    bra     write_digits

write_nul:
    /* Write terminating NUL */
    clr.b   (%a0)

    /* Calculate length */
    sub.w   %d2, %d1
    clr.l   %d0
    move.w  %d1, %d0

    /* Restore registers */
    movem.l (%a7)+, %d2-%d5/%a2-%a4
    rts

powers_of_10:
    dc.l 1000000000
    dc.l 100000000
    dc.l 10000000
    dc.l 1000000
    dc.l 100000
    dc.l 10000
    dc.l 1000
    dc.l 100
    dc.l 10
    dc.l 1
