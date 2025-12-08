/*
    Copyright 2025 Johan Gill. Licensed under AROS Public License 1.1.
    See workbench/libs/rexxsyslib/strlen.c for function documentation.
*/

#include "aros/m68k/asm.h"

    .text
    .balign 4

    .globl  AROS_SLIB_ENTRY(Strlen,RexxSys,48)
    .type   AROS_SLIB_ENTRY(Strlen,RexxSys,48),@function

AROS_SLIB_ENTRY(Strlen,RexxSys,48):
    move.l  %a0, -(%sp)
    clr.l    %d0

strlen_next_char:
    /* Process characters starting from where A0 points */
    move.b  (%a0)+, %d1
    beq     strlen_end_of_string
    addq.l  #1, %d0
    bra     strlen_next_char

strlen_end_of_string:
    move.l (%sp)+, %a0
    tst.l   %d0  /* Set the Z flag if D0 is zero */
    rts
