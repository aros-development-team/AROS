/*
 * crc16_ccitt.S
 * 
 * Table-driven CRC16-CCITT implementation.
 *
 * Usage: Call crc16_ccitt_gentable to generate the 512-byte lookup table.
 * This table can then be passed to crc16_ccitt to perform fast CRC
 * calculations. The usual start value for crc_in (d0.w) is 0xFFFF.
 *
 * Written & released by Keir Fraser <keir.xen@gmail.com>
 * 
 * This is free and unencumbered software released into the public domain.
 * See the file COPYING for more details, or visit <http://unlicense.org>.
 */

        /* a0 = data, a1 = table, d0 = crc_in, d1 = # bytes */
        /* d0 = crc_out, all other regs preserved */
crc16_ccitt:
        movem.l d1-d4/a0/a2,-(sp)
        move.b  d0,d2           /* d2 = crc.lo */
        lsr.w   #8,d0           /* d0 = crc.hi */
        subq.l  #1,d1           /* d1.w = count.lo */
        move.l  d1,d4
        swap    d4              /* d4.w = count.hi */
1:      move.b  (a0)+,d3        /* 8 */
        eor.b   d3,d0           /* 4    crc.hi ^= *p++ */
        lea     (a1,d0.w),a2    /* 12 */
        move.b  (a2),d0         /* 8    crc.hi  = crc16tab[].hi */
        eor.b   d2,d0           /* 4    crc.hi ^= crc.lo */
        move.b  256(a2),d2      /* 12   crc.lo  = crc16tab[].lo */
        dbf     d1,1b           /* 10 */ /* == 58 */
        moveq   #-1,d1          /* reset inner loop counter */
        dbf     d4,1b           /* outer loop on count.hi */
        lsl.w   #8,d0
        move.b  d2,d0           /* d0.w = crc */
        movem.l (sp)+,d1-d4/a0/a2
        tst.w   d0              /* set CC.Z */
        rts

        /* a1 = table */
crc16_ccitt_gentable:
        movem.l d0-d2/a1,-(sp)
        moveq   #0,d1           /* d1 = byte */
        moveq   #0,d0           /* d0 = crc */
1:      move.b  d1,d0
        lsl.w   #8,d0           /* d0 = byte << 8 */
        moveq   #7,d2
2:      lsl.w   #1,d0
        jcc     3f
        eor.w   #0x1021,d0
3:      dbf     d2,2b
        move.b  d0,256(a1)      /* crc.lo */
        lsr.w   #8,d0
        move.b  d0,(a1)+        /* crc.hi */
        addq.b  #1,d1
        jne     1b
        movem.l (sp)+,d0-d2/a1
        rts
