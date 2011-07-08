/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Utility 64 bit multiplication routines. m68k version.
    Lang: english
*/

/* SMult64()/UMult64():
    These are the signed/unsigned 64 bit multiplication routines.
    There are two possibilities here because as of the 060 the
    32*32->64 bit result instructions are not supported, and I haven't
    quite figured out how to do this using the 32 bit ops yet (can't be
    that hard though).

    Still, emulating is faster than a unsup integer instruction except.

*/

    #include "aros/m68k/asm.h"

    .text
    .balign 4

    .globl  AROS_SLIB_ENTRY(UMult64,Utility,34)
    .globl  AROS_SLIB_ENTRY(UMult64_020,Utility,34)

    .type   AROS_SLIB_ENTRY(UMult64,Utility,34),@function
    .type   AROS_SLIB_ENTRY(UMult64_020,Utility,34),@function


AROS_SLIB_ENTRY(UMult64_020,Utility,34):
    mulu.l  %d0,%d0:%d1
    rts

/* How do I do this, again consider:
      (a^16 + b) * (c^16 + d)
    = ac^32 + (ad + bc)^16 + bd

    I tried to think of a way of doing this with the mulu.l instr,
    but I couldn't so I'll just use the mulu.w. Its quicker than
    an unsupp integer instruction anyway :)
*/

    .balign 4
AROS_SLIB_ENTRY(UMult64,Utility,34):
    movem.l %d2-%d5,-(%sp)
    /* Set up some registers */
    move.l  %d0,%d2
    move.l  %d1,%d3
    move.l  %d0,%d4   /* d */
    move.l  %d1,%d5   /* b */
    swap    %d2      /* a */
    swap    %d3      /* c */


    /* Firstly, find the product bd */
    mulu    %d5,%d1   /* d1 = bd */
    swap    %d1      /* d1 = (bd)^16 */

    /* Then find ac, put in d0 */
    mulu    %d3,%d0   /* d0 = ac */

    /* Next find ad, bc, and add together */
    mulu    %d2,%d4
    mulu    %d3,%d5
    add.l   %d5,%d4

    /*
        Add the low 16 bits to d1, then add upper 16 bits to d0
        But make sure we carry the 1...

        Apparently swap doesn't affect the X bit.
    */
    add.w   %d4,%d1
    swap    %d4
    addx.w  %d4,%d0

    /* All that remains to do is to flip d1 around the right way */
    swap    %d1
    movem.l (%sp)+,%d2-%d5

    rts
