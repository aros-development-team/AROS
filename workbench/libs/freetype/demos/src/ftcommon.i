/****************************************************************************/
/*                                                                          */
/*  The FreeType project -- a free and portable quality TrueType renderer.  */
/*                                                                          */
/*  Copyright 1996-2000, 2001, 2002 by                                      */
/*  D. Turner, R.Wilhelm, and W. Lemberg                                    */
/*                                                                          */
/*                                                                          */
/*  ftcommon.i - common routines for the FreeType demo programs.            */
/*                                                                          */
/****************************************************************************/


#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_CACHE_H

#include FT_CACHE_IMAGE_H
#include FT_CACHE_SMALL_BITMAPS_H
#include FT_CACHE_CHARMAP_H

  /* the following header shouldn't be used in normal programs */
#include FT_INTERNAL_DEBUG_H

#include "common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>


  /* forward declarations */
  extern void  PanicZ( const char*  message );

  static FT_Error  error;


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                 DISPLAY-SPECIFIC DEFINITIONS                  *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/


#include "graph.h"
#include "grfont.h"


#define DIM_X      500
#define DIM_Y      400

#define CENTER_X   ( bit.width / 2 )
#define CENTER_Y   ( bit.rows / 2 )

#define MAXPTSIZE  500                 /* dtp */


  char   Header[128];
  char*  new_header = 0;

  const unsigned char*  Text = (unsigned char*)
    "The quick brown fox jumps over the lazy dog 0123456789 "
    "\342\352\356\373\364\344\353\357\366\374\377\340\371\351\350\347 "
    "&#~\"\'(-`_^@)=+\260 ABCDEFGHIJKLMNOPQRSTUVWXYZ "
    "$\243^\250*\265\371%!\247:/;.,?<>";

  grSurface*  surface;      /* current display surface */
  grBitmap    bit;          /* current display bitmap  */

  static grColor  fore_color = { 0 };

  int  Fail;

  int  graph_init  = 0;

  int  render_mode = 0;
  int  debug       = 0;
  int  trace_level = 0;


#define RASTER_BUFF_SIZE   32768
  char  raster_buff[RASTER_BUFF_SIZE];


#undef  NODEBUG

#ifndef NODEBUG

#define LOG( x )  LogMessage##x


  void
  LogMessage( const char*  fmt, ... )
  {
    va_list  ap;


    va_start( ap, fmt );
    vfprintf( stderr, fmt, ap );
    va_end( ap );
  }

#else /* !DEBUG */

#define LOG( x )  /* */

#endif


  /* PanicZ */
  void
  PanicZ( const char*  message )
  {
    fprintf( stderr, "%s\n  error = 0x%04x\n", message, error );
    exit( 1 );
  }


  /* clear the `bit' bitmap/pixmap */
  static void
  Clear_Display( void )
  {
    long  image_size = (long)bit.pitch * bit.rows;


    if ( image_size < 0 )
      image_size = -image_size;
    memset( bit.buffer, 255, image_size );
  }


  /* initialize the display bitmap `bit' */
  static void
  Init_Display( void )
  {
    grInitDevices();

    bit.mode  = gr_pixel_mode_rgb24;
    bit.width = DIM_X;
    bit.rows  = DIM_Y;
    bit.grays = 256;

    surface = grNewSurface( 0, &bit );
    if ( !surface )
      PanicZ( "could not allocate display surface\n" );

    graph_init = 1;
  }


#define MAX_BUFFER  300000


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****               FREETYPE-SPECIFIC DEFINITIONS                   *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/


  FT_Library      library;       /* the FreeType library            */
  FTC_Manager     cache_manager; /* the cache manager               */
  FTC_ImageCache  image_cache;   /* the glyph image cache           */
  FTC_SBitCache   sbits_cache;   /* the glyph small bitmaps cache   */
  FTC_CMapCache   cmap_cache;    /* the charmap cache..             */

  FT_Face         face;          /* the font face                   */
  FT_Size         size;          /* the font size                   */
  FT_GlyphSlot    glyph;         /* the glyph slot                  */

  FTC_ImageTypeRec  current_font;

  int  dump_cache_stats = 0;  /* do we need to dump cache statistics? */
  int  use_sbits_cache  = 1;

  int  num_indices;           /* number of glyphs or characters */
  int  ptsize;                /* current point size             */

  FT_Encoding  encoding = ft_encoding_none;

  int  hinted    = 1;         /* is glyph hinting active?    */
  int  antialias = 1;         /* is anti-aliasing active?    */
  int  use_sbits = 1;         /* do we use embedded bitmaps? */
  int  low_prec  = 0;         /* force low precision         */
  int  autohint  = 0;         /* force auto-hinting          */
  int  lcd_mode  = 0;         /* 0 - 4                       */
  int  Num;                   /* current first index         */

  int  res = 72;


#define MAX_GLYPH_BYTES  150000   /* 150kB for the glyph image cache */


#define FLOOR( x )  (   (x)        & -64 )
#define CEIL( x )   ( ( (x) + 63 ) & -64 )
#define TRUNC( x )  (   (x) >> 6 )


  /* this simple record is used to model a given `installed' face */
  typedef struct  TFont_
  {
    const char*  filepathname;
    int          face_index;

    int          num_indices;

  } TFont, *PFont;

  static PFont*  fonts;
  static int     num_fonts;
  static int     max_fonts = 0;

  static
  const char*  file_suffixes[] =
  {
    ".ttf",
    ".ttc",
    ".pfa",
    ".pfb",
    0
  };


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


  static int
  install_font_file( char*  filepath )
  {
    static char   filename[1024 + 5];
    int           i, len, suffix_len, num_faces;
    const char**  suffix;


    len = strlen( filepath );
    if ( len > 1024 )
      len = 1024;

    strncpy( filename, filepath, len );
    filename[len] = 0;

    error = FT_New_Face( library, filename, 0, &face );
    if ( !error )
      goto Success;

    /* could not open the file directly; we will now try various */
    /* suffixes like `.ttf' or `.pfb'                            */

#ifndef macintosh

    suffix     = file_suffixes;
    suffix_len = 0;
    i          = len - 1;

    while ( i > 0 && filename[i] != '\\' && filename[i] != '/' )
    {
      if ( filename[i] == '.' )
      {
        suffix_len = i;
        break;
      }
      i--;
    }
    if ( suffix_len == 0 )
    {
      for ( suffix = file_suffixes; suffix[0]; suffix++ )
      {
        /* try with current suffix */
        strcpy( filename + len, suffix[0] );

        error = FT_New_Face( library, filename, 0, &face );
        if ( !error )
          goto Success;
      }
    }

#endif /* !macintosh */

    /* really couldn't open this file */
    return -1;

  Success:

    /* allocate new font object */
    num_faces = face->num_faces;
    for ( i = 0; i < num_faces; i++ )
    {
      PFont  font;


      if ( i > 0 )
      {
        error = FT_New_Face( library, filename, i, &face );
        if ( error )
          continue;
      }

      if ( encoding != ft_encoding_none )
      {
        error = FT_Select_Charmap( face, encoding );
        if ( error )
        {
          FT_Done_Face( face );
          continue;
        }
      }

      font = (PFont)malloc( sizeof ( *font ) );
      font->filepathname = (char*)malloc( strlen( filename ) + 1 );
      font->face_index   = i;
      font->num_indices  = encoding != ft_encoding_none ? 0x10000L
                                                        : face->num_glyphs;

      strcpy( (char*)font->filepathname, filename );

      FT_Done_Face( face );

      if ( max_fonts == 0 )
      {
        max_fonts = 16;
        fonts     = (PFont*)malloc( max_fonts * sizeof ( PFont ) );
      }
      else if ( num_fonts >= max_fonts )
      {
        max_fonts *= 2;
        fonts      = (PFont*)realloc( fonts, max_fonts * sizeof ( PFont ) );
      }

      fonts[num_fonts++] = font;
    }

    return 0;
  }


  /*************************************************************************/
  /*                                                                       */
  /* The face requester is a function provided by the client application   */
  /* to the cache manager, whose role is to translate an `abstract' face   */
  /* ID into a real FT_Face object.                                        */
  /*                                                                       */
  /* In this program, the face IDs are simply pointers to TFont objects.   */
  /*                                                                       */
  FT_CALLBACK_DEF( FT_Error )
  my_face_requester( FTC_FaceID  face_id,
                     FT_Library  lib,
                     FT_Pointer  request_data,
                     FT_Face*    aface )
  {
    PFont  font = (PFont)face_id;

    FT_UNUSED( request_data );


    error = FT_New_Face( lib,
                         font->filepathname,
                         font->face_index,
                         aface );
    if ( encoding == ft_encoding_none || error )
      return error;

    return FT_Select_Charmap( *aface, encoding );
  }


  static void
  init_freetype( void )
  {
    error = FT_Init_FreeType( &library );
    if ( error )
      PanicZ( "could not initialize FreeType" );

    error = FTC_Manager_New( library, 0, 0, 0,
                             my_face_requester, 0, &cache_manager );
    if ( error )
      PanicZ( "could not initialize cache manager" );

    error = FTC_SBitCache_New( cache_manager, &sbits_cache );
    if ( error )
      PanicZ( "could not initialize small bitmaps cache" );

    error = FTC_ImageCache_New( cache_manager, &image_cache );
    if ( error )
      PanicZ( "could not initialize glyph image cache" );

    error = FTC_CMapCache_New( cache_manager, &cmap_cache );
    if ( error )
      PanicZ( "could not initialize charmap cache" );
  }


  static void
  done_freetype( void )
  {
    FTC_Manager_Done( cache_manager );
    FT_Done_FreeType( library );
  }


  static void
  set_current_face( PFont  font )
  {
    current_font.font.face_id = (FTC_FaceID)font;
  }


  static void
  set_current_size( int  pixel_size )
  {
    current_font.font.pix_width  = (FT_UShort) pixel_size;
    current_font.font.pix_height = (FT_UShort) pixel_size;
  }


  static void
  set_current_pointsize( int  point_size )
  {
    set_current_size( ( point_size * res + 36 ) / 72 );
  }


  static void
  set_current_image_type( void )
  {
    current_font.flags = antialias ? FT_LOAD_DEFAULT : FT_LOAD_TARGET_MONO;

    if ( !hinted )
      current_font.flags |= FT_LOAD_NO_HINTING;

    if ( autohint )
      current_font.flags |= FT_LOAD_FORCE_AUTOHINT;

    if ( !use_sbits )
      current_font.flags |= FT_LOAD_NO_BITMAP;

    if ( antialias && lcd_mode > 0 )
    {
      if ( lcd_mode == 1 || lcd_mode == 3 )
        current_font.flags |= FT_LOAD_TARGET_LCD;
      else
        current_font.flags |= FT_LOAD_TARGET_LCD_V;
    }
  }


  static void
  done_glyph_bitmap( FT_Pointer  _glyf )
  {
    if ( _glyf )
    {
      FT_Glyph  glyf = (FT_Glyph)_glyf;


      FT_Done_Glyph( glyf );
    }
  }


  static FT_UInt
  get_glyph_index( FT_UInt32  charcode )
  {
    FTC_CMapDescRec  desc;

    desc.face_id    = current_font.font.face_id;

    desc.type       = FTC_CMAP_BY_ENCODING;
    desc.u.encoding = encoding != ft_encoding_none ? encoding
                                                   : ft_encoding_unicode;

    return FTC_CMapCache_Lookup( cmap_cache, &desc, charcode );
  }


  static FT_Error
  get_glyph_bitmap( FT_ULong     Index,
                    grBitmap*    target,
                    int         *left,
                    int         *top,
                    int         *x_advance,
                    int         *y_advance,
                    FT_Pointer  *aglyf )
  {
    *aglyf = NULL;

    if ( encoding != ft_encoding_none )
      Index = get_glyph_index( Index );

    /* use the SBits cache to store small glyph bitmaps; this is a lot */
    /* more memory-efficient                                           */
    /*                                                                 */
    if ( use_sbits_cache                   &&
         current_font.font.pix_width  < 48 &&
         current_font.font.pix_height < 48 )
    {
      FTC_SBit  sbit;


      error = FTC_SBitCache_Lookup( sbits_cache,
                                    &current_font,
                                    Index,
                                    &sbit,
                                    NULL );
      if ( error )
        goto Exit;

      if ( sbit->buffer )
      {
        target->rows   = sbit->height;
        target->width  = sbit->width;
        target->pitch  = sbit->pitch;
        target->buffer = sbit->buffer;

        switch ( sbit->format )
        {
        case FT_PIXEL_MODE_MONO:
          target->mode = gr_pixel_mode_mono;
          break;

        case FT_PIXEL_MODE_GRAY:
          target->mode  = gr_pixel_mode_gray;
          target->grays = sbit->max_grays + 1;
          break;

        case FT_PIXEL_MODE_LCD:
          target->mode  = gr_pixel_mode_lcd;
          target->grays = sbit->max_grays + 1;
          break;

        case FT_PIXEL_MODE_LCD_V:
          target->mode  = gr_pixel_mode_lcdv;
          target->grays = sbit->max_grays + 1;
          break;

        default:
          return FT_Err_Invalid_Glyph_Format;
        }

        *left      = sbit->left;
        *top       = sbit->top;
        *x_advance = sbit->xadvance;
        *y_advance = sbit->yadvance;

        goto Exit;
      }
    }

    /* otherwise, use an image cache to store glyph outlines, and render */
    /* them on demand. we can thus support very large sizes easily..     */
    {
      FT_Glyph   glyf;

      error = FTC_ImageCache_Lookup( image_cache,
                                     &current_font,
                                     Index,
                                     &glyf,
                                     NULL );

      if ( !error )
      {
        FT_BitmapGlyph  bitmap;
        FT_Bitmap*      source;


        if ( glyf->format == ft_glyph_format_outline )
        {
          /* render the glyph to a bitmap, don't destroy original */
          error = FT_Glyph_To_Bitmap( &glyf,
                                      antialias ? FT_RENDER_MODE_NORMAL
                                                : FT_RENDER_MODE_MONO,
                                      NULL, 0 );
          if ( error )
            goto Exit;

          *aglyf = glyf;
        }

        if ( glyf->format != FT_GLYPH_FORMAT_BITMAP )
          PanicZ( "invalid glyph format returned!" );

        bitmap = (FT_BitmapGlyph)glyf;
        source = &bitmap->bitmap;

        target->rows   = source->rows;
        target->width  = source->width;
        target->pitch  = source->pitch;
        target->buffer = source->buffer;

        switch ( source->pixel_mode )
        {
        case FT_PIXEL_MODE_MONO:
          target->mode = gr_pixel_mode_mono;
          break;

        case FT_PIXEL_MODE_GRAY:
          target->mode  = gr_pixel_mode_gray;
          target->grays = source->num_grays;
          break;

        case FT_PIXEL_MODE_LCD:
          target->mode  = gr_pixel_mode_lcd;
          target->grays = source->num_grays;
          break;

        case FT_PIXEL_MODE_LCD_V:
          target->mode  = gr_pixel_mode_lcdv;
          target->grays = source->num_grays;
          break;

        default:
          return FT_Err_Invalid_Glyph_Format;
        }

        *left = bitmap->left;
        *top  = bitmap->top;

        *x_advance = ( glyf->advance.x + 0x8000 ) >> 16;
        *y_advance = ( glyf->advance.y + 0x8000 ) >> 16;
      }
    }

  Exit:
    return error;
  }


/* End */
