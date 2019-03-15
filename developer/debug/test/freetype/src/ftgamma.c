/****************************************************************************/
/*                                                                          */
/*  The FreeType project -- a free and portable quality TrueType renderer.  */
/*                                                                          */
/*  Copyright 2004-2018 by                                                  */
/*  D. Turner, R.Wilhelm, and W. Lemberg                                    */
/*                                                                          */
/*                                                                          */
/*  ftgamma - gamma matcher                                                 */
/*                                                                          */
/****************************************************************************/


#include "ftcommon.h"
#include <math.h>


  static FTDemo_Display*  display;

  static grBitmap   bit1 = { 300, 600, 600, gr_pixel_mode_gray, 256, NULL };
  static grBitmap   bit2 = { 288, 600, 600, gr_pixel_mode_gray, 256, NULL };
  static int        status = 0;


  static void
  do_ptrn( grBitmap*  bitmap,
           int        x,
           int        y,
           int        w,
           int        h )
  {
    int     pitch = bitmap->pitch;
    int     i, j, k;
    double  p[4];

    unsigned char*  line;

    for ( i = 0; i < h; i++ )
    {
      for ( k = 0; k < 4; k++)
      {
        j = 2 * i + 1 + ( k - 4 ) * h / 2;
        if ( j > h )
          j -= 2 * h;
        if ( j < -h )
          j += 2 * h;
        if ( j < 0 )
          j = -j;
        j -= h / 4;
        if ( j < 0 )
          j = 0;
        if ( j > h / 2 )
          j = h / 2;

        p[k] = 2. * j / h;
      }

      line = bitmap->buffer + ( y + i ) * pitch + x;
      for ( j = 0, k = 0; j < w; j++ )
      {
        line[j] = (unsigned char)( 0.5 +
                                   255. * pow ( p[k],
                                                1. / (1. + 2. * j / w ) ) );
        k++;
        if ( k == 4 )
          k = 0;
      }
    }
  }


  static FT_Error
  GammaPtrn( grBitmap*  bitmap )
  {
    int  x = 0;
    int  y = 0;
    int  h = ( bitmap->rows - 2 * y ) / 2;
    int  w = bitmap->width - 2 * x;


    do_ptrn( bitmap, x,    y, w, h );
    do_ptrn( bitmap, x, y+=h, w, h );

    return 0;
  }


  static void
  do_fill( grBitmap*  bitmap,
           int        x,
           int        y,
           int        w,
           int        h,
           int        back,
           int        fore )
  {
    int     pitch = bitmap->pitch;
    int     i;
    double  b, f;

    unsigned char*  line = bitmap->buffer + y*pitch + x;


    if ( back == 0 || back == 255 )
      for ( i = 0; i < w; i++ )
        line[i + ( i & 1 ) * pitch] = (unsigned char)back;
    else
      for ( b = back / 255., i = 0; i < w; i++ )
        line[i + ( i & 1 ) * pitch] =
          (unsigned char)( 0.5 + 255. * pow ( b, 1. / (1. + 2. * i / w ) ) );

    if ( fore == 0 || fore == 255 )
      for ( i = 0; i < w; i++ )
        line[i + ( ~i & 1 ) * pitch] = (unsigned char)fore;
    else
      for ( f = fore / 255., i = 0; i < w; i++ )
        line[i + ( ~i & 1 ) * pitch] =
          (unsigned char)( 0.5 + 255. * pow ( f, 1. / (1. + 2. * i / w ) ) );

    for ( i = 2; i < h; i += 2 )
    {
      memcpy( line + i * pitch, line, (size_t)w );
      memcpy( line + i * pitch + pitch, line + pitch, (size_t)w );
    }
  }


  static FT_Error
  GammaGrid( grBitmap*  bitmap )
  {
    int  x = 0;
    int  y = 0;
    int  h = ( bitmap->rows - 2 * y ) / 15;
    int  w = bitmap->width - 2 * x;


    do_fill( bitmap, x,    y, w, h,  85, 255 );
    do_fill( bitmap, x, y+=h, w, h, 170, 170 );
    do_fill( bitmap, x, y+=h, w, h,  85, 255 );
    do_fill( bitmap, x, y+=h, w, h, 170, 170 );
    do_fill( bitmap, x, y+=h, w, h,  85, 255 );

    do_fill( bitmap, x, y+=h, w, h,   0, 255 );
    do_fill( bitmap, x, y+=h, w, h, 127, 127 );
    do_fill( bitmap, x, y+=h, w, h,   0, 255 );
    do_fill( bitmap, x, y+=h, w, h, 127, 127 );
    do_fill( bitmap, x, y+=h, w, h,   0, 255 );

    do_fill( bitmap, x, y+=h, w, h,   0, 170 );
    do_fill( bitmap, x, y+=h, w, h,  85,  85 );
    do_fill( bitmap, x, y+=h, w, h,   0, 170 );
    do_fill( bitmap, x, y+=h, w, h,  85,  85 );
    do_fill( bitmap, x, y+=h, w, h,   0, 170 );

    return 0;
  }


  static void
  event_help( void )
  {
    grEvent  dummy_event;


    FTDemo_Display_Clear( display );
    grSetLineHeight( 10 );
    grGotoxy( 0, 0 );
    grSetMargin( 2, 1 );
    grGotobitmap( display->bitmap );


    grWriteln( "FreeType Gamma Matcher" );
    grLn();
    grWriteln( "Use the following keys:" );
    grLn();
    grWriteln( "F1, ?       display this help screen" );
    grLn();
    grWriteln( "space       cycle through color");
    grWriteln( "tab         alternate patterns");
    grWriteln( "G           show gamma ramp" );
    grLn();
    grLn();
    grWriteln( "press any key to exit this help screen" );

    grRefreshSurface( display->surface );
    grListenSurface( display->surface, gr_event_key, &dummy_event );
  }


  static void
  event_color_change( void )
  {
    static int     i = 7;
    unsigned char  r = i & 4 ? 0xff : 0;
    unsigned char  g = i & 2 ? 0xff : 0;
    unsigned char  b = i & 1 ? 0xff : 0;


    display->back_color = grFindColor( display->bitmap, 0, 0, 0, 0xff );
    display->fore_color = grFindColor( display->bitmap, r, g, b, 0xff );

    i++;
    if ( ( i & 0x7 ) == 0 )
      i = 1;
  }


  static void
  event_gamma_grid( void )
  {
    grEvent  dummy_event;
    int      g;
    int      yside  = 11;
    int      xside  = 10;
    int      levels = 17;
    int      gammas = 30;
    int      x_0    = ( display->bitmap->width - levels * xside ) / 2;
    int      y_0    = ( display->bitmap->rows - gammas * ( yside + 1 ) ) / 2;
    int      pitch  = display->bitmap->pitch;


    FTDemo_Display_Clear( display );
    grGotobitmap( display->bitmap );

    if ( pitch < 0 )
      pitch = -pitch;

    memset( display->bitmap->buffer,
            100,
            (unsigned int)( pitch * display->bitmap->rows ) );

    grWriteCellString( display->bitmap, 0, 0, "Gamma grid",
                       display->fore_color );


    for ( g = 1; g <= gammas; g++ )
    {
      double  ggamma = 0.1 * g;
      char    temp[6];
      int     y = y_0 + ( yside + 1 ) * ( g - 1 );
      int     nx, ny;

      unsigned char*  line = display->bitmap->buffer +
                             y * display->bitmap->pitch;


      if ( display->bitmap->pitch < 0 )
        line -= display->bitmap->pitch * ( display->bitmap->rows - 1 );

      line += x_0 * 3;

      grSetPixelMargin( x_0 - 32, y + ( yside - 8 ) / 2 );
      grGotoxy( 0, 0 );

      sprintf( temp, "%.1f", ggamma );
      grWrite( temp );

      for ( ny = 0; ny < yside; ny++, line += display->bitmap->pitch )
      {
        unsigned char*  dst = line;


        for ( nx = 0; nx < levels; nx++, dst += 3 * xside )
        {
          double  p   = nx / (double)( levels - 1 );
          int     gm  = (int)( 255.0 * pow( p, ggamma ) + 0.5 );


          memset( dst, gm, (unsigned int)( xside * 3 ) );
        }
      }
    }

    grRefreshSurface( display->surface );
    grListenSurface( display->surface, gr_event_key, &dummy_event );
  }


  static void
  Render_Bitmap( grBitmap*  out,
                 grBitmap*  in,
                 int        x,
                 int        y,
                 grColor    color,
                 int        lcd )
  {
    int  pitch = abs( out->pitch );
    int  i, ii, j;

    unsigned char*  src;
    unsigned char*  dst;


    if ( color.chroma[0] == 255 )
      for ( src = in->buffer, i = 0; i < in->rows; i++ )
      {
        ii = ( i + 24 * lcd ) % in->rows;
        dst = out->buffer + ( y + ii ) * pitch + 3 * x;
        for ( j = 0; j < in->width; j++, src++, dst += 3 )
          *dst = *src;
      }

    if ( color.chroma[1] == 255 )
      for ( src = in->buffer, i = 0; i < in->rows; i++ )
      {
        ii = ( i + 12 * lcd ) % in->rows;
        dst = out->buffer + ( y + ii ) * pitch + 3 * x + 1;
        for ( j = 0; j < in->width; j++, src++, dst += 3 )
          *dst = *src;
      }

    if ( color.chroma[2] == 255 )
      for ( src = in->buffer, i = 0; i < in->rows; i++ )
      {
        ii = ( i +  0 * lcd ) % in->rows;
        dst = out->buffer + ( y + ii ) * pitch + 3 * x + 2;
        for ( j = 0; j < in->width; j++, src++, dst += 3 )
          *dst = *src;
      }
  }


  static int
  Process_Event( grEvent*  event )
  {
    int  ret = 0;

    switch ( event->key )
    {
    case grKeyEsc:
    case grKEY( 'q' ):
      ret = 1;
      break;

    case grKeyF1:
    case grKEY( '?' ):
      event_help();
      break;

    case grKeySpace:
      event_color_change();
      break;

    case grKeyTab:
      status++;
      if ( status > 2 )
        status = 0;
      break;

    case grKEY( 'G' ):
      event_gamma_grid();
      break;

    default:
      break;
    }

    return ret;
  }


  int
  main( void )
  {
    grEvent          event;
    char             buf[4];
    int              i;

    display = FTDemo_Display_New( gr_pixel_mode_rgb24, DIM_X, DIM_Y );
    if ( !display )
    {
      PanicZ( "could not allocate display surface" );
    }

    grSetTitle( display->surface, "FreeType Gamma Matcher - press ? for help" );

    grNewBitmap( bit1.mode, bit1.grays, bit1.width, bit1.rows, &bit1 );
    GammaGrid( &bit1 );

    grNewBitmap( bit2.mode, bit2.grays, bit2.width, bit2.rows, &bit2 );
    GammaPtrn( &bit2 );

    event_color_change();

    do
    {
      FTDemo_Display_Clear( display );

      switch ( status )
      {
      case 0:
        grWriteCellString( display->bitmap, 236, 75, "Solid-Checkered Pattern",
                           display->fore_color );
        Render_Bitmap( display->bitmap, &bit1, 20, 90, display->fore_color, 0 );
        break;
      case 1:
        grWriteCellString( display->bitmap, 236, 75, "Grayscale Anti-Aliasing",
                           display->fore_color );
        Render_Bitmap( display->bitmap, &bit2, 20, 96, display->fore_color, 0 );
        break;
      case 2:
        grWriteCellString( display->bitmap, 236, 75, "Subpixel  Anti-Aliasing",
                           display->fore_color );
        Render_Bitmap( display->bitmap, &bit2, 20, 96, display->fore_color, 1 );
        break;
      }

      for ( i = 0; i <= 10; i++ )
      {
        sprintf( buf, "%.1f", 1. + .2 * i );
        grWriteCellString( display->bitmap, 9 + i * 60, 395, buf,
                           display->fore_color );
      }

      grWriteCellString( display->bitmap, DIM_X / 2 - 20, 410, "Gamma",
                         display->fore_color );

      grRefreshSurface( display->surface );
      grListenSurface( display->surface, 0, &event );
    } while ( Process_Event( &event ) == 0 );

    FTDemo_Display_Done( display );

    exit( 0 );      /* for safety reasons */
    /* return 0; */ /* never reached */
  }


/* End */
