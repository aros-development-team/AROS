
/*
**
**  $VER: mpeg16bit.c 1.11 (2.11.97)
**  mpegvideo.datatype 1.10
**
**  16 bit dither routines, got from mpeg_play 2.3
**
**  Written 1997 by Roland 'Gizzy' Mainz
**
*/

#ifndef DEBUG
#   define DEBUG 0
#endif
#include <aros/debug.h>

/* project includes */
#include "mpegvideo.h"
#include "mpegproto.h"

/* ansi includes */
#include <math.h>

/*
#define INTERPOLATE 1
*/

#define GAMMA_CORRECTION(x) ((int)(pow((x) / 255.0, 1.0 / gammaCorrect) * 255.0))
#define CHROMA_CORRECTION256(x) ((x) >= 128 \
                        ? 128 + MIN(127, (int)(((x) - 128.0) * chromaCorrect)) \
                        : 128 - MIN(128, (int)((128.0 - (x)) * chromaCorrect)))
#define CHROMA_CORRECTION128(x) ((x) >= 0 \
                        ? MIN(127,  (int)(((x) * chromaCorrect))) \
                        : MAX(-128, (int)(((x) * chromaCorrect))))
#define CHROMA_CORRECTION256D(x) ((x) >= 128 \
                        ? 128.0 + MIN(127.0, (((x) - 128.0) * chromaCorrect)) \
                        : 128.0 - MIN(128.0, (((128.0 - (x)) * chromaCorrect))))
#define CHROMA_CORRECTION128D(x) ((x) >= 0 \
                        ? MIN(127.0,  ((x) * chromaCorrect)) \
                        : MAX(-128.0, ((x) * chromaCorrect)))


/*
 * Erik Corry's multi-byte dither routines.
 *
 * The basic idea is that the Init generates all the necessary tables.
 * The tables incorporate the information about the layout of pixels
 * in the XImage, so that it should be able to cope with 15-bit, 16-bit
 * 24-bit (non-packed) and 32-bit (10-11 bits per color!) screens.
 * At present it cannot cope with 24-bit packed mode, since this involves
 * getting down to byte level again. It is assumed that the bits for each
 * color are contiguous in the longword.
 *
 * Writing to memory is done in shorts or ints. (Unfortunately, short is not
 * very fast on Alpha, so there is room for improvement here). There is no
 * dither time check for overflow - instead the tables have slack at
 * each end. This is likely to be faster than an 'if' test as many modern
 * architectures are really bad at ifs. Potentially, each '&&' causes a 
 * pipeline flush!
 *
 * There is no shifting and fixed point arithmetic, as I really doubt you
 * can see the difference, and it costs. This may be just my bias, since I
 * heard that Intel is really bad at shifting.
 */

/*
 * How many 1 bits are there in the longword.
 * Low performance, do not call often.
 */
static
int number_of_bits_set( unsigned long a )
{
    if( !a )
    {
      return( 0 );
    }

    if( a & 1 )
      return( (1 + number_of_bits_set( (a >> 1) )) );

    return( number_of_bits_set( (a >> 1) ) );
}


/*
 * How many 0 bits are there at least significant end of longword.
 * Low performance, do not call often.
 */
static
int free_bits_at_bottom( unsigned long a )
{
    /* assume char is 8 bits */
    if( !a )
    {
      return( sizeof( unsigned long ) * 8 );
    }

    if( ((long)a) & 1L )
    {
      return( 0 );
    }

    return( (1 + free_bits_at_bottom ( (a >> 1) )) );
}


/*
 *--------------------------------------------------------------
 *
 * InitColor16Dither --
 *
 *    To get rid of the multiply and other conversions in color
 *    dither, we use a lookup table.
 *
 * Results:
 *    None.
 *
 * Side effects:
 *    The lookup tables are initialized.
 *
 *--------------------------------------------------------------
 */

void InitColorDither( struct MPEGVideoInstData *mvid )
{
    unsigned long red_mask;
    unsigned long green_mask;
    unsigned long blue_mask;
    BOOL          thirty2;
    BOOL          gammaCorrectFlag  = (gammaCorrect  != 1.0);
    BOOL          chromaCorrectFlag = (chromaCorrect != 1.0);
    int           CR,
                  CB,
                  i;

    D(bug("[mpegvideo.datatype] %s()\n", __func__));

    /* set up pixel masks */
    switch( anim_depth )
    {
      case 32: /* XRGB, 8R, 8G, 8B */
      case 24: /* RGB, 8R, 8G, 8B */
      case 8:  /* HAM-8 */
      case 6:  /* HAM-6 */
      {
          thirty2 = TRUE;
          red_mask   = 0x000000FFUL;
          green_mask = 0x0000FF00UL;
          blue_mask  = 0x00FF0000UL;
      }
          break;

      case 16: /* 5R, 5G, 5B */
      {
          thirty2 = FALSE;
          red_mask   = 0x001FUL;
          green_mask = 0x07E0UL;
          blue_mask  = 0xF800UL;
      }
          break;

      default:
      {
          thirty2 = FALSE;
          red_mask   = 0UL;
          green_mask = 0UL;
          blue_mask  = 0UL;
          
          error_printf( mvid, "unsupported chunkypixel depth %d\n", anim_depth);
      }
          break;
    }

    if( L_tab == NULL )          L_tab          = (long *)mymalloc( mvid, 256 * sizeof( long ) );
    if( Cr_r_tab == NULL )       Cr_r_tab       = (long *)mymalloc( mvid, 256 * sizeof( long ) );
    if( Cr_g_tab == NULL )       Cr_g_tab       = (long *)mymalloc( mvid, 256 * sizeof( long ) );
    if( Cb_g_tab == NULL )       Cb_g_tab       = (long *)mymalloc( mvid, 256 * sizeof( long ) );
    if( Cb_b_tab == NULL )       Cb_b_tab       = (long *)mymalloc( mvid, 256 * sizeof( long ) );
    if( r_2_pix_alloc == NULL )  r_2_pix_alloc  = (long *)mymalloc( mvid, 768 * sizeof( long ) );
    if( g_2_pix_alloc == NULL )  g_2_pix_alloc  = (long *)mymalloc( mvid, 768 * sizeof( long ) );
    if( b_2_pix_alloc == NULL )  b_2_pix_alloc  = (long *)mymalloc( mvid, 768 * sizeof( long ) );

    for( i = 0 ; i < 256 ; i++ )
    {
      L_tab[ i ] = i;

      if( gammaCorrectFlag )
      {
        L_tab[ i ] = GAMMA_CORRECTION( i );
      }

      CB = CR = i;

      if( chromaCorrectFlag )
      {
        CB -= 128;
        CB = CHROMA_CORRECTION128( CB );
        CR -= 128;
        CR = CHROMA_CORRECTION128( CR );
      }
      else
      {
        CB -= 128; CR -= 128;
      }
/* was
 *    Cr_r_tab[i] =  1.596 * CR;
 *    Cr_g_tab[i] = -0.813 * CR;
 *    Cb_g_tab[i] = -0.391 * CB;
 *    Cb_b_tab[i] =  2.018 * CB;
 *  but they were just messed up.
 *  Then was (_Video Deymstified_):
 *    Cr_r_tab[i] =  1.366 * CR;
 *    Cr_g_tab[i] = -0.700 * CR;
 *    Cb_g_tab[i] = -0.334 * CB;
 *    Cb_b_tab[i] =  1.732 * CB;
 *  but really should be:
 *   (from ITU-R BT.470-2 System B, G and SMPTE 170M )
 */
      Cr_r_tab[ i ] =  (0.419 / 0.299) * CR;
      Cr_g_tab[ i ] = -(0.299 / 0.419) * CR;
      Cb_g_tab[ i ] = -(0.114 / 0.331) * CB;
      Cb_b_tab[ i ] =  (0.587 / 0.331) * CB;

/*
 *  though you could argue for:
 *    SMPTE 240M
 *      Cr_r_tab[i] =  (0.445/0.212) * CR;
 *      Cr_g_tab[i] = -(0.212/0.445) * CR;
 *      Cb_g_tab[i] = -(0.087/0.384) * CB;
 *      Cb_b_tab[i] =  (0.701/0.384) * CB;
 *    FCC
 *      Cr_r_tab[i] =  (0.421/0.30) * CR;
 *      Cr_g_tab[i] = -(0.30/0.421) * CR;
 *      Cb_g_tab[i] = -(0.11/0.331) * CB;
 *      Cb_b_tab[i] =  (0.59/0.331) * CB;
 *    ITU-R BT.709
 *      Cr_r_tab[i] =  (0.454/0.2125) * CR;
 *      Cr_g_tab[i] = -(0.2125/0.454) * CR;
 *      Cb_g_tab[i] = -(0.0721/0.386) * CB;
 *      Cb_b_tab[i] =  (0.7154/0.386) * CB;
 *
 */
    }

    /*
     * Set up entries 0-255 in rgb-to-pixel value tables.
     */
    for( i = 0 ; i < 256 ; i++ )
    {
      r_2_pix_alloc[ i + 256 ]   = i >> (8 - number_of_bits_set( red_mask ));
      r_2_pix_alloc[ i + 256 ] <<= free_bits_at_bottom( red_mask );
      g_2_pix_alloc[ i + 256 ]   = i >> (8 - number_of_bits_set( green_mask ));
      g_2_pix_alloc[ i + 256 ] <<= free_bits_at_bottom( green_mask );
      b_2_pix_alloc[ i + 256 ]   = i >> (8 - number_of_bits_set( blue_mask ));
      b_2_pix_alloc[ i + 256 ] <<= free_bits_at_bottom( blue_mask );
      /*
       * If we have 16-bit output depth, then we double the value
       * in the top word. This means that we can write out both
       * pixels in the pixel doubling mode with one op. It is
       * harmless in the normal case as storing a 32-bit value
       * through a short pointer will lose the top bits anyway.
       * A similar optimisation for Alpha for 64 bit has been
       * prepared for, but is not yet implemented.
       */
      if( !thirty2 )
      {
        r_2_pix_alloc[ i + 256 ] |= (r_2_pix_alloc[ i + 256 ]) << 16;
        g_2_pix_alloc[ i + 256 ] |= (g_2_pix_alloc[ i + 256 ]) << 16;
        b_2_pix_alloc[ i + 256 ] |= (b_2_pix_alloc[ i + 256 ]) << 16;
      }
    }

    /*
     * Spread out the values we have to the rest of the array so that
     * we do not need to check for overflow.
     */
    for( i = 0 ; i < 256 ; i++ )
    {
      r_2_pix_alloc[ i       ] = r_2_pix_alloc[ 256 ];
      r_2_pix_alloc[ i + 512 ] = r_2_pix_alloc[ 511 ];
      g_2_pix_alloc[ i       ] = g_2_pix_alloc[ 256 ];
      g_2_pix_alloc[ i + 512 ] = g_2_pix_alloc[ 511 ];
      b_2_pix_alloc[ i       ] = b_2_pix_alloc[ 256 ];
      b_2_pix_alloc[ i + 512 ] = b_2_pix_alloc[ 511 ];
    }

    r_2_pix = r_2_pix_alloc + 256;
    g_2_pix = g_2_pix_alloc + 256;
    b_2_pix = b_2_pix_alloc + 256;
}


/*
 *--------------------------------------------------------------
 *
 * Color16DitherImage --
 *
 *    Converts image into 16 bit color.
 *
 * Results:
 *    None.
 *
 * Side effects:
 *    None.
 *
 *--------------------------------------------------------------
 */

void Color16DitherImage( struct MPEGVideoInstData *mvid, UBYTE *lum, UBYTE *cr, UBYTE *cb, UBYTE *out, UWORD cols, UWORD rows )
{
    int             L, 
                    CR, 
                    CB;
    unsigned short *row1,
                   *row2;
    unsigned char  *lum2;
    UWORD           x,
                    y;
    int             cr_r;
    int             cr_g;
    int             cb_g;
    int             cb_b;
    int             cols_2 = cols/2;

    D(bug("[mpegvideo.datatype] %s()\n", __func__));

    row1 = (unsigned short *)out;
    row2 = row1 + cols_2 + cols_2;
    lum2 = lum  + cols_2 + cols_2;

    for( y = 0 ; y < rows ; y += 2 )
    {
      for( x = 0 ; x < cols_2 ; x++ )
      {
        int R, G, B;

        CR = *cr++;
        CB = *cb++;
        cr_r = Cr_r_tab[CR];
        cr_g = Cr_g_tab[CR];
        cb_g = Cb_g_tab[CB];
        cb_b = Cb_b_tab[CB];

            L = L_tab[(int) *lum++];

        R = L + cr_r;
        G = L + cr_g + cb_g;
        B = L + cb_b;

        *row1++ = (r_2_pix[R] | g_2_pix[G] | b_2_pix[B]);

#ifdef INTERPOLATE
        if(x != cols_2 - 1)
        {
          CR = (CR + *cr) >> 1;
          CB = (CB + *cb) >> 1;
          cr_r = Cr_r_tab[CR];
          cr_g = Cr_g_tab[CR];
          cb_g = Cb_g_tab[CB];
          cb_b = Cb_b_tab[CB];
        }
#endif

            L = L_tab[(int) *lum++];

        R = L + cr_r;
        G = L + cr_g + cb_g;
        B = L + cb_b;

        *row1++ = (r_2_pix[R] | g_2_pix[G] | b_2_pix[B]);

        /*
         * Now, do second row.
         */
#ifdef INTERPOLATE
        if(y != rows - 2) 
        {
          CR = (CR + *(cr + cols_2 - 1)) >> 1;
          CB = (CB + *(cb + cols_2 - 1)) >> 1;
          cr_r = Cr_r_tab[CR];
          cr_g = Cr_g_tab[CR];
          cb_g = Cb_g_tab[CB];
          cb_b = Cb_b_tab[CB];
        }
#endif

        L = L_tab[(int) *lum2++];
        R = L + cr_r;
        G = L + cr_g + cb_g;
        B = L + cb_b;

        *row2++ = (r_2_pix[R] | g_2_pix[G] | b_2_pix[B]);

        L = L_tab[(int) *lum2++];
        R = L + cr_r;
        G = L + cr_g + cb_g;
        B = L + cb_b;

        *row2++ = (r_2_pix[R] | g_2_pix[G] | b_2_pix[B]);
      }

      /*
       * These values are at the start of the next line, (due
       * to the ++'s above),but they need to be at the start
       * of the line after that.
       */
      lum += cols_2 + cols_2;
      lum2 += cols_2 + cols_2;
      row1 += cols_2 + cols_2;
      row2 += cols_2 + cols_2;
    }
}


/*
 *--------------------------------------------------------------
 *
 * Color32DitherImage --
 *
 *    Converts image into 32 bit color (or 24-bit non-packed).
 *
 * Results:
 *    None.
 *
 * Side effects:
 *    None.
 *
 *--------------------------------------------------------------
 */

/*
 * This is a copysoft version of the function above with ints instead
 * of shorts to cause a 4-byte pixel size
 */

void Color32DitherImage( struct MPEGVideoInstData *mvid, UBYTE *lum, UBYTE *cr, UBYTE *cb, UBYTE *out, UWORD cols, UWORD rows )
{
    int            L, CR, CB;
    unsigned int  *row1,
                  *row2;
    unsigned char *lum2;
    UWORD          x, 
                   y;
    int            cr_r;
    int            cr_g;
    int            cb_g;
    int            cb_b;
    int            cols_2 = cols / 2;

    D(bug("[mpegvideo.datatype] %s()\n", __func__));

    row1 = (unsigned int *)out;
    row2 = row1 + cols_2 + cols_2;
    lum2 = lum + cols_2 + cols_2;

    for( y = 0 ; y < rows ; y += 2 )
    {
      for( x = 0 ; x < cols_2 ; x++ )
      {
        int R, G, B;

        CR = *cr++;
        CB = *cb++;
        cr_r = Cr_r_tab[CR];
        cr_g = Cr_g_tab[CR];
        cb_g = Cb_g_tab[CB];
        cb_b = Cb_b_tab[CB];

            L = L_tab[(int) *lum++];

        R = L + cr_r;
        G = L + cr_g + cb_g;
        B = L + cb_b;

        *row1++ = (r_2_pix[R] | g_2_pix[G] | b_2_pix[B]);

#ifdef INTERPOLATE
        if(x != cols_2 - 1)
        {
          CR = (CR + *cr) >> 1;
          CB = (CB + *cb) >> 1;
          cr_r = Cr_r_tab[CR];
          cr_g = Cr_g_tab[CR];
          cb_g = Cb_g_tab[CB];
          cb_b = Cb_b_tab[CB];
        }
#endif

            L = L_tab[(int) *lum++];

        R = L + cr_r;
        G = L + cr_g + cb_g;
        B = L + cb_b;

        *row1++ = (r_2_pix[R] | g_2_pix[G] | b_2_pix[B]);

        /*
         * Now, do second row.
         */

#ifdef INTERPOLATE
        if(y != rows - 2) 
        {
          CR = (CR + *(cr + cols_2 - 1)) >> 1;
          CB = (CB + *(cb + cols_2 - 1)) >> 1;
          cr_r = Cr_r_tab[CR];
          cr_g = Cr_g_tab[CR];
          cb_g = Cb_g_tab[CB];
          cb_b = Cb_b_tab[CB];
        }
#endif

        L = L_tab [(int) *lum2++];
        R = L + cr_r;
        G = L + cr_g + cb_g;
        B = L + cb_b;

        *row2++ = (r_2_pix[R] | g_2_pix[G] | b_2_pix[B]);

        L = L_tab [(int) *lum2++];
        R = L + cr_r;
        G = L + cr_g + cb_g;
        B = L + cb_b;

        *row2++ = (r_2_pix[R] | g_2_pix[G] | b_2_pix[B]);
      }

      lum  += cols_2 + cols_2;
      lum2 += cols_2 + cols_2;
      row1 += cols_2 + cols_2;
      row2 += cols_2 + cols_2;
    }
}


#ifdef COMMENTED_OUT
/*
 * Erik Corry's pixel doubling routines for 15/16/24/32 bit screens.
 */


/*
 *--------------------------------------------------------------
 *
 * Twox2Color16DitherImage --
 *
 *    Converts image into 16 bit color at double size.
 *
 * Results:
 *    None.
 *
 * Side effects:
 *    None.
 *
 *--------------------------------------------------------------
 */

/*
 * In this function I make use of a nasty trick. The tables have the lower
 * 16 bits replicated in the upper 16. This means I can write ints and get
 * the horisontal doubling for free (almost).
 */

void Twox2Color16DitherImage( struct MPEGVideoInstData *mvid, UBYTE *lum, UBYTE *cr, UBYTE *cb, UBYTE *out, UWORD cols, UWORD rows )
{
    int            L,
                   CR,
                   CB;
    unsigned int  *row1 = (unsigned int *)out;
    unsigned int  *row2 = row1 + cols;
    unsigned int  *row3 = row2 + cols;
    unsigned int  *row4 = row3 + cols;
    unsigned char *lum2;
    UWORD          x,
                   y;
    int            cr_r;
    int            cr_g;
    int            cb_g;
    int            cb_b;
    int            cols_2 = cols/2;

    D(bug("[mpegvideo.datatype] %s()\n", __func__));

    lum2 = lum + cols_2 + cols_2;

    for( y = 0 ; y < rows ; y += 2 )
    {
      for( x = 0 ; x < cols_2 ; x++ )
      {
        int R, G, B;
        int t;

        CR = *cr++;
        CB = *cb++;
        cr_r = Cr_r_tab[CR];
        cr_g = Cr_g_tab[CR];
        cb_g = Cb_g_tab[CB];
        cb_b = Cb_b_tab[CB];

            L = L_tab[(int) *lum++];

        R = L + cr_r;
        G = L + cr_g + cb_g;
        B = L + cb_b;

        t = (r_2_pix[R] | g_2_pix[G] | b_2_pix[B]);
        row1[0] = t;
        row1++;
        row2[0] = t;
        row2++;

#ifdef INTERPOLATE
        if( x != cols_2 - 1 )
        {
          CR = (CR + *cr) >> 1;
          CB = (CB + *cb) >> 1;
          cr_r = Cr_r_tab[CR];
          cr_g = Cr_g_tab[CR];
          cb_g = Cb_g_tab[CB];
          cb_b = Cb_b_tab[CB];
        }
#endif
            L = L_tab[(int) *lum++];

        R = L + cr_r;
        G = L + cr_g + cb_g;
        B = L + cb_b;

        t = (r_2_pix[R] | g_2_pix[G] | b_2_pix[B]);
        row1[0] = t;
        row1++;
        row2[0] = t;
        row2++;

        /*
         * Now, do second row.
         */
#ifdef INTERPOLATE
        if(y != rows - 2) 
        {
          CR = (CR + *(cr + cols_2 - 1)) >> 1;
          CB = (CB + *(cb + cols_2 - 1)) >> 1;
          cr_r = Cr_r_tab[CR];
          cr_g = Cr_g_tab[CR];
          cb_g = Cb_g_tab[CB];
          cb_b = Cb_b_tab[CB];
        }
#endif
        L = L_tab[(int) *lum2++];
        R = L + cr_r;
        G = L + cr_g + cb_g;
        B = L + cb_b;

        t = (r_2_pix[R] | g_2_pix[G] | b_2_pix[B]);
        row3[0] = t;
        row3++;
        row4[0] = t;
        row4++;

        L = L_tab[(int) *lum2++];
        R = L + cr_r;
        G = L + cr_g + cb_g;
        B = L + cb_b;

        t = (r_2_pix[R] | g_2_pix[G] | b_2_pix[B]);
        row3[0] = t;
        row3++;
        row4[0] = t;
        row4++;
      }

      lum += cols_2 + cols_2;
      lum2 += cols_2 + cols_2;
      row1 += 6 * cols_2;
      row3 += 6 * cols_2;
      row2 += 6 * cols_2;
      row4 += 6 * cols_2;
    }
}


/*
 *--------------------------------------------------------------
 *
 * Twox2Color32 --
 *
 *    Converts image into 24/32 bit color.
 *
 * Results:
 *    None.
 *
 * Side effects:
 *    None.
 *
 *--------------------------------------------------------------
 */

#define ONE_TWO 2 /* used to skip positions on 32/64-bit architectures */

void Twox2Color32DitherImage( struct MPEGVideoInstData *mvid, UBYTE *lum, UBYTE *cr, UBYTE *cb, UBYTE *out, UWORD cols, UWORD rows )
{
    int            L, 
                   CR, 
                   CB;
    unsigned long *row1 = (unsigned long *)out;
    unsigned long *row2 = row1 + cols * ONE_TWO;
    unsigned long *row3 = row2 + cols * ONE_TWO;
    unsigned long *row4 = row3 + cols * ONE_TWO;
    unsigned char *lum2;
    UWORD          x, 
                   y;
    int            cr_r;
    int            cr_g;
    int            cb_g;
    int            cb_b;
    int            cols_2 = cols/2;

    D(bug("[mpegvideo.datatype] %s()\n", __func__));

    lum2 = lum + cols_2 + cols_2;

    for( y = 0 ; y < rows ; y += 2 )
    {
      for( x = 0 ; x < cols_2 ; x++ )
      {
        int R, G, B;
            long t;

        CR = *cr++;
        CB = *cb++;
        cr_r = Cr_r_tab[CR];
        cr_g = Cr_g_tab[CR];
        cb_g = Cb_g_tab[CB];
        cb_b = Cb_b_tab[CB];

            L = L_tab[ (int) *lum++];

        R = L + cr_r;
        G = L + cr_g + cb_g;
        B = L + cb_b;

        t = (r_2_pix[R] | g_2_pix[G] | b_2_pix[B]);
        row1[0] = t;
        row2[0] = t;
        row1 += ONE_TWO;
        row2 += ONE_TWO;

/* INTERPOLATE is now standard */
#ifdef INTERPOLATE
        if(x != cols_2 - 1) 
        {
          CR = (CR + *cr) >> 1;
          CB = (CB + *cb) >> 1;
          cr_r = Cr_r_tab[CR];
          cr_g = Cr_g_tab[CR];
          cb_g = Cb_g_tab[CB];
          cb_b = Cb_b_tab[CB];
        }
#endif
/* end INTERPOLATE */
            L = L_tab[ (int) *lum++];

        R = L + cr_r;
        G = L + cr_g + cb_g;
        B = L + cb_b;

        t = (r_2_pix[R] | g_2_pix[G] | b_2_pix[B]);
        row1[0] = t;
        row2[0] = t;
        row1 += ONE_TWO;
        row2 += ONE_TWO;

        /*
         * Now, do second row.
         */
/* INTERPOLATE is now standard */
#ifdef INTERPOLATE
        if(y != rows - 2) 
        {
          CR = (unsigned int) (CR + *(cr + cols_2 - 1)) >> 1;
          CB = (unsigned int) (CB + *(cb + cols_2 - 1)) >> 1;
          cr_r = Cr_r_tab[CR];
          cr_g = Cr_g_tab[CR];
          cb_g = Cb_g_tab[CB];
          cb_b = Cb_b_tab[CB];
        }
#endif
/* endif */
        L = L_tab[ (int) *lum2++];
        R = L + cr_r;
        G = L + cr_g + cb_g;
        B = L + cb_b;

        t = (r_2_pix[R] | g_2_pix[G] | b_2_pix[B]);
        row3[0] = t;
        row4[0] = t;
        row3 += ONE_TWO;
        row4 += ONE_TWO;

        L = L_tab[(int) *lum2++];
        R = L + cr_r;
        G = L + cr_g + cb_g;
        B = L + cb_b;

        t = (r_2_pix[R] | g_2_pix[G] | b_2_pix[B]);
        row3[0] = t;
        row4[0] = t;
        row3 += ONE_TWO;
        row4 += ONE_TWO;
      }

      lum += cols_2 + cols_2;
      lum2 += cols_2 + cols_2;

      row1 += ONE_TWO * 6 *cols_2;
      row3 += ONE_TWO * 6 *cols_2;
      row2 += ONE_TWO * 6 *cols_2;
      row4 += ONE_TWO * 6 *cols_2;
    }
}

#endif /* COMMENTED_OUT */


