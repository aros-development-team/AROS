
/*
**
**  $VER: mpeggdith.c 1.11 (2.11.97)
**  mpegvideo.datatype 1.11
**
**  dithering
**
**  Written 1996/1997 by Roland 'Gizzy' Mainz
**
*/

/* amiga includes */
#include <exec/types.h>

/* ansi includes */
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* project includes */
#include "mpegmyassert.h"
#include "mpegvideo.h"
#include "mpegproto.h"

/* local prototypes */
static void ConvertColor( UBYTE, UBYTE, UBYTE, struct ColorRegister * );


/*
 *--------------------------------------------------------------
 *
 * InitColor --
 *
 *      Initialized lum, cr, and cb quantized range value arrays.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      None.
 *
 *--------------------------------------------------------------
 */


void InitColor( struct MPEGVideoInstData *mvid )
{
    ULONG i;

    for( i = 0UL ; i < LUM_RANGE ; i++ )
    {
      lum_values[ i ] = (((i * 255) / (LUM_RANGE)) + (255 / (LUM_RANGE * 2))) % 255UL;
    }

    for( i = 0UL ; i < CR_RANGE ; i++ )
    {
      cr_values[ i ] = (((i * 255) / (CR_RANGE)) + (255 / (CR_RANGE * 2))) % 255UL;
    }

    for( i = 0UL ; i < CB_RANGE ; i++ )
    {
      cb_values[ i ] = (((i * 255) / (CB_RANGE)) + (255 / (CB_RANGE * 2))) % 255UL;
    }
}


/*
 *--------------------------------------------------------------
 *
 * ConvertColor --
 *
 *    Given a l, cr, cb tuple, converts it to r,g,b.
 *
 * Results:
 *    r,g,b values returned in pointers passed as parameters.
 *
 * Side effects:
 *      None.
 *
 *--------------------------------------------------------------
 */


static
void ConvertColor( UBYTE l, UBYTE cr, UBYTE cb, struct ColorRegister *rgb )
{
    double fl, fcr, fcb, fr, fg, fb;

    fl  = (double)l;
    fcr = ((double)cr) - 128.0;
    fcb = ((double)cb) - 128.0;

    fr = fl + (1.40200 * fcb);
    fg = fl - (0.71414 * fcb) - (0.34414 * fcr);
    fb = fl + (1.77200 * fcr);

    /* Store colors */
    rgb -> red   = (UBYTE)(fr < 0.0)?(0.0):((fr > 255.0)?(255.0):(fr));
    rgb -> green = (UBYTE)(fg < 0.0)?(0.0):((fg > 255.0)?(255.0):(fg));
    rgb -> blue  = (UBYTE)(fb < 0.0)?(0.0):((fb > 255.0)?(255.0):(fb));
}


/*
 *--------------------------------------------------------------
 *
 * InitDisplay --
 *
 *    Initialized display, sets up colormap, etc.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      None.
 *
 *--------------------------------------------------------------
 */


void InitDisplay( struct MPEGVideoInstData *mvid )
{
    long                 ncolors = LUM_RANGE * CB_RANGE * CR_RANGE;
    long                 i,
                         lum_num,
                         cr_num,
                         cb_num;
    struct ColorRegister color;

    /* maxpixel will be set up dynamically */
    if( mvid -> mvid_PalettePerFrame )
      return;

    if( ncolors > (1UL << anim_depth) )
    {
      verbose_printf( mvid, "truncating colormap from %lu down to %lu colors\n", ncolors, (1UL << anim_depth) );
    }

retrycolors:
    for( i = 0L ; i < ncolors ; i++ )
    {
      lum_num = (i / (CR_RANGE * CB_RANGE)) % LUM_RANGE;
      cr_num  = (i / CB_RANGE) % CR_RANGE;
      cb_num  = i % CB_RANGE;

      ConvertColor( lum_values[ lum_num ], cr_values[ cr_num ], cb_values[ cb_num ], (&color) );

      mappixel[ i ] = SearchColor( mvid, used_colors, (&used_cnt), (1UL << anim_depth), (&color) );

      /* color table overflow ? */
      if( (used_cnt >= (1UL << anim_depth)) && (i < (ncolors - 1)) )
      {
        mvid -> mvid_ColorError += 1UL;
        used_cnt = 0UL;

        verbose_printf( mvid, "color table overflow, retry with colorerr=%lu\n", (ULONG)(mvid -> mvid_ColorError) );

        goto retrycolors;
      }
    }

    /* statistics... */
    verbose_printf( mvid, "%lu colors used of %lu possible\n", (ULONG)used_cnt, (ULONG)(1UL << anim_depth) );
}


/*
 *--------------------------------------------------------------
 *
 * InitGrayDisplay --
 *
 *    Initialized display for gray scale dither.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      None.
 *
 *--------------------------------------------------------------
 */


void InitGrayDisplay( struct MPEGVideoInstData *mvid )
{
    ULONG                   i,
                            j,
                            numcolors,
                            fac;
    ULONG                   pixel;
    struct ColorRegister    color;

    numcolors  = (1UL << anim_depth);
    fac        = INTDIVR( 255UL, numcolors );

retrycolors:

    for( i = 0UL ; i < numcolors ; i++ )
    {
      color . red = color . green = color . blue = (i * fac);

      pixel = SearchColor( mvid, used_colors, (&used_cnt), (1UL << anim_depth), (&color) );

      /* color table overflow ? */
      if( (used_cnt >= (1UL << anim_depth)) && (i < (numcolors - 1)) )
      {
        mvid -> mvid_ColorError += 1UL;
        used_cnt = 0UL;

        verbose_printf( mvid, "gray color table overflow, retry with colorerr=%lu\n", (ULONG)(mvid -> mvid_ColorError) );

        goto retrycolors;
      }

      for( j = 0UL ; j < fac ; j++ )
      {
        mappixel[ (i * fac) + j ] = pixel;
      }
    }

    /* statistics... */
    verbose_printf( mvid, "%lu colors used of %lu possible\n", (ULONG)used_cnt, (ULONG)(1UL << anim_depth) );
}


/*
 *--------------------------------------------------------------
 *
 * InitHAMDisplay --
 *
 *    Initialized display for HAM dither.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      None.
 *
 *--------------------------------------------------------------
 */


void InitHAMDisplay( struct MPEGVideoInstData *mvid )
{
    ULONG i  = 0UL;
    ULONG value;

    switch( anim_depth )
    {
      case 8: /* HAM-8 */
      {
          for( i = 0UL ; i < 64UL ; i++ )
          {
            value = (255UL * i) / 63UL;

            used_colors[ i ] . red = used_colors[ i ] . green = used_colors[ i ] . blue = value;
          }
      }
          break;

      case 6: /* HAM-6 */
      {
          for( i = 0UL ; i < 16UL ; i++ )
          {
            value = (255UL * i) / 15UL;

            used_colors[ i ] . red = used_colors[ i ] . green = used_colors[ i ] . blue = value;
          }
      }
          break;
    }

    used_cnt = i;

    /* statistics... */
    verbose_printf( mvid, "HAM %lu colors used of %lu possible\n", (ULONG)used_cnt, (ULONG)(1UL << (anim_depth - 2)) );
}


/*
 *--------------------------------------------------------------
 *
 * ExecuteDisplay --
 *
 *    Actually displays display plane in previously created window.
 *
 * Results:
 *    None.
 *
 * Side effects:
 *    None.
 *
 *--------------------------------------------------------------
 */


void ExecuteDisplay( struct MPEGVideoInstData *mvid, VidStream *vid_stream )
{
    if( mvid -> mvid_SkipFrames )
    {
      /* Skip n pictures (Exept the first one) */
      if( (totNumFrames % (mvid -> mvid_SkipFrames)) && (totNumFrames > 1UL) )
      {
        if( mvid -> mvid_IndexScan )
        {
          verbose_printf( mvid, "skipping frame %ld\n", totNumFrames );
        }

        /* Add empty frame node (for sound etc.) */
        AddFrame( mvid, NULL, NULL );

        return;
      }
    }

    if( mvid -> mvid_IndexScan )
    {
      verbose_printf( mvid, "loading frame %ld\n", totNumFrames );
    }

    StoreFrame( mvid, (vid_stream -> current -> display) );
}


