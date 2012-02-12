/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: longjmp
    Lang: english

        The jmp_buf is filled as follows (d0/d1/a0/a1 are not saved):

        _jmp_buf        offset  contents
        [0]             0       old pc
        [1]             4       d2
        [2]             8       d3
        [3]             12      d4
        [4]             16      d5
        [5]             20      d6
        [6]             24      d7
        [7]             28      a2
        [8]             32      a3
        [9]             36      a4
        [10]            40      a5
        [11]            44      a6
        [12]            48      old sp
        [13]            52      padding
        [14]            56      padding
        [15]            60      DEBUG_MAGIC
*/

#include "aros/m68k/asm.h"

        .text
        .balign 4
        .global AROS_CDEFNAME(longjmp)
        .type AROS_CDEFNAME(longjmp),%function

AROS_CDEFNAME(longjmp):
        /* New version adapted from libnix instead of ixemul.
         * Note the slightly different register save order.
         */
        addq.l  #4,%sp                  /* returns to other address */
        move.l  %sp@+,%a0               /* get address of jmp_buf */
        move.l  %sp@+,%d0               /* get return code */
        jne     .okret
        moveq.l #1,%d0                  /* make sure it isn't 0 */
.okret:
        movem.l (%a0),%d1-%d7/%a2-%a6/%sp       /* restore all registers except scratch */
#ifdef DEBUG
        cmp.l   #DEBUG_MAGIC,%a0@(4 * 15)
        beq     0f
        trap    #1
0:
#endif
        move.l  %d1,%a0
        jmp     %a0@
