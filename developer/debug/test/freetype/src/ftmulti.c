/****************************************************************************/
/*                                                                          */
/*  The FreeType project -- a free and portable quality TrueType renderer.  */
/*                                                                          */
/*  Copyright 1996-2018 by                                                  */
/*  D. Turner, R.Wilhelm, and W. Lemberg                                    */
/*                                                                          */
/*                                                                          */
/*  FTMulti- a simple multiple masters font viewer                          */
/*                                                                          */
/*  Press ? when running this program to have a list of key-bindings        */
/*                                                                          */
/****************************************************************************/

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_FONT_FORMATS_H
#include FT_MODULE_H
#include FT_DRIVER_H
#include FT_MULTIPLE_MASTERS_H

#include "common.h"
#include "mlgetopt.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "graph.h"
#include "grfont.h"

#define  DIM_X   640
#define  DIM_Y   480

#define  MAXPTSIZE    500               /* dtp */
#define  MAX_MM_AXES    6

  /* definitions in ftcommon.c */
  unsigned int
  FTDemo_Event_Cff_Hinting_Engine_Change( FT_Library     library,
                                          unsigned int*  current,
                                          unsigned int   delta );
  unsigned int
  FTDemo_Event_Type1_Hinting_Engine_Change( FT_Library     library,
                                            unsigned int*  current,
                                            unsigned int   delta );
  unsigned int
  FTDemo_Event_T1cid_Hinting_Engine_Change( FT_Library     library,
                                            unsigned int*  current,
                                            unsigned int   delta );


  static char   Header[256];
  static char*  new_header = NULL;

  static const unsigned char*  Text = (unsigned char*)
    "The quick brown fox jumps over the lazy dog 0123456789 "
    "\342\352\356\373\364\344\353\357\366\374\377\340\371\351\350\347 "
    "&#~\"\'(-`_^@)=+\260 ABCDEFGHIJKLMNOPQRSTUVWXYZ "
    "$\243^\250*\265\371%!\247:/;.,?<>";

  static FT_Library    library;      /* the FreeType library        */
  static FT_Face       face;         /* the font face               */
  static FT_Size       size;         /* the font size               */
  static FT_GlyphSlot  glyph;        /* the glyph slot              */

  static unsigned long  encoding = FT_ENCODING_NONE;

  static unsigned int  cff_hinting_engine;
  static unsigned int  type1_hinting_engine;
  static unsigned int  t1cid_hinting_engine;
  static unsigned int  tt_interpreter_versions[3];
  static unsigned int  num_tt_interpreter_versions;
  static unsigned int  tt_interpreter_version_idx;

  static const char*  font_format;

  static FT_Error      error;        /* error returned by FreeType? */

  static grSurface*    surface;      /* current display surface     */
  static grBitmap      bit;          /* current display bitmap      */

  static int  width     = DIM_X;     /* window width                */
  static int  height    = DIM_Y;     /* window height               */

  static int  num_glyphs;            /* number of glyphs            */
  static int  ptsize;                /* current point size          */

  static int  hinted    = 1;         /* is glyph hinting active?    */
  static int  antialias = 1;         /* is anti-aliasing active?    */
  static int  use_sbits = 1;         /* do we use embedded bitmaps? */
  static int  Num;                   /* current first glyph index   */

  static int  res       = 72;

  static grColor  fore_color = { 255 };

  static int  Fail;

  static int  graph_init  = 0;

  static int  render_mode = 1;

  static FT_MM_Var    *multimaster   = NULL;
  static FT_Fixed      design_pos   [MAX_MM_AXES];
  static FT_Fixed      requested_pos[MAX_MM_AXES];
  static unsigned int  requested_cnt = 0;
  static unsigned int  used_num_axis = 0;


#define DEBUGxxx

#ifdef DEBUG
#define LOG( x )  LogMessage x
#else
#define LOG( x )  /* empty */
#endif


#ifdef DEBUG
  static void
  LogMessage( const char*  fmt, ... )
  {
    va_list  ap;


    va_start( ap, fmt );
    vfprintf( stderr, fmt, ap );
    va_end( ap );
  }
#endif


  /* PanicZ */
  static void
  PanicZ( const char*  message )
  {
    fprintf( stderr, "%s\n  error = 0x%04x\n", message, error );
    exit( 1 );
  }


  static unsigned long
  make_tag( char  *s )
  {
    int            i;
    unsigned long  l = 0;


    for ( i = 0; i < 4; i++ )
    {
      if ( !s[i] )
        break;
      l <<= 8;
      l  += (unsigned long)s[i];
    }

    return l;
  }


  static void
  parse_design_coords( char  *s )
  {
    for ( requested_cnt = 0; requested_cnt < MAX_MM_AXES && *s;
          requested_cnt++ )
    {
      requested_pos[requested_cnt] = (FT_Fixed)( strtod( s, &s ) * 65536.0 );

      while ( *s==' ' )
        ++s;
    }
  }


  /* Clears the Bit bitmap/pixmap */
  static void
  Clear_Display( void )
  {
    long  bitmap_size = (long)bit.pitch * bit.rows;


    if ( bitmap_size < 0 )
      bitmap_size = -bitmap_size;
    memset( bit.buffer, 0, (unsigned long)bitmap_size );
  }


  /* Initialize the display bitmap named `bit' */
  static void
  Init_Display( void )
  {
    grInitDevices();

    bit.mode  = gr_pixel_mode_gray;
    bit.width = width;
    bit.rows  = height;
    bit.grays = 256;

    surface = grNewSurface( 0, &bit );
    if ( !surface )
      PanicZ( "could not allocate display surface\n" );

    graph_init = 1;
  }


  /* Render a single glyph with the `grays' component */
  static FT_Error
  Render_Glyph( int  x_offset,
                int  y_offset )
  {
    grBitmap  bit3;
    FT_Pos    x_top, y_top;


    /* first, render the glyph image into a bitmap */
    if ( glyph->format != FT_GLYPH_FORMAT_BITMAP )
    {
      error = FT_Render_Glyph( glyph, antialias ? FT_RENDER_MODE_NORMAL
                                                : FT_RENDER_MODE_MONO );
      if ( error )
        return error;
    }

    /* now blit it to our display screen */
    bit3.rows   = (int)glyph->bitmap.rows;
    bit3.width  = (int)glyph->bitmap.width;
    bit3.pitch  = glyph->bitmap.pitch;
    bit3.buffer = glyph->bitmap.buffer;

    switch ( glyph->bitmap.pixel_mode )
    {
    case FT_PIXEL_MODE_MONO:
      bit3.mode  = gr_pixel_mode_mono;
      bit3.grays = 0;
      break;

    case FT_PIXEL_MODE_GRAY:
      bit3.mode  = gr_pixel_mode_gray;
      bit3.grays = glyph->bitmap.num_grays;
    }

    /* Then, blit the image to the target surface */
    x_top = x_offset + glyph->bitmap_left;
    y_top = y_offset - glyph->bitmap_top;

    grBlitGlyphToBitmap( &bit, &bit3, x_top, y_top, fore_color );

    return 0;
  }


  static void
  Reset_Scale( int  pointSize )
  {
    (void)FT_Set_Char_Size( face,
                            pointSize << 6, pointSize << 6,
                            (FT_UInt)res, (FT_UInt)res );
  }


  static FT_Error
  LoadChar( unsigned int  idx,
            int           hint )
  {
    int  flags;


    flags = FT_LOAD_DEFAULT;

    if ( !hint )
      flags |= FT_LOAD_NO_HINTING;

    if ( !use_sbits )
      flags |= FT_LOAD_NO_BITMAP;

    return FT_Load_Glyph( face, idx, flags );
  }


  static FT_Error
  Render_All( unsigned int  first_glyph,
              int           pt_size )
  {
    FT_F26Dot6    start_x, start_y, step_y, x, y;
    unsigned int  i;


    start_x = 4;
    start_y = pt_size + ( used_num_axis > MAX_MM_AXES / 2 ? 52 : 44 );

    step_y = size->metrics.y_ppem + 10;

    x = start_x;
    y = start_y;

    i = first_glyph;

#if 0
    while ( i < first_glyph + 1 )
#else
    while ( i < (unsigned int)num_glyphs )
#endif
    {
      if ( !( error = LoadChar( i, hinted ) ) )
      {
#ifdef DEBUG
        if ( i <= first_glyph + 6 )
        {
          LOG(( "metrics[%02d] = [%x %x]\n",
                i,
                glyph->metrics.horiBearingX,
                glyph->metrics.horiAdvance ));

          if ( i == first_glyph + 6 )
            LOG(( "-------------------------\n" ));
        }
#endif

        Render_Glyph( x, y );

        x += ( ( glyph->metrics.horiAdvance + 32 ) >> 6 ) + 1;

        if ( x + size->metrics.x_ppem > bit.width )
        {
          x  = start_x;
          y += step_y;

          if ( y >= bit.rows )
            return FT_Err_Ok;
        }
      }
      else
        Fail++;

      i++;
    }

    return FT_Err_Ok;
  }


  static FT_Error
  Render_Text( unsigned int  first_glyph,
               int           pt_size )
  {
    FT_F26Dot6    start_x, start_y, step_y, x, y;
    unsigned int  i;

    const unsigned char*  p;


    start_x = 4;
    start_y = pt_size + ( used_num_axis > MAX_MM_AXES / 2 ? 52 : 44 );

    step_y = size->metrics.y_ppem + 10;

    x = start_x;
    y = start_y;

    i = first_glyph;
    p = Text;
    while ( i > 0 && *p )
    {
      p++;
      i--;
    }

    while ( *p )
    {
      if ( !( error = LoadChar( FT_Get_Char_Index( face,
                                                   (unsigned char)*p ),
                                hinted ) ) )
      {
#ifdef DEBUG
        if ( i <= first_glyph + 6 )
        {
          LOG(( "metrics[%02d] = [%x %x]\n",
                i,
                glyph->metrics.horiBearingX,
                glyph->metrics.horiAdvance ));

          if ( i == first_glyph + 6 )
          LOG(( "-------------------------\n" ));
        }
#endif

        Render_Glyph( x, y );

        x += ( ( glyph->metrics.horiAdvance + 32 ) >> 6 ) + 1;

        if ( x + size->metrics.x_ppem > bit.width )
        {
          x  = start_x;
          y += step_y;

          if ( y >= bit.rows )
            return FT_Err_Ok;
        }
      }
      else
        Fail++;

      i++;
      p++;
    }

    return FT_Err_Ok;
  }


  static void
  Help( void )
  {
    char  buf[256];
    char  version[64];

    const char*  format;
    FT_Int       major, minor, patch;

    grEvent  dummy_event;


    FT_Library_Version( library, &major, &minor, &patch );

    format = patch ? "%d.%d.%d" : "%d.%d";
    sprintf( version, format, major, minor, patch );

    Clear_Display();
    grSetLineHeight( 10 );
    grGotoxy( 0, 0 );
    grSetMargin( 2, 1 );
    grGotobitmap( &bit );

    sprintf( buf,
             "FreeType MM Glyph Viewer - part of the FreeType %s test suite",
             version );

    grWriteln( buf );
    grLn();
    grWriteln( "This program displays all glyphs from one or several" );
    grWriteln( "Multiple Masters or GX font files, with the FreeType library." );
    grLn();
    grWriteln( "Use the following keys:");
    grLn();
    grWriteln( "?           display this help screen" );
    grWriteln( "a           toggle anti-aliasing" );
    grWriteln( "h           toggle outline hinting" );
    grWriteln( "b           toggle embedded bitmaps" );
    grWriteln( "space       toggle rendering mode" );
    grLn();
    grWriteln( "p, n        previous/next font" );
    grLn();
    grWriteln( "H           cycle through hinting engines (if available)" );
    grLn();
    grWriteln( "Up, Down    change pointsize by 1 unit" );
    grWriteln( "PgUp, PgDn  change pointsize by 10 units" );
    grLn();
    grWriteln( "Left, Right adjust index by 1" );
    grWriteln( "F7, F8      adjust index by 10" );
    grWriteln( "F9, F10     adjust index by 100" );
    grWriteln( "F11, F12    adjust index by 1000" );
    grLn();
    grWriteln( "F1, F2      adjust first axis by 1/50th of its range" );
    grWriteln( "F3, F4      adjust second axis by 1/50th of its range" );
    grWriteln( "F5, F6      adjust third axis by 1/50th of its range" );
    grWriteln( "1, 2        adjust fourth axis by 1/50th of its range" );
    grWriteln( "3, 4        adjust fifth axis by 1/50th of its range" );
    grWriteln( "5, 6        adjust sixth axis by 1/50th of its range" );
    grLn();
    grLn();
    grWriteln( "press any key to exit this help screen" );

    grRefreshSurface( surface );
    grListenSurface( surface, gr_event_key, &dummy_event );
  }


  static void
  tt_interpreter_version_change( void )
  {
    tt_interpreter_version_idx += 1;
    tt_interpreter_version_idx %= num_tt_interpreter_versions;

    FT_Property_Set( library,
                     "truetype",
                     "interpreter-version",
                     &tt_interpreter_versions[tt_interpreter_version_idx] );
  }


  static int
  Process_Event( grEvent*  event )
  {
    int           i;
    unsigned int  axis;


    switch ( event->key )
    {
    case grKeyEsc:            /* ESC or q */
    case grKEY( 'q' ):
      return 0;

    case grKEY( '?' ):
      Help();
      return 1;

    /* mode keys */

    case grKEY( 'a' ):
      antialias  = !antialias;
      new_header = antialias ? (char *)"anti-aliasing is now on"
                             : (char *)"anti-aliasing is now off";
      return 1;

    case grKEY( 'b' ):
      use_sbits  = !use_sbits;
      new_header = use_sbits
                     ? (char *)"embedded bitmaps are now used if available"
                     : (char *)"embedded bitmaps are now ignored";
      return 1;

    case grKEY( 'n' ):
    case grKEY( 'p' ):
      return (int)event->key;

    case grKEY( 'h' ):
      hinted     = !hinted;
      new_header = hinted ? (char *)"glyph hinting is now active"
                          : (char *)"glyph hinting is now ignored";
      break;

    case grKEY( ' ' ):
      render_mode ^= 1;
      new_header   = render_mode ? (char *)"rendering all glyphs in font"
                                 : (char *)"rendering test text string";
      break;

    case grKEY( 'H' ):
      if ( !strcmp( font_format, "CFF" ) )
        FTDemo_Event_Cff_Hinting_Engine_Change( library,
                                                &cff_hinting_engine,
                                                1);
      else if ( !strcmp( font_format, "Type 1" ) )
        FTDemo_Event_Type1_Hinting_Engine_Change( library,
                                                  &type1_hinting_engine,
                                                  1);
      else if ( !strcmp( font_format, "CID Type 1" ) )
        FTDemo_Event_T1cid_Hinting_Engine_Change( library,
                                                  &t1cid_hinting_engine,
                                                  1);
      else if ( !strcmp( font_format, "TrueType" ) )
        tt_interpreter_version_change();
      break;

    /* MM related keys */

    case grKeyF1:
      i = -20;
      axis = 0;
      goto Do_Axis;

    case grKeyF2:
      i = 20;
      axis = 0;
      goto Do_Axis;

    case grKeyF3:
      i = -20;
      axis = 1;
      goto Do_Axis;

    case grKeyF4:
      i = 20;
      axis = 1;
      goto Do_Axis;

    case grKeyF5:
      i = -20;
      axis = 2;
      goto Do_Axis;

    case grKeyF6:
      i = 20;
      axis = 2;
      goto Do_Axis;

    case grKEY( '1' ):
      i = -20;
      axis = 3;
      goto Do_Axis;

    case grKEY( '2' ):
      i = 20;
      axis = 3;
      goto Do_Axis;

    case grKEY( '3' ):
      i = -20;
      axis = 4;
      goto Do_Axis;

    case grKEY( '4' ):
      i = 20;
      axis = 4;
      goto Do_Axis;

    case grKEY( '5' ):
      i = -20;
      axis = 5;
      goto Do_Axis;

    case grKEY( '6' ):
      i = 20;
      axis = 5;
      goto Do_Axis;

    /* scaling related keys */

    case grKeyPageUp:
      i = 10;
      goto Do_Scale;

    case grKeyPageDown:
      i = -10;
      goto Do_Scale;

    case grKeyUp:
      i = 1;
      goto Do_Scale;

    case grKeyDown:
      i = -1;
      goto Do_Scale;

    /* glyph index related keys */

    case grKeyLeft:
      i = -1;
      goto Do_Glyph;

    case grKeyRight:
      i = 1;
      goto Do_Glyph;

    case grKeyF7:
      i = -10;
      goto Do_Glyph;

    case grKeyF8:
      i = 10;
      goto Do_Glyph;

    case grKeyF9:
      i = -100;
      goto Do_Glyph;

    case grKeyF10:
      i = 100;
      goto Do_Glyph;

    case grKeyF11:
      i = -1000;
      goto Do_Glyph;

    case grKeyF12:
      i = 1000;
      goto Do_Glyph;

    default:
      ;
    }
    return 1;

  Do_Axis:
    if ( axis < used_num_axis )
    {
      FT_Var_Axis*  a   = multimaster->axis + axis;
      FT_Fixed      pos = design_pos[axis];


      /*
       * Normalize i.  Changing by 20 is all very well for PostScript fonts,
       * which tend to have a range of ~1000 per axis, but it's not useful
       * for mac fonts, which have a range of ~3.  And it's rather extreme
       * for optical size even in PS.
       */
      pos += FT_MulDiv( i, a->maximum - a->minimum, 1000 );
      if ( pos < a->minimum )
        pos = a->minimum;
      if ( pos > a->maximum )
        pos = a->maximum;

      design_pos[axis] = pos;

      /* for MM fonts, round the design coordinates to integers,         */
      /* otherwise round to two decimal digits to make the PS name short */
      if ( !FT_IS_SFNT( face ) )
        design_pos[axis] = FT_RoundFix( design_pos[axis] );
      else
      {
        double  x;


        x  = design_pos[axis] / 65536.0 * 100.0;
        x += x < 0.0 ? -0.5 : 0.5;
        x  = (int)x;
        x  = x / 100.0 * 65536.0;
        x += x < 0.0 ? -0.5 : 0.5;

        design_pos[axis] = (int)x;
      }

      FT_Set_Var_Design_Coordinates( face, used_num_axis, design_pos );
    }
    return 1;

  Do_Scale:
    ptsize += i;
    if ( ptsize < 1 )
      ptsize = 1;
    if ( ptsize > MAXPTSIZE )
      ptsize = MAXPTSIZE;
    return 1;

  Do_Glyph:
    Num += i;
    if ( Num < 0 )
      Num = 0;
    if ( Num >= num_glyphs )
      Num = num_glyphs - 1;
    return 1;
  }


  static void
  usage( char*  execname )
  {
    fprintf( stderr,
      "\n"
      "ftmulti: multiple masters font viewer - part of FreeType\n"
      "--------------------------------------------------------\n"
      "\n" );
    fprintf( stderr,
      "Usage: %s [options] pt font ...\n"
      "\n",
             execname );
    fprintf( stderr,
      "  pt           The point size for the given resolution.\n"
      "               If resolution is 72dpi, this directly gives the\n"
      "               ppem value (pixels per EM).\n" );
    fprintf( stderr,
      "  font         The font file(s) to display.\n"
      "\n" );
    fprintf( stderr,
      "  -w W         Set window width to W pixels (default: %dpx).\n"
      "  -h H         Set window height to H pixels (default: %dpx).\n"
      "\n",
             DIM_X, DIM_Y );
    fprintf( stderr,
      "  -e encoding  Specify encoding tag (default: no encoding).\n"
      "               Common values: `unic' (Unicode), `symb' (symbol),\n"
      "               `ADOB' (Adobe standard), `ADBC' (Adobe custom).\n"
      "  -r R         Use resolution R dpi (default: 72dpi).\n"
      "  -f index     Specify first glyph index to display.\n"
      "  -d \"axis1 axis2 ...\"\n"
      "               Specify the design coordinates for each\n"
      "               Multiple Master axis at start-up.\n"
      "\n"
      "  -v           Show version."
      "\n" );

    exit( 1 );
  }


  int
  main( int    argc,
        char*  argv[] )
  {
    int    old_ptsize, orig_ptsize, file;
    int    first_glyph = 0;
    int    XisSetup = 0;
    char*  execname;
    int    option;
    int    file_loaded;

    unsigned int  n;

    grEvent  event;

    unsigned int  dflt_tt_interpreter_version;
    unsigned int  versions[3] = { TT_INTERPRETER_VERSION_35,
                                  TT_INTERPRETER_VERSION_38,
                                  TT_INTERPRETER_VERSION_40 };


    execname = ft_basename( argv[0] );

    /* Initialize engine */
    error = FT_Init_FreeType( &library );
    if ( error )
      PanicZ( "Could not initialize FreeType library" );

    /* get the default value as compiled into FreeType */
    FT_Property_Get( library,
                     "cff",
                     "hinting-engine", &cff_hinting_engine );
    FT_Property_Get( library,
                     "type1",
                     "hinting-engine", &type1_hinting_engine );
    FT_Property_Get( library,
                     "t1cid",
                     "hinting-engine", &t1cid_hinting_engine );

    /* collect all available versions, then set again the default */
    FT_Property_Get( library,
                     "truetype",
                     "interpreter-version", &dflt_tt_interpreter_version );
    for ( n = 0; n < 3; n++ )
    {
      error = FT_Property_Set( library,
                               "truetype",
                               "interpreter-version", &versions[n] );
      if ( !error )
        tt_interpreter_versions[
          num_tt_interpreter_versions++] = versions[n];
      if ( versions[n] == dflt_tt_interpreter_version )
        tt_interpreter_version_idx = n;
    }
    FT_Property_Set( library,
                     "truetype",
                     "interpreter-version", &dflt_tt_interpreter_version );

    while ( 1 )
    {
      option = getopt( argc, argv, "d:e:f:h:r:vw:" );

      if ( option == -1 )
        break;

      switch ( option )
      {
      case 'd':
        parse_design_coords( optarg );
        break;

      case 'e':
        encoding = make_tag( optarg );
        break;

      case 'f':
        first_glyph = atoi( optarg );
        break;

      case 'h':
        height = atoi( optarg );
        if ( height < 1 )
          usage( execname );
        break;

      case 'r':
        res = atoi( optarg );
        if ( res < 1 )
          usage( execname );
        break;

      case 'v':
        {
          FT_Int  major, minor, patch;


          FT_Library_Version( library, &major, &minor, &patch );

          printf( "ftmulti (FreeType) %d.%d", major, minor );
          if ( patch )
            printf( ".%d", patch );
          printf( "\n" );
          exit( 0 );
        }
        /* break; */

      case 'w':
        width = atoi( optarg );
        if ( width < 1 )
          usage( execname );
        break;

      default:
        usage( execname );
        break;
      }
    }

    argc -= optind;
    argv += optind;

    if ( argc <= 1 )
      usage( execname );

    if ( sscanf( argv[0], "%d", &orig_ptsize ) != 1 )
      orig_ptsize = 64;

    file = 1;

  NewFile:
    ptsize      = orig_ptsize;
    hinted      = 1;
    file_loaded = 0;

    /* Load face */
    error = FT_New_Face( library, argv[file], 0, &face );
    if ( error )
    {
      face = NULL;
      goto Display_Font;
    }

    font_format = FT_Get_Font_Format( face );

    if ( encoding != FT_ENCODING_NONE )
    {
      error = FT_Select_Charmap( face, (FT_Encoding)encoding );
      if ( error )
        goto Display_Font;
    }

    /* retrieve multiple master information */
    FT_Done_MM_Var( library, multimaster );
    error = FT_Get_MM_Var( face, &multimaster );
    if ( error )
    {
      multimaster = NULL;
      goto Display_Font;
    }

    /* if the user specified a position, use it, otherwise */
    /* set the current position to the median of each axis */
    if ( multimaster->num_axis > MAX_MM_AXES )
    {
      fprintf( stderr, "only handling first %d GX axes (of %d)\n",
                       MAX_MM_AXES, multimaster->num_axis );
      used_num_axis = MAX_MM_AXES;
    }
    else
      used_num_axis = multimaster->num_axis;

    for ( n = 0; n < used_num_axis; n++ )
    {
      design_pos[n] = n < requested_cnt ? requested_pos[n]
                                        : multimaster->axis[n].def;
      if ( design_pos[n] < multimaster->axis[n].minimum )
        design_pos[n] = multimaster->axis[n].minimum;
      else if ( design_pos[n] > multimaster->axis[n].maximum )
        design_pos[n] = multimaster->axis[n].maximum;

      /* for MM fonts, round the design coordinates to integers */
      if ( !FT_IS_SFNT( face ) )
        design_pos[n] = FT_RoundFix( design_pos[n] );
    }

    error = FT_Set_Var_Design_Coordinates( face, used_num_axis, design_pos );
    if ( error )
      goto Display_Font;

    file_loaded++;

    Reset_Scale( ptsize );

    num_glyphs = face->num_glyphs;
    glyph      = face->glyph;
    size       = face->size;

  Display_Font:
    /* initialize graphics if needed */
    if ( !XisSetup )
    {
      XisSetup = 1;
      Init_Display();
    }

    grSetTitle( surface, "FreeType Glyph Viewer - press ? for help" );
    old_ptsize = ptsize;

    if ( file_loaded >= 1 )
    {
      Fail = 0;
      Num  = first_glyph;

      if ( Num >= num_glyphs )
        Num = num_glyphs - 1;

      if ( Num < 0 )
        Num = 0;
    }

    for ( ;; )
    {
      int  key;


      Clear_Display();

      if ( file_loaded >= 1 )
      {
        switch ( render_mode )
        {
        case 0:
          Render_Text( (unsigned int)Num, ptsize );
          break;

        default:
          Render_All( (unsigned int)Num, ptsize );
        }

        sprintf( Header, "%.50s %.50s (file %.100s)",
                         face->family_name,
                         face->style_name,
                         ft_basename( argv[file] ) );

        if ( !new_header )
          new_header = Header;

        grWriteCellString( &bit, 0, 0, new_header, fore_color );
        new_header = NULL;

        sprintf( Header, "PS name: %s",
                         FT_Get_Postscript_Name( face ) );
        grWriteCellString( &bit, 0, 16, Header, fore_color );

        sprintf( Header, "axes:" );
        {
          unsigned int  limit = used_num_axis > MAX_MM_AXES / 2
                                  ? MAX_MM_AXES / 2
                                  : used_num_axis;


          for ( n = 0; n < limit; n++ )
          {
            char  temp[100];


            sprintf( temp, "  %.50s: %.02f",
                           multimaster->axis[n].name,
                           design_pos[n] / 65536.0 );
            strncat( Header, temp,
                     sizeof ( Header ) - strlen( Header ) - 1 );
          }
        }
        grWriteCellString( &bit, 0, 24, Header, fore_color );

        if ( used_num_axis > MAX_MM_AXES / 2 )
        {
          unsigned int  limit = used_num_axis;


          sprintf( Header, "     " );

          for ( n = MAX_MM_AXES / 2; n < limit; n++ )
          {
            char  temp[100];


            sprintf( temp, "  %.50s: %.02f",
                           multimaster->axis[n].name,
                           design_pos[n] / 65536.0 );
            strncat( Header, temp,
                     sizeof ( Header ) - strlen( Header ) - 1 );
          }

          grWriteCellString( &bit, 0, 32, Header, fore_color );
        }

        {
          unsigned int  tt_ver = tt_interpreter_versions[
                                   tt_interpreter_version_idx];

          const char*  format_str = NULL;

          if ( !strcmp( font_format, "CFF" ) )
            format_str = ( cff_hinting_engine == FT_HINTING_FREETYPE
                         ? "CFF (FreeType)"
                         : "CFF (Adobe)" );
          else if ( !strcmp( font_format, "Type 1" ) )
            format_str = ( type1_hinting_engine == FT_HINTING_FREETYPE
                         ? "Type 1 (FreeType)"
                         : "Type 1 (Adobe)" );
          else if ( !strcmp( font_format, "CID Type 1" ) )
            format_str = ( t1cid_hinting_engine == FT_HINTING_FREETYPE
                         ? "CID Type 1 (FreeType)"
                         : "CID Type 1 (Adobe)" );
          else if ( !strcmp( font_format, "TrueType" ) )
            format_str = ( tt_ver == TT_INTERPRETER_VERSION_35
                                   ? "TrueType (v35)"
                                   : ( tt_ver == TT_INTERPRETER_VERSION_38
                                       ? "TrueType (v38)"
                                       : "TrueType (v40)" ) );

          sprintf( Header, "at %d points, first glyph = %d, format = %s",
                           ptsize,
                           Num,
                           format_str );
        }
      }
      else
      {
        sprintf( Header, "%.100s: not an MM font file, or could not be opened",
                         ft_basename( argv[file] ) );
      }

      grWriteCellString( &bit, 0, 8, Header, fore_color );
      grRefreshSurface( surface );

      grListenSurface( surface, 0, &event );
      if ( !( key = Process_Event( &event ) ) )
        goto End;

      if ( key == 'n' )
      {
        if ( file_loaded >= 1 )
          FT_Done_Face( face );

        if ( file < argc - 1 )
          file++;

        goto NewFile;
      }

      if ( key == 'p' )
      {
        if ( file_loaded >= 1 )
          FT_Done_Face( face );

        if ( file > 1 )
          file--;

        goto NewFile;
      }

      if ( key == 'H' )
      {
        /* enforce reloading */
        if ( file_loaded >= 1 )
          FT_Done_Face( face );

        goto NewFile;
      }

      if ( ptsize != old_ptsize )
      {
        Reset_Scale( ptsize );

        old_ptsize = ptsize;
      }
    }

  End:
    grDoneSurface( surface );
    grDoneDevices();

    free            ( multimaster );
    FT_Done_Face    ( face        );
    FT_Done_FreeType( library     );

    printf( "Execution completed successfully.\n" );
    printf( "Fails = %d\n", Fail );

    exit( 0 );      /* for safety reasons */
    /* return 0; */ /* never reached */
  }


/* End */
