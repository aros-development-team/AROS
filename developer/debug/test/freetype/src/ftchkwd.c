/****************************************************************************/
/*                                                                          */
/*  The FreeType project -- a free and portable quality font engine         */
/*                                                                          */
/*  Copyright (C) 2003-2019 by                                              */
/*  D. Turner, R.Wilhelm, and W. Lemberg                                    */
/*                                                                          */
/*  ftchkwd                                                                 */
/*                                                                          */
/*  NOTE:  This is just a test program that is used to show off and         */
/*         debug the current engine.                                        */
/*                                                                          */
/****************************************************************************/

#include <ft2build.h>
#include FT_FREETYPE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


  FT_Error  error;


  static void
  Usage( char*  name )
  {
    printf( "ftchkwd: test fixed font width -- part of the FreeType project\n" );
    printf( "---------------------------------------------------------------------\n" );
    printf( "\n" );
    printf( "Usage: %s fontname[.ttf|.ttc] [fontname2..]\n", name );
    printf( "\n" );

    exit( 1 );
  }


  static void
  Panic( const char*  message )
  {
    fprintf( stderr, "%s\n  error code = 0x%04x\n", message, error );
    exit(1);
  }


  static const char*
  file_basename( const char*  pathname )
  {
    const char*  base = pathname;
    const char*  p    = pathname;


    while ( *p )
    {
      if ( *p == '/' || *p == '\\' )
        base = p + 1;

      p++;
    }

    return base;
  }


  static void
  check_face( FT_Face      face,
              const char*  filepathname,
              int          idx )
  {
    int  face_has_fixed_flag = FT_IS_FIXED_WIDTH(face);
    int  face_max_advance    = face->max_advance_width;
    int  num_proportional    = 0;
    int  n;

    FT_UNUSED( idx );


    printf( "%15s : %20s : ",
            file_basename( filepathname ),
            face->family_name ? face->family_name : "UNKNOWN FAMILY" );

    for ( n = 0; n < face->num_glyphs; n++ )
    {
      /* load the glyph outline */
      error = FT_Load_Glyph( face, n, FT_LOAD_NO_SCALE );
      if ( error )
        continue;

      if ( face->glyph->metrics.horiAdvance != face_max_advance )
        num_proportional++;
    }

    if ( num_proportional > 0 )
    {
      if ( face_has_fixed_flag )
        printf( "KO!  Tagged as fixed, but has %d `proportional' glyphs",
                 num_proportional );
      else
        printf( "OK (proportional)" );
    }
    else
    {
      if ( face_has_fixed_flag )
        printf( "OK (fixed-width)" );
      else
        printf( "KO!  Tagged as proportional but has fixed width" );
    }
    printf( "\n" );
  }


  int
  main( int     argc,
        char**  argv )
  {
    FT_Face     face;
    FT_Library  library;

    int         i, file_index;
    char        filename[1024 + 4];
    char        alt_filename[1024 + 4];
    char*       execname;
    char*       fname;


    execname = argv[0];

    if ( argc < 2 )
      Usage( execname );

    error = FT_Init_FreeType( &library );
    if ( error )
      Panic( "Could not create library object" );

    /* Now check all files */
    for ( file_index = 1; file_index < argc; file_index++ )
    {
      fname = argv[file_index];

      /* try to open the file with no extra extension first */
      error = FT_New_Face( library, fname, 0, &face );
      if ( !error )
        goto Success;

      if ( error == FT_Err_Unknown_File_Format )
      {
        fprintf( stderr, "%s: unknown format\n", fname );
        continue;
      }

      /* Ok, we could not load the file.  Try to add an extension to */
      /* its name if possible.                                       */

      i = strlen( fname );
      while ( i > 0 && fname[i] != '\\' && fname[i] != '/' )
      {
        if ( fname[i] == '.' )
          i = 0;
        i--;
      }

      filename[1024] = '\0';
      alt_filename[1024] = '\0';

      strncpy( filename, fname, 1024 );
      strncpy( alt_filename, fname, 1024 );

#ifndef macintosh
      if ( i >= 0 )
      {
        strncpy( filename + strlen( filename ), ".ttf", 4 );
        strncpy( alt_filename + strlen( alt_filename ), ".ttc", 4 );
      }
#endif

      /* Load face */
      error = FT_New_Face( library, filename, 0, &face );
      if ( error )
      {
        if ( error == FT_Err_Unknown_File_Format )
          printf( "unknown format\n" );
        else
          printf( "could not find/open file (error: %d)\n", error );
        continue;
      }

  Success:
      check_face( face, fname, face->face_index );

      FT_Done_Face( face );
    }

    FT_Done_FreeType( library );
    exit( 0 );      /* for safety reasons */

    return 0;       /* never reached */
  }


/* End */
