/****************************************************************************/
/*                                                                          */
/*  The FreeType project -- a free and portable quality TrueType renderer.  */
/*                                                                          */
/*  Copyright 2007 by                                                       */
/*  D. Turner, R.Wilhelm, and W. Lemberg                                    */
/*                                                                          */
/*                                                                          */
/*  FTpatchk - a simple program which tests whether patented hinting is     */
/*             necessary.                                                   */
/*                                                                          */
/*  Press F1 when running this program to have a list of key-bindings       */
/*                                                                          */
/****************************************************************************/


#include <stdio.h>
#include <stdlib.h>

#include <ft2build.h>
#include FT_FREETYPE_H


  int  main( int    argc,
             char*  argv[] )
  {
    FT_Error    error;
    FT_Library  library;


    error = FT_Init_FreeType( &library );
    if ( error )
    {
      fprintf( stderr, "could not create FreeType instance\n" );
      exit( 1 );
    }

    for ( ; argc > 1; argc--, argv++ )
    {
      FT_Face  face;


      error = FT_New_Face( library, argv[1], 0, &face );
      if ( error )
      {
        fprintf( stderr, "could not open as a valid font: `%s'\n", argv[1] );
        continue;
      }
      printf( "%-50s %s\n", argv[1],
              FT_Face_CheckTrueTypePatents( face )
                ? "uses patented opcodes"
                : "doesn't use patented opcodes" );
    }

    exit( 0 );
    return 0;
  }


/* End */
