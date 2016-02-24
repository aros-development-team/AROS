
/*
**
**  $VER: mpegparseblock.c 1.8 (27.5.97)
**  mpegvideo.datatype 1.8
**
**  block parsing
**
**  Written 1996/1997 by Roland 'Gizzy' Mainz
**
*/

/*
#define NO_SANITY_CHECKS 1
*/

/* ansi includes */
#include <stdlib.h>
#include <string.h>

/* project includes */
#include "mpegmyassert.h"
#include "mpegvideo.h"
#include "mpegproto.h"
#include "mpegdecoders.h"

/* External declarations. */
extern const UBYTE zigzag_direct[ 64 ];

/*
 *--------------------------------------------------------------
 *
 * ParseReconBlock --
 *
 *    Parse values for block structure from bitstream.
 *      n is an indication of the position of the block within
 *      the macroblock (i.e. 0-5) and indicates the type of
 *      block (i.e. luminance or chrominance). Reconstructs
 *      coefficients from values parsed and puts in
 *      block.dct_recon array in vid stream structure.
 *      sparseFlag is set when the block contains only one
 *      coeffictient and is used by the IDCT.
 *
 * Results:
 *
 *
 * Side effects:
 *      Bit stream irreversibly parsed.
 *
 *--------------------------------------------------------------
 */


#define DECODE_DCT_COEFF_FIRST DecodeDCTCoeffFirst
#define DECODE_DCT_COEFF_NEXT  DecodeDCTCoeffNext


void ParseReconBlock( struct MPEGVideoInstData *mvid, int n )
{
  Block *blockPtr = (&(curVidStream -> block));
  int    coeffCount = 0;

  if( bufLength < (100 * SSP) )
  {
    correct_underflow( mvid );
  }

  {
    int         diff;
    int         size,
                level,
                i,
                run,
                pos,
                coeff;
    short int  *reconptr;
    UBYTE      *iqmatrixptr,
               *niqmatrixptr;
    int         qscale;

    reconptr = (blockPtr -> dct_recon)[ 0 ];

    /* Clear dct coeff matrix. */
    memset( (void *)(blockPtr -> dct_recon), 0, sizeof( (blockPtr -> dct_recon) ) );

    if( curVidStream -> mblock . mb_intra )
    {
      if( n < 4 )
      {
        /*
         * Get the luminance bits.  This code has been hand optimized to
         * get by the normal bit parsing routines.  We get some speedup
         * by grabbing the next 16 bits and parsing things locally.
         * Thus, calls are translated as:
         *
         *    show_bitsX  <-->   next16bits >> (16-X)
         *    get_bitsX   <-->   val = next16bits >> (16-flushed-X);
         *               flushed += X;
         *               next16bits &= bitMask[flushed];
         *    flush_bitsX <-->   flushed += X;
         *               next16bits &= bitMask[flushed];
         *
         * I've streamlined the code a lot, so that we don't have to mask
         * out the low order bits and a few of the extra adds are removed.
         *    bsmith
         */
        unsigned int next16bits,
                     index,
                     flushed;

        show_bits16( next16bits );
        index = next16bits >> (16 - 7);
        size    = dct_dc_size_luminance[ index ] . value;
        flushed = dct_dc_size_luminance[ index ] . num_bits;
        next16bits &= bitMask[ 16 + flushed ];

        if( size != 0 )
        {
          flushed += size;
          diff = next16bits >> (16 - flushed);

          if( !(diff & bitTest[ 32 - size ]) )
          {
            diff = rBitMask[ size ] | (diff + 1);
          }
        }
        else
        {
          diff = 0;
        }

        flush_bits( flushed );

        if( n == 0 )
        {
          coeff = diff << 3;

          if( ((curVidStream -> mblock . mb_address) - (curVidStream -> mblock . past_intra_addr)) > 1 )
          {
            coeff += 1024;
          }
          else
          {
            coeff += (blockPtr -> dct_dc_y_past);
          }

          (blockPtr -> dct_dc_y_past) = coeff;
        }
        else
        {
          coeff = (blockPtr -> dct_dc_y_past) + (diff << 3);
          (blockPtr -> dct_dc_y_past) = coeff;
        }
      }
      else
      {
        /*
         * Get the chrominance bits.  This code has been hand optimized to
         * as described above
         */
        unsigned int next16bits,
                     index,
                     flushed;

        show_bits16( next16bits );
        index = next16bits >> (16 - 8);
        size    = dct_dc_size_chrominance[ index ] . value;
        flushed = dct_dc_size_chrominance[ index ] . num_bits;
        next16bits &= bitMask[ 16 + flushed ];

        if( size != 0 )
        {
          flushed += size;
          diff = next16bits >> (16 - flushed);

          if (!(diff & bitTest[ 32 - size ]))
          {
            diff = rBitMask[ size ] | (diff + 1);
          }
        }
        else
        {
          diff = 0;
        }

        flush_bits( flushed );

        if( n == 4 )
        {
          coeff = diff << 3;

          if( curVidStream -> mblock . mb_address - curVidStream -> mblock . past_intra_addr > 1 )
          {
            coeff += 1024;
          }
          else
          {
            coeff += (blockPtr -> dct_dc_cr_past);
          }

          (blockPtr -> dct_dc_cr_past) = coeff;
        }
        else
        {
          coeff = diff << 3;

          if( curVidStream -> mblock . mb_address - curVidStream -> mblock . past_intra_addr > 1 )
          {
            coeff += 1024;
          }
          else
          {
            coeff += (blockPtr -> dct_dc_cb_past);
          }

          (blockPtr -> dct_dc_cb_past) = coeff;
        }
      }

      *reconptr = coeff;
      i   = 0;
      pos = 0;
      coeffCount = (coeff != 0);

      if( (curVidStream -> picture . code_type) != D_TYPE )
      {
        qscale      = curVidStream -> slice . quant_scale;
        iqmatrixptr = curVidStream -> intra_quant_matrix[ 0 ];

        while( 1 )
        {
          DECODE_DCT_COEFF_NEXT( run, level );

          if( run == END_OF_BLOCK )
            break;

          assert( run != ESCAPE );
          assert( run != END_OF_BLOCK );

          i = i + run + 1;

          assert( (i < 64) && (i >= 0) );
          i = ABS( i ) % 64;
          pos = zigzag_direct[ i ];

          coeff = (level * qscale * ((int)iqmatrixptr[ pos ])) >> 3;

          if( level < 0 )
          {
            coeff += (coeff & 1);
          }
          else
          {
            coeff -= (coeff & 1);
          }

          reconptr[ pos ] = coeff;

          if( coeff )
          {
            coeffCount++;
          }
        }

        flush_bits( 2 );

        goto end;
      }
    }
    else
    {
      niqmatrixptr = curVidStream -> non_intra_quant_matrix[ 0 ];
      qscale       = curVidStream -> slice . quant_scale;

      DECODE_DCT_COEFF_FIRST( run, level );

/*
      assert( run != ESCAPE );
      assert( run != END_OF_BLOCK );
*/

      i = run;

      assert( (i < 64) && (i >= 0) );
      i = ABS( i ) % 64;
      pos = zigzag_direct[ i ];

      if( level < 0 )
      {
        coeff = (((level << 1) - 1) * qscale * ((int)(niqmatrixptr[ pos ]))) >> 4;
        coeff += (coeff & 1);
      }
      else
      {
        coeff = (((level << 1) + 1) * qscale * ((int)(*(niqmatrixptr + pos)))) >> 4;
        coeff -= (coeff & 1);
      }

      reconptr[ pos ] = coeff;

      if( coeff )
      {
        coeffCount = 1;
      }

      if( curVidStream -> picture . code_type != D_TYPE )
      {
        while( 1 )
        {
          DECODE_DCT_COEFF_NEXT( run, level );

          if( run == END_OF_BLOCK )
            break;

#ifdef COMMENTED_OUT
          assert( run != ESCAPE );
#endif /* COMMENTED_OUT */

          i = i + run + 1;

          assert( (i < 64) && (i >= 0) );
          i = ABS( i ) % 64;
          pos = zigzag_direct[ i ];

          if( level < 0 )
          {
            coeff = (((level << 1) - 1) * qscale * ((int)(niqmatrixptr[ pos ]))) >> 4;
            coeff += (coeff & 1);
          }
          else
          {
            coeff = (((level << 1) + 1) * qscale * ((int)(*(niqmatrixptr + pos)))) >> 4;
            coeff -= (coeff & 1);
          }

          reconptr[ pos ] = coeff;

          if( coeff )
          {
            coeffCount++;
          }
        }

        flush_bits( 2 );

        goto end;
      }
    }

end:

    if( coeffCount == 1 )
    {
      j_rev_dct_sparse( mvid, reconptr, pos );
    }
    else
    {
#ifdef FLOATDCT
      if( mvid -> mvid_Quality )
      {
        float_idct( reconptr );
      } 
      else
#endif
      {
        j_rev_dct( reconptr );
      }
    }
  }
}


/*
 *--------------------------------------------------------------
 *
 * ParseAwayBlock --
 *
 *    Parses off block values, throwing them away.
 *      Used with grayscale dithering.
 *
 * Results:
 *    None.
 *
 * Side effects:
 *      None.
 *
 *--------------------------------------------------------------
 */


void ParseAwayBlock( struct MPEGVideoInstData *mvid, int n )
{
  unsigned int __unused diff;
  unsigned int size,
               run;
  int          level;

  if( bufLength < (100 * SSP) )
  {
    correct_underflow(mvid);
  }

  if( curVidStream -> mblock . mb_intra )
  {
    /* If the block is a luminance block... */
    if( n < 4 )
    {
      /* Parse and decode size of first coefficient. */
      DecodeDCTDCSizeLum( size );

      /* Parse first coefficient. */
      if( size != 0 )
      {
        get_bitsn( size, diff );
      }
    }
    else /* Otherwise, block is chrominance block... */
    {
      /* Parse and decode size of first coefficient. */
      DecodeDCTDCSizeChrom( size );

      /* Parse first coefficient. */
      if( size != 0 )
      {
        get_bitsn( size, diff );
      }
    }
  }
  else /* Otherwise, block is not intracoded... */
  {
    /* Decode and set first coefficient. */
    DECODE_DCT_COEFF_FIRST( run, level );

    assert( run != ESCAPE );
    assert( run != END_OF_BLOCK );
  }

  /* If picture is not D type (i.e. I, P, or B)... */
  if( curVidStream -> picture . code_type != D_TYPE )
  {
    /* While end of macroblock has not been reached... */
    while( 1 )
    {
      /* Get the dct_coeff_next */
      DECODE_DCT_COEFF_NEXT( run, level );

      if( run == END_OF_BLOCK )
        break;

      assert( run != ESCAPE );
      assert( run != END_OF_BLOCK );
    }

    /* End_of_block */
    flush_bits( 2 );
  }
}


