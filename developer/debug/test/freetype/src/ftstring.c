/****************************************************************************/
/*                                                                          */
/*  The FreeType project -- a free and portable quality TrueType renderer.  */
/*                                                                          */
/*  Copyright 1996-2018 by                                                  */
/*  D. Turner, R.Wilhelm, and W. Lemberg                                    */
/*                                                                          */
/*                                                                          */
/*  ftstring.c - simple text string display                                 */
/*                                                                          */
/****************************************************************************/


#include "ftcommon.h"
#include "common.h"
#include "mlgetopt.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>

#include FT_LCD_FILTER_H

#define CELLSTRING_HEIGHT  8
#define MAXPTSIZE          500   /* dtp */


  static const char*  Sample[] =
  {
    "The quick brown fox jumps over the lazy dog",

    /* Luís argüia à Júlia que «brações, fé, chá, óxido, pôr, zângão» */
    /* eram palavras do português */
    "Lu\u00EDs arg\u00FCia \u00E0 J\u00FAlia que \u00ABbra\u00E7\u00F5es, "
    "f\u00E9, ch\u00E1, \u00F3xido, p\u00F4r, z\u00E2ng\u00E3o\u00BB eram "
    "palavras do portugu\u00EAs",

    /* Ο καλύμνιος σφουγγαράς ψιθύρισε πως θα βουτήξει χωρίς να διστάζει */
    "\u039F \u03BA\u03B1\u03BB\u03CD\u03BC\u03BD\u03B9\u03BF\u03C2 \u03C3"
    "\u03C6\u03BF\u03C5\u03B3\u03B3\u03B1\u03C1\u03AC\u03C2 \u03C8\u03B9"
    "\u03B8\u03CD\u03C1\u03B9\u03C3\u03B5 \u03C0\u03C9\u03C2 \u03B8\u03B1 "
    "\u03B2\u03BF\u03C5\u03C4\u03AE\u03BE\u03B5\u03B9 \u03C7\u03C9\u03C1"
    "\u03AF\u03C2 \u03BD\u03B1 \u03B4\u03B9\u03C3\u03C4\u03AC\u03B6\u03B5"
    "\u03B9",

    /* Съешь ещё этих мягких французских булок да выпей же чаю */
    "\u0421\u044A\u0435\u0448\u044C \u0435\u0449\u0451 \u044D\u0442\u0438"
    "\u0445 \u043C\u044F\u0433\u043A\u0438\u0445 \u0444\u0440\u0430\u043D"
    "\u0446\u0443\u0437\u0441\u043A\u0438\u0445 \u0431\u0443\u043B\u043E"
    "\u043A \u0434\u0430 \u0432\u044B\u043F\u0435\u0439 \u0436\u0435 "
    "\u0447\u0430\u044E",

    /* 天地玄黃，宇宙洪荒。日月盈昃，辰宿列張。寒來暑往，秋收冬藏。*/
    "\u5929\u5730\u7384\u9EC3\uFF0C\u5B87\u5B99\u6D2A\u8352\u3002\u65E5"
    "\u6708\u76C8\u6603\uFF0C\u8FB0\u5BBF\u5217\u5F35\u3002\u5BD2\u4F86"
    "\u6691\u5F80\uFF0C\u79CB\u6536\u51AC\u85CF\u3002",

    /* いろはにほへと ちりぬるを わかよたれそ つねならむ */
    /* うゐのおくやま けふこえて あさきゆめみし ゑひもせす */
    "\u3044\u308D\u306F\u306B\u307B\u3078\u3068 \u3061\u308A\u306C\u308B"
    "\u3092 \u308F\u304B\u3088\u305F\u308C\u305D \u3064\u306D\u306A\u3089"
    "\u3080 \u3046\u3090\u306E\u304A\u304F\u3084\u307E \u3051\u3075\u3053"
    "\u3048\u3066 \u3042\u3055\u304D\u3086\u3081\u307F\u3057 \u3091\u3072"
    "\u3082\u305B\u3059",

    /* 키스의 고유조건은 입술끼리 만나야 하고 특별한 기술은 필요치 않다 */
    "\uD0A4\uC2A4\uC758 \uACE0\uC720\uC870\uAC74\uC740 \uC785\uC220\uB07C"
    "\uB9AC \uB9CC\uB098\uC57C \uD558\uACE0 \uD2B9\uBCC4\uD55C \uAE30"
    "\uC220\uC740 \uD544\uC694\uCE58 \uC54A\uB2E4"
  };

  enum
  {
    RENDER_MODE_STRING,
    RENDER_MODE_KERNCMP,
    N_RENDER_MODES
  };

  static struct  status_
  {
    int  width;
    int  height;

    int            render_mode;
    unsigned long  encoding;
    int            res;
    int            ptsize;            /* current point size */
    int            angle;
    const char*    text;

    FTDemo_String_Context  sc;

    FT_Byte    gamma_ramp[256];   /* for show only */
    FT_Matrix  trans_matrix;
    int        font_index;
    char*      header;
    char       header_buffer[256];

  } status = { DIM_X, DIM_Y,
               RENDER_MODE_STRING, FT_ENCODING_UNICODE, 72, 48, 0, NULL,
               { 0, 0, 0x8000, 0, NULL },
               { 0 }, { 0, 0, 0, 0 }, 0, NULL, { 0 } };

  static FTDemo_Display*  display;
  static FTDemo_Handle*   handle;


  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/
  /****                                                                 ****/
  /****                   E V E N T   H A N D L I N G                   ****/
  /****                                                                 ****/
  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/

  static void
  event_help( void )
  {
    char  buf[256];
    char  version[64];

    const char*  format;
    FT_Int       major, minor, patch;

    grEvent  dummy_event;


    FT_Library_Version( handle->library, &major, &minor, &patch );

    format = patch ? "%d.%d.%d" : "%d.%d";
    sprintf( version, format, major, minor, patch );

    FTDemo_Display_Clear( display );
    grSetLineHeight( 10 );
    grGotoxy( 0, 0 );
    grSetMargin( 2, 1 );
    grGotobitmap( display->bitmap );

    sprintf( buf,
             "FreeType String Viewer - part of the FreeType %s test suite",
             version );

    grWriteln( buf );
    grLn();
    grWriteln( "This program is used to display a string of text using" );
    grWriteln( "the new convenience API of the FreeType 2 library." );
    grLn();
    grWriteln( "Use the following keys :" );
    grLn();
    grWriteln( "  F1 or ?   : display this help screen" );
    grLn();
    grWriteln( "  b         : toggle embedded bitmaps (and disable rotation)" );
    grWriteln( "  f         : toggle forced auto-hinting" );
    grWriteln( "  h         : toggle outline hinting" );
    grLn();
    grWriteln( "  1-2       : select rendering mode" );
    grWriteln( "  l         : cycle through anti-aliasing modes" );
    grWriteln( "  k         : cycle through kerning modes" );
    grWriteln( "  t         : cycle through kerning degrees" );
    grWriteln( "  Space     : cycle through color" );
    grWriteln( "  Tab       : cycle through sample strings" );
    grWriteln( "  V         : toggle vertical rendering" );
    grLn();
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
    grWriteln( "press any key to exit this help screen" );

    grRefreshSurface( display->surface );
    grListenSurface( display->surface, gr_event_key, &dummy_event );
  }


  static void
  event_font_change( int  delta )
  {
    if ( status.font_index + delta >= handle->num_fonts ||
         status.font_index + delta < 0                  )
      return;

    status.font_index += delta;

    FTDemo_Set_Current_Font( handle, handle->fonts[status.font_index] );
    FTDemo_Set_Current_Charsize( handle, status.ptsize, status.res );
    FTDemo_Update_Current_Flags( handle );

    FTDemo_String_Set( handle, status.text );
  }


  static void
  event_angle_change( int  delta )
  {
    double    radian;
    FT_Fixed  cosinus;
    FT_Fixed  sinus;


    status.angle += delta;

    if ( status.angle <= -180 )
      status.angle += 360;
    if ( status.angle > 180 )
      status.angle -= 360;

    if ( status.angle == 0 )
    {
      status.sc.matrix = NULL;

      return;
    }

    status.sc.matrix = &status.trans_matrix;

    radian  = status.angle * 3.14159265 / 180.0;
    cosinus = (FT_Fixed)( cos( radian ) * 65536.0 );
    sinus   = (FT_Fixed)( sin( radian ) * 65536.0 );

    status.trans_matrix.xx = cosinus;
    status.trans_matrix.yx = sinus;
    status.trans_matrix.xy = -sinus;
    status.trans_matrix.yy = cosinus;
  }


  static void
  event_lcdmode_change( void )
  {
    const char  *lcd_mode;


    handle->lcd_mode++;

    switch ( handle->lcd_mode )
    {
    case LCD_MODE_AA:
      lcd_mode = " normal AA";
      break;
    case LCD_MODE_LIGHT:
      lcd_mode = " light AA";
      break;
    case LCD_MODE_LIGHT_SUBPIXEL:
      lcd_mode = " light AA (subpixel positioning)";
      break;
    case LCD_MODE_RGB:
      lcd_mode = " LCD (horiz. RGB)";
      break;
    case LCD_MODE_BGR:
      lcd_mode = " LCD (horiz. BGR)";
      break;
    case LCD_MODE_VRGB:
      lcd_mode = " LCD (vert. RGB)";
      break;
    case LCD_MODE_VBGR:
      lcd_mode = " LCD (vert. BGR)";
      break;
    default:
      handle->lcd_mode = LCD_MODE_MONO;
      lcd_mode = " monochrome";
    }

    sprintf( status.header_buffer, "mode changed to %s", lcd_mode );
    status.header = status.header_buffer;
  }


  static void
  event_color_change( void )
  {
    static int     i = 0;
    unsigned char  r = i & 4 ? 0xff : 0;
    unsigned char  g = i & 2 ? 0xff : 0;
    unsigned char  b = i & 1 ? 0xff : 0;


    display->back_color = grFindColor( display->bitmap,  r,  g,  b, 0xff );
    display->fore_color = grFindColor( display->bitmap, ~r, ~g, ~b, 0xff );

    i++;
  }


  static void
  event_text_change( void )
  {
    static int  i = 0;

    status.text = Sample[i];

    i++;
    if ( i >= (int)( sizeof( Sample ) / sizeof( Sample[0] ) ) )
      i = 0;
  }

  static void
  event_gamma_change( double  delta )
  {
    int     i;


    display->gamma += delta;

    if ( display->gamma > 3.0 )
      display->gamma = 3.0;
    else if ( display->gamma < 0.1 )
      display->gamma = 0.1;

    grSetGlyphGamma( display->gamma );

    for ( i = 0; i < 256; i++ )
      status.gamma_ramp[i] = (FT_Byte)( pow( (double)i / 255., display->gamma )
                                        * 255. + 0.5 );
  }


  static void
  event_size_change( int  delta )
  {
    status.ptsize += delta;

    if ( status.ptsize < 1 * 64 )
      status.ptsize = 1 * 64;
    else if ( status.ptsize > MAXPTSIZE * 64 )
      status.ptsize = MAXPTSIZE * 64;

    FTDemo_Set_Current_Charsize( handle, status.ptsize, status.res );
  }


  static void
  event_render_mode_change( int  delta )
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
    FTDemo_String_Context*  sc  = &status.sc;
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
      event_lcdmode_change();

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

    case grKeySpace:
      event_color_change();
      break;

    case grKeyTab:
      event_text_change();
      FTDemo_String_Set( handle, status.text );
      break;

    case grKEY( 'V' ):
      sc->vertical  = !sc->vertical;
      status.header = sc->vertical
                      ? (char *)"using vertical layout"
                      : (char *)"using horizontal layout";
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
    case grKeyPageUp:   event_size_change(  640 ); break;
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
    int       bpp = bitmap->pitch / bitmap->width;
    FT_Byte*  p = (FT_Byte*)bitmap->buffer;


    if ( bitmap->pitch < 0 )
      p += -bitmap->pitch * ( bitmap->rows - 1 );

    x = ( bitmap->width - 256 ) / 2;
    y = ( bitmap->rows + 256 ) / 2;

    for (i = 0; i < 256; i++)
      p[bitmap->pitch * ( y - i ) + bpp * ( x + gamma_ramp[i] )] ^= 0xFF;
  }


  static void
  write_header( FT_Error  error_code )
  {
    FTDemo_Draw_Header( handle, display, status.ptsize, status.res,
                        -1, error_code );

    if ( status.header )
      grWriteCellString( display->bitmap, 0, 2 * HEADER_HEIGHT,
                         status.header, display->fore_color );

    grRefreshSurface( display->surface );
  }


  static void
  usage( char*  execname )
  {
    fprintf( stderr,
      "\n"
      "ftstring: string viewer -- part of the FreeType project\n"
      "-------------------------------------------------------\n"
      "\n" );
    fprintf( stderr,
      "Usage: %s [options] pt font ...\n"
      "\n",
             execname );
    fprintf( stderr,
      "  pt        The point size for the given resolution.\n"
      "            If resolution is 72dpi, this directly gives the\n"
      "            ppem value (pixels per EM).\n" );
    fprintf( stderr,
      "  font      The font file(s) to display.\n"
      "            For Type 1 font files, ftstring also tries to attach\n"
      "            the corresponding metrics file (with extension\n"
      "            `.afm' or `.pfm').\n"
      "\n" );
    fprintf( stderr,
      "  -w W      Set the window width to W pixels (default: %dpx).\n"
      "  -h H      Set the window height to H pixels (default: %dpx).\n"
      "\n",
             DIM_X, DIM_Y );
    fprintf( stderr,
      "  -r R      Use resolution R dpi (default: 72dpi).\n"
      "  -e enc    Specify encoding tag (default: no encoding).\n"
      "            Common values: `unic' (Unicode), `symb' (symbol),\n"
      "            `ADOB' (Adobe standard), `ADBC' (Adobe custom).\n"
      "  -m text   Use `text' for rendering.\n"
      "\n"
      "  -v        Show version.\n"
      "\n" );

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
      option = getopt( *argc, *argv, "e:h:m:r:vw:" );

      if ( option == -1 )
        break;

      switch ( option )
      {
      case 'e':
        status.encoding = FTDemo_Make_Encoding_Tag( optarg );
        break;

      case 'h':
        status.height = atoi( optarg );
        if ( status.height < 1 )
          usage( execname );
        break;

      case 'm':
        if ( *argc < 3 )
          usage( execname );
        status.text = optarg;
        break;

      case 'r':
        status.res = atoi( optarg );
        if ( status.res < 1 )
          usage( execname );
        break;

      case 'v':
        {
          FT_Int  major, minor, patch;


          FT_Library_Version( handle->library, &major, &minor, &patch );

          printf( "ftstring (FreeType) %d.%d", major, minor );
          if ( patch )
            printf( ".%d", patch );
          printf( "\n" );
          exit( 0 );
        }
        /* break; */

      case 'w':
        status.width = atoi( optarg );
        if ( status.width < 1 )
          usage( execname );
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

    status.ptsize = (int)( atof( *argv[0] ) * 64.0 );
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


    /* Initialize engine */
    handle = FTDemo_New();

    parse_cmdline( &argc, &argv );

    FT_Library_SetLcdFilter( handle->library, FT_LCD_FILTER_LIGHT );

    handle->encoding  = status.encoding;
    handle->use_sbits = 0;
    FTDemo_Update_Current_Flags( handle );

    for ( ; argc > 0; argc--, argv++ )
    {
      error = FTDemo_Install_Font( handle, argv[0], 0, 0 );

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

    display = FTDemo_Display_New( gr_pixel_mode_rgb24,
                                  status.width, status.height );

    if ( !display )
      PanicZ( "could not allocate display surface" );

    grSetTitle( display->surface,
                "FreeType String Viewer - press ? for help" );

    status.header = NULL;

    if ( !status.text )
      event_text_change();

    event_color_change();
    event_gamma_change( 0 );
    event_font_change( 0 );

    do
    {
      FTDemo_Display_Clear( display );

      gamma_ramp_draw( status.gamma_ramp, display->bitmap );

      switch ( status.render_mode )
      {
      case RENDER_MODE_STRING:
        error = FTDemo_String_Draw( handle, display,
                                    &status.sc,
                                    display->bitmap->width / 2,
                                    display->bitmap->rows / 2 );
        break;

      case RENDER_MODE_KERNCMP:
        {
          FT_Size                size;
          FTDemo_String_Context  sc = { 0, 0, 0, 0, NULL };
          FT_Int                 x, y;
          FT_Int                 height;


          x = 55;

          FTDemo_Get_Size( handle, &size );
          height = size->metrics.y_ppem;
          if ( height < CELLSTRING_HEIGHT )
            height = CELLSTRING_HEIGHT;

          /* First line: none */
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
          sc.kerning_mode = status.sc.kerning_mode;

          y += height;
          grWriteCellString( display->bitmap, 5,
                             y - ( height + CELLSTRING_HEIGHT ) / 2,
                             "both", display->fore_color );
          error = FTDemo_String_Draw( handle, display, &sc, x, y );
        }
        break;
      }

      write_header( error );

      status.header = 0;
      grListenSurface( display->surface, 0, &event );
    } while ( !Process_Event( &event ) );

    printf( "Execution completed successfully.\n" );

    FTDemo_Display_Done( display );
    FTDemo_Done( handle );
    exit( 0 );      /* for safety reasons */

    /* return 0; */ /* never reached */
  }


/* End */
