/*
    Copyright 2025 Johan Gill. Licensed under AROS Public License 1.1.
    See workbench/libs/rexxsyslib/cvi2arg.c for function documentation.
*/

#include "aros/m68k/asm.h"

    .text
    .balign 4

    .globl  AROS_SLIB_ENTRY(CVi2arg,RexxSys,52)
    .type   AROS_SLIB_ENTRY(CVi2arg,RexxSys,52),@function

AROS_SLIB_ENTRY(CVi2arg,RexxSys,52):
    /* Allocate temp buffer on stack */
    suba.l #12, %a7

    /* Call CVi2a to write the digits to the temp buffer */
    move.l %a7, %a0
    clr.l  %d1
    jsr    -306(%a6)

    /* Call CreateArgstring */
    move.l %a7, %a0
    jsr    -126(%a6)
    adda.l #12, %a7
    rts
