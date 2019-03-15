/****************************************************************************/
/*                                                                          */
/*  The FreeType project -- a free and portable quality TrueType renderer.  */
/*                                                                          */
/*  Copyright 1996-2017                                                     */
/*  D. Turner, R.Wilhelm, and W. Lemberg                                    */
/*                                                                          */
/****************************************************************************/


#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_SFNT_NAMES_H
#include FT_TRUETYPE_IDS_H
#include FT_TRUETYPE_TABLES_H
#include FT_TRUETYPE_TAGS_H
#include FT_MULTIPLE_MASTERS_H

  /* the following header shouldn't be used in normal programs */
#include FT_INTERNAL_DEBUG_H

  /* showing driver name */
#include FT_MODULE_H
#include FT_INTERNAL_OBJECTS_H
#include FT_INTERNAL_DRIVER_H

  /* error messages */
#undef FTERRORS_H_
#define FT_ERROR_START_LIST     {
#define FT_ERRORDEF( e, v, s )  case v: str = s; break;
#define FT_ERROR_END_LIST       default: str = "unknown error"; }

#include "common.h"
#include "output.h"
#include "mlgetopt.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


  static FT_Error  error;

  static int  comma_flag  = 0;
  static int  verbose     = 0;
  static int  name_tables = 0;
  static int  bytecode    = 0;
  static int  tables      = 0;
  static int  utf8        = 0;


  /* PanicZ */
  static void
  PanicZ( FT_Library   library,
          const char*  message )
  {
    const FT_String  *str;


    FT_Done_FreeType( library );

    switch( error )
    #include FT_ERRORS_H

    fprintf( stderr, "%s\n  error = 0x%04x, %s\n", message, error, str );
    exit( 1 );
  }


  static void
  Print_Comma( const char*  message )
  {
    if ( comma_flag )
      printf( ", " );

    printf( "%s", message );
    comma_flag = 1;
  }


  static void
  usage( FT_Library  library,
         char*       execname )
  {
    FT_Done_FreeType( library );

    fprintf( stderr,
      "\n"
      "ftdump: simple font dumper -- part of the FreeType project\n"
      "-----------------------------------------------------------\n"
      "\n"
      "Usage: %s [options] fontname\n"
      "\n",
             execname );

    fprintf( stderr,
      "  -n        Print SFNT name tables.\n"
      "  -p        Print TrueType programs.\n"
      "  -t        Print SFNT table list.\n"
      "  -u        Emit UTF8.\n"
      "  -V        Be verbose.\n"
      "\n"
      "  -v        Show version.\n"
      "\n" );

    exit( 1 );
  }


  static void
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


  static void
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


#define XEXPAND( x )  #x
#define EXPAND( x )   XEXPAND( x )

#define NAME_ID( tag, description ) \
          case TT_NAME_ID_ ## tag: \
            return description " (ID " EXPAND( TT_NAME_ID_ ## tag ) ")"


  static const char*
  name_id( int  id )
  {
    switch ( id )
    {
      NAME_ID( COPYRIGHT, "copyright" );
      NAME_ID( FONT_FAMILY, "font family" );
      NAME_ID( FONT_SUBFAMILY, "font subfamily" );
      NAME_ID( UNIQUE_ID, "unique font identifier" );
      NAME_ID( FULL_NAME, "full name" );
      NAME_ID( VERSION_STRING, "version string" );
      NAME_ID( PS_NAME, "PostScript name" );
      NAME_ID( TRADEMARK, "trademark" );

      /* the following values are from the OpenType spec */
      NAME_ID( MANUFACTURER, "manufacturer" );
      NAME_ID( DESIGNER, "designer" );
      NAME_ID( DESCRIPTION, "description" );
      NAME_ID( VENDOR_URL, "vendor URL" );
      NAME_ID( DESIGNER_URL, "designer URL" );
      NAME_ID( LICENSE, "license" );
      NAME_ID( LICENSE_URL, "license URL" );
      /* number 15 is reserved */
      NAME_ID( TYPOGRAPHIC_FAMILY, "typographic family" );
      NAME_ID( TYPOGRAPHIC_SUBFAMILY, "typographic subfamily" );
      NAME_ID( MAC_FULL_NAME, "Mac full name" );

      /* the following code is new as of 2000-01-21 */
      NAME_ID( SAMPLE_TEXT, "sample text" );

      /* this is new in OpenType 1.3 */
      NAME_ID( CID_FINDFONT_NAME, "CID `findfont' name" );

      /* this is new in OpenType 1.5 */
      NAME_ID( WWS_FAMILY, "WWS family name" );
      NAME_ID( WWS_SUBFAMILY, "WWS subfamily name" );

      /* this is new in OpenType 1.7 */
      NAME_ID( LIGHT_BACKGROUND, "light background palette" );
      NAME_ID( DARK_BACKGROUND, "dark background palette" );

      /* this is new in OpenType 1.8 */
      NAME_ID( VARIATIONS_PREFIX, "variations PostScript name prefix" );

    default:
      return NULL;
    }
  }


  static void
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
        const char*  NameID     = name_id( name.name_id );
        const char*  PlatformID = platform_id( name.platform_id );


        if ( NameID )
          printf( "   %-15s [%s]", NameID, PlatformID );
        else
          printf( "   Name ID %-5d   [%s]", name.name_id, PlatformID );

        switch ( name.platform_id )
        {
        case TT_PLATFORM_APPLE_UNICODE:
          fputs( ":\n", stdout );
          switch ( name.encoding_id )
          {
          case TT_APPLE_ID_DEFAULT:
          case TT_APPLE_ID_UNICODE_1_1:
          case TT_APPLE_ID_ISO_10646:
          case TT_APPLE_ID_UNICODE_2_0:
            put_unicode_be16( name.string, name.string_len, 6, utf8 );
            break;

          default:
            printf( "{unsupported Unicode encoding %d}", name.encoding_id );
            break;
          }
          break;

        case TT_PLATFORM_MACINTOSH:
          if ( name.language_id != TT_MAC_LANGID_ENGLISH )
            printf( " (language=%u)", name.language_id );
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
            printf( "      [data in encoding %d]", name.encoding_id );
            break;
          }

          break;

        case TT_PLATFORM_ISO:
          fputs( ":\n", stdout );
          switch ( name.encoding_id )
          {
          case TT_ISO_ID_7BIT_ASCII:
          case TT_ISO_ID_8859_1:
            put_ascii( name.string, name.string_len, 6 );
            break;

          case TT_ISO_ID_10646:
            put_unicode_be16( name.string, name.string_len, 6, utf8 );
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
            /* TT_MS_ID_SYMBOL_CS is Unicode, similar to PID/EID=3/1 */
          case TT_MS_ID_SYMBOL_CS:
          case TT_MS_ID_UNICODE_CS:
            put_unicode_be16( name.string, name.string_len, 6, utf8 );
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


  static void
  Print_Sfnt_Tables( FT_Face  face )
  {
    FT_ULong  num_tables, i;
    FT_ULong  tag, length;
    FT_Byte   buffer[4];


    FT_Sfnt_Table_Info( face, 0, NULL, &num_tables );

    printf( "font tables (%lu)\n", num_tables );

    for ( i = 0; i < num_tables; i++ )
    {
      FT_Sfnt_Table_Info( face, (FT_UInt)i, &tag, &length );

      if ( length >= 4 )
      {
        length = 4;
        FT_Load_Sfnt_Table( face, tag, 0, buffer, &length );
      }
      else
        continue;

      printf( "  %2lu: %c%c%c%c %02X%02X%02X%02X...\n", i,
                                   (FT_Char)( tag >> 24 ),
                                   (FT_Char)( tag >> 16 ),
                                   (FT_Char)( tag >>  8 ),
                                   (FT_Char)( tag ),
                                       (FT_UInt)buffer[0],
                                       (FT_UInt)buffer[1],
                                       (FT_UInt)buffer[2],
                                       (FT_UInt)buffer[3] );
    }
  }


  static void
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


  static void
  Print_Charmaps( FT_Face  face )
  {
    int  i, active = -1;


    if ( face->charmap )
      active = FT_Get_Charmap_Index( face->charmap );

    /* CharMaps */
    printf( "charmaps (%d)\n", face->num_charmaps );

    for( i = 0; i < face->num_charmaps; i++ )
    {
      FT_Long   format  = FT_Get_CMap_Format( face->charmaps[i] );
      FT_ULong  lang_id = FT_Get_CMap_Language_ID( face->charmaps[i] );


      if ( format >= 0 )
        printf( "  %2d: format %2ld, platform %u, encoding %2u",
                i,
                format,
                face->charmaps[i]->platform_id,
                face->charmaps[i]->encoding_id );
      else
        printf( "  %2d: synthetic, platform %u, encoding %2u",
                i,
                face->charmaps[i]->platform_id,
                face->charmaps[i]->encoding_id );

      if ( lang_id == 0xFFFFFFFFUL )
        printf( "   (Unicode Variation Sequences)" );
      else
        printf( "   language %lu",
                lang_id );

      if ( i == active )
        printf( " (active)" );

      printf ( "\n" );

      if ( verbose )
      {
        FT_ULong   charcode;
        FT_UInt    gindex;
        FT_String  buf[32];


        FT_Set_Charmap( face, face->charmaps[i] );

        charcode = FT_Get_First_Char( face, &gindex );
        while ( gindex )
        {
          if ( FT_HAS_GLYPH_NAMES ( face ) )
            FT_Get_Glyph_Name( face, gindex, buf, 32 );
          else
            buf[0] = '\0';

          printf( "      0x%04lx => %d %s\n", charcode, gindex, buf );
          charcode = FT_Get_Next_Char( face, charcode, &gindex );
        }
        printf( "\n" );
      }
    }
  }


  static void
  Print_MM_Axes( FT_Face  face )
  {
    FT_MM_Var*       mm;
    FT_Multi_Master  dummy;
    FT_UInt          is_GX, i, num_names;


    /* MM or GX axes */
    error = FT_Get_Multi_Master( face, &dummy );
    is_GX = error ? 1 : 0;

    printf( "%s axes\n", is_GX ? "GX" : "MM" );

    error = FT_Get_MM_Var( face, &mm );
    if ( error )
    {
      printf( "   Can't access axis data (error code %d)\n", error );
      return;
    }

    num_names = FT_Get_Sfnt_Name_Count( face );

    for ( i = 0; i < mm->num_axis; i++ )
    {
      FT_SfntName  name;


      name.string = NULL;

      if ( is_GX )
      {
        FT_UInt  strid = mm->axis[i].strid;
        FT_UInt  j;


        /* iterate over all name entries        */
        /* to find an English entry for `strid' */

        for ( j = 0; j < num_names; j++ )
        {
          error = FT_Get_Sfnt_Name( face, j, &name );
          if ( error )
            continue;

          if ( name.name_id == strid )
          {
            /* XXX we don't have support for Apple's new `ltag' table yet, */
            /* thus we ignore TT_PLATFORM_APPLE_UNICODE                    */
            if ( ( name.platform_id == TT_PLATFORM_MACINTOSH &&
                   name.language_id == TT_MAC_LANGID_ENGLISH )        ||
                 ( name.platform_id == TT_PLATFORM_MICROSOFT        &&
                   ( name.language_id & 0xFF )
                                    == TT_MS_LANGID_ENGLISH_GENERAL ) )
              break;
          }
        }
      }

      if ( name.string )
      {
        if ( name.platform_id == TT_PLATFORM_MACINTOSH )
          put_ascii( name.string, name.string_len, 3 );
        else
          put_unicode_be16( name.string, name.string_len, 3, utf8 );
      }
      else
        printf( "   %s", mm->axis[i].name );

      printf( ": [%g;%g], default %g\n",
              mm->axis[i].minimum / 65536.0,
              mm->axis[i].maximum / 65536.0,
              mm->axis[i].def / 65536.0 );
    }

    FT_Done_MM_Var( face->glyph->library, mm );
  }


  static void
  Print_Bytecode( FT_Byte*   buffer,
                  FT_UShort  length,
                  char*      tag )
  {
    FT_UShort  i;
    int        j = 0;  /* status counter */


    for ( i = 0; i < length; i++ )
    {
      if ( ( i & 15 ) == 0 )
        printf( "\n%s:%04hx ", tag, i );

      if ( j == 0 )
      {
        printf( " %02x", (FT_UInt)buffer[i] );

        if ( buffer[i] == 0x40 )
          j = -1;
        else if ( buffer[i] == 0x41 )
          j = -2;
        else if ( 0xB0 <= buffer[i] && buffer[i] <= 0xB7 )
          j = buffer[i] - 0xAF;
        else if ( 0xB8 <= buffer[i] && buffer[i] <= 0xBF )
          j = 2 * ( buffer[i] - 0xB7 );
      }
      else
      {
        printf( "_%02x", (FT_UInt)buffer[i] );

        if ( j == -1 )
          j = buffer[i];
        else if ( j == -2 )
          j = 2 * buffer[i];
        else
          j--;
      }
    }
    printf( "\n" );
  }


  static void
  Print_Programs( FT_Face face )
  {
    FT_ULong    length = 0;
    FT_UShort   i;
    FT_Byte*    buffer = NULL;
    FT_Byte*    offset = NULL;

    TT_Header*      head;
    TT_MaxProfile*  maxp;


    error = FT_Load_Sfnt_Table( face, TTAG_fpgm, 0, NULL, &length );
    if ( error || length == 0 )
      goto Prep;

    buffer = (FT_Byte*)malloc( length );
    if ( buffer == NULL )
      goto Exit;

    error = FT_Load_Sfnt_Table( face, TTAG_fpgm, 0, buffer, &length );
    if ( error )
      goto Exit;

    printf( "font program" );
    Print_Bytecode( buffer, (FT_UShort)length, (char*)"fpgm" );

  Prep:
    length = 0;

    error = FT_Load_Sfnt_Table( face, TTAG_prep, 0, NULL, &length );
    if ( error || length == 0 )
      goto Glyf;

    buffer = (FT_Byte*)realloc( buffer, length );
    if ( buffer == NULL )
      goto Exit;

    error = FT_Load_Sfnt_Table( face, TTAG_prep, 0, buffer, &length );
    if ( error )
      goto Exit;

    printf( "\ncontrol value program" );
    Print_Bytecode( buffer, (FT_UShort)length, (char*)"prep" );

  Glyf:
    length = 0;

    error = FT_Load_Sfnt_Table( face, TTAG_glyf, 0, NULL, &length );
    if ( error || length == 0 )
      goto Exit;

    buffer = (FT_Byte*)realloc( buffer, length );
    if ( buffer == NULL )
      goto Exit;

    error = FT_Load_Sfnt_Table( face, TTAG_glyf, 0, buffer, &length );
    if ( error )
      goto Exit;

    length = 0;

    error = FT_Load_Sfnt_Table( face, TTAG_loca, 0, NULL, &length );
    if ( error || length == 0 )
      goto Exit;

    offset = (FT_Byte*)malloc( length );
    if ( offset == NULL )
      goto Exit;

    error = FT_Load_Sfnt_Table( face, TTAG_loca, 0, offset, &length );
    if ( error )
      goto Exit;

    head =     (TT_Header*)FT_Get_Sfnt_Table( face, FT_SFNT_HEAD );
    maxp = (TT_MaxProfile*)FT_Get_Sfnt_Table( face, FT_SFNT_MAXP );

    for ( i = 0; i < maxp->numGlyphs; i++ )
    {
      FT_UInt32  loc;
      FT_UInt16  len;
      char       tag[5];


      if ( head->Index_To_Loc_Format )
        loc = (FT_UInt32)offset[4 * i    ] << 24 |
              (FT_UInt32)offset[4 * i + 1] << 16 |
              (FT_UInt32)offset[4 * i + 2] << 8  |
              (FT_UInt32)offset[4 * i + 3];
      else
        loc = (FT_UInt32)offset[2 * i    ] << 9 |
              (FT_UInt32)offset[2 * i + 1] << 1;

      len = (FT_UInt16)( buffer[loc] << 8 | buffer[loc + 1] );

      loc += 10;

      if ( (FT_Int16)len < 0 )  /* composite */
      {
        FT_UShort  flags;


        do
        {
          flags = (FT_UInt16)( buffer[loc] << 8 | buffer[loc + 1] );

          loc += 4;

          loc += flags & FT_SUBGLYPH_FLAG_ARGS_ARE_WORDS ? 4 : 2;

          loc += flags & FT_SUBGLYPH_FLAG_SCALE ? 2
                   : flags & FT_SUBGLYPH_FLAG_XY_SCALE ? 4
                       : flags & FT_SUBGLYPH_FLAG_2X2 ? 8 : 0;
        } while ( flags & 0x20 );  /* more components */

        if ( ( flags & 0x100 ) == 0 )
          continue;
      }
      else
        loc += 2 * len;

      len = (FT_UInt16)( buffer[loc] << 8 | buffer[loc + 1] );

      if ( len == 0 )
        continue;

      loc += 2;

      sprintf( tag, "%04hx", i );
      printf("\nglyf program %hd (%.4s)", i, tag );
      Print_Bytecode( buffer + loc, len, tag );
    }

  Exit:
    free( buffer );
    free( offset );
  }


  int
  main( int    argc,
        char*  argv[] )
  {
    int    i, file;
    char   filename[1024 + 4];
    char   alt_filename[1024 + 4];
    char*  execname;
    int    num_faces;
    int    option;

    FT_Library  library;      /* the FreeType library */
    FT_Face     face;         /* the font face        */


    execname = ft_basename( argv[0] );

    /* Initialize engine */
    error = FT_Init_FreeType( &library );
    if ( error )
      PanicZ( library, "Could not initialize FreeType library" );

    while ( 1 )
    {
      option = getopt( argc, argv, "nptuvV" );

      if ( option == -1 )
        break;

      switch ( option )
      {
      case 'n':
        name_tables = 1;
        break;

      case 'p':
        bytecode = 1;
        break;

      case 't':
        tables = 1;
        break;

      case 'u':
        utf8 = 1;
        break;

      case 'v':
        {
          FT_Int  major, minor, patch;


          FT_Library_Version( library, &major, &minor, &patch );

          printf( "ftdump (FreeType) %d.%d", major, minor );
          if ( patch )
            printf( ".%d", patch );
          printf( "\n" );
          exit( 0 );
        }
        /* break; */

      case 'V':
        verbose = 1;
        break;

      default:
        usage( library, execname );
        break;
      }
    }

    argc -= optind;
    argv += optind;

    if ( argc != 1 )
      usage( library, execname );

    file = 0;

    filename[1024]     = '\0';
    alt_filename[1024] = '\0';

    strncpy( filename, argv[file], 1024 );
    strncpy( alt_filename, argv[file], 1024 );

    /* try to load the file name as is, first */
    error = FT_New_Face( library, argv[file], 0, &face );
    if ( !error )
      goto Success;

#ifndef macintosh
    i = (int)strlen( argv[file] );
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
      PanicZ( library, "Could not open face." );

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
        PanicZ( library, "Could not open face." );

      printf( "\n----- Face number: %d -----\n\n", i );
      Print_Name( face );
      printf( "\n" );
      Print_Type( face );

      printf( "   glyph count:     %ld\n", face->num_glyphs );

      if ( name_tables && FT_IS_SFNT( face ) )
      {
        printf( "\n" );
        Print_Sfnt_Names( face );
      }

      if ( tables && FT_IS_SFNT( face ) )
      {
        printf( "\n" );
        Print_Sfnt_Tables( face );
      }

      if ( bytecode && FT_IS_SFNT( face ) )
      {
        printf( "\n" );
        Print_Programs( face );
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

      if ( FT_HAS_MULTIPLE_MASTERS( face ) )
      {
        printf( "\n" );
        Print_MM_Axes( face );
      }

      FT_Done_Face( face );
    }

    FT_Done_FreeType( library );

    exit( 0 );      /* for safety reasons */
    /* return 0; */ /* never reached */
  }


/* End */
