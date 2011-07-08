/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: unsigned 32 bit division/modulus functions for Amiga/m68k
    Lang: english
*/

    #include "aros/m68k/asm.h"

    .text
    .balign 4

    .globl  AROS_SLIB_ENTRY(UDivMod32,Utility,26)
    .globl  AROS_SLIB_ENTRY(UDivMod32_020,Utility,26)

    .type   AROS_SLIB_ENTRY(UDivMod32,Utility,26),@function
    .type   AROS_SLIB_ENTRY(UDivMod32_020,Utility,26),@function

AROS_SLIB_ENTRY(UDivMod32_020,Utility,26):
    divul.l %d1,%d1:%d0
    rts

/*
    This next algorithm is from the ixemul 41.0 source, the file
    ./ixemul-41.0/gnulib/common.h
*/

    .balign 4
AROS_SLIB_ENTRY(UDivMod32,Utility,26):
    movem.l %d2-%d3,-(%sp)
    cmp.l   #0xFFFF,%d1
    bhi.s   .Lfull_division
    move.l  %d1,%d3
    swap    %d0
    move.w  %d0,%d3
    beq.s   .Lsmall_division
    divu    %d1,%d3
    move.w  %d3,%d0
.Lsmall_division:
    swap    %d0
    move.w  %d0,%d3
    divu    %d1,%d3
    move.w  %d3,%d0
    swap    %d3
    move.w  %d3,%d1
    movem.l (%sp)+,%d2-%d3
    rts

.Lfull_division:
    move.l  %d1,%d3
    move.l  %d0,%d1
    clr.w   %d1
    swap    %d0
    swap    %d1
    clr.w   %d0
    moveq   #0xF,%d2
.Lloop:
    add.l   %d0,%d0
    addx.l  %d1,%d1
    cmp.l   %d1,%d3
    bhi.s   .Lloopend
    sub.l   %d3,%d1
    addq.w  #1,%d0
.Lloopend:
    dbra    %d2,.Lloop
    movem.l (%sp)+,%d2-%d3
    rts
