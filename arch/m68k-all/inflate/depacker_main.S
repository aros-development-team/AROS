/*
 * depacker_main.S
 *
 * Main depacker/loader self-unpacking executables.
 *
 * Written & released by Keir Fraser <keir.xen@gmail.com>
 * 
 * This is free and unencumbered software released into the public domain.
 * See the file COPYING for more details, or visit <http://unlicense.org>.
 */

/* LibBase offsets */
#define LibBase_Version 20

/* Exec library calls */
#define Exec_CacheClearU  -0x27c

        .text
start:
        /* Decompress segment/reloc info in this segment. */
        lea.l   sizes(pc),a0
        move.l  (a0)+,d0        /* d0 = stream_bytes */
        move.l  a0,a4           /* a4 = buffer_start */
        move.l  a0,a2           /* a2 = buffer_start */
        jeq     skip_this_seg  /* zero -> not compressed */
        add.l   d0,a0           /* a0 = stream_end */
        lea.l   start-8(pc),a5
        add.l   (a5),a5         /* a5 = buffer_end */
1:      move.l  -(a0),-(a5)
        subq.l  #4,d0
        jne     1b
        jbsr    inflate
skip_this_seg:

        /* Decompress all other segments, first to last.
         * a2 = stream_len[seg] */
        move.l  (sp),a3         /* a3 = SegList */
unpack_next_seg:
        lea.l   4(a3),a4        /* a4 = buffer_start */
        move.l  (a2)+,d0        /* d0 = stream_bytes */
        jeq     skip_seg       /* zero -> not compressed */
        lea.l   (a4,d0.l),a0    /* a0 = stream_end */
        lea.l   -8(a4),a5
        add.l   (a5),a5         /* a5 = buffer_end */
        move.l  a5,a1           /* a1 = buffer_end */
        lsr.l   #2,d0
        cmp.l   (sp),a3         /* First segment?... */
        jne     1f
        subq.l  #6,d0           /* ...Then adjust for depacker_entry */
1:      move.l  -(a0),-(a5)
        subq.l  #1,d0
        jne     1b
        jbsr    inflate
        /* Fill remainder of segment allocation with zeroes. */
        cmp.l   a4,a1
        jeq     skip_seg
1:      move.l  d0,(a4)+
        cmp.l   a4,a1
        jne     1b
skip_seg:
        move.l  (a3),d0
        lsl.l   #2,d0
        move.l  d0,a3           /* a3 = next Segment */
        jne     unpack_next_seg

        /* Remainder of the loader is packed up with the reloc info.
         * Flush caches if possible, then jump at the depacked loader. */
        move.l  4.w,a6
        cmp.w   #37,LibBase_Version(a6) /* exec.lib_version >= 37? */
        jcs     1f
        jsr     Exec_CacheClearU(a6)
1:      jmp     (a2)

#define SAVE_RESTORE_REGS d0/a1-a3
#include "inflate.S"

        .balign 4
sizes:
