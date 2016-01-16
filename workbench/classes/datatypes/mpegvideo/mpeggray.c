
/*
**
**  $VER: mpeggray.c 1.4 (9.11.96)
**  mpegvideo.datatype 1.0
**
**  grayscale dithering
**
**  Written 1996 by Roland 'Gizzy' Mainz
**
*/

/* project includes */
#include "mpegmyassert.h"
#include "mpegvideo.h"
#include "mpegproto.h"


/*
 *--------------------------------------------------------------
 *
 * GrayDitherImage --
 *
 *    Dithers image into 128 gray scales. Simply maps luminance
 *      value into 1 of 128 gray scale colors (divide by two, essentially).
 *
 * Results:
 *    None.
 *
 * Side effects:
 *    None.
 *
 *--------------------------------------------------------------
 */


void GrayDitherImage( struct MPEGVideoInstData *mvid, UBYTE *lum, UBYTE *cr, UBYTE *cb, UBYTE *out, UWORD h, UWORD w )
{
    ULONG i,
          max = (w * h) / 16;

    for( i = 0UL ; i < max ; i++ )
    {
      out[  0 ] = mappixel[ lum[  0 ] ];
      out[  1 ] = mappixel[ lum[  1 ] ];
      out[  2 ] = mappixel[ lum[  2 ] ];
      out[  3 ] = mappixel[ lum[  3 ] ];
      out[  4 ] = mappixel[ lum[  4 ] ];
      out[  5 ] = mappixel[ lum[  5 ] ];
      out[  6 ] = mappixel[ lum[  6 ] ];
      out[  7 ] = mappixel[ lum[  7 ] ];
      out[  8 ] = mappixel[ lum[  8 ] ];
      out[  9 ] = mappixel[ lum[  9 ] ];
      out[ 10 ] = mappixel[ lum[ 10 ] ];
      out[ 11 ] = mappixel[ lum[ 11 ] ];
      out[ 12 ] = mappixel[ lum[ 12 ] ];
      out[ 13 ] = mappixel[ lum[ 13 ] ];
      out[ 14 ] = mappixel[ lum[ 14 ] ];
      out[ 15 ] = mappixel[ lum[ 15 ] ];

      out += 16;
      lum += 16;
    }
}

