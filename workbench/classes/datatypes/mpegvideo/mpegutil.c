
/*
**
**  $VER: mpegutil.c 1.11 (1.11.97)
**  mpegvideo.datatype 1.11
**
**  mpeg utility functions
**
**  Written 1996/1997 by Roland 'Gizzy' Mainz
**
*/

#ifndef DEBUG
#   define DEBUG 0
#endif
#include <aros/debug.h>

/* ansi includes */
#include <stdlib.h>
#include <string.h>
#include <limits.h>

/* project includes */
#include "mpegvideo.h"
#include "mpegproto.h"
#include "mpegutil.h"

/* Bit masks used by bit i/o operations. */
const
ULONG nBitMask[] =
{
    0x00000000, 0x80000000, 0xc0000000, 0xe0000000,
    0xf0000000, 0xf8000000, 0xfc000000, 0xfe000000,
    0xff000000, 0xff800000, 0xffc00000, 0xffe00000,
    0xfff00000, 0xfff80000, 0xfffc0000, 0xfffe0000,
    0xffff0000, 0xffff8000, 0xffffc000, 0xffffe000,
    0xfffff000, 0xfffff800, 0xfffffc00, 0xfffffe00,
    0xffffff00, 0xffffff80, 0xffffffc0, 0xffffffe0,
    0xfffffff0, 0xfffffff8, 0xfffffffc, 0xfffffffe,
    0xffffffff
};


const
ULONG bitMask[] =
{
    0xffffffff, 0x7fffffff, 0x3fffffff, 0x1fffffff,
    0x0fffffff, 0x07ffffff, 0x03ffffff, 0x01ffffff,
    0x00ffffff, 0x007fffff, 0x003fffff, 0x001fffff,
    0x000fffff, 0x0007ffff, 0x0003ffff, 0x0001ffff,
    0x0000ffff, 0x00007fff, 0x00003fff, 0x00001fff,
    0x00000fff, 0x000007ff, 0x000003ff, 0x000001ff,
    0x000000ff, 0x0000007f, 0x0000003f, 0x0000001f,
    0x0000000f, 0x00000007, 0x00000003, 0x00000001,
    0x00000000
};


const
ULONG rBitMask[] =
{
    0xffffffff, 0xfffffffe, 0xfffffffc, 0xfffffff8,
    0xfffffff0, 0xffffffe0, 0xffffffc0, 0xffffff80,
    0xffffff00, 0xfffffe00, 0xfffffc00, 0xfffff800,
    0xfffff000, 0xffffe000, 0xffffc000, 0xffff8000,
    0xffff0000, 0xfffe0000, 0xfffc0000, 0xfff80000,
    0xfff00000, 0xffe00000, 0xffc00000, 0xff800000,
    0xff000000, 0xfe000000, 0xfc000000, 0xf8000000,
    0xf0000000, 0xe0000000, 0xc0000000, 0x80000000,
    0x00000000
};


const
ULONG bitTest[] =
{
    0x80000000, 0x40000000, 0x20000000, 0x10000000,
    0x08000000, 0x04000000, 0x02000000, 0x01000000,
    0x00800000, 0x00400000, 0x00200000, 0x00100000,
    0x00080000, 0x00040000, 0x00020000, 0x00010000,
    0x00008000, 0x00004000, 0x00002000, 0x00001000,
    0x00000800, 0x00000400, 0x00000200, 0x00000100,
    0x00000080, 0x00000040, 0x00000020, 0x00000010,
    0x00000008, 0x00000004, 0x00000002, 0x00000001,
    0x00000000
};


/*
 *--------------------------------------------------------------
 *
 * correct_underflow --
 *
 *    Called when buffer does not have sufficient data to
 *      satisfy request for bits.
 *      Calls get_more_data, an application specific routine
 *      required to fill the buffer with more data.
 *
 * Results:
 *      None really.
 *
 * Side effects:
 *    buf_length and buffer fields in curVidStream structure
 *      may be changed.
 *
 *--------------------------------------------------------------
 */


void correct_underflow( struct MPEGVideoInstData *mvid )
{
    int status;

    D(bug("[mpegvideo.datatype] %s()\n", __func__));

    status = get_more_data( mvid, (curVidStream -> buf_start), (curVidStream -> max_buf_length), (int *)(&bufLength), (unsigned int **)(&bitBuffer) );

    if( status < 0 )
    {
      error_printf( mvid, "Unexpected read error (status = %x).\n", status);

      myexit( mvid, RETURN_ERROR, ERROR_SEEK_ERROR );
    }
    else
    {
      if( (status == 0) && (bufLength < 1) )
      {
        error_printf( mvid, "\nImproper or missing sequence end code. Done ! (status=%x, bufLength = %d)\n", status, bufLength );

        myexit( mvid, RETURN_WARN, ERROR_SEEK_ERROR );
      }
    }

    curBits = *bitBuffer << bitOffset;
}


/*
 *--------------------------------------------------------------
 *
 * next_bits --
 *
 *    Compares next num bits to low order position in mask.
 *      Buffer pointer is NOT advanced.
 *
 * Results:
 *    TRUE, FALSE, or error code.
 *
 * Side effects:
 *    None.
 *
 *--------------------------------------------------------------
 */


int next_bits( struct MPEGVideoInstData *mvid, int num, unsigned int mask )
{
    unsigned int stream;

    /* Get next num bits, no buffer pointer advance. */
    show_bitsn( num, stream );

    /* Compare bit stream and mask. Set return value to TRUE if equal, FALSE if differs. */
    return( (int)(mask == stream) );
}


/*
 *--------------------------------------------------------------
 *
 * get_ext_data --
 *
 *    Assumes that bit stream is at begining of extension
 *      data. Parses off extension data into dynamically
 *      allocated space until start code is hit.
 *
 * Results:
 *    Pointer to dynamically allocated memory containing
 *      extension data.
 *
 * Side effects:
 *    Bit stream irreversibly parsed.
 *
 *--------------------------------------------------------------
 */


char *get_ext_data( struct MPEGVideoInstData *mvid )
{
    int           size,
                  marker;
    char         *dataPtr;
    unsigned int  data;

    D(bug("[mpegvideo.datatype] %s()\n", __func__));

    /* Set initial ext data buffer size. */
    size = EXT_BUF_SIZE;

    /* Allocate ext data buffer. */
    dataPtr = (char *)mymalloc( mvid, size );

    /* Initialize marker to keep place in ext data buffer. */
    marker = 0;

    /* While next data is not start code... */
    while( !next_bits( mvid, 24, 0x000001 ) )
    {
      /* Get next byte of ext data. */
      get_bits8( data );

      /* Put ext data into ext data buffer. Advance marker. */
      dataPtr[ marker ] = (char)data;
      marker++;

      /* If end of ext data buffer reached, resize data buffer. */
      if( marker == size )
      {
        size += EXT_BUF_SIZE;
        dataPtr = (char *)myrealloc( mvid, dataPtr, size );
      }
    }

    /* Realloc data buffer to free any extra space. */
    dataPtr = (char *)myrealloc( mvid, dataPtr, marker );

    /* Return pointer to ext data buffer. */
    return( dataPtr );
}


/*
 *--------------------------------------------------------------
 *
 * next_start_code --
 *
 *    Parses off bitstream until start code reached. When done
 *      next 4 bytes of bitstream will be start code. Bit offset
 *      reset to 0.
 *
 * Results:
 *    Status code.
 *
 * Side effects:
 *    Bit stream irreversibly parsed.
 *
 *--------------------------------------------------------------
 */


int next_start_code( struct MPEGVideoInstData *mvid )
{
  int          byteoff;
  unsigned int data;
  UWORD        state;

    D(bug("[mpegvideo.datatype] %s()\n", __func__));

  /* If insufficient buffer length, correct underflow. */
  if( bufLength < SSP )
  {
    correct_underflow( mvid );
  }

  /* If bit offset not zero, reset and advance buffer pointer. */
  byteoff = bitOffset % CHAR_BIT;

  if( byteoff != 0 )
  {
    /* Align to byte boundary */
    flush_bits( (CHAR_BIT - byteoff) );
  }

  /* Set state = 0U. */
  state = 0U;

  /* While buffer has data ... */
  while( bufLength > 0 )
  {
    /* If insufficient data exists, correct underflow. */
    if( bufLength < SSP )
    {
      correct_underflow( mvid );
    }

    /* If next byte is zero... */
    get_bits8( data );

    if( data == 0 )
    {
      /* If state < 2U, advance state. */
      if( state < 2U )
      {
        state++;
      }
    }
    /* If next byte is one... */
    else
    {
      if( data == 1 )
      {
        /* If (state == 2U), advance state (state = 3U) (i.e. start code found),
         * otherwise, reset state to zero.
         */
        state = (state == 2U)?(3U):(0U);
      }
      else
      {
        /* Otherwise byte is neither 1 or 0, reset state to 0U. */
        state = 0U;
      }
    }

    /* If state == 3U (i.e. start code found)... */
    if( state == 3U )
    {
      /* Set buffer pointer back and reset length & bit offsets so next bytes will be beginning of start code. */
      bitOffset = bitOffset - 24;

      if( bitOffset < 0 )
      {
        bitOffset = 32 + bitOffset;
        bufLength++;
        bitBuffer--;
      }

      curBits = *bitBuffer << bitOffset;

      D(bug("[mpegvideo.datatype] %s: ok\n", __func__));
      /* Return success. */
      return( OK );
    }
  }

  D(bug("[mpegvideo.datatype] %s: underflow\n", __func__));
  /* Return underflow error. */
  return( UNDERFLOW );
}


/*
 *--------------------------------------------------------------
 *
 * get_extra_bit_info --
 *
 *    Parses off extra bit info stream into dynamically
 *      allocated memory. Extra bit info is indicated by
 *      a flag bit set to 1, followed by 8 bits of data.
 *      This continues until the flag bit is zero. Assumes
 *      that bit stream set to first flag bit in extra
 *      bit info stream.
 *
 * Results:
 *    Pointer to dynamically allocated memory with extra
 *      bit info in it. Flag bits are NOT included.
 *
 * Side effects:
 *    Bit stream irreversibly parsed.
 *
 *--------------------------------------------------------------
 */


char *get_extra_bit_info( struct MPEGVideoInstData *mvid )
{
    int           size,
                  marker;
    char         *dataPtr;
    unsigned int  data;

    D(bug("[mpegvideo.datatype] %s()\n", __func__));

    /* Get first flag bit. */
    get_bits1( data );

    /* If flag is false, return NULL pointer (i.e. no extra bit info). */
    if( !data )
    {
      return( NULL );
    }

    /* Initialize size of extra bit info buffer and allocate. */
    size    = EXT_BUF_SIZE;
    dataPtr = (char *)mymalloc( mvid, size );

    /* Reset marker to hold place in buffer. */
    marker = 0;

    /* While flag bit is true. */
    while( data )
    {
      /* Get next 8 bits of data. */
      get_bits8( data );

      /* Place in extra bit info buffer. */
      dataPtr[ marker ] = (char)data;
      marker++;

      /* If buffer is full, reallocate. */
      if( marker == size )
      {
        size += EXT_BUF_SIZE;
        dataPtr = (char *)myrealloc( mvid, dataPtr, size );
      }

      /* Get next flag bit. */
      get_bits1( data );
    }

    /* Reallocate buffer to free extra space. */
    dataPtr = (char *)myrealloc( mvid, dataPtr, marker );

    /* Return pointer to extra bit info buffer. */
    return( dataPtr );
}


long stream_pos( struct MPEGVideoInstData *mvid )
{
    return( (long)(((mvid -> mvid_ReadMarkPos) + ((UBYTE *)bitBuffer - (mvid -> mvid_ReadMark))) + (bitOffset / CHAR_BIT)) );
}

