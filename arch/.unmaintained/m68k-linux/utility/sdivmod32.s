/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: signed 32 bit division/modulus functions for m68k
    Lang: english
*/

#include "machine.i"

    .text
    .balign 16
    .globl  AROS_SLIB_ENTRY(SDivMod32,Utility)
    .globl  AROS_SLIB_ENTRY(SDivMod32_020,Utility)

    /* Needed for SDivMod32 */
    .globl  AROS_SLIB_ENTRY(UDivMod32,Utility)

    .type   AROS_SLIB_ENTRY(SDivMod32,Utility),@function
    .type   AROS_SLIB_ENTRY(SDivMod32_020,Utility),@function

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
AROS_SLIB_ENTRY(SDivMod32,Utility):
    tst.l   %d0
    jbpl    .nispos
    neg.l   %d0
    tst.l   %d1
    jbpl    .nisneg
    neg.l   %d1
    jbsr    AROS_SLIB_ENTRY(UDivMod32,Utility)
    neg.l   %d1
    rts
.nisneg:
    jbsr    AROS_SLIB_ENTRY(UDivMod32,Utility)
    neg.l   %d1
    neg.l   %d0
    rts
.nispos:
    tst.l   %d1
    jbpl    AROS_SLIB_ENTRY(UDivMod32,Utility)
    neg.l   %d1
    jbsr    AROS_SLIB_ENTRY(UDivMod32,Utility)
    neg.l   %d0
    rts
