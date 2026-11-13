/* m68k entrypoint for exec.library function Debug */

    #include "aros/m68k/asm.h"

    .text
    .balign 4
    .globl  AROS_SLIB_ENTRY(Debug,Exec,19)

AROS_SLIB_ENTRY(Debug,Exec,19):
    move.l %sp, %d0
    movem.l %d2-%d7/%a2-%a6, -(%sp)
    move.l %d0, -(%sp)
    jsr InternalDebug
    move.l (%sp)+, %d0
    movem.l (%sp)+, %d2-%d7/%a2-%a6
    rts
