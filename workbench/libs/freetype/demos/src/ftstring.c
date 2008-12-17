/****************************************************************************/
/*                                                                          */
/*  The FreeType project -- a free and portable quality TrueType renderer.  */
/*                                                                          */
/*  Copyright 1996-2002, 2003, 2004, 2005, 2006, 2007 by                    */
/*  D. Turner, R.Wilhelm, and W. Lemberg                                    */
/*                                                                          */
/*                                                                          */
/*  ftstring.c - simple text string display                                 */
/*                                                                          */
/****************************************************************************/

#include <ft2build.h>
#include FT_FREETYPE_H

#include "common.h"
#include "ftcommon.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>

#define CELLSTRING_HEIGHT  8
#define MAXPTSIZE  500                 /* dtp */


  static char*  Text = (char *)"The quick brown fox jumps over the lazy dog";

  enum {
    RENDER_MODE_STRING,
    RENDER_MODE_KERNCMP,
    N_RENDER_MODES
  };

  static struct {
    int          render_mode;
    FT_Encoding  encoding;
    int          res;
    int          ptsize;            /* current point size */
    double       gamma;
    int          angle;

    FTDemo_String_Context  sc;

    FT_Byte      gamma_ramp[256];
    FT_Matrix    trans_matrix;
    int          font_index;
    char*        header;
    char         header_buffer[256];

  } status = { RENDER_MODE_STRING, FT_ENCODING_UNICODE, 72, 48, 2.0, 0 };

  static FTDemo_Display*  display;
  static FTDemo_Handle*   handle;


/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
/****                                                                    ****/
/****                     E V E N T   H A N D L I N G                    ****/
/****                                                                    ****/
/****************************************************************************/
/****************************************************************************/
/****************************************************************************/

  static void
  event_help( void )
  {
    grEvent  dummy_event;


    FTDemo_Display_Clear( display );
    grGotoxy( 0, 0 );
    grSetMargin( 2, 1 );
    grGotobitmap( display->bitmap );

    grWriteln( "FreeType String Viewer - part of the FreeType test suite" );
    grLn();
    grWriteln( "This program is used to display a string of text using" );
    grWriteln( "the new convenience API of the FreeType 2 library." );
    grLn();
    grWriteln( "Use the following keys :" );
    grLn();
    grWriteln( "  F1 or ?   : display this help screen" );
    grLn();
    grWriteln( "  a         : toggle anti-aliasing" );
    grWriteln( "  b         : toggle embedded bitmaps (and disable rotation)" );
    grWriteln( "  f         : toggle forced auto-hinting" );
    grWriteln( "  h         : toggle outline hinting" );
    grWriteln( "  l         : toggle low precision rendering" );
    grLn();
    grWriteln( "  1-2       : select rendering mode" );
    grWriteln( "  k         : cycle through kerning modes" );
    grWriteln( "  t         : cycle through kerning degrees" );
    grWriteln( "  V         : toggle vertical rendering" );
    grLn();
    grWriteln( "  G         : toggle gamma correction" );
    grWriteln( "  g         : increase gamma by 0.1" );
    grWriteln( "  v         : decrease gamma by 0.1" );
    grLn();
    grWriteln( "  n         : next font" );
    grWriteln( "  p         : previous font" );
    grLn();
    grWriteln( "  Up        : increase pointsize by 1 unit" );
    grWriteln( "  Down      : decrease pointsize by 1 unit" );
    grWriteln( "  Page Up   : increase pointsize by 10 units" );
    grWriteln( "  Page Down : decrease pointsize by 10 units" );
    grLn();
    grWriteln( "  Right     : rotate counter-clockwise" );
    grWriteln( "  Left      : rotate clockwise" );
    grWriteln( "  F7        : big rotate counter-clockwise" );
    grWriteln( "  F8        : big rotate clockwise" );
    grLn();
    grLn();
    grWriteln( "press any key to exit this help screen" );

    grRefreshSurface( display->surface );
    grListenSurface( display->surface, gr_event_key, &dummy_event );
  }


  static void
  event_font_change( int  delta )
  {
    if ( status.font_index + delta >= handle->num_fonts ||
         status.font_index + delta < 0 )
      return;

    status.font_index += delta;

    FTDemo_Set_Current_Font( handle, handle->fonts[status.font_index] );
    FTDemo_Set_Current_Charsize( handle, status.ptsize, status.res );
    FTDemo_Update_Current_Flags( handle );

    FTDemo_String_Set( handle, (unsigned char*)Text );
  }


  static void
  event_angle_change( int  delta )
  {
    double    radian;
    FT_Fixed  cosinus;
    FT_Fixed  sinus;


    status.angle = ( status.angle + delta ) % 360;

    if ( status.angle == 0 )
    {
      status.sc.matrix = NULL;

      return;
    }

    status.sc.matrix = &status.trans_matrix;

    if ( status.angle < 0 )
      status.angle += 360;

    radian  = status.angle * 3.14159 / 180.0;
    cosinus = (FT_Fixed)( cos( radian ) * 65536.0 );
    sinus   = (FT_Fixed)( sin( radian ) * 65536.0 );

    status.trans_matrix.xx = cosinus;
    status.trans_matrix.yx = sinus;
    status.trans_matrix.xy = -sinus;
    status.trans_matrix.yy = cosinus;
  }


  static void
  event_gamma_change( double delta )
  {
    int i;
    double gamma_inv;

    status.gamma += delta;

    if ( status.gamma > 3.0 )
      status.gamma = 3.0;
    else if ( status.gamma < 0.1 )
      status.gamma = 0.1;

    sprintf( status.header_buffer, "gamma changed to %.1f", status.gamma );
    status.header = status.header_buffer;

    gamma_inv = 1.0f / status.gamma;

    for ( i = 0; i < 256; i++ )
      status.gamma_ramp[i] = (FT_Byte)( pow( (double)i / 255.0f, gamma_inv )
                                        * 255.0f );
  }


  static void
  event_size_change( int delta )
  {
    status.ptsize += delta;

    if ( status.ptsize < 1*64 )
      status.ptsize = 1*64;
    else if ( status.ptsize > MAXPTSIZE*64 )
      status.ptsize = MAXPTSIZE*64;

    FTDemo_Set_Current_Charsize( handle, status.ptsize, status.res );
  }


  static void
  event_render_mode_change( int delta )
  {
    if ( delta )
    {
      status.render_mode = ( status.render_mode + delta ) % N_RENDER_MODES;

      if ( status.render_mode < 0 )
        status.render_mode += N_RENDER_MODES;
    }

    switch ( status.render_mode )
    {
    case RENDER_MODE_STRING:
      status.header = NULL;
      break;

    case RENDER_MODE_KERNCMP:
      status.header = (char *)"Kerning comparison";
      break;
    }
  }


  static int
  Process_Event( grEvent*  event )
  {
    FTDemo_String_Context*  sc = &status.sc;
    int                     ret = 0;


    if ( event->key >= '1' && event->key < '1' + N_RENDER_MODES )
    {
      status.render_mode = event->key - '1';
      event_render_mode_change( 0 );

      return ret;
    }

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

    case grKEY( 'a' ):
      handle->antialias = !handle->antialias;
      status.header     = handle->antialias
                          ? (char *)"anti-aliasing is now on"
                          : (char *)"anti-aliasing is now off";

      FTDemo_Update_Current_Flags( handle );
      break;

    case grKEY( 'b' ):
      handle->use_sbits = !handle->use_sbits;
      status.header     = handle->use_sbits
                          ? (char *)"embedded bitmaps are now used when available"
                          : (char *)"embedded bitmaps are now ignored";

      FTDemo_Update_Current_Flags( handle );
      break;

    case grKEY( 'f' ):
      handle->autohint = !handle->autohint;
      status.header     = handle->autohint
                          ? (char *)"forced auto-hinting is now on"
                          : (char *)"forced auto-hinting is now off";

      FTDemo_Update_Current_Flags( handle );
      break;

    case grKEY( 'h' ):
      handle->hinted = !handle->hinted;
      status.header   = handle->hinted
                        ? (char *)"glyph hinting is now active"
                        : (char *)"glyph hinting is now ignored";

      FTDemo_Update_Current_Flags( handle );
      break;

    case grKEY( 'l' ):
      handle->low_prec = !handle->low_prec;
      status.header    = handle->low_prec
                         ? (char *)"rendering precision is now forced to low"
                         : (char *)"rendering precision is now normal";

      FTDemo_Update_Current_Flags( handle );
      break;

    case grKEY( 'k' ):
      sc->kerning_mode = ( sc->kerning_mode + 1 ) % N_KERNING_MODES;
      status.header =
        sc->kerning_mode == KERNING_MODE_SMART
        ? (char *)"pair kerning and side bearing correction is now active"
        : sc->kerning_mode == KERNING_MODE_NORMAL
          ? (char *)"pair kerning is now active"
          : (char *)"pair kerning is now ignored";
      break;

    case grKEY( 't' ):
      sc->kerning_degree = ( sc->kerning_degree + 1 ) % N_KERNING_DEGREES;
      status.header =
        sc->kerning_degree == KERNING_DEGREE_NONE
        ? (char *)"no track kerning"
        : sc->kerning_degree == KERNING_DEGREE_LIGHT
          ? (char *)"light track kerning active"
          : sc->kerning_degree == KERNING_DEGREE_MEDIUM
            ? (char *)"medium track kerning active"
            : (char *)"tight track kerning active";
      break;

    case grKEY( 'V' ):
      sc->vertical  = !sc->vertical;
      status.header = sc->vertical
                      ? (char *)"using vertical layout"
                      : (char *)"using horizontal layout";
      break;

    case grKEY( 'G' ):
      sc->gamma_ramp = sc->gamma_ramp ? NULL : status.gamma_ramp;
      status.header  = sc->gamma_ramp
                       ? (char *)"gamma correction is now on"
                       : (char *)"gamma correction is now off";
      break;

    case grKEY( 'g' ):
      event_gamma_change( 0.1 );
      break;

    case grKEY( 'v' ):
      event_gamma_change( -0.1 );
      break;

    case grKEY( 'n' ):
      event_font_change( 1 );
      break;

    case grKEY( 'p' ):
      event_font_change( -1 );
      break;

    case grKeyUp:       event_size_change(   64 ); break;
    case grKeyDown:     event_size_change(  -64 ); break;
    case grKeyPageUp:   event_size_change(  640); break;
    case grKeyPageDown: event_size_change( -640 ); break;

    case grKeyLeft:  event_angle_change(    -3 ); break;
    case grKeyRight: event_angle_change(     3 ); break;
    case grKeyF7:    event_angle_change(   -30 ); break;
    case grKeyF8:    event_angle_change(    30 ); break;

    default:
      break;
    }

    return ret;
  }


  static void
  gamma_ramp_draw( FT_Byte    gamma_ramp[256],
                   grBitmap*  bitmap )
  {
    int       i, x, y;
    FT_Byte*  p = (FT_Byte*)bitmap->buffer;

    if ( bitmap->pitch < 0 )
      p += -bitmap->pitch * ( bitmap->rows - 1 );

    x = ( bitmap->width - 256 ) / 2;
    y = ( bitmap->rows + 256 ) / 2;

    for (i = 0; i < 256; i++)
      p[bitmap->pitch * ( y - gamma_ramp[i] ) + ( x + i )] = 80;
  }


  static void
  write_header( FT_Error  error_code )
  {
    FT_Face      face;
    const char*  basename;


    error = FTC_Manager_LookupFace( handle->cache_manager,
                                    handle->scaler.face_id, &face );
    if ( error )
      PanicZ( "can't access font file" );

    if ( !status.header )
    {
      basename = ft_basename( handle->current_font->filepathname );

      switch ( error_code )
      {
      case FT_Err_Ok:
        sprintf( status.header_buffer, "%s %s (file `%s')", face->family_name,
                 face->style_name, basename );
        break;
      case FT_Err_Invalid_Pixel_Size:
        sprintf( status.header_buffer, "Invalid pixel size (file `%s')",
                 basename );
        break;
      case FT_Err_Invalid_PPem:
        sprintf( status.header_buffer, "Invalid ppem value (file `%s')",
                 basename );
        break;
      default:
        sprintf( status.header_buffer, "File `%s': error 0x%04x", basename,
            (FT_UShort)error_code );
        break;
      }

      status.header = status.header_buffer;
    }

    grWriteCellString( display->bitmap, 0, 0,
                       status.header, display->fore_color );

    sprintf( status.header_buffer, "at %g points, angle = %d",
             status.ptsize/64.0, status.angle );
    grWriteCellString( display->bitmap, 0, CELLSTRING_HEIGHT,
                       status.header_buffer, display->fore_color );

    grRefreshSurface( display->surface );
  }


  static void
  usage( char*  execname )
  {
    fprintf( stderr,  "\n" );
    fprintf( stderr,  "ftstring: string viewer -- part of the FreeType project\n" );
    fprintf( stderr,  "-------------------------------------------------------\n" );
    fprintf( stderr,  "\n" );
    fprintf( stderr,  "Usage: %s [options below] ppem fontname[.ttf|.ttc] ...\n",
             execname );
    fprintf( stderr,  "\n" );
    fprintf( stderr,  "  -e enc      specify encoding tag (default: unic)\n" );
    fprintf( stderr,  "  -r R        use resolution R dpi (default: 72 dpi)\n" );
    fprintf( stderr,  "  -m message  message to display\n" );
    fprintf( stderr,  "\n" );

    exit( 1 );
  }


  static void
  parse_cmdline( int*     argc,
                 char***  argv )
  {
    char*  execname;
    int    option;


    execname = ft_basename( (*argv)[0] );

    while ( 1 )
    {
      option = getopt( *argc, *argv, "e:m:r:" );

      if ( option == -1 )
        break;

      switch ( option )
      {
      case 'e':
        status.encoding = FTDemo_Make_Encoding_Tag( optarg );
        break;

      case 'r':
        status.res = atoi( optarg );
        if ( status.res < 1 )
          usage( execname );
        break;

      case 'm':
        if ( *argc < 3 )
          usage( execname );
        Text = optarg;
        break;

      default:
        usage( execname );
        break;
      }
    }

    *argc -= optind;
    *argv += optind;

    if ( *argc <= 1 )
      usage( execname );

    status.ptsize = (int)(atof( *argv[0] ) * 64.0);
    if ( status.ptsize == 0 )
      status.ptsize = 64;

    (*argc)--;
    (*argv)++;
  }


  int
  main( int     argc,
        char**  argv )
  {
    grEvent  event;


    parse_cmdline( &argc, &argv );

    /* Initialize engine */
    handle = FTDemo_New( status.encoding );

    handle->use_sbits = 0;
    FTDemo_Update_Current_Flags( handle );

    for ( ; argc > 0; argc--, argv++ )
    {
      error = FTDemo_Install_Font( handle, argv[0] );

      if ( error )
      {
        fprintf( stderr, "failed to install %s", argv[0] );
        if ( error == FT_Err_Invalid_CharMap_Handle )
          fprintf( stderr, ": missing valid charmap\n" );
        else
          fprintf( stderr, "\n" );
      }
    }

    if ( handle->num_fonts == 0 )
      PanicZ( "could not open any font file" );

    display = FTDemo_Display_New( gr_pixel_mode_gray );
    display->back_color.value = 0;
    display->fore_color.value = 0xff;

    if ( !display )
      PanicZ( "could not allocate display surface" );

    grSetTitle( display->surface,
                "FreeType String Viewer - press F1 for help" );

    event_gamma_change( 0 );
    event_font_change( 0 );
    status.header = 0;

    for ( ;; )
    {
      FTDemo_Display_Clear( display );

      switch ( status.render_mode )
      {
      case RENDER_MODE_STRING:
        status.sc.center = 1L << 15;
        error = FTDemo_String_Draw( handle, display,
                                    &status.sc,
                                    display->bitmap->width / 2,
                                    display->bitmap->rows / 2 );
        break;

      case RENDER_MODE_KERNCMP:
        {
          FTDemo_String_Context  sc = status.sc;
          FT_Int                 x, y;
          FT_UInt                height;


          x = 55;

          /* whatever.. */
          height = status.ptsize * status.res / 72;
          if ( height < CELLSTRING_HEIGHT )
            height = CELLSTRING_HEIGHT;

          /* First line: none */
          sc.center         = 0;
          sc.kerning_mode   = 0;
          sc.kerning_degree = 0;
          sc.vertical       = 0;
          sc.matrix         = NULL;

          y = CELLSTRING_HEIGHT * 2 + display->bitmap->rows / 4 + height;
          grWriteCellString( display->bitmap, 5,
                             y - ( height + CELLSTRING_HEIGHT ) / 2,
                             "none", display->fore_color );
          error = FTDemo_String_Draw( handle, display, &sc, x, y );


          /* Second line: track kern only */
          sc.kerning_degree = status.sc.kerning_degree;

          y += height;
          grWriteCellString( display->bitmap, 5,
                             y - ( height + CELLSTRING_HEIGHT ) / 2,
                             "track", display->fore_color );
          error = FTDemo_String_Draw( handle, display, &sc, x, y );


          /* Third line: track kern + pair kern */
          sc.kerning_mode      = status.sc.kerning_mode;

          y += height;
          grWriteCellString( display->bitmap, 5,
                             y - ( height + CELLSTRING_HEIGHT ) / 2,
                             "both", display->fore_color );
          error = FTDemo_String_Draw( handle, display, &sc, x, y );
        }
        break;
      }

      if ( !error && status.sc.gamma_ramp )
        gamma_ramp_draw( status.gamma_ramp, display->bitmap );

      write_header( error );

      status.header = 0;
      grListenSurface( display->surface, 0, &event );
      if ( Process_Event( &event ) )
        break;
    }

    printf( "Execution completed successfully.\n" );

    FTDemo_Display_Done( display );
    FTDemo_Done( handle );
    exit( 0 );      /* for safety reasons */

    return 0;       /* never reached */
  }


/* End */
