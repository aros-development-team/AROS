/****************************************************************************/
/*                                                                          */
/*  The FreeType project -- a free and portable quality TrueType renderer.  */
/*                                                                          */
/*  Copyright 1996-1999 by                                                  */
/*  D. Turner, R.Wilhelm, and W. Lemberg                                    */
/*                                                                          */
/*                                                                          */
/*  FTString.c - simple text string display                                 */
/*                                                                          */
/****************************************************************************/

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include "common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>

#include "graph.h"
#include "grfont.h"

#define DIM_X  500
#define DIM_Y  400

#define CENTER_X  ( bit.width / 2 )
#define CENTER_Y  ( bit.rows  / 2 )

#define MAXPTSIZE  500                 /* dtp */

  static char   Header[128];
  static char*  new_header = 0;

  static char*  Text = (char *)"The quick brown fox jumps over the lazy dog";

  static FT_Library   library;      /* the FreeType library            */
  static FT_Face      face;         /* the font face                   */
  static FT_Error     error;        /* error returned by FreeType?     */

  static FT_Encoding  encoding = ft_encoding_unicode;

  static grSurface*   surface;      /* current display surface         */
  static grBitmap     bit;          /* current display bitmap          */

  static int  ptsize;               /* current point size */
  static int  Num;
  static int  Rotation = 0;
  static int  Fail;

  static int  hinted    = 1;        /* is glyph hinting active ?    */
  static int  antialias = 1;        /* is anti-aliasing active ?    */
  static int  use_sbits = 0;        /* do we use embedded bitmaps ? */
  static int  kerning   = 1;
  static int  use_gamma = 0;

  static int  res = 72;             /* default resolution in dpi */

  static grColor  fore_color = { 255 };

  static int  graph_init  = 0;

  static FT_Matrix  trans_matrix;
  static int        transform = 0;

  static FT_Vector  string_center;

  typedef struct  TGlyph_
  {
    FT_UInt    glyph_index;         /* glyph index in face      */
    FT_Vector  pos;                 /* position of glyph origin */
    FT_Glyph   image;               /* glyph image              */

  } TGlyph, *PGlyph;


#define FLOOR( x )  ( (x) & -64 )
#define CEIL( x )   ( ( (x) + 63 ) & -64 )
#define TRUNC ( x)  ( (x) >> 6 )



/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
/****                                                                    ****/
/****                 U T I L I T Y   F U N C T I O N S                  ****/
/****                                                                    ****/
/****************************************************************************/
/****************************************************************************/
/****************************************************************************/

#define DEBUGxxx

#ifdef DEBUG
#define LOG( x )  LogMessage##x
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
  PanicZ( const char* message )
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


/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
/****                                                                    ****/
/****                D I S P L A Y   M A N A G E M E N T                 ****/
/****                                                                    ****/
/****************************************************************************/
/****************************************************************************/
/****************************************************************************/

#define MAX_GLYPHS 512

  /*************************************************************************/
  /*                                                                       */
  /*  The following arrays are used to store the glyph set that makes      */
  /*  up a string of text.                                                 */
  /*                                                                       */
  /*                                                                       */
  static TGlyph  glyphs[ MAX_GLYPHS ];
  static int     num_glyphs;

  /*************************************************************************/
  /*                                                                       */
  /* Gamma correction.                                                     */
  /*                                                                       */
  /*                                                                       */
  static double  gamma_value = 2.0f;
  static FT_Byte gamma_ramp[256];

  static void
  init_gamma( void )
  {
    int i;
    double gamma_inv = 1.0f / gamma_value;

    for (i = 0; i < 256; i++)
      gamma_ramp[i] = (FT_Byte)( pow((double)i / 255.0f, gamma_inv) * 255.0f );
  }

  static void
  apply_gamma( grBitmap* bmp )
  {
    int       i, j;
    FT_Byte*  buffer = (FT_Byte*) bmp->buffer;

    for (i = 0; i < bmp->rows; i++)
    {
      for (j = 0; j < bmp->width; j++)
        /* bitmap color is the inverse of coverage values, hence the '255-x' */
        buffer[j] = (FT_Byte)(255 - gamma_ramp[255 - buffer[j]]);

      buffer += bmp->pitch;
    }
  }

  static void
  draw_gamma_ramp( void )
  {
    int   i, x, y;
    long  pitch = bit.pitch;
    long  start = 0;

    if ( pitch < 0 )
      start = -pitch*(bit.rows-1);

    x = (bit.width - 256) / 2;
    y = (bit.rows + 256) / 2;
    for (i = 0; i < 256; i++, x++)
    {
      bit.buffer[start + pitch*(y - gamma_ramp[i]) + x ] = 80;
    }
  }


  /*************************************************************************/
  /*                                                                       */
  /* Initialize the display surface.                                       */
  /*                                                                       */
  /*                                                                       */
  static int
  init_display( void )
  {
    grInitDevices();

    bit.mode  = gr_pixel_mode_gray;
    bit.width = DIM_X;
    bit.rows  = DIM_Y;
    bit.grays = 256;

    surface = grNewSurface( 0, &bit );
    if ( !surface )
      PanicZ( "could not allocate display surface\n" );

    graph_init = 1;
    return 0;
  }


  /*************************************************************************/
  /*                                                                       */
  /*  Clear the display surface.                                           */
  /*                                                                       */
  /*                                                                       */
  static void
  clear_display( void )
  {
    long  size = (long)bit.pitch * bit.rows;


    if ( size < 0 )
      size = -size;
    memset( bit.buffer, 0, size );
  }


  static void
  reset_scale( int  pointSize )
  {
    (void)FT_Set_Char_Size( face,
                            pointSize << 6,
                            pointSize << 6,
                            res,
                            res );
  }


  /*************************************************************************/
  /*                                                                       */
  /*  Layout a string of glyphs.  The glyphs are untransformed.            */
  /*                                                                       */
  /*                                                                       */
  static void
  layout_glyphs( void )
  {
    PGlyph     glyph = glyphs;
    int        n;
    FT_Vector  origin;
    FT_Pos     origin_x = 0;
    FT_UInt    load_flags;
    FT_UInt    num_grays;
    FT_UInt    prev_index = 0;


    load_flags = FT_LOAD_DEFAULT;
    if ( !hinted )
      load_flags |= FT_LOAD_NO_HINTING;
    if ( !use_sbits )
      load_flags |= FT_LOAD_NO_BITMAP;

    num_grays = 256;
    if ( !antialias )
      num_grays = 0;

    for ( n = 0; n < num_glyphs; n++, glyph++ )
    {
      /* compute glyph origin */
      if ( kerning )
      {
        if ( prev_index )
        {
          FT_Vector  kern;


          FT_Get_Kerning( face, prev_index, glyph->glyph_index,
                          hinted ? ft_kerning_default : ft_kerning_unfitted,
                          &kern );

          origin_x += kern.x;
        }
        prev_index = glyph->glyph_index;
      }

      origin.x = origin_x;
      origin.y = 0;

      /* clear existing image if there is one */
      if ( glyph->image )
        FT_Done_Glyph( glyph->image );

      /* load the glyph image (in its native format); */
      /* for now, we take a monochrome glyph bitmap   */
      error = FT_Load_Glyph( face, glyph->glyph_index, load_flags );
      if ( error )
        continue;

      error = FT_Get_Glyph ( face->glyph, &glyph->image );
      if ( error )
        continue;

      glyph->pos = origin;

      origin_x  += face->glyph->advance.x;
    }

    string_center.x = ( origin_x / 2 ) & -64;
    string_center.y = 0;

    if ( transform )
      FT_Vector_Transform( &string_center, &trans_matrix );
  }


  /*************************************************************************/
  /*                                                                       */
  /* Render a given glyph vector set.                                      */
  /*                                                                       */
  /*                                                                       */
  static void
  render_string( FT_Pos  x, FT_Pos  y )
  {
    PGlyph     glyph = glyphs;
    grBitmap   bit3;
    int        n;
    FT_Vector  delta;


    /* first of all, we must compute the general delta for the glyph set */
    delta.x = ( x << 6 ) - string_center.x;
    delta.y = ( ( bit.rows - y ) << 6 ) - string_center.y;

    for ( n = 0; n < num_glyphs; n++, glyph++ )
    {
      FT_Glyph   image;
      FT_Vector  vec;


      if ( !glyph->image )
        continue;

      /* copy image */
      error = FT_Glyph_Copy( glyph->image, &image );
      if ( error )
        continue;

      /* transform it */
      vec = glyph->pos;
      FT_Vector_Transform( &vec, &trans_matrix );
      vec.x += delta.x;
      vec.y += delta.y;
      error = FT_Glyph_Transform( image, &trans_matrix, &vec );
      if ( !error )
      {
        FT_BBox  bbox;


        /* check bounding box; if it is not within the display surface, */
        /* we don't need to render it                                   */

        FT_Glyph_Get_CBox( image, ft_glyph_bbox_pixels, &bbox );

#if 0
        if ( n == 0 )
        {
          fprintf( stderr, "bbox = [%ld %ld %ld %ld]\n",
                    bbox.xMin, bbox.yMin, bbox.xMax, bbox.yMax );
        }
#endif

        if ( bbox.xMax > 0         && bbox.yMax > 0        &&
             bbox.xMin < bit.width && bbox.yMin < bit.rows )
        {
          /* convert to a bitmap - destroy native image */
          error = FT_Glyph_To_Bitmap( &image,
                                      antialias ? ft_render_mode_normal
                                                : ft_render_mode_mono,
                                      0, 1 );
          if ( !error )
          {
            FT_BitmapGlyph  bitmap = (FT_BitmapGlyph)image;
            FT_Bitmap*      source = &bitmap->bitmap;
            FT_Pos          x_top, y_top;

#if 0
            if ( n == 0 )
            {
              fprintf( stderr, "bearing = [%d %d] dims = [%d %d]\n",
                       bitmap->left, bitmap->top, source->width, source->rows );
            }
#endif


            bit3.rows   = source->rows;
            bit3.width  = source->width;
            bit3.pitch  = source->pitch;
            bit3.buffer = source->buffer;

            switch ( source->pixel_mode )
            {
            case ft_pixel_mode_mono:
              bit3.mode = gr_pixel_mode_mono;
              break;

            case ft_pixel_mode_grays:
              bit3.mode  = gr_pixel_mode_gray;
              bit3.grays = source->num_grays;
              if (use_gamma)
                apply_gamma(&bit3);
              break;

            default:
              continue;
            }

            /* now render the bitmap into the display surface */
            x_top = bitmap->left;
            y_top = bit.rows - bitmap->top;
            grBlitGlyphToBitmap( 0, &bit, &bit3, x_top, y_top, fore_color );
          }
        }
      }
      FT_Done_Glyph( image );
    }
  }


  /*************************************************************************/
  /*                                                                       */
  /*  Convert a string of text into a glyph vector.                        */
  /*                                                                       */
  /*  XXX: For now, we perform a trivial conversion.                       */
  /*                                                                       */
  /*                                                                       */
  static void
  prepare_text( const unsigned char*  string )
  {
    const unsigned char*  p     = (const unsigned char*)string;
    PGlyph                glyph = glyphs;
    FT_UInt               glyph_index;


    error = FT_Select_Charmap( face, encoding );
    if ( error )
      PanicZ( "invalid charmap\n" );

    num_glyphs = 0;
    while ( *p )
    {
      glyph_index = FT_Get_Char_Index( face, (FT_ULong)*p );
      glyph->glyph_index = glyph_index;
      glyph++;
      num_glyphs++;
      if ( num_glyphs >= MAX_GLYPHS )
        break;
      p++;
    }
  }


  static void
  reset_transform( void )
  {
    double    angle   = Rotation * 3.14159 / 64.0;
    FT_Fixed  cosinus = (FT_Fixed)( cos( angle ) * 65536.0 );
    FT_Fixed  sinus   = (FT_Fixed)( sin( angle ) * 65536.0 );


    transform       = ( angle != 0 );
    trans_matrix.xx = cosinus;
    trans_matrix.yx = sinus;
    trans_matrix.xy = -sinus;
    trans_matrix.yy = cosinus;
  }


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
  Help( void )
  {
    grEvent  dummy_event;


    clear_display();
    grGotoxy( 0, 0 );
    grSetMargin( 2, 1 );
    grGotobitmap( &bit );

    grWriteln("FreeType String Viewer - part of the FreeType test suite" );
    grLn();
    grWriteln("This program is used to display a string of text using" );
    grWriteln("the new convenience API of the FreeType 2 library.");
    grLn();
    grWriteln("Use the following keys :");
    grLn();
    grWriteln("  F1 or ?   : display this help screen" );
    grWriteln("  a         : toggle anti-aliasing" );
    grWriteln("  h         : toggle outline hinting" );
    grWriteln("  k         : toggle kerning" );
    grWriteln("  g         : toggle gamma correction" );
    grLn();
    grWriteln("  Up        : increase pointsize by 1 unit" );
    grWriteln("  Down      : decrease pointsize by 1 unit" );
    grWriteln("  Page Up   : increase pointsize by 10 units" );
    grWriteln("  Page Down : decrease pointsize by 10 units" );
    grLn();
    grWriteln("  Right     : rotate counter-clockwise" );
    grWriteln("  Left      : rotate clockwise" );
    grWriteln("  F7        : big rotate counter-clockwise");
    grWriteln("  F8        : big rotate clockwise");
    grLn();
    grWriteln("  F9        : decrease gamma by 0.1" );
    grWriteln("  F10       : increase gamma by 0.1" );
    grLn();
    grWriteln("press any key to exit this help screen");

    grRefreshSurface( surface );
    grListenSurface( surface, gr_event_key, &dummy_event );
  }


  static int
  Process_Event( grEvent*  event )
  {
    int  i;


    switch ( event->key )
    {
    case grKeyEsc:            /* ESC or q */
    case grKEY( 'q' ):
      return 0;

    case grKEY( 'k' ):
      kerning = !kerning;
      new_header = kerning
                     ? (char *)"kerning is now active"
                     : (char *)"kerning is now ignored";
      return 1;

    case grKEY( 'a' ):
      antialias  = !antialias;
      new_header = antialias
                     ? (char *)"anti-aliasing is now on"
                     : (char *)"anti-aliasing is now off";
      return 1;

    case grKEY( 'b' ):
#if 0
      use_sbits  = !use_sbits;
      new_header = use_sbits
                     ? (char *)"embedded bitmaps are now used when available"
                     : (char *)"embedded bitmaps are now ignored";
#else
      new_header = (char *)"embedded bitmaps can't be rotated/transformed";
#endif
      return 1;

    case grKEY( 'n' ):
    case grKEY( 'p' ):
      return (int)event->key;

    case grKEY( 'h' ):
      hinted     = !hinted;
      new_header = hinted
                     ? (char *)"glyph hinting is now active"
                     : (char *)"glyph hinting is now ignored";
      break;

    case grKEY( 'g' ):
      use_gamma = !use_gamma;
      new_header = use_gamma
                     ? (char *)"gamma correction is now on"
                     : (char *)"gamma correction is now off";
      return 1;

    case grKeyF1:
    case grKEY( '?' ):
      Help();
      return 1;

#if 0
    case grKeyF3: i =  16; goto Do_Rotate;
    case grKeyF4: i = -16; goto Do_Rotate;
    case grKeyF5: i =   1; goto Do_Rotate;
    case grKeyF6: i =  -1; goto Do_Rotate;
#endif

    case grKeyPageUp:   i =  10; goto Do_Scale;
    case grKeyPageDown: i = -10; goto Do_Scale;
    case grKeyUp:       i =   1; goto Do_Scale;
    case grKeyDown:     i =  -1; goto Do_Scale;

    case grKeyLeft:  i =  -1; goto Do_Rotate;
    case grKeyRight: i =   1; goto Do_Rotate;
    case grKeyF7:    i = -10; goto Do_Rotate;
    case grKeyF8:    i =  10; goto Do_Rotate;

    case grKeyF9 : if (gamma_value > 0.1f) gamma_value -= 0.1f; goto Do_Gamma_Set;
    case grKeyF10:                         gamma_value += 0.1f; goto Do_Gamma_Set;
    default:
      ;
    }
    return 1;

  Do_Rotate:
    Rotation = ( Rotation + i ) & 127;
    return 1;

  Do_Gamma_Set:
  {
    static char header_buffer[64];

    sprintf(header_buffer, "gamma is %.1f", gamma_value);
    new_header = header_buffer;
    init_gamma();
    return 1;
  }

  Do_Scale:
    ptsize += i;
    if ( ptsize < 1 )         ptsize = 1;
    if ( ptsize > MAXPTSIZE ) ptsize = MAXPTSIZE;
    return 1;

#if 0
  Do_Glyph:
    Num += i;
    if ( Num < 0 )           Num = 0;
    if ( Num >= num_glyphs ) Num = num_glyphs - 1;
    return 1;
#endif
  }


/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
/****                                                                    ****/
/****                       M A I N   P R O G R A M                      ****/
/****                                                                    ****/
/****************************************************************************/
/****************************************************************************/
/****************************************************************************/


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


  int
  main( int     argc,
        char**  argv )
  {
    int    i, old_ptsize, orig_ptsize, file;
    int    first_glyph = 0;
    int    XisSetup = 0;
    char   filename[128 + 4];
    char   alt_filename[128 + 4];
    char*  execname;
    int    option;
    int    file_loaded;

    grEvent   event;


    execname = ft_basename( argv[0] );

    while ( 1 )
    {
      option = getopt( argc, argv, "e:m:r:" );

      if ( option == -1 )
        break;

      switch ( option )
      {
      case 'e':
        encoding = (FT_Encoding)make_tag( optarg );
        break;

      case 'r':
        res = atoi( optarg );
        if ( res < 1 )
          usage( execname );
        break;

      case 'm':
        if ( argc < 3 )
          usage( execname );
        Text = optarg;
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

    /* Initialize engine */
    error = FT_Init_FreeType( &library );
    if ( error )
      PanicZ( "Could not initialize FreeType library" );

  NewFile:
    ptsize      = orig_ptsize;
    hinted      = 1;
    file_loaded = 0;

#ifndef macintosh
    i = strlen( argv[file] );
    while ( i > 0 && argv[file][i] != '\\' && argv[file][i] != '/' )
    {
      if ( argv[file][i] == '.' )
        i = 0;
      i--;
    }
#endif

    filename[128] = '\0';
    alt_filename[128] = '\0';

    strncpy( filename, argv[file], 128 );
    strncpy( alt_filename, argv[file], 128 );

    /* first, try to load the glyph name as-is */
    error = FT_New_Face( library, filename, 0, &face );
    if ( !error )
      goto Success;

#ifndef macintosh
    if ( i >= 0 )
    {
      strncpy( filename + strlen( filename ), ".ttf", 4 );
      strncpy( alt_filename + strlen( alt_filename ), ".ttc", 4 );
    }
#endif

    /* if it didn't work, try to add ".ttf" at the end */
    error = FT_New_Face( library, filename, 0, &face );
    if ( error )
      goto Display_Font;

  Success:
    init_gamma();

    /* prepare the text to be rendered */
    prepare_text( (unsigned char*)Text );

    file_loaded++;

    reset_scale( ptsize );

  Display_Font:
    /* initialise graphics if needed */
    if ( !XisSetup )
    {
      XisSetup = 1;
      init_display();
    }

    grSetTitle( surface, "FreeType String Viewer - press F1 for help" );
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

      clear_display();

      if ( file_loaded >= 1 )
      {
        /* layout & render string */
        {
          reset_transform();
          layout_glyphs();
          if (use_gamma)
            draw_gamma_ramp();
          render_string( bit.width/2, bit.rows/2 );
        }

        sprintf( Header, "%s %s (file %s)",
                         face->family_name,
                         face->style_name,
                         ft_basename( filename ) );

        if (!new_header)
          new_header = Header;

        grWriteCellString( &bit, 0, 0, new_header, fore_color );
        new_header = 0;

        sprintf( Header, "at %d points, rotation = %d",
                         ptsize,
                         Rotation );
      }
      else
      {
        sprintf( Header, "`%s' is not a font file or could not be opened",
                         ft_basename(filename) );
      }

      grWriteCellString( &bit, 0, 8, Header, fore_color );
      grRefreshSurface( surface );

      grListenSurface( surface, 0, &event );
      if ( !( key = Process_Event( &event ) ) )
        goto Fin;

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

      if ( ptsize != old_ptsize )
      {
        reset_scale( ptsize );

        old_ptsize = ptsize;
      }
    }

  Fin:
#if 0
    grDoneSurface( surface );
    grDone();
#endif
    printf( "Execution completed successfully.\n" );
    printf( "Fails = %d\n", Fail );

    exit( 0 );      /* for safety reasons */
    return 0;       /* never reached */
}


/* End */
