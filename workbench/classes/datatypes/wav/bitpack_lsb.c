/*
 *	wave.datatype
 *	(c) Fredrik Wikstrom
 */

#include <exec/types.h>
#include <dos/dos.h>

#include "endian.h"
#include "bitpack.h"

// LSb style functions:

extern const ULONG bp_mask[33];

ULONG bitpack_read1_lsb (BitPack_buffer *b) {
    ULONG ret=0;
    if (b->ptr[0]&(1 << b->endbit))
        ret=1;
    b->endbit++;
    if (b->endbit==8) {
        b->endbit=0;
        b->ptr++;
        b->endbyte++;
    }
    return (ret);
}

ULONG bitpack_read_lsb (BitPack_buffer *b, LONG bits) {
    ULONG ret;

    bits+=b->endbit;

    ret=b->ptr[0]>>b->endbit;
    if (bits>8) {
        ret|=b->ptr[1]<<(8-b->endbit);
        if (bits>16) {
            ret|=b->ptr[2]<<(16-b->endbit);
            if (bits>24) {
                ret|=b->ptr[3]<<(24-b->endbit);
                if (bits>32) {
                    ret|=b->ptr[4]<<(32-b->endbit);
                }
            }
        }
    }
    ret&=bp_mask[bits];

    b->ptr+=(bits >> 3);
    b->endbyte+=(bits >> 3);
    b->endbit=bits&7;

    return (ret);
}

#if BP_WRITE_CMDS

void bitpack_write1_lsb (BitPack_buffer *b, ULONG val) {
	if (val & 1) {
		b->ptr[0]|=(1 << b->endbit);
	} else {
		b->ptr[0]&=~(1 << b->endbit);
	}
    b->endbit++;
    if (b->endbit==8) {
        b->endbit=0;
        b->ptr++;
        b->endbyte++;
    }
}

void bitpack_write_lsb (BitPack_buffer *b, ULONG val, LONG bits) {
	val&=bp_mask[bits];
	bits+=b->endbit;

	b->ptr[0]|=val<<b->endbit;
	if (bits>=8) {
		b->ptr[1]=val>>(8-b->endbit);
		if (bits>=16) {
			b->ptr[2]=val>>(16-b->endbit);
			if (bits>=32) {
				if (b->endbit)
					b->ptr[4]=val>>(32-b->endbit);
				else
					b->ptr[4]=0;
			}
		}
	}

	b->endbyte+=(bits>>3);
	b->ptr+=(bits>>3);
	b->endbit=bits&7;
}

#endif

#if BP_FAST_CMDS

void bitpack_iobits_lsb (ExtBitPack_buffer *b, LONG bits) {
}

ULONG bitpack_fastread_lsb (ExtBitPack_buffer *b) {
}

#if BP_WRITE_CMDS

void bitpack_fastwrite_lsb (ExtBitPack_buffer *b, ULONG val) {
}

#endif

#endif
