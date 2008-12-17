/****************************************************************************/
/*                                                                          */
/*  The FreeType project -- a free and portable quality TrueType renderer.  */
/*                                                                          */
/*  Copyright 2004, 2005 by                                                 */
/*  D. Turner, R.Wilhelm, and W. Lemberg                                    */
/*                                                                          */
/*                                                                          */
/*  ftgamma - gamma matcher                                                 */
/*                                                                          */
/****************************************************************************/


#include "ftcommon.h"
#include <math.h>


static void
do_rect( grBitmap*  bitmap,
         int        x,
         int        y,
         int        w,
         int        h,
         int        gray )
{
  unsigned char*  line = bitmap->buffer + y*bitmap->pitch;

  if ( bitmap->pitch < 0 )
    line -= bitmap->pitch*(bitmap->rows-1);

  line += 3*x;

  if ( gray >= 0 )
  {
    for ( ; h > 0; h--, line += bitmap->pitch )
      memset( line, gray, 3*w );
  }
  else
  {
    for ( ; h > 0; h--, line+= bitmap->pitch )
    {
      int             w2 = w;
      unsigned char*  dst = line;

      for ( ; w2 > 0; w2--, dst += 3 )
      {
        int  color = ((w2+h) & 1)*255;

        dst[0] = dst[1] = dst[2] = (unsigned char)color;
      }
    }
  }
}


static FT_Error
Render_GammaGrid( grBitmap*  bitmap )
{
  int   g;
  int   xmargin = 10;
  int   gamma_first = 16;
  int   gamma_last  = 26;
  int   gammas      = gamma_last - gamma_first + 1;
  int   xside       = (bitmap->width-100)/gammas - xmargin;
  int   yside       = (bitmap->rows-100)/2;
  int   yrepeat     = 1;

  int   x_0     = (bitmap->width - gammas*(xside+xmargin)+xmargin)/2;
  int   y_0     = (bitmap->rows  - (8+yside*2*yrepeat))/2;
  int   pitch   = bitmap->pitch;


  if ( pitch < 0 )
    pitch = -pitch;

#if 1
  memset( bitmap->buffer, 255, pitch*bitmap->rows );
#else
 /* fill the background with a simple pattern corresponding to 50%
  * linear gray from a reasonable viewing distance
  */
  {
    int             nx, ny;
    unsigned char*  line = bitmap->buffer;
    if ( bitmap->pitch < 0 )
      line -= (bitmap->pitch*(bitmap->rows-1));

    for ( ny = 0; ny < bitmap->rows; ny++, line += bitmap->pitch )
    {
      unsigned char*  dst = line;
      int             nx;

      for ( nx = 0; nx < bitmap->width; nx++, dst += 3 )
      {
        int  color = ((nx+ny) & 1)*255;

        dst[0] = dst[1] = dst[2] = (unsigned char)color;
      }
    }
  }
#endif

  grGotobitmap( bitmap );

  for ( g = gamma_first; g <= gamma_last; g += 1 )
  {
    double gamma_value = g/10.0;
    char   temp[6];
    int    x = x_0 + (xside+xmargin)*(g-gamma_first);
    int    y = y_0;
    int    ny;

    grSetPixelMargin( x, y_0-8 );
    grGotoxy( 0, 0 );

    sprintf( temp, "%.1f", gamma_value );
    grWrite( temp );

    for ( ny = 0; ny < yrepeat; ny++, y += 2*yside )
    {
      do_rect( bitmap, x, y, xside, yside,
               (int)( 255.0 * pow( 0.5, 1.0 / gamma_value ) ) );
      do_rect( bitmap, x, y+yside, xside, yside, -1 );
    }
  }
  return 0;
}



int
main( void )
{
  FTDemo_Display*  display;
  grEvent          dummy;

  display = FTDemo_Display_New( gr_pixel_mode_rgb24 );
  if ( !display )
  {
    PanicZ( "could not allocate display surface" );
  }

  grSetTitle( display->surface, "FreeType Gamma Matcher" );

  Render_GammaGrid( display->bitmap );

  grRefreshSurface( display->surface );
  grListenSurface( display->surface, 0, &dummy );

  exit( 0 );      /* for safety reasons */
  return 0;       /* never reached */
}


/* End */
