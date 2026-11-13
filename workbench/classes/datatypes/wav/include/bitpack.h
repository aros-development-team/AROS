/*
 *	wave.datatype
 *	(c) Fredrik Wikstrom
 */

#ifndef BITPACK_H
#define BITPACK_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#ifndef DOS_DOS_H
#include <dos/dos.h>
#endif

#define BP_FAST_CMDS (FALSE)
#define BP_WRITE_CMDS (FALSE)

struct BitPack_buffer_struct {
	UBYTE *buffer;
    UBYTE *ptr;
    LONG endbyte;
    LONG endbit;
    LONG size;
};

// For use with the faster read/write commands (?)
struct ExtBitPack_buffer_struct {
    UBYTE *ptr;
    LONG endbyte;
    LONG endbit;
    LONG size;

    LONG bits; // amount of bits to read or write
    ULONG mask; // pre-calced mask
};

typedef struct BitPack_buffer_struct BitPack_buffer;
typedef struct ExtBitPack_buffer_struct ExtBitPack_buffer;

void bitpack_init (BitPack_buffer *b, void *ptr, LONG size);
LONG bitpack_seek (BitPack_buffer *b, LONG offs, LONG type);
void * bitpack_align (BitPack_buffer *b);
void * bitpack_align_even (BitPack_buffer *b);

#define bitpack_init_lsb bitpack_init
#define bitpack_seek_lsb bitpack_seek
#define bitpack_align_lsb bitpack_align
#define bitpack_align_even_lsb bitpack_align_even
ULONG bitpack_read1_lsb (BitPack_buffer *b);
ULONG bitpack_read_lsb (BitPack_buffer *b, LONG bits);
#if BP_WRITE_CMDS
void bitpack_write1_lsb (BitPack_buffer *b, ULONG val);
void bitpack_write_lsb (BitPack_buffer *b, ULONG val, LONG bits);
#endif
#if BP_FAST_CMDS
void bitpack_iobits_lsb (ExtBitPack_buffer *b, LONG bits);
ULONG bitpack_fastread_lsb (ExtBitPack_buffer *b);
#if BP_WRITE_CMDS
void bitpack_fastwrite_lsb (ExtBitPack_buffer *b, ULONG val);
#endif
#endif

#define bitpack_init_msb bitpack_init
#define bitpack_seek_msb bitpack_seek
#define bitpack_align_msb bitpack_align
#define bitpack_align_even_msb bitpack_align_even
ULONG bitpack_read1_msb (BitPack_buffer *b);
ULONG bitpack_read_msb (BitPack_buffer *b, LONG bits);
#if BP_WRITE_CMDS
void bitpack_write1_msb (BitPack_buffer *b, ULONG val);
void bitpack_write_msb (BitPack_buffer *b, ULONG val, LONG bits);
#endif
#if BP_FAST_CMDS
void bitpack_iobits_msb (ExtBitPack_buffer *b, LONG bits);
ULONG bitpack_fastread_msb (ExtBitPack_buffer *b);
#if BP_WRITE_CMDS
void bitpack_fastwrite_msb (ExtBitPack_buffer *b, ULONG val);
#endif
#endif

#endif /* BITPACK_H */
