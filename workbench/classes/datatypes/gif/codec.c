/*
    Copyright © 1995-2005, The AROS Development Team. All rights reserved.
    $Id$

    This file does not contain LZW code. It can read uncompressed files only,
    but it creates uncompressed files, that can be read by standard decoders, too.
*/

/**********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dostags.h>

#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>

#include "debug.h"

#include "gifclass.h"
#include "codec.h"

/**************************************************************************************************/

#define BLOCKLEN 254

struct codecvars
{   /* used by Decode */
    short min_code_size;
    short code_size;
    short clear_code;
    short eof_code;
    /* used by Encode */
    short max_clear_count;
    UBYTE *block_start;
    /* used by read_code */
    short bytes_unread;
    long  old_temp;
    short old_bits;
};

const short mask[12] =
{
    0x001, 0x003, 0x007, 0x00F,
    0x01F, 0x03F, 0x07F, 0x0FF,
    0x1FF, 0x3FF, 0x7FF, 0xFFF
};


static void init_table(struct codecvars *d)
{
    d->code_size = d->min_code_size + 1;
    d->clear_code = 1 << d->min_code_size;
    d->eof_code = d->clear_code + 1;
}

/**************************************************************************************************/

static short read_code(GifHandleType *gifhandle, struct codecvars *d)
{
    short temp_bits, byte, bytes, ret;
    unsigned long temp;

    temp = d->old_temp;
    temp_bits = d->old_bits;
    bytes =  0;
    byte = 0;
    while (temp_bits < d->code_size)
    {
	if (d->bytes_unread == 0)
	{
	    /* Get the length of the next record. A zero-length record
	     * denotes "end of data".
	     */
	    if ( !(gifhandle->filebufbytes--) && !LoadGIF_FillBuf(gifhandle, 1) )
	    {
		D(bug("gif.datatype/read_code() --- buffer underrun 1\n"));
		return -2;
	    }
	    d->bytes_unread = *(gifhandle->filebufpos)++;
//          D(bug("gif --- end of record, new len %ld, bytes %ld\n", (long)(d->bytes_unread), (long)(gifhandle->filebufbytes)));
	    if (d->bytes_unread == 0)   /* end of data */
		return -1;
	}
	if ( !(gifhandle->filebufbytes--) && !LoadGIF_FillBuf(gifhandle, 1) )
	{
	    D(bug("gif.datatype/read_code() --- buffer underrun 2\n"));
	    return -3;
	}
	byte = *(gifhandle->filebufpos)++;
	d->bytes_unread--;
	bytes++;

	temp |= byte << temp_bits;
	temp_bits += 8;
    }

    ret = temp & mask[d->code_size - 1];
    
//  D(bug("gif --- bytes %d, byte x%x, bits %d->%d, csiz %d, temp x%lx->x%lx, ret x%x\n", bytes, byte, d->old_bits, temp_bits, d->code_size, d->old_temp, temp, ret));

    d->old_bits = temp_bits - d->code_size;
    d->old_temp = temp >> (d->code_size);
    return ret;
}

/**************************************************************************************************/
    
short DecodeInit(GifHandleType *gifhandle)
{
    struct codecvars *d;
    
    gifhandle->codecvars = d = (struct codecvars *) AllocVec( sizeof(struct codecvars), MEMF_ANY);
    if ( !d )   return -2;

    /* Get the minimum code size (2 to 8) */
    if ( !(gifhandle->filebufbytes--) && !LoadGIF_FillBuf(gifhandle, 1) )
    {
	D(bug("gif.datatype/DecodeInit() --- buffer underrun\n"));
	return -4;
    }
    d->min_code_size = *(gifhandle->filebufpos)++;

    if (d->min_code_size < 2 || d->min_code_size > 8)
	return -3;

    init_table(d);
    d->old_temp = 0;
    d->old_bits = 0;
    d->bytes_unread = 0;
    return TRUE;
}

short DecodeLines(GifHandleType *gifhandle)
{
    struct codecvars *d;
    short code;
    
    d = gifhandle->codecvars;
    
    while ((code = read_code(gifhandle, d)) != d->eof_code)
    {
	if (code < 0) return code;
	if (code != d->clear_code)
	{
	    if ( !(gifhandle->linebufbytes--) )
	    {
		D(bug("gif.datatype/DecodeLines() --- line buffer full\n"));
		return -5;
	    }
	    *(gifhandle->linebufpos)++ = (UBYTE)(code);
//          D(bug("gif -- pixel x%lx\n", (long)(code)));
	}
    }
    return TRUE;
}

short DecodeEnd(GifHandleType *gifhandle)
{
    short ret;

    ret = read_code(gifhandle, gifhandle->codecvars);
    if (ret != -1) return FALSE;
    return TRUE;
}

/**************************************************************************************************/

static short write_code(GifHandleType *gifhandle, struct codecvars *d, short code)
{
    short temp_bits, byte, bytes;
    unsigned long temp;

    temp_bits = d->old_bits;
    temp =  ( code << temp_bits ) | d->old_temp;
    temp_bits += d->code_size;
    bytes =  0;
    byte = 0;
    while (temp_bits >= 8)
    {
	if ( !d->bytes_unread-- )
	{
//          D(bug("gif --- end of record, bytes %ld, %ld\n", (long)(gifhandle->filebufsize-gifhandle->filebufbytes), (long)(gifhandle->filebufpos-gifhandle->filebuf)));
	    if ( (gifhandle->filebufbytes -= (BLOCKLEN+1)) < 0 && !SaveGIF_EmptyBuf(gifhandle, (BLOCKLEN+1)) )
	    {
		return -2;
	    }
	    d->block_start = gifhandle->filebufpos;
	    *(gifhandle->filebufpos)++ = BLOCKLEN;
	    d->bytes_unread = BLOCKLEN - 1;
	}
	*(gifhandle->filebufpos)++ = temp & 0xff;
	temp = temp >> 8;
	temp_bits -= 8;
	bytes++;
    }
    
//  D(bug("gif --- bytes %d, byte x%x, bits %d->%d, csiz %d, temp x%lx->x%lx, ret x%x\n", bytes, byte, d->old_bits, temp_bits, d->code_size, d->old_temp, temp, ret));

    d->old_bits = temp_bits;
    d->old_temp = temp;
    return 0;
}

/**************************************************************************************************/

short EncodeInit(GifHandleType *gifhandle, short numplanes)
{
    struct codecvars *d;
    
    gifhandle->codecvars = d = (struct codecvars *) AllocVec( sizeof(struct codecvars), MEMF_ANY);
    if ( !d )   return -2;

    /* Write the minimum code size (2 to 8) */
    d->min_code_size = numplanes;
    if ( numplanes == 1 )
	d->min_code_size = 2;
    if ( !(gifhandle->filebufbytes--) && !SaveGIF_EmptyBuf(gifhandle, 1) )
    {
	D(bug("gif.datatype/DecompressInit() --- buffer overrun\n"));
	return -4;
    }
    *(gifhandle->filebufpos)++ = d->min_code_size;

    init_table(d);
    d->max_clear_count = (1 << d->min_code_size) - 3;
    d->old_temp = 0;
    d->old_bits = 0;
    d->bytes_unread = 0;            /* record */
    return TRUE;
}

short EncodeLines(GifHandleType *gifhandle)
{
    struct codecvars *d;
    short clear_count;
    
    d = gifhandle->codecvars;
    clear_count = 0;
    while (gifhandle->linebufbytes--)
    {
	if ( !clear_count-- )
	{
	    clear_count = d->max_clear_count;
	    write_code (gifhandle, d, d->clear_code);
	}
	write_code (gifhandle, d, *(gifhandle->linebufpos)++);
    }
    return TRUE;
}

short EncodeEnd(GifHandleType *gifhandle)
{
    struct codecvars *d;
    
    d = gifhandle->codecvars;
    write_code (gifhandle, d, d->eof_code);
    if ( d->old_bits )
    {
	d->code_size = 8 - d->old_bits;
	write_code (gifhandle, d, 0);   /* flush remaining bits */
    }
    *(d->block_start) = BLOCKLEN - d->bytes_unread; /* correct last block length... */
    gifhandle->filebufbytes += d->bytes_unread; /* and number of bytes to write */
    D(bug("gif --- end, bytes %ld, %ld, ubytes %ld\n", (long)(gifhandle->filebufsize-gifhandle->filebufbytes), (long)(gifhandle->filebufpos-gifhandle->filebuf), (long)(BLOCKLEN - d->bytes_unread)));
    if ( !gifhandle->filebufbytes-- && !SaveGIF_EmptyBuf(gifhandle, 1) )
    {
	return FALSE;
    }
    *(gifhandle->filebufpos)++ = 0; /* zero block length as termination */
    return TRUE;
}
