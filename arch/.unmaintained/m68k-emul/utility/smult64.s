/*
    Copyright � 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Utility 64 bit multiplication routines. m68k version.
    Lang: english
*/

#include "machine.i"

/* SMult64()/UMult64():
    These are the signed/unsigned 64 bit multiplication routines.
    There are two possibilities here because as of the 060 the
    32*32->64 bit result instructions are not supported, and I haven't
    quite figured out how to do this using the 32 bit ops yet (can't be
    that hard though).

    Still, emulating is faster than a unsup integer instruction except.

*/
    .text
    .balign 16

    .globl  AROS_SLIB_ENTRY(SMult64,Utility)
    .globl  AROS_SLIB_ENTRY(SMult64_020,Utility)

    /* Required by SMult64() */
    .globl  AROS_SLIB_ENTRY(UMult64,Utility)

    .type   AROS_SLIB_ENTRY(SMult64,Utility),@function
    .type   AROS_SLIB_ENTRY(SMult64_020,Utility),@function


AROS_SLIB_ENTRY(SMult64_020,Utility):
    muls.l  %d0,%d0:%d1
    rts

/* How do I do this, again consider:
      (a^16 + b) * (c^16 + d)
    = ac^32 + (ad + bc)^16 + bd

    I tried to think of a way of doing this with the mulu.l instr,
    but I couldn't so I'll just use the mulu.w. Its quicker than
    an unsupp integer instruction anyway :)
*/

/* Have to change the sign... */
AROS_SLIB_ENTRY(SMult64,Utility):
    move.l  %d2,-(%a7)
    moveq   #0,%d2
    tst.l   %d0
    jbpl    .ispos1
    neg.l   %d0
    addq.l  #1,%d2
.ispos1:
    tst.l   %d1
    jbpl    .ispos2
    neg.l   %d1
    subq.l  #1,%d2

    /* Ok, so if d2 != 0, then the sign was changed */
.ispos2:
    bsr.s   AROS_SLIB_ENTRY(UMult64,Utility)
    tst.l   %d0
    jbeq    .ispos
    moveq   #0,%d2

    /* Ok we have to change the sign, 2's comp = 1's comp + 1 */
    not.l   %d0
    not.l   %d1
    /* Add 1 to low order 32 bits */
    addq.l  #1,%d1
    /* Add 0 and the carry to the high 32 bits */
    addx.l  %d2,%d0
.ispos:
    move.l  (%sp)+,%d2
    rts

