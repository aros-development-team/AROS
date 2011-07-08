/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Utility 32 bit multiplication routines. m68k version.
    Lang: english
*/

/* SMult32()/UMult32():
    These are the signed/unsigned 32 bit multiplication routines.
    I have two different possibilities here since I could be running
    on a system without the corresponding muls.l/mulu.l calls.

    After some soul searching I am happy that these routines don't
    have to be separated between signed/unsigned since the sign is
    correct, or the number overflows.

    The native versions do make the difference however.

    What I do is SetFunction() the correct function in later.
*/

    #include "aros/m68k/asm.h"

    .text
    .balign 4

    .globl  AROS_SLIB_ENTRY(SMult32,Utility,23)
    .globl  AROS_SLIB_ENTRY(UMult32,Utility,24)
    .globl  AROS_SLIB_ENTRY(UMult32_020,Utility,24)

    .type   AROS_SLIB_ENTRY(SMult32,Utility,23),@function
    .type   AROS_SLIB_ENTRY(UMult32,Utility,24),@function
    .type   AROS_SLIB_ENTRY(UMult32_020,Utility,24),@function

AROS_SLIB_ENTRY(UMult32_020,Utility,24):
    mulu.l  %d1,%d0
    rts

    .balign 4
AROS_SLIB_ENTRY(SMult32,Utility,23):
AROS_SLIB_ENTRY(UMult32,Utility,24):
/*
    What do we have to do
    d0 = (a^16 + b), d1 = (c^16 = d)
    res = ac^32 + (ad + bc)^16 + bd

    Now, ac^32 can be thrown away, as can the high 16 bits of ad + bd
*/
    movem.l %d2/%d3,-(%sp)
    moveq   #0,%d2
    moveq   #0,%d3

    /* ad */
    move.w  %d1,%d3
    swap    %d0
    mulu    %d0,%d3

    /* bc */
    swap    %d0
    move.w  %d0,%d2
    swap    %d1
    mulu    %d1,%d2

    /* (ad + bc)^16 */
    add.l   %d3,%d2
    swap    %d2
    clr.w   %d2

    /* bd + (ad + bc)^16 */
    swap    %d1
    mulu    %d1,%d0
    add.l   %d2,%d0
    movem.l (%sp)+,%d2/%d3
    rts

