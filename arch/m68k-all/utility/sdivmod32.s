/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: signed 32 bit division/modulus functions for Amiga/m68k
    Lang: english
*/

    #include "aros/m68k/asm.h"

    .text
    .globl  AROS_SLIB_ENTRY(SDivMod32,Utility)
    .globl  AROS_SLIB_ENTRY(SDivMod32_020,Utility)

    /* Needed for SDivMod32 */
    .globl  AROS_SLIB_ENTRY(UDivMod32,Utility)

    .type   AROS_SLIB_ENTRY(SDivMod32,Utility),@function
    .type   AROS_SLIB_ENTRY(SDivMod32_020,Utility),@function

    .balign 4
AROS_SLIB_ENTRY(SDivMod32_020,Utility):
    divsl.l %d1,%d1:%d0
    rts

/* All we do is remember the sign and get UDivMod32 to do all the work,
    this is actually a bit harder than just changing both, doing the
    division and then changing the other...

    If both are positive, do nothing,
    if one is negative, change both,
    if both are negative, change remainder
*/

    .balign 4
AROS_SLIB_ENTRY(SDivMod32,Utility):
    tst.l   %d0
    bpl.s   nispos
    neg.l   %d0
    tst.l   %d1
    bpl.s   nisneg
    neg.l   %d1
    bsr.s   AROS_SLIB_ENTRY(UDivMod32,Utility)
    neg.l   %d1
    rts
nisneg:
    bsr.s   AROS_SLIB_ENTRY(UDivMod32,Utility)
    neg.l   %d1
    neg.l   %d0
    rts
nispos:
    tst.l   %d1
    bpl.s   AROS_SLIB_ENTRY(UDivMod32,Utility)
    neg.l   %d1
    bsr.s   AROS_SLIB_ENTRY(UDivMod32,Utility)
    neg.l   %d0
    rts
