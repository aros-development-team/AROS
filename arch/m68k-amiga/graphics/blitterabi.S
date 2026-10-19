/* Copyright (C) 2026, The AROS Development Team. All rights reserved. */

#include "aros/m68k/asm.h"

    .text
    .balign 4
    .globl AmigaOwnBlitter
    .globl AmigaDisownBlitter

/* Classic Amiga callers keep blitter dimensions and pointers in these
 * registers across the ownership calls. Keep the shared implementation. */
AmigaOwnBlitter:
    movem.l %d0-%d1/%a0-%a1,%sp@-
    jsr AROS_SLIB_ENTRY(OwnBlitter,Graphics,76)
    movem.l %sp@+,%d0-%d1/%a0-%a1
    rts

AmigaDisownBlitter:
    movem.l %d0-%d1/%a0-%a1,%sp@-
    jsr AROS_SLIB_ENTRY(DisownBlitter,Graphics,77)
    movem.l %sp@+,%d0-%d1/%a0-%a1
    rts
