/****************************************************************************/
/*                                                                          */
/*  The FreeType project -- a free and portable quality TrueType renderer.  */
/*                                                                          */
/*  Copyright 2005-2018 by                                                  */
/*  D. Turner, R.Wilhelm, and W. Lemberg                                    */
/*                                                                          */
/*                                                                          */
/*  ftcommon.c - common routines for the graphic FreeType demo programs.    */
/*                                                                          */
/****************************************************************************/


#ifndef  _GNU_SOURCE
#define  _GNU_SOURCE /* we use `strcasecmp' */
#endif

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_MODULE_H

#include FT_CACHE_H
#include FT_CACHE_MANAGER_H

#include FT_BITMAP_H
#include FT_FONT_FORMATS_H

  /* error messages */
#undef FTERRORS_H_
#define FT_ERROR_START_LIST     {
#define FT_ERRORDEF( e, v, s )  case v: str = s; break;
#define FT_ERROR_END_LIST       default: str = "unknown error"; }

#include "common.h"
#include "ftcommon.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>


#ifdef _WIN32
#define strcasecmp  _stricmp
#endif


#define N_HINTING_ENGINES  2


  FT_Error  error;


#undef  NODEBUG

#ifndef NODEBUG

  void
  LogMessage( const char*  fmt,
              ... )
  {
    va_list  ap;


    va_start( ap, fmt );
    vfprintf( stderr, fmt, ap );
    va_end( ap );
  }

#endif /* NODEBUG */


  /* PanicZ */
  void
  PanicZ( const char*  message )
  {
    const FT_String  *str;


    switch( error )
    #include FT_ERRORS_H

    fprintf( stderr, "%s\n  error = 0x%04x, %s\n", message, error, str );
    exit( 1 );
  }


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                 DISPLAY-SPECIFIC DEFINITIONS                  *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/


  FTDemo_Display*
  FTDemo_Display_New( grPixelMode  mode,
                      int          width,
                      int          height )
  {
    FTDemo_Display*  display;
    grSurface*       surface;
    grBitmap         bit;


    display = (FTDemo_Display *)malloc( sizeof ( FTDemo_Display ) );
    if ( !display )
      return NULL;

    if ( mode != gr_pixel_mode_gray  &&
         mode != gr_pixel_mode_rgb24 )
      return NULL;

    grInitDevices();

    bit.mode  = mode;
    bit.width = width;
    bit.rows  = height;
    bit.grays = 256;

    surface = grNewSurface( 0, &bit );

    if ( !surface )
    {
      free( display );
      return NULL;
    }

    display->surface = surface;
    display->bitmap  = &surface->bitmap;

    display->fore_color = grFindColor( display->bitmap,
                                       0x00, 0x00, 0x00, 0xff );
    display->back_color = grFindColor( display->bitmap,
                                       0xff, 0xff, 0xff, 0xff );
    display->warn_color = grFindColor( display->bitmap,
                                       0xff, 0x00, 0x00, 0xff );

    display->gamma = GAMMA;

    return display;
  }


  void
  FTDemo_Display_Done( FTDemo_Display*  display )
  {
    if ( !display )
      return;

    grDoneBitmap( display->bitmap );
    grDoneSurface( display->surface );

    grDoneDevices();

    free( display );
  }


  void
  FTDemo_Display_Clear( FTDemo_Display*  display )
  {
    grBitmap*  bit   = display->bitmap;


    grFillRect( bit, 0, 0, bit->width, bit->rows, display->back_color );
  }


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****               FREETYPE-SPECIFIC DEFINITIONS                   *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/


#define FLOOR( x )  (   (x)        & -64 )
#define CEIL( x )   ( ( (x) + 63 ) & -64 )
#define ROUND( x )  ( ( (x) + 32 ) & -64 )
#define TRUNC( x )  (   (x) >> 6 )


  /*************************************************************************/
  /*                                                                       */
  /* The face requester is a function provided by the client application   */
  /* to the cache manager, whose role is to translate an `abstract' face   */
  /* ID into a real FT_Face object.                                        */
  /*                                                                       */
  /* In this program, the face IDs are simply pointers to TFont objects.   */
  /*                                                                       */
  static FT_Error
  my_face_requester( FTC_FaceID  face_id,
                     FT_Library  lib,
                     FT_Pointer  request_data,
                     FT_Face*    aface )
  {
    PFont  font = (PFont)face_id;

    FT_UNUSED( request_data );


    if ( font->file_address != NULL )
      error = FT_New_Memory_Face( lib,
                                  (const FT_Byte*)font->file_address,
                                  (FT_Long)font->file_size,
                                  font->face_index,
                                  aface );
    else
      error = FT_New_Face( lib,
                           font->filepathname,
                           font->face_index,
                           aface );
    if ( !error )
    {
      const char*  format = FT_Get_Font_Format( *aface );


      if ( !strcmp( format, "Type 1" ) )
      {
        char   orig[5];
        char*  suffix        = (char*)strrchr( font->filepathname, '.' );
        int    has_extension = suffix                                &&
                               ( strcasecmp( suffix, ".pfa" ) == 0 ||
                                 strcasecmp( suffix, ".pfb" ) == 0 );


        if ( has_extension )
          memcpy( orig, suffix, 5 );
        else
          /* we have already allocated four more bytes */
          suffix = (char*)font->filepathname + strlen( font->filepathname );

        memcpy( suffix, ".afm", 5 );
        if ( FT_Attach_File( *aface, font->filepathname ) )
        {
          memcpy( suffix, ".pfm", 5 );
          FT_Attach_File( *aface, font->filepathname );
        }

        if ( has_extension )
          memcpy( suffix, orig, 5 );
        else
          *suffix = '\0';
      }

      if ( (*aface)->charmaps )
        (*aface)->charmap = (*aface)->charmaps[font->cmap_index];
    }

    return error;
  }


  FTDemo_Handle*
  FTDemo_New( void )
  {
    FTDemo_Handle*  handle;


    handle = (FTDemo_Handle *)malloc( sizeof ( FTDemo_Handle ) );
    if ( !handle )
      return NULL;

    memset( handle, 0, sizeof ( FTDemo_Handle ) );

    error = FT_Init_FreeType( &handle->library );
    if ( error )
      PanicZ( "could not initialize FreeType" );

    error = FTC_Manager_New( handle->library, 0, 0, 0,
                             my_face_requester, 0, &handle->cache_manager );
    if ( error )
      PanicZ( "could not initialize cache manager" );

    error = FTC_SBitCache_New( handle->cache_manager, &handle->sbits_cache );
    if ( error )
      PanicZ( "could not initialize small bitmaps cache" );

    error = FTC_ImageCache_New( handle->cache_manager, &handle->image_cache );
    if ( error )
      PanicZ( "could not initialize glyph image cache" );

    error = FTC_CMapCache_New( handle->cache_manager, &handle->cmap_cache );
    if ( error )
      PanicZ( "could not initialize charmap cache" );

    FT_Bitmap_New( &handle->bitmap );

    FT_Stroker_New( handle->library, &handle->stroker );

    handle->encoding = FT_ENCODING_ORDER;

    handle->hinted    = 1;
    handle->use_sbits = 2;  /* including color bitmaps */
    handle->autohint  = 0;
    handle->lcd_mode  = LCD_MODE_AA;

    handle->use_sbits_cache = 1;

    /* string_init */
    memset( handle->string, 0, sizeof ( TGlyph ) * MAX_GLYPHS );
    handle->string_length = 0;
    handle->string_reload = 1;

    return handle;
  }


  void
  FTDemo_Done( FTDemo_Handle*  handle )
  {
    int  i;


    if ( !handle )
      return;

    for ( i = 0; i < handle->max_fonts; i++ )
    {
      if ( handle->fonts[i] )
      {
        if ( handle->fonts[i]->filepathname )
          free( (void*)handle->fonts[i]->filepathname );
        free( handle->fonts[i] );
      }
    }
    free( handle->fonts );

    /* string_done */
    for ( i = 0; i < MAX_GLYPHS; i++ )
    {
      PGlyph  glyph = handle->string + i;


      if ( glyph->image )
        FT_Done_Glyph( glyph->image );
    }

    FT_Stroker_Done( handle->stroker );
    FT_Bitmap_Done( handle->library, &handle->bitmap );
    FTC_Manager_Done( handle->cache_manager );
    FT_Done_FreeType( handle->library );

    free( handle );
  }


  FT_Error
  FTDemo_Install_Font( FTDemo_Handle*  handle,
                       const char*     filepath,
                       FT_Bool         outline_only,
                       FT_Bool         no_instances )
  {
    static char   filename[1024 + 5];
    long          i, num_faces;
    unsigned int  len;
    FT_Face       face;


    len = strlen( filepath );
    if ( len > 1024 )
      len = 1024;

    strncpy( filename, filepath, len );
    filename[len] = 0;

    /* We use a conservative approach here, at the cost of calling     */
    /* `FT_New_Face' quite often.  The idea is that our demo programs  */
    /* should be able to try all faces and named instances of a font,  */
    /* expecting that some faces don't work for various reasons, e.g., */
    /* a broken subfont, or an unsupported NFNT bitmap font in a Mac   */
    /* dfont resource that holds more than a single font.              */

    error = FT_New_Face( handle->library, filename, -1, &face );
    if ( error )
      return error;
    num_faces = face->num_faces;
    FT_Done_Face( face );

    /* allocate new font object(s) */
    for ( i = 0; i < num_faces; i++ )
    {
      PFont  font;
      long   j, instance_count;


      error = FT_New_Face( handle->library, filename, -( i + 1 ), &face );
      if ( error )
        continue;
      instance_count = no_instances ? 0 : face->style_flags >> 16;
      FT_Done_Face( face );

      /* load face with and without named instances */
      for ( j = 0; j < instance_count + 1; j++ )
      {
        error = FT_New_Face( handle->library,
                             filename,
                             ( j << 16 ) + i,
                             &face );
        if ( error )
          continue;

        if ( outline_only && !FT_IS_SCALABLE( face ) )
        {
          FT_Done_Face( face );
          continue;
        }

        if ( handle->encoding != FT_ENCODING_ORDER )
        {
          error = FT_Select_Charmap( face, (FT_Encoding)handle->encoding );
          if ( error )
            handle->encoding = FT_ENCODING_ORDER;
        }

        font = (PFont)malloc( sizeof ( *font ) );

        /* We allocate four more bytes since we want to attach an AFM */
        /* or PFM file for Type 1 fonts (if available).  Such fonts   */
        /* always have the extension `.afm' or `.pfm'.                */
        font->filepathname = (char*)malloc( strlen( filename ) + 4 + 1 );
        strcpy( (char*)font->filepathname, filename );

        font->face_index = ( j << 16 ) + i;
        font->cmap_index = face->charmap ? FT_Get_Charmap_Index( face->charmap )
                                         : 0;

        if ( handle->preload )
        {
          FILE*   file = fopen( filename, "rb" );
          size_t  file_size;


          if ( file == NULL )  /* shouldn't happen */
          {
            free( font );
            return FT_Err_Invalid_Argument;
          }

          fseek( file, 0, SEEK_END );
          file_size = (size_t)ftell( file );
          fseek( file, 0, SEEK_SET );

          if ( file_size <= 0 )
          {
            free( font );
            fclose( file );
            return FT_Err_Invalid_Stream_Operation;
          }

          font->file_address = malloc( file_size );
          if ( !font->file_address )
          {
            free( font );
            fclose( file );
            return FT_Err_Out_Of_Memory;
          }

          if ( !fread( font->file_address, file_size, 1, file ) )
          {
            free( font->file_address );
            free( font );
            fclose( file );
            return FT_Err_Invalid_Stream_Read;
          }

          font->file_size = file_size;

          fclose( file );
        }
        else
        {
          font->file_address = NULL;
          font->file_size    = 0;
        }

        switch ( handle->encoding )
        {
        case FT_ENCODING_ORDER:
          font->num_indices = face->num_glyphs;
          break;

        case FT_ENCODING_UNICODE:
          font->num_indices = 0x110000L;
          break;

        case FT_ENCODING_ADOBE_LATIN_1:
        case FT_ENCODING_ADOBE_STANDARD:
        case FT_ENCODING_ADOBE_EXPERT:
        case FT_ENCODING_ADOBE_CUSTOM:
        case FT_ENCODING_APPLE_ROMAN:
          font->num_indices = 0x100L;
          break;

          /* some fonts use range 0x00-0x100, others have 0xF000-0xF0FF */
        case FT_ENCODING_MS_SYMBOL:
          font->num_indices = 0x10000L;

        default:
          font->num_indices = 0x10000L;
        }

        FT_Done_Face( face );
        face = NULL;

        if ( handle->max_fonts == 0 )
        {
          handle->max_fonts = 16;
          handle->fonts     = (PFont*)calloc( (size_t)handle->max_fonts,
                                              sizeof ( PFont ) );
        }
        else if ( handle->num_fonts >= handle->max_fonts )
        {
          handle->max_fonts *= 2;
          handle->fonts      = (PFont*)realloc( handle->fonts,
                                                (size_t)handle->max_fonts *
                                                  sizeof ( PFont ) );

          memset( &handle->fonts[handle->num_fonts], 0,
                  (size_t)( handle->max_fonts - handle->num_fonts ) *
                    sizeof ( PFont ) );
        }

        handle->fonts[handle->num_fonts++] = font;
      }
    }

    return FT_Err_Ok;
  }


  void
  FTDemo_Set_Current_Font( FTDemo_Handle*  handle,
                           PFont           font )
  {
    handle->current_font   = font;
    handle->scaler.face_id = (FTC_FaceID)font;

    handle->string_reload = 1;
  }


  void
  FTDemo_Set_Current_Size( FTDemo_Handle*  handle,
                           int             pixel_size )
  {
    FT_Face  face;


    if ( pixel_size > 0xFFFF )
      pixel_size = 0xFFFF;

    error = FTC_Manager_LookupFace( handle->cache_manager,
                                    handle->scaler.face_id, &face );

    if ( !error && !FT_IS_SCALABLE ( face ) )
    {
      int  i, j = 0;
      int  c, d = abs( (int)face->available_sizes[j].y_ppem - pixel_size * 64 );


      for ( i = 1; i < face->num_fixed_sizes; i++ )
      {
        c = abs( (int)face->available_sizes[i].y_ppem - pixel_size * 64 );

        if ( c < d )
        {
          d = c;
          j = i;
        }
      }

      pixel_size = face->available_sizes[j].y_ppem / 64 ;
    }

    handle->scaler.width  = (FT_UInt)pixel_size;
    handle->scaler.height = (FT_UInt)pixel_size;
    handle->scaler.pixel  = 1;                  /* activate integer format */
    handle->scaler.x_res  = 0;
    handle->scaler.y_res  = 0;

    handle->string_reload = 1;
  }


  void
  FTDemo_Set_Current_Charsize( FTDemo_Handle*  handle,
                               int             char_size,
                               int             resolution )
  {
    FT_Face  face;


    /* in 26.6 format, corresponding to (almost) 0x4000ppem */
    if ( char_size > 0xFFFFF )
      char_size = 0xFFFFF;

    error = FTC_Manager_LookupFace( handle->cache_manager,
                                    handle->scaler.face_id, &face );

    if ( !error && !FT_IS_SCALABLE ( face ) )
    {
      int  pixel_size = char_size * resolution / 72;
      int  i, j = 0;
      int  c, d = abs( (int)face->available_sizes[j].y_ppem - pixel_size );


      for ( i = 1; i < face->num_fixed_sizes; i++ )
      {
        c = abs( (int)face->available_sizes[i].y_ppem - pixel_size );

        if ( c < d )
        {
          d = c;
          j = i;
        }
      }

      char_size = face->available_sizes[j].y_ppem * 72 / resolution;
    }

    handle->scaler.width  = (FT_UInt)char_size;
    handle->scaler.height = (FT_UInt)char_size;
    handle->scaler.pixel  = 0;                     /* activate 26.6 format */
    handle->scaler.x_res  = (FT_UInt)resolution;
    handle->scaler.y_res  = (FT_UInt)resolution;

    handle->string_reload = 1;
  }


  void
  FTDemo_Set_Preload( FTDemo_Handle*  handle,
                      int             preload )
  {
    handle->preload = !!preload;
  }


  void
  FTDemo_Update_Current_Flags( FTDemo_Handle*  handle )
  {
    FT_Int32  flags, target;


    flags = FT_LOAD_DEFAULT;  /* really 0 */

    if ( handle->autohint )
      flags |= FT_LOAD_FORCE_AUTOHINT;

    if ( handle->use_sbits == 0 )
      flags |= FT_LOAD_NO_BITMAP;
    else if ( handle->use_sbits == 2 )
      flags |= FT_LOAD_COLOR;

    if ( handle->hinted )
    {
      target = 0;

      switch ( handle->lcd_mode )
      {
      case LCD_MODE_MONO:
        target = FT_LOAD_TARGET_MONO;
        break;

      case LCD_MODE_LIGHT:
      case LCD_MODE_LIGHT_SUBPIXEL:
        target = FT_LOAD_TARGET_LIGHT;
        break;

      case LCD_MODE_RGB:
      case LCD_MODE_BGR:
        target = FT_LOAD_TARGET_LCD;
        break;

      case LCD_MODE_VRGB:
      case LCD_MODE_VBGR:
        target = FT_LOAD_TARGET_LCD_V;
        break;

      default:
        target = FT_LOAD_TARGET_NORMAL;
      }

      flags |= target;
    }
    else
    {
      flags |= FT_LOAD_NO_HINTING;

      if ( handle->lcd_mode == LCD_MODE_MONO )
        flags |= FT_LOAD_MONOCHROME;
    }

    handle->load_flags    = flags;
    handle->string_reload = 1;
  }


  FT_UInt
  FTDemo_Get_Index( FTDemo_Handle*  handle,
                    FT_UInt32       charcode )
  {
    FTC_FaceID  face_id = handle->scaler.face_id;
    PFont       font    = handle->current_font;


    return FTC_CMapCache_Lookup( handle->cmap_cache, face_id,
                                 font->cmap_index, charcode );
  }


  FT_Error
  FTDemo_Get_Size( FTDemo_Handle*  handle,
                   FT_Size*        asize )
  {
    FT_Size  size;


    error = FTC_Manager_LookupSize( handle->cache_manager,
                                    &handle->scaler,
                                    &size );

    if ( !error )
      *asize = size;

    return error;
  }


  void
  FTDemo_Draw_Header( FTDemo_Handle*   handle,
                      FTDemo_Display*  display,
                      int              ptsize,
                      int              res,
                      int              idx,
                      int              error_code )
  {
    FT_Face      face;
    char         buf[256];
    const char*  basename;
    int          ppem;

    int          line = 0;
    int          x;


    error = FTC_Manager_LookupFace( handle->cache_manager,
                                    handle->scaler.face_id, &face );
    if ( error )
    {
      FTDemo_Display_Done( display );
      FTDemo_Done( handle );
      PanicZ( "can't access font file" );
    }


    /* font and file name */
    basename = ft_basename( handle->current_font->filepathname );
    sprintf( buf, "%.50s %.50s (file `%.100s')",
             face->family_name, face->style_name, basename );
    grWriteCellString( display->bitmap, 0, (line++) * HEADER_HEIGHT,
                       buf, display->fore_color );

    /* ppem, pt and dpi, instance */
    ppem = FT_IS_SCALABLE( face ) ? FT_MulFix( face->units_per_EM,
                                               face->size->metrics.y_scale )
                                  : face->size->metrics.y_ppem * 64;

    if ( res == 72 )
      x  = sprintf( buf, "%.4g ppem", ppem / 64.0 );
    else
      x  = sprintf( buf, "%g pt at %d dpi, %.4g ppem",
                         ptsize / 64.0, res, ppem / 64.0 );

    if ( face->face_index >> 16 )
      x += sprintf( buf + x, ", instance %ld/%ld",
                             face->face_index >> 16,
                             face->style_flags >> 16 );

    grWriteCellString( display->bitmap, 0, line * HEADER_HEIGHT,
                       buf, display->fore_color );

    if ( abs( ptsize * res / 64 - face->size->metrics.y_ppem * 72 ) > 36 ||
         error_code                                                      )
    {
      switch ( error_code )
      {
      case FT_Err_Ok:
        sprintf( buf, "Available size shown" );
        break;
      case FT_Err_Invalid_Pixel_Size:
        sprintf( buf, "Invalid pixel size" );
        break;
      case FT_Err_Invalid_PPem:
        sprintf( buf, "Invalid ppem value" );
        break;
      default:
        sprintf( buf, "Error 0x%04x", (FT_UShort)error_code );
      }
      grWriteCellString( display->bitmap, 8 * x + 16, line * HEADER_HEIGHT,
                         buf, display->warn_color );
    }

    /* gamma */
    if ( display->gamma == 0.0 )
      sprintf( buf, "gamma: sRGB" );
    else
      sprintf( buf, "gamma = %.1f", display->gamma );
    grWriteCellString( display->bitmap, DIM_X - 8 * 11, line * HEADER_HEIGHT,
                       buf, display->fore_color );

    line++;

    /* encoding charcode or glyph index, glyph name */
    if ( idx >= 0 )
    {
      const char*  encoding = NULL;


      switch ( handle->encoding )
      {
      case FT_ENCODING_ORDER:
        encoding = "glyph order";
        break;
      case FT_ENCODING_MS_SYMBOL:
        encoding = "MS Symbol";
        break;
      case FT_ENCODING_UNICODE:
        encoding = "Unicode";
        break;
      case FT_ENCODING_SJIS:
        encoding = "SJIS";
        break;
      case FT_ENCODING_PRC:
        encoding = "PRC";
        break;
      case FT_ENCODING_BIG5:
        encoding = "Big5";
        break;
      case FT_ENCODING_WANSUNG:
        encoding = "Wansung";
        break;
      case FT_ENCODING_JOHAB:
        encoding = "Johab";
        break;
      case FT_ENCODING_ADOBE_STANDARD:
        encoding = "Adobe Standard";
        break;
      case FT_ENCODING_ADOBE_EXPERT:
        encoding = "Adobe Expert";
        break;
      case FT_ENCODING_ADOBE_CUSTOM:
        encoding = "Adobe Custom";
        break;
      case FT_ENCODING_ADOBE_LATIN_1:
        encoding = "Latin 1";
        break;
      case FT_ENCODING_OLD_LATIN_2:
        encoding = "Latin 2";
        break;
      case FT_ENCODING_APPLE_ROMAN:
        encoding = "Apple Roman";
        break;
      default:
        encoding = "Other";
      }

      if ( handle->encoding == FT_ENCODING_ORDER )
        x = sprintf( buf, "%s idx: %d",
                          encoding, idx );
      else if ( handle->encoding == FT_ENCODING_UNICODE )
        x = sprintf( buf, "%s charcode: U+%04X (glyph idx %d)",
                          encoding, idx,
                          FTDemo_Get_Index( handle, (FT_UInt32)idx ) );
      else
        x = sprintf( buf, "%s charcode: 0x%X (glyph idx %d)",
                          encoding, idx,
                          FTDemo_Get_Index( handle, (FT_UInt32)idx ) );

      if ( FT_HAS_GLYPH_NAMES( face ) )
      {
        unsigned int  glyph_idx;


        x += sprintf( buf + x, ", name: " );

        glyph_idx = (unsigned int)idx;
        if ( handle->encoding != FT_ENCODING_ORDER )
          glyph_idx = FTDemo_Get_Index( handle, (FT_UInt32)idx );

        FT_Get_Glyph_Name( face, glyph_idx, buf + x, (FT_UInt)( 256 - x ) );
      }

      grWriteCellString( display->bitmap, 0, (line++) * HEADER_HEIGHT,
                         buf, display->fore_color );
    }

  }


  FT_Error
  FTDemo_Glyph_To_Bitmap( FTDemo_Handle*  handle,
                          FT_Glyph        glyf,
                          grBitmap*       target,
                          int*            left,
                          int*            top,
                          int*            x_advance,
                          int*            y_advance,
                          FT_Glyph*       aglyf )
  {
    FT_BitmapGlyph  bitmap;
    FT_Bitmap*      source;


    *aglyf = NULL;

    error = FT_Err_Ok;

    if ( glyf->format == FT_GLYPH_FORMAT_OUTLINE )
    {
      FT_Render_Mode  render_mode;


      switch ( handle->lcd_mode )
      {
      case LCD_MODE_MONO:
        render_mode = FT_RENDER_MODE_MONO;
        break;

      case LCD_MODE_LIGHT:
      case LCD_MODE_LIGHT_SUBPIXEL:
        render_mode = FT_RENDER_MODE_LIGHT;
        break;

      case LCD_MODE_RGB:
      case LCD_MODE_BGR:
        render_mode = FT_RENDER_MODE_LCD;
        break;

      case LCD_MODE_VRGB:
      case LCD_MODE_VBGR:
        render_mode = FT_RENDER_MODE_LCD_V;
        break;

      default:
        render_mode = FT_RENDER_MODE_NORMAL;
      }

      /* render the glyph to a bitmap, don't destroy original */
      error = FT_Glyph_To_Bitmap( &glyf, render_mode, NULL, 0 );
      if ( error )
        return error;

      *aglyf = glyf;
    }

    if ( glyf->format != FT_GLYPH_FORMAT_BITMAP )
      PanicZ( "invalid glyph format returned!" );

    bitmap = (FT_BitmapGlyph)glyf;
    source = &bitmap->bitmap;

    target->rows   = (int)source->rows;
    target->width  = (int)source->width;
    target->pitch  = source->pitch;
    target->buffer = source->buffer;
    target->grays  = source->num_grays;

    switch ( source->pixel_mode )
    {
    case FT_PIXEL_MODE_MONO:
      target->mode = gr_pixel_mode_mono;
      break;

    case FT_PIXEL_MODE_GRAY:
      target->mode  = gr_pixel_mode_gray;
      target->grays = source->num_grays;
      break;

    case FT_PIXEL_MODE_GRAY2:
    case FT_PIXEL_MODE_GRAY4:
      (void)FT_Bitmap_Convert( handle->library, source, &handle->bitmap, 1 );
      target->pitch  = handle->bitmap.pitch;
      target->buffer = handle->bitmap.buffer;
      target->mode   = gr_pixel_mode_gray;
      target->grays  = handle->bitmap.num_grays;
      break;

    case FT_PIXEL_MODE_LCD:
      target->mode  = handle->lcd_mode == LCD_MODE_RGB
                      ? gr_pixel_mode_lcd
                      : gr_pixel_mode_lcd2;
      target->grays = source->num_grays;
      break;

    case FT_PIXEL_MODE_LCD_V:
      target->mode  = handle->lcd_mode == LCD_MODE_VRGB
                      ? gr_pixel_mode_lcdv
                      : gr_pixel_mode_lcdv2;
      target->grays = source->num_grays;
      break;

    case FT_PIXEL_MODE_BGRA:
      target->mode  = gr_pixel_mode_bgra;
      target->grays = source->num_grays;
      break;

    default:
      return FT_Err_Invalid_Glyph_Format;
    }

    *left = bitmap->left;
    *top  = bitmap->top;

    *x_advance = ( glyf->advance.x + 0x8000 ) >> 16;
    *y_advance = ( glyf->advance.y + 0x8000 ) >> 16;

    return error;
  }


  FT_Error
  FTDemo_Index_To_Bitmap( FTDemo_Handle*  handle,
                          FT_ULong        Index,
                          grBitmap*       target,
                          int*            left,
                          int*            top,
                          int*            x_advance,
                          int*            y_advance,
                          FT_Glyph*       aglyf )
  {
    unsigned int  width, height;


    *aglyf     = NULL;
    *x_advance = 0;

    /* use the SBits cache to store small glyph bitmaps; this is a lot */
    /* more memory-efficient                                           */
    /*                                                                 */

    width  = handle->scaler.width;
    height = handle->scaler.height;
    if ( handle->use_sbits_cache && !handle->scaler.pixel )
    {
      width  = ( ( width * handle->scaler.x_res + 36 ) / 72 )  >> 6;
      height = ( ( height * handle->scaler.y_res + 36 ) / 72 ) >> 6;
    }

    if ( handle->use_sbits_cache && width < 48 && height < 48 )
    {
      FTC_SBit   sbit;
      FT_Bitmap  source;


      error = FTC_SBitCache_LookupScaler( handle->sbits_cache,
                                          &handle->scaler,
                                          (FT_ULong)handle->load_flags,
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
        target->grays  = sbit->max_grays + 1;

        switch ( sbit->format )
        {
        case FT_PIXEL_MODE_MONO:
          target->mode = gr_pixel_mode_mono;
          break;

        case FT_PIXEL_MODE_GRAY:
          target->mode  = gr_pixel_mode_gray;
          break;

        case FT_PIXEL_MODE_GRAY2:
        case FT_PIXEL_MODE_GRAY4:
          source.rows       = sbit->height;
          source.width      = sbit->width;
          source.pitch      = sbit->pitch;
          source.buffer     = sbit->buffer;
          source.pixel_mode = sbit->format;

          (void)FT_Bitmap_Convert( handle->library, &source,
                                   &handle->bitmap, 1 );

          target->pitch  = handle->bitmap.pitch;
          target->buffer = handle->bitmap.buffer;
          target->mode   = gr_pixel_mode_gray;
          target->grays  = handle->bitmap.num_grays;
          break;

        case FT_PIXEL_MODE_LCD:
          target->mode  = handle->lcd_mode == LCD_MODE_RGB
                          ? gr_pixel_mode_lcd
                          : gr_pixel_mode_lcd2;
          break;

        case FT_PIXEL_MODE_LCD_V:
          target->mode  = handle->lcd_mode == LCD_MODE_VRGB
                          ? gr_pixel_mode_lcdv
                          : gr_pixel_mode_lcdv2;
          break;

        case FT_PIXEL_MODE_BGRA:
          target->mode  = gr_pixel_mode_bgra;
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
      FT_Glyph  glyf;


      error = FTC_ImageCache_LookupScaler( handle->image_cache,
                                           &handle->scaler,
                                           (FT_ULong)handle->load_flags,
                                           Index,
                                           &glyf,
                                           NULL );

      if ( !error )
        error = FTDemo_Glyph_To_Bitmap( handle, glyf, target, left, top,
                                        x_advance, y_advance, aglyf );
    }

  Exit:
    /* don't accept a `missing' character with zero or negative width */
    if ( Index == 0 && *x_advance <= 0 )
      *x_advance = 1;

    return error;
  }


  FT_Error
  FTDemo_Draw_Index( FTDemo_Handle*   handle,
                     FTDemo_Display*  display,
                     unsigned int     gindex,
                     int*             pen_x,
                     int*             pen_y )
  {
    int       left, top, x_advance, y_advance;
    grBitmap  bit3;
    FT_Glyph  glyf;


    error = FTDemo_Index_To_Bitmap( handle,
                                    gindex,
                                    &bit3,
                                    &left, &top,
                                    &x_advance, &y_advance,
                                    &glyf );
    if ( error )
      return error;

    /* now render the bitmap into the display surface */
    grBlitGlyphToBitmap( display->bitmap, &bit3, *pen_x + left,
                         *pen_y - top, display->fore_color );

    if ( glyf )
      FT_Done_Glyph( glyf );

    *pen_x += x_advance + 1;

    return FT_Err_Ok;
  }


  FT_Error
  FTDemo_Draw_Glyph_Color( FTDemo_Handle*   handle,
                           FTDemo_Display*  display,
                           FT_Glyph         glyph,
                           int*             pen_x,
                           int*             pen_y,
                           grColor          color )
  {
    int       left, top, x_advance, y_advance;
    grBitmap  bit3;
    FT_Glyph  glyf;


    error = FTDemo_Glyph_To_Bitmap( handle, glyph, &bit3, &left, &top,
                                    &x_advance, &y_advance, &glyf );
    if ( error )
    {
      FT_Done_Glyph( glyph );

      return error;
    }

    /* now render the bitmap into the display surface */
    grBlitGlyphToBitmap( display->bitmap, &bit3, *pen_x + left,
                         *pen_y - top, color );

    if ( glyf )
      FT_Done_Glyph( glyf );

    *pen_x += x_advance + 1;

    return FT_Err_Ok;
  }


  FT_Error
  FTDemo_Draw_Glyph( FTDemo_Handle*   handle,
                     FTDemo_Display*  display,
                     FT_Glyph         glyph,
                     int*             pen_x,
                     int*             pen_y )
  {
    return FTDemo_Draw_Glyph_Color( handle, display,
                                    glyph, pen_x, pen_y,
                                    display->fore_color );
  }


  FT_Error
  FTDemo_Draw_Slot( FTDemo_Handle*   handle,
                    FTDemo_Display*  display,
                    FT_GlyphSlot     slot,
                    int*             pen_x,
                    int*             pen_y )
  {
    FT_Glyph  glyph;


    error = FT_Get_Glyph( slot, &glyph );
    if ( error )
      return error;

    error = FTDemo_Draw_Glyph( handle, display, glyph, pen_x, pen_y );
    if ( !error )
      FT_Done_Glyph( glyph );

    return error;
  }


  void
  FTDemo_String_Set( FTDemo_Handle*  handle,
                     const char*     string )
  {
    const char*    p = string;
    const char*    end = p + strlen( string );
    unsigned long  codepoint;
    int            ch;
    int            expect;
    PGlyph         glyph = handle->string;


    handle->string_length = 0;
    codepoint = expect = 0;

    for (;;)
    {
      ch = utf8_next( &p, end );
      if ( ch < 0 )
        break;

      codepoint = (unsigned long)ch;

      if ( handle->encoding != FT_ENCODING_ORDER )
        glyph->glyph_index = FTDemo_Get_Index( handle, codepoint );
      else
        glyph->glyph_index = codepoint;

      glyph++;
      handle->string_length++;

      if ( handle->string_length >= MAX_GLYPHS )
        break;
    }

    handle->string_reload = 1;
  }


  static FT_Error
  string_load( FTDemo_Handle*  handle )
  {
    int      n;
    FT_Size  size;
    FT_Face  face;
    FT_Pos   prev_rsb_delta = 0;


    error = FTDemo_Get_Size( handle, &size );
    if ( error )
      return error;

    face = size->face;

    for ( n = 0; n < handle->string_length; n++ )
    {
      PGlyph  glyph = handle->string + n;


      /* clear existing image if there is one */
      if ( glyph->image )
      {
        FT_Done_Glyph( glyph->image );
        glyph->image = NULL;
      }

      /* load the glyph and get the image */
      if ( !FT_Load_Glyph( face, glyph->glyph_index,
                           handle->load_flags )        &&
           !FT_Get_Glyph( face->glyph, &glyph->image ) )
      {
        FT_Glyph_Metrics*  metrics = &face->glyph->metrics;


        /* note that in vertical layout, y-positive goes downwards */

        glyph->vvector.x  =  metrics->vertBearingX - metrics->horiBearingX;
        glyph->vvector.y  = -metrics->vertBearingY - metrics->horiBearingY;

        glyph->vadvance.x = 0;
        glyph->vadvance.y = -metrics->vertAdvance;

        if ( handle->lcd_mode == LCD_MODE_LIGHT_SUBPIXEL )
          glyph->delta = face->glyph->lsb_delta - face->glyph->rsb_delta;
        else
        {
          if ( prev_rsb_delta - face->glyph->lsb_delta > 32 )
            glyph->delta = -1 * 64;
          else if ( prev_rsb_delta - face->glyph->lsb_delta < -31 )
            glyph->delta = 1 * 64;
          else
            glyph->delta = 0;

          prev_rsb_delta = face->glyph->rsb_delta;
        }
      }
    }

    return FT_Err_Ok;
  }


  static FT_Error
  string_render_prepare( FTDemo_Handle*          handle,
                         FTDemo_String_Context*  sc,
                         FT_Vector*              advances )
  {
    FT_Face     face;
    FT_Size     size;
    PGlyph      glyph;
    FT_Pos      track_kern   = 0;
    FT_UInt     prev_index   = 0;
    FT_Vector*  prev_advance = NULL;
    FT_Vector   extent       = { 0, 0 };
    FT_Int      i;


    error = FTDemo_Get_Size( handle, &size );
    if ( error )
      return error;

    face = size->face;

    if ( !sc->vertical && sc->kerning_degree )
    {
      /* this function needs and returns points, not pixels */
      if ( FT_Get_Track_Kerning( face,
                                 (FT_Fixed)handle->scaler.width << 10,
                                 -sc->kerning_degree,
                                 &track_kern ) )
        track_kern = 0;
      else
        track_kern = (FT_Pos)(
                       ( track_kern / 1024.0 * handle->scaler.x_res ) /
                       72.0 );
    }

    for ( i = 0; i < handle->string_length; i++ )
    {
      glyph = handle->string + i;

      if ( !glyph->image )
        continue;

      if ( sc->vertical )
        advances[i] = glyph->vadvance;
      else
      {
        advances[i]     = glyph->image->advance;
        advances[i].x >>= 10;
        advances[i].y >>= 10;

        if ( handle->lcd_mode == LCD_MODE_LIGHT_SUBPIXEL )
          advances[i].x += glyph->delta;

        if ( prev_advance )
        {
          prev_advance->x += track_kern;

          if ( sc->kerning_mode )
          {
            FT_Vector  kern;


            FT_Get_Kerning( face, prev_index, glyph->glyph_index,
                FT_KERNING_UNFITTED, &kern );

            prev_advance->x += kern.x;
            prev_advance->y += kern.y;

            if ( handle->lcd_mode != LCD_MODE_LIGHT_SUBPIXEL &&
                 sc->kerning_mode > KERNING_MODE_NORMAL      )
              prev_advance->x += glyph->delta;
          }
        }
      }

      if ( prev_advance )
      {
        if ( handle->lcd_mode != LCD_MODE_LIGHT_SUBPIXEL &&
             handle->hinted                              )
        {
          prev_advance->x = ROUND( prev_advance->x );
          prev_advance->y = ROUND( prev_advance->y );
        }

        extent.x += prev_advance->x;
        extent.y += prev_advance->y;
      }

      prev_index   = glyph->glyph_index;
      prev_advance = advances + i;
    }

    if ( prev_advance )
    {
      if ( handle->lcd_mode != LCD_MODE_LIGHT_SUBPIXEL &&
           handle->hinted                              )
      {
        prev_advance->x = ROUND( prev_advance->x );
        prev_advance->y = ROUND( prev_advance->y );
      }

      extent.x += prev_advance->x;
      extent.y += prev_advance->y;
    }

    /* store the extent in the last slot */
    i = handle->string_length - 1;
    advances[i] = extent;

    return FT_Err_Ok;
  }


  FT_Error
  FTDemo_String_Draw( FTDemo_Handle*          handle,
                      FTDemo_Display*         display,
                      FTDemo_String_Context*  sc,
                      int                     x,
                      int                     y )
  {
    int        n;
    FT_Vector  pen, advances[MAX_GLYPHS];
    FT_Size    size;
    FT_Face    face;


    if ( !sc                        ||
         x < 0                      ||
         y < 0                      ||
         x > display->bitmap->width ||
         y > display->bitmap->rows  )
      return FT_Err_Invalid_Argument;

    error = FTDemo_Get_Size( handle, &size );
    if ( error )
      return error;

    face = size->face;

    if ( handle->string_reload )
    {
      error = string_load( handle );
      if ( error )
        return error;

      handle->string_reload = 0;
    }

    error = string_render_prepare( handle, sc, advances );
    if ( error )
      return error;

    /* change to Cartesian coordinates */
    y = display->bitmap->rows - y;

    /* get the extent, which we store in the last slot */
    pen = advances[handle->string_length - 1];

    pen.x = FT_MulFix( pen.x, sc->center );
    pen.y = FT_MulFix( pen.y, sc->center );

    /* XXX sbits */
    /* get pen position */
    if ( sc->matrix && FT_IS_SCALABLE( face ) )
    {
      FT_Vector_Transform( &pen, sc->matrix );

      pen.x = ( x << 6 ) - pen.x;
      pen.y = ( y << 6 ) - pen.y;
    }
    else
    {
      pen.x = ROUND( ( x << 6 ) - pen.x );
      pen.y = ROUND( ( y << 6 ) - pen.y );
    }

    for ( n = 0; n < handle->string_length; n++ )
    {
      PGlyph    glyph = handle->string + n;
      FT_Glyph  image;
      FT_BBox   bbox;


      if ( !glyph->image )
        continue;

      /* copy image */
      error = FT_Glyph_Copy( glyph->image, &image );
      if ( error )
        continue;

      if ( image->format != FT_GLYPH_FORMAT_BITMAP )
      {
        if ( sc->vertical )
          error = FT_Glyph_Transform( image, NULL, &glyph->vvector );

        if ( !error )
          error = FT_Glyph_Transform( image, sc->matrix, &pen );

        if ( error )
        {
          FT_Done_Glyph( image );
          continue;
        }
      }
      else
      {
        FT_BitmapGlyph  bitmap = (FT_BitmapGlyph)image;


        if ( sc->vertical )
        {
          bitmap->left += ( glyph->vvector.x + pen.x ) >> 6;
          bitmap->top  += ( glyph->vvector.y + pen.y ) >> 6;
        }
        else
        {
          bitmap->left += pen.x >> 6;
          bitmap->top  += pen.y >> 6;
        }
      }

      if ( sc->matrix )
        FT_Vector_Transform( advances + n, sc->matrix );

      pen.x += advances[n].x;
      pen.y += advances[n].y;

      FT_Glyph_Get_CBox( image, FT_GLYPH_BBOX_PIXELS, &bbox );

#if 0
      if ( n == 0 )
      {
        fprintf( stderr, "bbox = [%ld %ld %ld %ld]\n",
            bbox.xMin, bbox.yMin, bbox.xMax, bbox.yMax );
      }
#endif

      /* check bounding box; if it is completely outside the */
      /* display surface, we don't need to render it         */
      if ( bbox.xMax > 0                      &&
           bbox.yMax > 0                      &&
           bbox.xMin < display->bitmap->width &&
           bbox.yMin < display->bitmap->rows  )
      {
        int       left, top, dummy1, dummy2;
        grBitmap  bit3;
        FT_Glyph  glyf;


        error = FTDemo_Glyph_To_Bitmap( handle, image, &bit3, &left, &top,
                                        &dummy1, &dummy2, &glyf );
        if ( !error )
        {
          /* change back to the usual coordinates */
          top = display->bitmap->rows - top;

          /* now render the bitmap into the display surface */
          grBlitGlyphToBitmap( display->bitmap, &bit3, left, top,
                               display->fore_color );

          if ( glyf )
            FT_Done_Glyph( glyf );
        }
      }

      FT_Done_Glyph( image );
    }

    return error;
  }


  unsigned long
  FTDemo_Make_Encoding_Tag( const char*  s )
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


  int
  FTDemo_Event_Cff_Hinting_Engine_Change( FT_Library     library,
                                          unsigned int*  current,
                                          unsigned int   delta )
  {
    unsigned int  new_cff_hinting_engine = 0;


    if ( delta )
      new_cff_hinting_engine = ( *current          +
                                 delta             +
                                 N_HINTING_ENGINES ) % N_HINTING_ENGINES;

    error = FT_Property_Set( library,
                             "cff",
                             "hinting-engine",
                             &new_cff_hinting_engine );

    if ( !error )
    {
      *current = new_cff_hinting_engine;
      return 1;
    }

    return 0;
  }


  int
  FTDemo_Event_Type1_Hinting_Engine_Change( FT_Library     library,
                                            unsigned int*  current,
                                            unsigned int   delta )
  {
    unsigned int  new_type1_hinting_engine = 0;


    if ( delta )
      new_type1_hinting_engine = ( *current          +
                                   delta             +
                                   N_HINTING_ENGINES ) % N_HINTING_ENGINES;

    error = FT_Property_Set( library,
                             "type1",
                             "hinting-engine",
                             &new_type1_hinting_engine );

    if ( !error )
    {
      *current = new_type1_hinting_engine;
      return 1;
    }

    return 0;
  }


  int
  FTDemo_Event_T1cid_Hinting_Engine_Change( FT_Library     library,
                                            unsigned int*  current,
                                            unsigned int   delta )
  {
    unsigned int  new_t1cid_hinting_engine = 0;


    if ( delta )
      new_t1cid_hinting_engine = ( *current          +
                                   delta             +
                                   N_HINTING_ENGINES ) % N_HINTING_ENGINES;

    error = FT_Property_Set( library,
                             "t1cid",
                             "hinting-engine",
                             &new_t1cid_hinting_engine );

    if ( !error )
    {
      *current = new_t1cid_hinting_engine;
      return 1;
    }

    return 0;
  }


/* End */
