/*
    Copyright � 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: unsigned 32 bit division/modulus functions for Amiga/m68k
    Lang: english
*/

#include "machine.i"

    .text
    .balign 16
    .globl  AROS_SLIB_ENTRY(UDivMod32,Utility)
    .globl  AROS_SLIB_ENTRY(UDivMod32_020,Utility)

    .type   AROS_SLIB_ENTRY(UDivMod32,Utility),@function
    .type   AROS_SLIB_ENTRY(UDivMod32_020,Utility),@function

AROS_SLIB_ENTRY(UDivMod32_020,Utility):
    divul.l %d1,%d1:%d0
    rts

/*
    This next algorithm is from the ixemul 41.0 source, the file
    ./ixemul-41.0/gnulib/common.h
*/

AROS_SLIB_ENTRY(UDivMod32,Utility):
    movem.l  %d2-%d3,-(%sp)
    cmp.l   #0xFFFF,%d1
    jbhi    .full_division
    move.l  %d1,%d3
    swap    %d0
    move.w  %d0,%d3
    jbeq    .small_division
    divu    %d1,%d3
    move.w  %d3,%d0
.small_division:
    swap    %d0
    move.w  %d0,%d3
    divu    %d1,%d3
    move.w  %d3,%d0
    swap    %d3
    move.w  %d3,%d1
    movem.l  (%sp)+,%d2-%d3
    rts

.full_division:
    move.l  %d1,%d3
    move.l  %d0,%d1
    clr.w   %d1
    swap    %d0
    swap    %d1
    clr.w   %d0
    moveq.l #0xF,%d2
.loop:
    add.l   %d0,%d0
    addx.l  %d1,%d1
    cmp.l   %d1,%d3
    jbhi    .loopend
    sub.l   %d3,%d1
    addq.w  #1,%d0
.loopend:
    dbra    %d2,.loop
    movem.l  (%sp)+,%d2-%d3
    rts
