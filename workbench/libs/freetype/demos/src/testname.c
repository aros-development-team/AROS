#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include <ft2build.h>
#include FT_FREETYPE_H


  int main( int    argc,
            char*  argv[] )
  {
    FT_Library       font_library;
    FT_Face          font_face;
    FT_Bitmap        bitmap;
    FT_GlyphSlot     cur_glyph;
    FT_Glyph_Metrics glyph_metrics;

    int  glyph_ind;
    int  num_chars;
    char char_name[256];


    if (argc != 2)
      exit( 1 );
    if ( FT_Init_FreeType( &font_library ) )
      exit( 1 );
    if ( FT_New_Face( font_library, argv[1], 0 , &font_face ) )
      exit( 1 );
    if ( FT_Set_Char_Size( font_face , 0 , 768 , 300 , 300 ) )
      exit( 1 );

    num_chars = (int)font_face->num_glyphs;
    FT_Set_Transform( font_face , NULL , NULL );

    for ( glyph_ind = 0 ; glyph_ind < num_chars; glyph_ind++ )
    {
      if ( FT_Load_Glyph( font_face, glyph_ind, FT_LOAD_DEFAULT ) )
        exit( 1 );
      cur_glyph = font_face->glyph;
      if ( cur_glyph->format != FT_GLYPH_FORMAT_BITMAP )
        if ( FT_Render_Glyph( font_face->glyph, FT_RENDER_MODE_MONO ) )
          exit( 1 );
      if ( FT_Get_Glyph_Name( font_face, glyph_ind, char_name, 16 ) )
        exit( 1 );

      bitmap = cur_glyph->bitmap;
      glyph_metrics = cur_glyph->metrics;

      printf( "Glyph %d  name %s %ld %ld %ld %d %d\n",
              glyph_ind,
              char_name,
              glyph_metrics.horiBearingX / 64,
              glyph_metrics.horiBearingY / 64,
              glyph_metrics.horiAdvance / 64,
              bitmap.width , bitmap.rows );
    }

    return 0;
  }


/* END */
