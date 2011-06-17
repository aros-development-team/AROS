/*
 *	wave.datatype
 *	(c) Fredrik Wikstrom
 */

#include <exec/types.h>
#include <dos/dos.h>

#include "endian.h"
#include "bitpack.h"

// Generic BitPack stuff:

void bitpack_init (BitPack_buffer *b, void *ptr, LONG size) {
    b->buffer=b->ptr=ptr;
    b->size=size;
    b->endbyte=0;
    b->endbit=0;
}

const ULONG bp_mask[33]={
	0x00000000,	0x00000001,	0x00000003,	0x00000007,
	0x0000000f,	0x0000001f,	0x0000003f,	0x0000007f,
	0x000000ff,	0x000001ff,	0x000003ff,	0x000007ff,
	0x00000fff,	0x00001fff,	0x00003fff,	0x00007fff,
	0x0000ffff,	0x0001ffff,	0x0003ffff,	0x0007ffff,
	0x000fffff,	0x001fffff,	0x003fffff,	0x007fffff,
	0x00ffffff,	0x01ffffff,	0x03ffffff,	0x07ffffff,
	0x0fffffff,	0x1fffffff,	0x3fffffff,	0x7fffffff,
	0xffffffff
};

LONG bitpack_seek (BitPack_buffer *b, LONG offs, LONG type) {
	LONG oldoffset;
	oldoffset=(b->endbyte<<3)+b->endbit;

    switch (type) {

        case OFFSET_CURRENT:
            offs+=oldoffset;
            break;

        case OFFSET_END:
            offs+=(b->size<<3);
            break;

    }

    b->endbyte=offs>>3;
    b->endbit=offs&7;

    b->ptr=b->buffer+b->endbyte;

    return (oldoffset);
}

void * bitpack_align (BitPack_buffer *b) {
    if (b->endbit) {
        b->endbit=0;
        b->endbyte++;
        b->ptr++;
    }
    return (b->ptr);
}

void * bitpack_align_even (BitPack_buffer *b) {
    bitpack_align_lsb(b);
    if (b->endbyte&1) {
        b->endbyte++;
        b->ptr++;
    }
    return (b->ptr);
}
