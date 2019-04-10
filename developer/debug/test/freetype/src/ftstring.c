/****************************************************************************/
/*                                                                          */
/*  The FreeType project -- a free and portable quality TrueType renderer.  */
/*                                                                          */
/*  Copyright (C) 1996-2019 by                                              */
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
#include FT_TRIGONOMETRY_H

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
    RENDER_MODE_TEXT,
    RENDER_MODE_WATERFALL,
    RENDER_MODE_KERNCMP,
    N_RENDER_MODES
  };

  static struct  status_
  {
    const char*    dims;

    int            render_mode;
    unsigned long  encoding;
    int            res;
    int            ptsize;            /* current point size */
    int            angle;
    const char*    text;

    FTDemo_String_Context  sc;

    FT_Matrix  trans_matrix;
    int        font_index;
    char*      header;
    char       header_buffer[256];

  } status = { DIM, RENDER_MODE_STRING, FT_ENCODING_UNICODE, 72, 48, 0, NULL,
               { 0, 0, 0x8000, 0, NULL, 0, 0 },
               { 0, 0, 0, 0 }, 0, NULL, { 0 } };

  static FTDemo_Display*  display;
  static FTDemo_Handle*   handle;

  static FT_Glyph  wheel, daisy, aster;  /* stress test glyphs */


  static void
  flower_init( FT_Glyph*     glyph,
               FT_F26Dot6    radius,
               unsigned int  i,
               int           v,
               int           w,
               int           reflect,
               char          order )
  {
    FT_Outline*  outline;
    FT_Vector*   vec;
    FT_Vector*   limit;
    char*        tag;
    FT_Fixed     s = FT_Sin( FT_ANGLE_PI4 / i );
    FT_Pos       b, d, p = 0, q = radius;


    FT_New_Glyph( handle->library, FT_GLYPH_FORMAT_OUTLINE, glyph );

    outline = &((FT_OutlineGlyph)*glyph)->outline;

    FT_Outline_New( handle->library, 6 * i, 1, outline );
    outline->contours[0] = outline->n_points - 1;

    if ( order == FT_CURVE_TAG_CUBIC )
      q += q / 3;

    for ( vec = outline->points, tag = outline->tags;
          i--;
          vec += 6,              tag += 6 )
    {
       b = p + FT_MulFix( q, s );
       d = q - FT_MulFix( p, s );
       vec[0].x = vec[3].y = 0;
       vec[0].y = vec[3].x = 0;
       tag[0]   = tag[3]   = FT_CURVE_TAG_ON;
       vec[1].x = vec[5].y = ( v * p + w * b ) / ( v + w );
       vec[1].y = vec[5].x = ( v * q + w * d ) / ( v + w );
       tag[1]   = tag[5]   = order;
       vec[2].x = vec[4].y = ( v * b + w * p ) / ( v + w );
       vec[2].y = vec[4].x = ( v * d + w * q ) / ( v + w );
       tag[2]   = tag[4]   = order;
       p = b;
       q = d;
    }

    limit = outline->points + outline->n_points;

    if ( reflect & 1 )
      for( vec = outline->points; vec < limit; vec++ )
        vec->x = -vec->x;

    if ( reflect & 2 )
      for( vec = outline->points; vec < limit; vec++ )
        vec->y = -vec->y;
  }


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
    grWriteln( "  1-4       : select rendering mode" );
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
    grWriteln( "  Left      : move left" );
    grWriteln( "  Right     : move right" );
    grWriteln( "  Home      : flush left" );
    grWriteln( "  End       : flush right" );
    grLn();
    grWriteln( "  F5        : rotate counter-clockwise" );
    grWriteln( "  F6        : rotate clockwise" );
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
  }


  static void
  event_center_change( FT_Fixed  delta )
  {
    status.sc.center += delta;

    if ( status.sc.center > 0x10000 )
      status.sc.center = 0x10000;
    else if ( status.sc.center < 0 )
      status.sc.center = 0;
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
    display->warn_color = grFindColor( display->bitmap,  r, ~g, ~b, 0xff );

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
    display->gamma += delta;

    if ( display->gamma > 3.0 )
      display->gamma = 3.0;
    else if ( display->gamma < 0.1 )
      display->gamma = 0.1;

    grSetGlyphGamma( display->gamma );
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
    case RENDER_MODE_KERNCMP:
      status.header = (char *)"Kerning comparison";
      break;

    default:
      status.header = NULL;
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
      status.render_mode = (int)( event->key - '1' );
      event_render_mode_change( 0 );

      return ret;
    }

    switch ( event->key )
    {
    case grKeyEsc:
    case grKEY( 'q' ):
      ret = 1;
      goto Exit;;

    case grKeyF1:
    case grKEY( '?' ):
      event_help();
      goto Exit;

    case grKEY( 'b' ):
      handle->use_sbits = !handle->use_sbits;
      status.header     = handle->use_sbits
                          ? (char *)"embedded bitmaps are now used when available"
                          : (char *)"embedded bitmaps are now ignored";
      goto Flags;

    case grKEY( 'f' ):
      handle->autohint = !handle->autohint;
      status.header     = handle->autohint
                          ? (char *)"forced auto-hinting is now on"
                          : (char *)"forced auto-hinting is now off";
      goto Flags;

    case grKEY( 'h' ):
      handle->hinted = !handle->hinted;
      status.header   = handle->hinted
                        ? (char *)"glyph hinting is now active"
                        : (char *)"glyph hinting is now ignored";
      goto Flags;

    case grKEY( 'l' ):
      event_lcdmode_change();
      goto Flags;

    case grKEY( 'k' ):
      sc->kerning_mode = ( sc->kerning_mode + 1 ) % N_KERNING_MODES;
      status.header =
        sc->kerning_mode == KERNING_MODE_SMART
        ? (char *)"pair kerning and side bearing correction is now active"
        : sc->kerning_mode == KERNING_MODE_NORMAL
          ? (char *)"pair kerning is now active"
          : (char *)"pair kerning is now ignored";
      goto String;

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
      goto String;

    case grKeySpace:
      event_color_change();
      goto Exit;

    case grKeyTab:
      event_text_change();
      FTDemo_String_Set( handle, status.text );
      goto String;

    case grKEY( 'V' ):
      sc->vertical  = !sc->vertical;
      status.header = sc->vertical
                      ? (char *)"using vertical layout"
                      : (char *)"using horizontal layout";
      goto Exit;

    case grKEY( 'g' ):
      event_gamma_change( 0.1 );
      goto Exit;

    case grKEY( 'v' ):
      event_gamma_change( -0.1 );
      goto Exit;

    case grKEY( 'n' ):
      event_font_change( 1 );
      FTDemo_String_Set( handle, status.text );
      goto Flags;

    case grKEY( 'p' ):
      event_font_change( -1 );
      FTDemo_String_Set( handle, status.text );
      goto Flags;

    case grKeyUp:       event_size_change(   64 ); goto String;
    case grKeyDown:     event_size_change(  -64 ); goto String;
    case grKeyPageUp:   event_size_change(  640 ); goto String;
    case grKeyPageDown: event_size_change( -640 ); goto String;

    case grKeyLeft:  event_center_change( -0x800 ); goto Exit;
    case grKeyRight: event_center_change(  0x800 ); goto Exit;
    case grKeyHome:  event_center_change( -0x10000 ); goto Exit;
    case grKeyEnd:   event_center_change(  0x10000 ); goto Exit;

    case grKeyF5: event_angle_change(  -3 ); goto Exit;
    case grKeyF6: event_angle_change(   3 ); goto Exit;
    case grKeyF7: event_angle_change( -30 ); goto Exit;
    case grKeyF8: event_angle_change(  30 ); goto Exit;

    default:
      break;
    }

  Flags:
    FTDemo_Update_Current_Flags( handle );

  String:
    FTDemo_String_Load( handle, &status.sc );

  Exit:
    return ret;
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
      "  -d WxHxD  Set the window width, height, and color depth\n"
      "            (default: 640x480x24).\n"
      "  -r R      Use resolution R dpi (default: 72dpi).\n"
      "  -e enc    Specify encoding tag (default: Unicode).\n"
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
      option = getopt( *argc, *argv, "d:e:m:r:v" );

      if ( option == -1 )
        break;

      switch ( option )
      {
      case 'd':
        status.dims = optarg;
        break;

      case 'e':
        status.encoding = FTDemo_Make_Encoding_Tag( optarg );
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


  static FT_Error
  Render_String( void )
  {
    int x, y = display->bitmap->rows - 4;


    x = 4;
    FTDemo_Draw_Glyph( handle, display, daisy, &x, &y );

    x = display->bitmap->width - 4;
    FTDemo_Draw_Glyph( handle, display, aster, &x, &y );

    FTDemo_String_Draw( handle, display,
                        &status.sc,
                        FT_MulFix( display->bitmap->width, status.sc.center),
                        display->bitmap->rows / 2 );

    return FT_Err_Ok;
  }


  static FT_Error
  Render_Text( void )
  {
    int      x = FT_MulFix( display->bitmap->width, status.sc.center);
    int      y, step_y;
    int      offset = 0;
    FT_Size  size;

    FTDemo_String_Context sc = status.sc;


    sc.extent   = display->bitmap->width * 64;
    sc.vertical = 0;

    error = FTDemo_Get_Size( handle, &size );
    if ( error )
      return error;

    step_y = ( size->metrics.height >> 6 ) + 1;
    y      = 40 + ( size->metrics.ascender >> 6 );

    for ( ; y < display->bitmap->rows + ( size->metrics.descender >> 6 );
            y += step_y )
    {
      sc.offset = offset;

      offset += FTDemo_String_Draw( handle, display, &sc, x, y );

      offset %= handle->string_length;
    }

    return FT_Err_Ok;
  }


  static FT_Error
  Render_Waterfall( void )
  {
    int      pt_size = status.ptsize, step, pt_height;
    int      y = 40;
    int      x = FT_MulFix( display->bitmap->width, status.sc.center);
    FT_Size  size;

    FTDemo_String_Context sc = status.sc;


    sc.vertical = 0;

    pt_height = 64 * 72 * display->bitmap->rows / status.res;
    step      = ( pt_size * pt_size / pt_height + 64 ) & ~63;
    pt_size   = pt_size - step * ( pt_size / step ); /* remainder */

    while ( 1 )
    {
      pt_size += step;

      FTDemo_Set_Current_Charsize( handle, pt_size, status.res );
      FTDemo_String_Load( handle, &status.sc );

      error = FTDemo_Get_Size( handle, &size );
      if ( error )
      {
        /* probably a non-existent bitmap font size */
        continue;
      }

      if ( pt_size == status.ptsize )
        grFillHLine( display->bitmap, x - 4, y, 8, display->warn_color );

      y += ( size->metrics.height >> 6 ) + 1;

      if ( y >= display->bitmap->rows )
        break;

      if ( pt_size == status.ptsize )
        grFillHLine( display->bitmap, x - 4, y, 8, display->warn_color );

      FTDemo_String_Draw( handle, display, &sc,
                          x, y + ( size->metrics.descender >> 6 ) );
    }

    FTDemo_Set_Current_Charsize( handle, status.ptsize, status.res );
    FTDemo_String_Load( handle, &status.sc );

    return FT_Err_Ok;
  }


  static FT_Error
  Render_KernCmp( void )
  {
    FT_Size                size;
    FTDemo_String_Context  sc = { 0, 0, 0, 0, NULL, 0, 0 };
    FT_Int                 x, y;
    FT_Int                 height;


    x = 55;

    FTDemo_Get_Size( handle, &size );
    height = size->metrics.y_ppem;
    if ( height < CELLSTRING_HEIGHT )
      height = CELLSTRING_HEIGHT;

    /* First line: none */
    FTDemo_String_Load( handle, &sc );

    y = CELLSTRING_HEIGHT * 2 + display->bitmap->rows / 4 + height;
    grWriteCellString( display->bitmap, 5,
                       y - ( height + CELLSTRING_HEIGHT ) / 2,
                       "none", display->fore_color );
    FTDemo_String_Draw( handle, display, &sc, x, y );

    /* Second line: track kern only */
    sc.kerning_degree = status.sc.kerning_degree;
    FTDemo_String_Load( handle, &sc );

    y += height;
    grWriteCellString( display->bitmap, 5,
                       y - ( height + CELLSTRING_HEIGHT ) / 2,
                       "track", display->fore_color );
    FTDemo_String_Draw( handle, display, &sc, x, y );

    /* Third line: track kern + pair kern */
    sc.kerning_mode = status.sc.kerning_mode;
    FTDemo_String_Load( handle, &sc );

    y += height;
    grWriteCellString( display->bitmap, 5,
                       y - ( height + CELLSTRING_HEIGHT ) / 2,
                       "both", display->fore_color );
    FTDemo_String_Draw( handle, display, &sc, x, y );

    return FT_Err_Ok;
  }


  int
  main( int     argc,
        char**  argv )
  {
    grEvent  event;


    /* Initialize engine */
    handle = FTDemo_New();

    parse_cmdline( &argc, &argv );

    flower_init( &wheel, 8192, 20, 3, 1, 0, FT_CURVE_TAG_ON    );
    flower_init( &daisy, 8192, 20, 4, 1, 0, FT_CURVE_TAG_CONIC );
    flower_init( &aster, 8192, 20, 5, 1, 1, FT_CURVE_TAG_CUBIC );

    FT_Library_SetLcdFilter( handle->library, FT_LCD_FILTER_LIGHT );

    handle->encoding  = status.encoding;

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

    display = FTDemo_Display_New( status.dims );

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
    FTDemo_String_Set( handle, status.text );
    FTDemo_Update_Current_Flags( handle );
    FTDemo_String_Load( handle, &status.sc );

    do
    {
      FTDemo_Display_Clear( display );

      switch ( status.render_mode )
      {
      case RENDER_MODE_STRING:
        error = Render_String();
        break;

      case RENDER_MODE_TEXT:
        error = Render_Text();
        break;

      case RENDER_MODE_WATERFALL:
        error = Render_Waterfall();
        break;

      case RENDER_MODE_KERNCMP:
        error = Render_KernCmp();
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
