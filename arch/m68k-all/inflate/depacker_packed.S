/*
 * depacker_packed.S
 *
 * Packed section of the loader for self-unpacking executables.
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
#define Exec_FreeMem      -0xd2

        /* On entry:
         *  a6   = ExecBase;
         *  (sp) = SegList, d0-d7/a0-a6, return_addresses[2] */
        .text
start:
        /* Perform relocations. */
        move.l  (sp),a3
        lea.l   reloc(pc),a2
next_reloc_table:
        /* Read next hunk nr (or -1 if last table). */
        move.w  (a2)+,d0
        jmi     relocs_done
        jbsr    get_seg_base
        move.l  a0,a1           /* a1 = segment being modified */
next_reloc_subtable:
        /* Read (#relocs, target_hunk). #relocs=0 means last sub-table. */
        move.w  (a2)+,d3        /* d3 = #relocs */
        jeq     next_reloc_table
        move.w  (a2)+,d0
        jbsr    get_seg_base
        move.l  a0,d1           /* d1 = target segment */
        moveq   #-4,d4          /* d4 = reloc offset */
next_reloc:
        /* Relocation offsets are delta-encoded:
         * Off(n) = Off(n-1) + 2 * (Delta_n + 1)  [1 <= n <= nr]
         * Off(0) = -4
         * len(Delta_n) = 1 [iff 1 <= Delta_n <= 255]
         *              = 4 [otherwise] */
        moveq   #0,d0
        move.b  (a2)+,d0
        jne     1f
        move.b  (a2)+,d0
        lsl.l   #8,d0
        move.b  (a2)+,d0
        lsl.l   #8,d0
        move.b  (a2)+,d0
1:      addq.l  #1,d0
        add.l   d0,d0
        add.l   d0,d4           /* d4 = new reloc offset */
        add.l   d1,(a1,d4.l)    /* perform the relocation */
        subq.w  #1,d3
        jne     next_reloc
        /* Word-align the relocation-table pointer for the next word access. */
        addq.w  #1,a2
        move.l  a2,d0
        and.b   #0xfe,d0
        move.l  d0,a2
        jra     next_reloc_subtable

/* a0 = get_seg_base(d0:seg#) */
get_seg_base:
        move.l  a3,a0
1:      subq.w  #1,d0
        jcs     1f
        move.l  (a0),a0
        add.l   a0,a0
        add.l   a0,a0
        jra     1b
1:      addq.l  #4,a0
        rts

relocs_done:
        /* Modify return address to jump at the unpacked exe. */
        move.l  (sp)+,a2        /* a2 = first segment BPTR */
        lea.l   4(a2),a0        /* a0 = unpacked entry point */
        move.l  a0,15*4(sp)     /* modify return address */
        /* Write register-restore epilogue code into the end of hunk#0. We will
         * jump there immediately after freeing this segment. It will restore
         * registers and then jump at the unpacked executable. */
        lea.l   -4(a2),a0
        add.l   (a0),a0           /* a0 = end of hunk#0 */
        move.w  #0x4e75,-(a0)     /* rts */
        move.l  #0x4cdf7fff,-(a0) /* movem.l (sp)+,d0-d7/a0-a6 */
        move.l  a0,-(sp)          /* push address of epilogue onto stack. */
        /* Walk the SegList to the end.
         * Result: a0-a2 = ptr to final two segments' BPTR fields. */
1:      move.l  (a2),d0
        lsl.l   #2,d0
        jeq     2f
        move.l  a2,a0
        move.l  d0,a2
        jra     1b
2:      /* Detach this segment from the process's SegList. */
        clr.l   (a0)
        /* Flush caches if possible. */
        cmp.w   #37,LibBase_Version(a6) /* exec.lib_version >= 37? */
        jcs     1f
        jsr     Exec_CacheClearU(a6)
1:      /* Atomically: Free this segment and jump to epilogue code in hunk0. */
        move.l  a2,a1
        move.l  -(a1),d0        /* a1 = this seg, d0 = this seg length */
        jmp     Exec_FreeMem(a6)

        .balign 4
reloc:
