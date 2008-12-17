/****************************************************************************/
/*                                                                          */
/*  The FreeType project -- a free and portable quality TrueType renderer.  */
/*                                                                          */
/*  Copyright 1996-2000, 2003, 2004, 2005, 2006 by                          */
/*  D. Turner, R.Wilhelm, and W. Lemberg                                    */
/*                                                                          */
/****************************************************************************/


#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_SFNT_NAMES_H
#include FT_TRUETYPE_IDS_H

  /* the following header shouldn't be used in normal programs */
#include FT_INTERNAL_DEBUG_H

  /* showing driver name */
#include FT_MODULE_H
#include FT_INTERNAL_OBJECTS_H
#include FT_INTERNAL_DRIVER_H

#include "common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


  FT_Error  error;

  int  comma_flag  = 0;
  int  verbose     = 0;
  int  debug       = 0;
  int  trace_level = 0;
  int  name_tables = 0;


  /* PanicZ */
  static
  void  PanicZ( const char*  message )
  {
    fprintf( stderr, "%s\n  error = 0x%04x\n", message, error );
    exit( 1 );
  }


  void
  Print_Comma( const char*  message )
  {
    if ( comma_flag )
      printf( ", " );

    printf( "%s", message );
    comma_flag = 1;
  }


  static void
  usage( char*  execname )
  {
    fprintf( stderr, "\n" );
    fprintf( stderr, "ftdump: simple font dumper -- part of the FreeType project\n" );
    fprintf( stderr, "-----------------------------------------------------------\n" );
    fprintf( stderr, "\n" );
    fprintf( stderr, "Usage: %s [options] fontname\n", execname );
    fprintf( stderr, "\n" );
#if FREETYPE_MAJOR == 2 && FREETYPE_MINOR == 0 && FREETYPE_PATCH <= 8
    fprintf( stderr, "  -d        enable debug information\n" );
#  ifdef FT_DEBUG_LEVEL_TRACE
    fprintf( stderr, "  -l level  trace level for debug information\n" );
#  endif
#endif
    fprintf( stderr, "  -n        print SFNT name tables\n" );
    fprintf( stderr, "  -v        be verbose\n" );
    fprintf( stderr, "\n" );

    exit( 1 );
  }


  void
  Print_Name( FT_Face  face )
  {
    const char*  ps_name;


    printf( "font name entries\n" );

    /* XXX: Foundry?  Copyright?  Version? ... */

    printf( "   family:     %s\n", face->family_name );
    printf( "   style:      %s\n", face->style_name );

    ps_name = FT_Get_Postscript_Name( face );
    if ( ps_name == NULL )
      ps_name = "UNAVAILABLE";

    printf( "   postscript: %s\n", ps_name );
  }


  void
  Print_Type( FT_Face  face )
  {
    FT_Module  module;


    printf( "font type entries\n" );

    module = &face->driver->root;
    printf( "   FreeType driver: %s\n", module->clazz->module_name );

    /* Is it better to dump all sfnt tag names? */
    printf( "   sfnt wrapped:    %s\n",
            FT_IS_SFNT( face ) ? (char *)"yes" : (char *)"no" );

    /* isScalable? */
    comma_flag = 0;
    printf( "   type:            " );
    if ( FT_IS_SCALABLE( face ) )
    {
      Print_Comma( "scalable" );
      if ( FT_HAS_MULTIPLE_MASTERS( face ) )
        Print_Comma( "multiple masters" );
    }
    if ( FT_HAS_FIXED_SIZES( face ) )
      Print_Comma( "fixed size" );
    printf( "\n" );

    /* Direction */
    comma_flag = 0;
    printf( "   direction:       " );
    if ( FT_HAS_HORIZONTAL( face ) )
      Print_Comma( "horizontal" );

    if ( FT_HAS_VERTICAL( face ) )
      Print_Comma( "vertical" );

    printf( "\n" );

    printf( "   fixed width:     %s\n",
            FT_IS_FIXED_WIDTH( face ) ? (char *)"yes" : (char *)"no" );

    printf( "   glyph names:     %s\n",
            FT_HAS_GLYPH_NAMES( face ) ? (char *)"yes" : (char *)"no" );

    if ( FT_IS_SCALABLE( face ) )
    {
      printf( "   EM size:         %d\n", face->units_per_EM );
      printf( "   global BBox:     (%ld,%ld):(%ld,%ld)\n",
              face->bbox.xMin, face->bbox.yMin,
              face->bbox.xMax, face->bbox.yMax );
      printf( "   ascent:          %d\n", face->ascender );
      printf( "   descent:         %d\n", face->descender );
      printf( "   text height:     %d\n", face->height );
    }
  }

  static const char*
  platform_id( int  id )
  {
    switch ( id )
    {
    case TT_PLATFORM_APPLE_UNICODE:
      return "Apple (Unicode)";
    case TT_PLATFORM_MACINTOSH:
      return "Macintosh";
    case TT_PLATFORM_ISO:
      return "ISO (deprecated)";
    case TT_PLATFORM_MICROSOFT:
      return "Microsoft";
    case TT_PLATFORM_CUSTOM:
      return "custom";
    case TT_PLATFORM_ADOBE:
      return "Adobe";

    default:
      return "UNKNOWN";
    }
  }


  static const char*
  name_id( int  id )
  {
    switch ( id )
    {
    case TT_NAME_ID_COPYRIGHT:
      return "copyright";
    case TT_NAME_ID_FONT_FAMILY:
      return "font family";
    case TT_NAME_ID_FONT_SUBFAMILY:
      return "font subfamily";
    case TT_NAME_ID_UNIQUE_ID:
      return "unique ID";
    case TT_NAME_ID_FULL_NAME:
      return "full name";
    case TT_NAME_ID_VERSION_STRING:
      return "version string";
    case TT_NAME_ID_PS_NAME:
      return "PostScript name";
    case TT_NAME_ID_TRADEMARK:
      return "trademark";

   /* the following values are from the OpenType spec */
    case TT_NAME_ID_MANUFACTURER:
      return "manufacturer";
    case TT_NAME_ID_DESIGNER:
      return "designer";
    case TT_NAME_ID_DESCRIPTION:
      return "description";
    case TT_NAME_ID_VENDOR_URL:
      return "vendor URL";
    case TT_NAME_ID_DESIGNER_URL:
      return "designer URL";
    case TT_NAME_ID_LICENSE:
      return "license";
    case TT_NAME_ID_LICENSE_URL:
      return "license URL";
    /* number 15 is reserved */
    case TT_NAME_ID_PREFERRED_FAMILY:
      return "preferred family";
    case TT_NAME_ID_PREFERRED_SUBFAMILY:
      return "preferred subfamily";
    case TT_NAME_ID_MAC_FULL_NAME:
      return "Mac full name";

   /* The following code is new as of 2000-01-21 */
    case TT_NAME_ID_SAMPLE_TEXT:
      return "sample text";

   /* This is new in OpenType 1.3 */
    case TT_NAME_ID_CID_FINDFONT_NAME:
      return "CID `findfont' name";

    default:
      return "UNKNOWN";
    }
  }


  static void
  put_ascii( FT_Byte*  string,
             FT_UInt   string_len,
             FT_UInt   indent )
  {
    FT_UInt  i, j;


    for ( j = 0; j < indent; j++ )
      putchar( ' ' );
    putchar( '"' );

    for ( i = 0; i < string_len; i++ )
    {
      switch ( string[i] )
      {
      case '\n':
        fputs( "\\n\"", stdout );
        if ( i + 1 < string_len )
        {
          putchar( '\n' );
          for ( j = 0; j < indent; j++ )
            putchar( ' ' );
          putchar( '"' );
        }
        break;
      case '\r':
        fputs( "\\r", stdout );
        break;
      case '\t':
        fputs( "\\t", stdout );
        break;
      case '\\':
        fputs( "\\\\", stdout );
        break;
      case '"':
        fputs( "\\\"", stdout );
        break;

      default:
        putchar( string[i] );
        break;
      }
    }
    if ( string[i - 1] != '\n' )
      putchar( '"' );
  }


  static void
  put_unicode_be16( FT_Byte*  string,
                    FT_UInt   string_len,
                    FT_UInt   indent )
  {
    FT_Int   ch = 0;
    FT_UInt  i, j;


    for ( j = 0; j < indent; j++ )
      putchar( ' ' );
    putchar( '"' );

    for ( i = 0; i < string_len; i += 2 )
    {
      ch = ( string[i] << 8 ) | string[i + 1];

      switch ( ch )
      {
      case '\n':
        fputs( "\\n\"", stdout );
        if ( i + 2 < string_len )
        {
          putchar( '\n' );
          for ( j = 0; j < indent; j++ )
            putchar( ' ' );
          putchar( '"' );
        }
        break;
      case '\r':
        fputs( "\\r", stdout );
        break;
      case '\t':
        fputs( "\\t", stdout );
        break;
      case '\\':
        fputs( "\\\\", stdout );
        break;
      case '"':
        fputs( "\\\"", stdout );
        break;

      case 0x00A9:
        fputs( "(c)", stdout );
        break;
      case 0x00AE:
        fputs( "(r)", stdout );
        break;

      case 0x2013:
        fputs( "--", stdout );
        break;
      case 0x2019:
        fputs( "\'", stdout );
        break;

      case 0x2122:
        fputs( "(tm)", stdout );
        break;

      default:
        if ( ch < 256 )
          putchar( ch );
        else
          printf( "\\U+%04X", ch );
        break;
      }
    }
    if ( ch != '\n' )
      putchar( '"' );
  }


  void
  Print_Sfnt_Names( FT_Face  face )
  {
    FT_SfntName  name;
    FT_UInt      num_names, i;


    printf( "font string entries\n" );

    num_names = FT_Get_Sfnt_Name_Count( face );
    for ( i = 0; i < num_names; i++ )
    {
      error = FT_Get_Sfnt_Name( face, i, &name );
      if ( error == FT_Err_Ok )
      {
        printf( "   %-15s [%s]", name_id( name.name_id ),
                                 platform_id( name.platform_id ) );

        switch ( name.platform_id )
        {
        case TT_PLATFORM_APPLE_UNICODE:
          switch ( name.encoding_id )
          {
          case TT_APPLE_ID_DEFAULT:
          case TT_APPLE_ID_UNICODE_1_1:
          case TT_APPLE_ID_ISO_10646:
          case TT_APPLE_ID_UNICODE_2_0:
            put_unicode_be16( name.string, name.string_len, 6 );
            break;

          default:
            printf( "{unsupported encoding %d}", name.encoding_id );
            break;
          }
          break;

        case TT_PLATFORM_MACINTOSH:
          if ( name.language_id != TT_MAC_LANGID_ENGLISH )
            printf( " (language=%d)", name.language_id );
          fputs( ":\n", stdout );

          switch ( name.encoding_id )
          {
          case TT_MAC_ID_ROMAN:
            /* FIXME: convert from MacRoman to ASCII/ISO8895-1/whatever */
            /* (MacRoman is mostly like ISO8895-1 but there are         */
            /* differences)                                             */
            put_ascii( name.string, name.string_len, 6 );
            break;

          default:
            printf( "{unsupported encoding %d}", name.encoding_id );
            break;
          }

          break;

        case TT_PLATFORM_ISO:
          switch ( name.encoding_id )
          {
          case TT_ISO_ID_7BIT_ASCII:
          case TT_ISO_ID_8859_1:
            put_ascii( name.string, name.string_len, 6 );
            break;

          case TT_ISO_ID_10646:
            put_unicode_be16( name.string, name.string_len, 6 );
            break;

          default:
            printf( "{unsupported encoding %d}", name.encoding_id );
            break;
          }
          break;

        case TT_PLATFORM_MICROSOFT:
          if ( name.language_id != TT_MS_LANGID_ENGLISH_UNITED_STATES )
            printf( " (language=0x%04x)", name.language_id );
          fputs( ":\n", stdout );

          switch ( name.encoding_id )
          {
          /* TT_MS_ID_SYMBOL_CS is supposed to be Unicode, according to */
          /* information from the MS font development team              */
          case TT_MS_ID_SYMBOL_CS:
          case TT_MS_ID_UNICODE_CS:
            put_unicode_be16( name.string, name.string_len, 6 );
            break;

          default:
            printf( "{unsupported encoding %d}", name.encoding_id );
            break;
          }

          break;

        default:
          printf( "{unsupported platform}" );
          break;
        }

        printf( "\n" );
      }
    }
  }


  void
  Print_Fixed( FT_Face  face )
  {
    int  i;


    /* num_fixed_size */
    printf( "fixed size\n" );

    /* available size */
    for ( i = 0; i < face->num_fixed_sizes; i++ )
    {
      FT_Bitmap_Size*  bsize = face->available_sizes + i;


      printf( "   %3d: height %d, width %d\n",
              i, bsize->height, bsize->width );
      printf( "        size %.3f, x_ppem %.3f, y_ppem %.3f\n",
              bsize->size / 64.0,
              bsize->x_ppem / 64.0, bsize->y_ppem / 64.0 );
    }
  }


  void
  Print_Charmaps( FT_Face  face )
  {
    int  i, active = -1;


    if ( face->charmap )
      active = FT_Get_Charmap_Index( face->charmap );

    /* CharMaps */
    printf( "charmaps\n" );

    for( i = 0; i < face->num_charmaps; i++ )
    {
      printf( "   %d: platform %d, encoding %d",
              i,
              face->charmaps[i]->platform_id,
              face->charmaps[i]->encoding_id );
      if ( i == active )
        printf( " (active)" );
      printf ( "\n" );

      if ( verbose )
      {
        FT_ULong  charcode;
        FT_UInt   gindex;


        FT_Set_Charmap( face, face->charmaps[i] );

        charcode = FT_Get_First_Char( face, &gindex );
        while ( gindex )
        {
          printf( "      0x%04lx => %d\n", charcode, gindex );
          charcode = FT_Get_Next_Char( face, charcode, &gindex );
        }
        printf( "\n" );
      }
    }
  }


  int
  main( int    argc,
        char*  argv[] )
  {
    int    i, file;
    char   filename[128 + 4];
    char   alt_filename[128 + 4];
    char*  execname;
    int    num_faces;
    int    option;

    FT_Library  library;      /* the FreeType library */
    FT_Face     face;         /* the font face        */


    execname = ft_basename( argv[0] );

    while ( 1 )
    {
      option = getopt( argc, argv, "dl:nv" );

      if ( option == -1 )
        break;

      switch ( option )
      {
      case 'd':
        debug = 1;
        break;

      case 'l':
        trace_level = atoi( optarg );
        if ( trace_level < 1 || trace_level > 7 )
          usage( execname );
        break;

      case 'n':
        name_tables = 1;
        break;

      case 'v':
        verbose = 1;
        break;

      default:
        usage( execname );
        break;
      }
    }

    argc -= optind;
    argv += optind;

    if ( argc != 1 )
      usage( execname );

#if FREETYPE_MAJOR == 2 && FREETYPE_MINOR == 0 && FREETYPE_PATCH <= 8
    if ( debug )
    {
#  ifdef FT_DEBUG_LEVEL_TRACE
      FT_SetTraceLevel( trace_any, (FT_Byte)trace_level );
#  else
      trace_level = 0;
#  endif
    }
#elif 0
       /* "setenv/putenv" is not ANSI and I don't want to mess */
       /* with this portability issue right now                */
    if ( debug )
    {
      char  temp[32];


      sprintf( temp, "any=%d", trace_level );
      setenv( "FT2_DEBUG", temp );
    }
#endif

    file = 0;

    /* Initialize engine */
    error = FT_Init_FreeType( &library );
    if ( error )
      PanicZ( "Could not initialize FreeType library" );

    filename[128]     = '\0';
    alt_filename[128] = '\0';

    strncpy( filename, argv[file], 128 );
    strncpy( alt_filename, argv[file], 128 );

    /* try to load the file name as is, first */
    error = FT_New_Face( library, argv[file], 0, &face );
    if ( !error )
      goto Success;

#ifndef macintosh
    i = strlen( argv[file] );
    while ( i > 0 && argv[file][i] != '\\' && argv[file][i] != '/' )
    {
      if ( argv[file][i] == '.' )
        i = 0;
      i--;
    }

    if ( i >= 0 )
    {
      strncpy( filename + strlen( filename ), ".ttf", 4 );
      strncpy( alt_filename + strlen( alt_filename ), ".ttc", 4 );
    }
#endif

    /* Load face */
    error = FT_New_Face( library, filename, 0, &face );
    if ( error )
      PanicZ( "Could not open face." );

  Success:
    num_faces = face->num_faces;
    FT_Done_Face( face );

    printf( "There %s %d %s in this file.\n",
            num_faces == 1 ? (char *)"is" : (char *)"are",
            num_faces,
            num_faces == 1 ? (char *)"face" : (char *)"faces" );

    for ( i = 0; i < num_faces; i++ )
    {
      error = FT_New_Face( library, filename, i, &face );
      if ( error )
        PanicZ( "Could not open face." );

      printf( "\n----- Face number: %d -----\n\n", i );
      Print_Name( face );
      printf( "\n" );
      Print_Type( face );

      printf( "   glyph count: %ld\n", face->num_glyphs );

      if ( name_tables && FT_IS_SFNT( face ) )
      {
        printf( "\n" );
        Print_Sfnt_Names( face );
      }

      if ( face->num_fixed_sizes )
      {
        printf( "\n" );
        Print_Fixed( face );
      }

      if ( face->num_charmaps )
      {
        printf( "\n" );
        Print_Charmaps( face );
      }

      FT_Done_Face( face );
    }

    FT_Done_FreeType( library );

    exit( 0 );      /* for safety reasons */
    return 0;       /* never reached */
  }


/* End */
