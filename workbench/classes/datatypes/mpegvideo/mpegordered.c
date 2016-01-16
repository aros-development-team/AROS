
/*
**
**  $VER: mpegordered.c 1.8 (3.5.97)
**  mpegvideo.datatype 1.8
**
**  This file contains C code to implement an
**  ordered dither.
**
**  Written 1996/1997 by Roland 'Gizzy' Mainz
**
*/

/* project includes */
#include "mpegvideo.h"
#include "mpegproto.h"


/*
 *--------------------------------------------------------------
 *
 *  InitOrderedDither--
 *
 *    Structures intialized for ordered dithering.
 *
 * Results:
 *    None.
 *
 * Side effects:
 *    None.
 *
 *--------------------------------------------------------------
 */


void InitOrderedDither( struct MPEGVideoInstData *mvid )
{
  long   i,
         j,
         k,
         err_range,
         threshval;
  ULONG *lmark,
        *cmark;

  for( i = 0 ; i < DITH_SIZE ; i++ )
  {
    lmark = l_darrays[ i ] = (ULONG *)mymalloc( mvid, (256 * sizeof( ULONG )) );

    for( j = 0 ; j < lum_values[ 0 ] ; j++)
    {
      *lmark++ = 0;
    }

    for( j = 0 ; j < (LUM_RANGE - 1) ; j++ )
    {
      err_range = lum_values[ j + 1 ] - lum_values[ j ];
      threshval = ((i * err_range) / DITH_SIZE) + lum_values[ j ];

      for( k = lum_values[ j ] ; k < lum_values[ j + 1 ] ; k++ )
      {
        if( k > threshval )
          *lmark++ = ((j + 1) * (CR_RANGE * CB_RANGE));
        else
          *lmark++ = (j * (CR_RANGE * CB_RANGE));
      }
    }

    for( j = lum_values[ LUM_RANGE - 1 ] ; j < 256 ; j++ )
    {
      *lmark++ = (LUM_RANGE - 1) * (CR_RANGE * CB_RANGE);
    }
  }

  for( i = 0 ; i < DITH_SIZE ; i++ )
  {
    cmark = cr_darrays[ i ] = (ULONG *)mymalloc( mvid, (256 * sizeof( ULONG )) );

    for( j = 0 ; j < cr_values[ 0 ] ; j++ )
    {
      *cmark++ = 0;
    }

    for( j = 0 ; j < (CR_RANGE - 1) ; j++ )
    {
      err_range = cr_values[ j + 1 ] - cr_values[ j ];
      threshval = ((i * err_range) / DITH_SIZE) + cr_values[ j ];

      for( k = cr_values[ j ] ; k < cr_values[ j + 1 ] ; k++ )
      {
        if( k > threshval )
          *cmark++ = ((j + 1) * CB_RANGE);
        else
          *cmark++ = (j * CB_RANGE);
      }
    }

    for( j = cr_values[ CR_RANGE - 1 ] ; j < 256 ; j++ )
    {
      *cmark++ = (CR_RANGE - 1) * (CB_RANGE);
    }
  }

  for( i = 0 ; i < DITH_SIZE ; i++ )
  {
    cmark = cb_darrays[ i ] = (ULONG *)mymalloc( mvid, (256 * sizeof( ULONG )) );

    for( j = 0 ; j < cb_values[ 0 ] ; j++ )
    {
      *cmark++ = 0;
    }

    for( j = 0 ; j < (CB_RANGE - 1) ; j++ )
    {
      err_range = cb_values[ j + 1 ] - cb_values[ j ];
      threshval = ((i * err_range) / DITH_SIZE) + cb_values[ j ];

      for( k = cb_values[ j ] ; k < cb_values[ j + 1 ] ; k++ )
      {
        if( k > threshval )
          *cmark++ = j+1;
        else
          *cmark++ = j;
      }
    }

    for( j = cb_values[ CB_RANGE - 1 ] ; j < 256 ; j++ )
    {
      *cmark++ = CB_RANGE - 1;
    }
  }
}


/*
 *--------------------------------------------------------------
 *
 * OrderedDitherImage --
 *
 *  Dithers an image using an ordered dither.
 *  Assumptions made:
 *    1) The color space is allocated y:cr:cb = 8:4:4
 *    2) The spatial resolution of y:cr:cb is 4:1:1
 *      The channels are dithered based on the standard
 *      ordered dither pattern for a 4x4 area.
 *
 * Results:
 *   None.
 *
 * Side effects:
 *   None.
 *
 *--------------------------------------------------------------
 */


void OrderedDitherImage( struct MPEGVideoInstData *mvid, UBYTE *lum, UBYTE *cr, UBYTE *cb, UBYTE *out, UWORD h, UWORD w )
{
    UBYTE *l, *r, *b, *o1, *o2;
    UBYTE *l2;
    UBYTE  L, R, B;
    UWORD  i,
           j;

    l     = lum;
    l2    = lum + w;
    r     = cr;
    b     = cb;
    o1    = out;
    o2    = out + w;

    for( i = 0U ; i < h ; i += 4U )
    {
      for( j = 0U ; j < w ; j += 8U )
      {
        R = r[ 0 ]; B = b[ 0 ];

        L = l[  0 ]; o1[ 0 ] = mappixel[ (l_darrays[  0 ][ L ]   + cr_darrays[  0 ][ R ]  + cb_darrays[  0 ][ B ]) ];
        L = l[  1 ]; o1[ 1 ] = mappixel[ (l_darrays[  8 ][ L ]   + cr_darrays[  8 ][ R ]  + cb_darrays[  8 ][ B ]) ];
        L = l2[ 0 ]; o2[ 0 ] = mappixel[ (l_darrays[ 12 ][ L ]   + cr_darrays[ 12 ][ R ]  + cb_darrays[ 12 ][ B ]) ];
        L = l2[ 1 ]; o2[ 1 ] = mappixel[ (l_darrays[  4 ][ L ]   + cr_darrays[  4 ][ R ]  + cb_darrays[  4 ][ B ]) ];

        R = r[ 1 ]; B = b[ 1 ];

        L = l[  2 ]; o1[ 2 ] = mappixel[ (l_darrays[  2 ][ L ]  + cr_darrays[  2 ][ R ] + cb_darrays[  2 ][ B ]) ];
        L = l[  3 ]; o1[ 3 ] = mappixel[ (l_darrays[ 10 ][ L ]  + cr_darrays[ 10 ][ R ] + cb_darrays[ 10 ][ B ]) ];
        L = l2[ 2 ]; o2[ 2 ] = mappixel[ (l_darrays[ 14 ][ L ]  + cr_darrays[ 14 ][ R ] + cb_darrays[ 14 ][ B ]) ];
        L = l2[ 3 ]; o2[ 3 ] = mappixel[ (l_darrays[  6 ][ L ]  + cr_darrays[  6 ][ R ] + cb_darrays[  6 ][ B ]) ];

        R = r[ 2 ]; B = b[ 2 ];

        L = l[  4 ]; o1[ 4 ] = mappixel[ (l_darrays[  0 ][ L ]  + cr_darrays[  0 ][ R ] + cb_darrays[  0 ][ B ]) ];
        L = l[  5 ]; o1[ 5 ] = mappixel[ (l_darrays[  8 ][ L ]  + cr_darrays[  8 ][ R ] + cb_darrays[  8 ][ B ]) ];
        L = l2[ 4 ]; o2[ 4 ] = mappixel[ (l_darrays[ 12 ][ L ]  + cr_darrays[ 12 ][ R ] + cb_darrays[ 12 ][ B ]) ];
        L = l2[ 5 ]; o2[ 5 ] = mappixel[ (l_darrays[  4 ][ L ]  + cr_darrays[  4 ][ R ] + cb_darrays[  4 ][ B ]) ];

        R = r[ 3 ]; B = b[ 3 ];

        L = l[  6 ]; o1[ 6 ] = mappixel[ (l_darrays[  2 ][ L ]  + cr_darrays[  2 ][ R ] + cb_darrays[  2 ][ B ]) ];
        L = l[  7 ]; o1[ 7 ] = mappixel[ (l_darrays[ 10 ][ L ]  + cr_darrays[ 10 ][ R ] + cb_darrays[ 10 ][ B ]) ];
        L = l2[ 6 ]; o2[ 6 ] = mappixel[ (l_darrays[ 14 ][ L ]  + cr_darrays[ 14 ][ R ] + cb_darrays[ 14 ][ B ]) ];
        L = l2[ 7 ]; o2[ 7 ] = mappixel[ (l_darrays[  6 ][ L ]  + cr_darrays[  6 ][ R ] + cb_darrays[  6 ][ B ]) ];

        l  += 8;
        l2 += 8;
        r  += 4;
        b  += 4;
        o1 += 8;
        o2 += 8;
      }

      l  += w; l2 += w;
      o1 += w; o2 += w;

      for( j = 0 ; j < w ; j += 8 )
      {
        R = r[ 0 ]; B = b[ 0 ];

        L = l[  0 ]; o1[ 0 ] = mappixel[ (l_darrays[  3 ][ L ]  + cr_darrays[  3 ][ R ] + cb_darrays[  3 ][ B ]) ];
        L = l[  1 ]; o1[ 1 ] = mappixel[ (l_darrays[ 11 ][ L ]  + cr_darrays[ 11 ][ R ] + cb_darrays[ 11 ][ B ]) ];
        L = l2[ 0 ]; o2[ 0 ] = mappixel[ (l_darrays[ 15 ][ L ]  + cr_darrays[ 15 ][ R ] + cb_darrays[ 15 ][ B ]) ];
        L = l2[ 1 ]; o2[ 1 ] = mappixel[ (l_darrays[  7 ][ L ]  + cr_darrays[  7 ][ R ] + cb_darrays[  7 ][ B ]) ];

        R = r[ 1 ]; B = b[ 1 ];

        L = l[  2 ]; o1[ 2 ] = mappixel[ (l_darrays[  1 ][ L ]  + cr_darrays[  1 ][ R ] + cb_darrays[  1 ][ B ]) ];
        L = l[  3 ]; o1[ 3 ] = mappixel[ (l_darrays[  9 ][ L ]  + cr_darrays[  9 ][ R ] + cb_darrays[  9 ][ B ]) ];
        L = l2[ 2 ]; o2[ 2 ] = mappixel[ (l_darrays[ 13 ][ L ]  + cr_darrays[ 13 ][ R ] + cb_darrays[ 13 ][ B ]) ];
        L = l2[ 3 ]; o2[ 3 ] = mappixel[ (l_darrays[  5 ][ L ]  + cr_darrays[  5 ][ R ] + cb_darrays[  5 ][ B ]) ];

        R = r[ 2 ]; B = b[ 2 ];

        L = l[  4 ]; o1[ 4 ] = mappixel[ (l_darrays[  3 ][ L ]  + cr_darrays[  3 ][ R ] + cb_darrays[  3 ][ B ]) ];
        L = l[  5 ]; o1[ 5 ] = mappixel[ (l_darrays[ 11 ][ L ]  + cr_darrays[ 11 ][ R ] + cb_darrays[ 11 ][ B ]) ];
        L = l2[ 4 ]; o2[ 4 ] = mappixel[ (l_darrays[ 15 ][ L ]  + cr_darrays[ 15 ][ R ] + cb_darrays[ 15 ][ B ]) ];
        L = l2[ 5 ]; o2[ 5 ] = mappixel[ (l_darrays[  7 ][ L ]  + cr_darrays[  7 ][ R ] + cb_darrays[  7 ][ B ]) ];

        R = r[ 3 ]; B = b[ 3 ];

        L = l[  6 ]; o1[ 6 ] = mappixel[ (l_darrays[  1 ][ L ]  + cr_darrays[  1 ][ R ] + cb_darrays[  1 ][ B ]) ];
        L = l[  7 ]; o1[ 7 ] = mappixel[ (l_darrays[  9 ][ L ]  + cr_darrays[  9 ][ R ] + cb_darrays[  9 ][ B ]) ];
        L = l2[ 6 ]; o2[ 6 ] = mappixel[ (l_darrays[ 13 ][ L ]  + cr_darrays[ 13 ][ R ] + cb_darrays[ 13 ][ B ]) ];
        L = l2[ 7 ]; o2[ 7 ] = mappixel[ (l_darrays[  5 ][ L ]  + cr_darrays[  5 ][ R ] + cb_darrays[  5 ][ B ]) ];

        l  += 8;
        l2 += 8;
        r  += 4;
        b  += 4;
        o1 += 8;
        o2 += 8;
      }

      l  += w; l2 += w;
      o1 += w; o2 += w;
    }
}



