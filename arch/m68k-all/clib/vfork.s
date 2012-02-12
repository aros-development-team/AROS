/*
 * vfork.s
 *
 *  Created on: Aug 23, 2010
 *      Author: misc
 *
 * $Id$

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
        .align 2
        .global AROS_CDEFNAME(vfork)
        .type AROS_CDEFNAME(vfork),%function

AROS_CDEFNAME(vfork):
        move.l  %sp@+,%d1               /* Return address -> d1 */
        move.l  %sp, %a0                /* Save aside %sp */
        lea.l   %sp@(-4 * 16),%sp       /* Reverse space for jmp_buf */
        movem.l %d1-%d7/%a2-%a6,%sp@    /* Registers */
        move.l  %a0, %sp@(4 * 12)       /* Set %sp in jmp_buf */
#if DEBUG
        move.l  #(DEBUG_MAGIC+1),%sp@(4 * 15)
#endif
        move.l  %sp,%sp@-               /* Push jmp_buf (sp) address */
        jsr     __vfork                /* __vfork call won't return */
