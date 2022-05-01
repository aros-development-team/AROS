/*
 * depacker_entry.S
 *
 * Tiny entry point for self-unpacking executables.
 *
 * Written & released by Keir Fraser <keir.xen@gmail.com>
 * 
 * This is free and unencumbered software released into the public domain.
 * See the file COPYING for more details, or visit <http://unlicense.org>.
 */

        .text
start:
        /* Save all registers and start of the SegList. */
        movem.l d0-d7/a0-a7,-(sp)
        lea.l   start-4(pc),a0
        move.l  a0,-(sp)
        /* Walk the SegList to find the last segment. */
1:      move.l  (a0),d0
        lsl.l   #2,d0
        jeq     2f
        move.l  d0,a0
        jra     1b
2:      /* Jump to last segment (depacker_main). */
        addq.l  #4,a0
        jmp     (a0)
