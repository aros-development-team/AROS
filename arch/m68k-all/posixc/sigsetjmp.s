/*
    Copyright © 2015, The AROS Development Team. All rights reserved.
    $Id$

    Desc: sigsetjmp
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
        .global AROS_CDEFNAME(sigsetjmp)
        .type AROS_CDEFNAME(sigsetjmp),%function

AROS_CDEFNAME(sigsetjmp):
        /* New version adapted from libnix instead of ixemul.
         * Note the slightly different register save order.
         */
        move.l  %sp@(0*4),%d1           /* store return address */
        move.l  %sp@(1*4),%a0           /* get address of jmp_buf */
        lea.l   %sp@(1*4),%a1
        move.l  %a1,%a0@(4 * 12)
        movem.l %d1-%d7/%a2-%a6,(%a0)   /* store all registers except scratch and sp*/
#if DEBUG
        move.l  #DEBUG_MAGIC,%a0@(4 * 15)
#endif
        moveq.l #0,%d0                  /* return 0 */
        rts
