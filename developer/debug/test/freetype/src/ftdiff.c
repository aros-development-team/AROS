/****************************************************************************/
/*                                                                          */
/*  The FreeType project -- a free and portable quality TrueType renderer.  */
/*                                                                          */
/*  Copyright (C) 2007-2019 by                                              */
/*  D. Turner, R.Wilhelm, and W. Lemberg                                    */
/*                                                                          */
/*                                                                          */
/*  FTDiff - a simple viewer to compare different hinting modes.            */
/*                                                                          */
/*  Press ? when running this program to have a list of key-bindings.       */
/*                                                                          */
/****************************************************************************/


#include "ftcommon.h"
#include "common.h"
#include "mlgetopt.h"

#include FT_OUTLINE_H
#include FT_LCD_FILTER_H
#include FT_DRIVER_H

  /* showing driver name -- the two internal header files */
  /* shouldn't be used in normal programs                 */
#include FT_MODULE_H
#include FT_INTERNAL_OBJECTS_H
#include FT_INTERNAL_DRIVER_H

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


  static void
  usage( char*  execname )
  {
    fprintf( stderr,
      "\n"
      "ftdiff: compare font hinting modes -- part of the FreeType project\n"
      "------------------------------------------------------------------\n"
      "\n" );
    fprintf( stderr,
      "Usage: %s [options] font ...\n"
      "\n",
             execname );
    fprintf( stderr,
      "  font      The font file(s) to display.\n"
      "            For Type 1 font files, ftdiff also tries to attach\n"
      "            the corresponding metrics file (with extension\n"
      "            `.afm' or `.pfm').\n"
      "\n" );
    fprintf( stderr,
      "  -w W         Set the window width to W pixels (default: 640px).\n"
      "  -h H         Set the window height to H pixels (default: 480px).\n"
      "  -r R         Use resolution R dpi (default: 72dpi).\n"
      "  -s S         Set character size to S points (default: 16pt).\n"
      "  -f TEXTFILE  Change displayed text, using text in TEXTFILE\n"
      "               (in UTF-8 encoding).\n"
      "\n"
      "  -v           Show version."
      "\n" );

    exit( 1 );
  }


  static void
  panic( const char*  fmt,
         ... )
  {
    va_list  args;


    va_start( args, fmt );
    vfprintf( stderr, fmt, args );
    va_end( args );
    exit( 1 );
  }


  /** DISPLAY ABSTRACTION **/

  typedef enum  DisplayMode_
  {
    DISPLAY_MODE_MONO = 0,
    DISPLAY_MODE_GRAY,
    DISPLAY_MODE_LCD

  } DisplayMode;


  typedef void
  (*Display_drawFunc)( void*        disp,
                       DisplayMode  mode,
                       int          x,
                       int          y,
                       int          width,
                       int          height,
                       int          pitch,
                       void*        buffer );

  typedef void
  (*Display_textFunc)( void*        disp,
                       int          x,
                       int          y,
                       const char*  msg );


  typedef struct  DisplayRec_
  {
    void*             disp;
    Display_drawFunc  disp_draw;
    Display_textFunc  disp_text;

  } DisplayRec, *Display;


  static const char*  default_text =
    "Lorem ipsum dolor sit amet, consectetuer adipiscing elit. Cras sit amet"
    " dui.  Nam sapien. Fusce vestibulum ornare metus. Maecenas ligula orci,"
    " consequat vitae, dictum nec, lacinia non, elit. Aliquam iaculis"
    " molestie neque. Maecenas suscipit felis ut pede convallis malesuada."
    " Aliquam erat volutpat. Nunc pulvinar condimentum nunc. Donec ac sem vel"
    " leo bibendum aliquam. Pellentesque habitant morbi tristique senectus et"
    " netus et malesuada fames ac turpis egestas.\n"
    "\n"
    "Sed commodo. Nulla ut libero sit amet justo varius blandit. Mauris vitae"
    " nulla eget lorem pretium ornare. Proin vulputate erat porta risus."
    " Vestibulum malesuada, odio at vehicula lobortis, nisi metus hendrerit"
    " est, vitae feugiat quam massa a ligula. Aenean in tellus. Praesent"
    " convallis. Nullam vel lacus.  Aliquam congue erat non urna mollis"
    " faucibus. Morbi vitae mauris faucibus quam condimentum ornare. Quisque"
    " sit amet augue. Morbi ullamcorper mattis enim. Aliquam erat volutpat."
    " Morbi nec felis non enim pulvinar lobortis.  Ut libero. Nullam id orci"
    " quis nisl dapibus rutrum. Suspendisse consequat vulputate leo. Aenean"
    " non orci non tellus iaculis vestibulum. Sed neque.\n"
    "\n";


  /***********************************************************************/
  /***********************************************************************/
  /*****                                                             *****/
  /*****               T E X T   R E N D E R I N G                   *****/
  /*****                                                             *****/
  /***********************************************************************/
  /***********************************************************************/

  typedef enum  HintMode_
  {
    HINT_MODE_UNHINTED,
    HINT_MODE_AUTOHINT,
    HINT_MODE_AUTOHINT_LIGHT,
    HINT_MODE_AUTOHINT_LIGHT_SUBPIXEL,
    HINT_MODE_BYTECODE,
    HINT_MODE_MAX

  } HintMode;

  static const char* const  render_mode_names[HINT_MODE_MAX] =
  {
    "unhinted",
    "auto-hinter",
    "light auto-hinter",
    "light auto-hinter (subp.)",
    "native hinter"
  };

  /** RENDER STATE **/

  typedef struct  ColumnStateRec_
  {
    int            use_cboxes;
    int            use_kerning;
    int            use_deltas;
    int            use_lcd_filter;
    FT_LcdFilter   lcd_filter;
    HintMode       hint_mode;
    DisplayMode    disp_mode;

    int            use_custom_lcd_filter;
    unsigned char  filter_weights[5];
    int            fw_index;

    unsigned int   cff_hinting_engine;
    unsigned int   type1_hinting_engine;
    unsigned int   t1cid_hinting_engine;
    unsigned int   tt_interpreter_versions[3];
    int            num_tt_interpreter_versions;
    int            tt_interpreter_version_idx;
    FT_Bool        warping;

  } ColumnStateRec, *ColumnState;


  typedef struct  FontFaceRec_
  {
    const char*  filepath;
    char*        family_name;
    char*        style_name;
    int          index;

  } FontFaceRec, *FontFace;


  typedef struct  RenderStateRec_
  {
    FT_Library      library;
    const char*     text;
    unsigned int    resolution;
    double          char_size;
    int             col;
    ColumnStateRec  columns[3];
    FontFace        faces;
    unsigned int    num_faces;
    int             face_index;
    const char*     filepath;
    const char*     filename;
    FT_Face         face;
    FT_Size         size;
    char**          files;
    DisplayRec      display;
    char            filepath0[1024];

  } RenderStateRec, *RenderState;


  static void
  render_state_init( RenderState  state,
                     Display      display,
                     FT_Library   library )
  {
    FT_UInt  cff_hinting_engine;
    FT_UInt  type1_hinting_engine;
    FT_UInt  t1cid_hinting_engine;
    FT_Bool  warping;

    unsigned int  tt_interpreter_versions[3]  = { 0, 0, 0 };
    int           num_tt_interpreter_versions = 0;
    int           tt_interpreter_version_idx  = 0;

    unsigned int  dflt_tt_interpreter_version;
    int           i;
    unsigned int  versions[3] = { TT_INTERPRETER_VERSION_35,
                                  TT_INTERPRETER_VERSION_38,
                                  TT_INTERPRETER_VERSION_40 };


    memset( state, 0, sizeof ( *state ) );

    state->library = library;

    state->text         = default_text;
    state->filepath     = state->filepath0;
    state->filename     = "<none>";
    state->filepath0[0] = 0;
    state->resolution   = 72;
    state->char_size    = 16;
    state->display      = display[0];

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
    for ( i = 0; i < 3; i++ )
    {
      error = FT_Property_Set( library,
                               "truetype",
                               "interpreter-version", &versions[i] );
      if ( !error )
        tt_interpreter_versions[num_tt_interpreter_versions++] = versions[i];
      if ( versions[i] == dflt_tt_interpreter_version )
        tt_interpreter_version_idx = i;
    }
    FT_Property_Set( library,
                     "truetype",
                     "interpreter-version", &dflt_tt_interpreter_version );

    FT_Property_Get( library,
                     "autofitter",
                     "warping", &warping );

    state->columns[0].use_cboxes             = 0;
    state->columns[0].use_kerning            = 1;
    state->columns[0].use_deltas             = 1;
    state->columns[0].use_lcd_filter         = 1;
    state->columns[0].lcd_filter             = FT_LCD_FILTER_DEFAULT;
    state->columns[0].hint_mode              = HINT_MODE_BYTECODE;
    state->columns[0].cff_hinting_engine     = cff_hinting_engine;
    state->columns[0].type1_hinting_engine   = type1_hinting_engine;
    state->columns[0].t1cid_hinting_engine   = t1cid_hinting_engine;

    state->columns[0].tt_interpreter_versions[0] =
      tt_interpreter_versions[0];
    state->columns[0].tt_interpreter_versions[1] =
      tt_interpreter_versions[1];
    state->columns[0].tt_interpreter_versions[2] =
      tt_interpreter_versions[2];
    state->columns[0].num_tt_interpreter_versions =
      num_tt_interpreter_versions;
    state->columns[0].tt_interpreter_version_idx =
      tt_interpreter_version_idx;

    state->columns[0].warping                = warping;
    state->columns[0].use_custom_lcd_filter  = 0;
    state->columns[0].fw_index               = 2;
    /* FreeType default filter weights */
    memcpy( state->columns[0].filter_weights, "\x08\x4D\x56\x4D\x08", 5 );

    state->columns[1]                        = state->columns[0];
    state->columns[1].hint_mode              = HINT_MODE_AUTOHINT;
    state->columns[1].use_custom_lcd_filter  = 1;

    state->columns[2]                        = state->columns[0];
    state->columns[2].hint_mode              = HINT_MODE_UNHINTED;

    state->col = 1;
  }


  static void
  render_state_done( RenderState  state )
  {
    if ( state->filepath != state->filepath0 )
    {
      free( (char*)state->filepath );
      state->filepath = state->filepath0;
    }
    state->filepath0[0] = 0;
    state->filename     = 0;

    if ( state->face )
    {
      FT_Done_Face( state->face );
      state->face = NULL;
      state->size = NULL;
    }

    if ( state->library )
    {
      FT_Done_FreeType( state->library );
      state->library = NULL;
    }
  }


  static void
  render_state_set_resolution( RenderState   state,
                               unsigned int  resolution )
  {
    state->resolution = resolution;
  }


  static void
  render_state_set_size( RenderState  state,
                         double       char_size )
  {
    state->char_size = char_size;
  }


  static void
  render_state_set_face_index( RenderState  state,
                               int          idx )
  {
    state->face_index = idx;
  }


  static void
  _render_state_rescale( RenderState  state )
  {
    if ( state->size )
      FT_Set_Char_Size( state->face, 0,
                        (FT_F26Dot6)( state->char_size * 64.0 ),
                        0, state->resolution );
  }


  static void
  render_state_set_files( RenderState  state,
                          char**       files,
                          char*        execname )
  {
    FontFace      faces     = NULL;
    unsigned int  num_faces = 0;
    unsigned int  max_faces = 0;


    state->files = files;
    for ( ; files[0] != NULL; files++ )
    {
      FT_Face  face;

      int  num_subfonts;
      int  count;


      error = FT_New_Face( state->library, files[0], -1, &face );
      if ( error )
      {
        fprintf( stderr,
                 "ftdiff: could not open font file `%s'\n",
                 files[0] );
        continue;
      }

      num_subfonts = face->num_faces;

      FT_Done_Face( face );

      for ( count = 0; count < num_subfonts; count++ )
      {
        char*  fn;
        char*  family_name;
        char*  sn;
        char*  style_name;


        error = FT_New_Face( state->library, files[0], count, &face );
        if ( error )
        {
          fprintf( stderr,
                   "ftdiff: Opening `%s' failed with code 0x%X, skipping\n",
                   files[0], error );
          break;
        }

        if ( !FT_IS_SCALABLE( face ) )
        {
          fprintf( stderr,
                   "ftdiff: font `%s' is not scalable, skipping\n",
                   files[0] );
          goto Done;
        }

        if ( num_faces >= max_faces )
        {
          max_faces += ( max_faces >> 1 ) + 8;
          faces = (FontFace)realloc( faces,
                                     max_faces * sizeof ( faces[0] ) );
          if ( faces == NULL )
            panic( "ftdiff: not enough memory\n" );
        }

        if ( face->family_name )
          fn = face->family_name;
        else
          fn = (char*)"(unknown family)";
        family_name = (char*)malloc( strlen( fn ) + 1 );
        if ( family_name == NULL )
          panic( "ftdiff: not enough memory\n" );
        strcpy( family_name, fn );

        if ( face->style_name )
          sn = face->style_name;
        else
          sn = (char*)"(unknown style)";
        style_name = (char*)malloc( strlen( sn ) + 1 );
        if ( style_name == NULL )
          panic( "ftdiff: not enough memory\n" );
        strcpy( style_name, sn );

        faces[num_faces].filepath    = files[0];
        faces[num_faces].index       = count;
        faces[num_faces].family_name = family_name;
        faces[num_faces].style_name  = style_name;
        num_faces++;

      Done:
        FT_Done_Face( face );
      }
    }

    state->faces     = faces;
    state->num_faces = num_faces;

    if ( num_faces == 0 )
    {
      fprintf( stderr, "ftdiff: no input font files!\n" );
      usage( execname );
    }

    state->face_index = 0;
  }


  static int
  render_state_set_file( RenderState  state )
  {
    const char*  filepath;


    filepath = state->faces[state->face_index].filepath;

    if ( state->face )
    {
      FT_Done_Face( state->face );
      state->face = NULL;
      state->size = NULL;
    }

    if ( filepath != NULL && filepath[0] != 0 )
    {
      error = FT_New_Face( state->library,
                           filepath,
                           state->faces[state->face_index].index,
                           &state->face );
      if ( error )
        return -1;

      {
        unsigned int  len = strlen( filepath );
        char*         p;


        if ( len + 1 > sizeof ( state->filepath0 ) )
        {
          state->filepath = (const char*)malloc( len + 1 );
          if ( state->filepath == NULL )
          {
            state->filepath = state->filepath0;
            return -1;
          }
        }
        memcpy( (char*)state->filepath, filepath, len + 1 );
        p = (char*)strrchr( state->filepath, '\\' );
        if ( p == NULL )
          p = (char*)strrchr( state->filepath, '/' );

        state->filename = p ? p + 1 : state->filepath;
      }

      state->size = state->face->size;
    }

    return 0;
  }


  /** RENDERING **/

  static void
  render_state_draw( RenderState  state,
                     const char*  text,
                     int          idx,
                     int          x,
                     int          y,
                     int          width,
                     int          height )
  {
    ColumnState  column         = &state->columns[idx];
    const char*  p              = text;
    const char*  p_end          = p + strlen( text );
    long         load_flags     = FT_LOAD_DEFAULT;
    FT_Face      face;
    int          left           = x;
    int          right          = x + width;
    int          bottom         = y + height;
    int          line_height;
    FT_UInt      prev_glyph     = 0;
    FT_Pos       prev_rsb_delta = 0;
    FT_Pos       x_origin       = x << 6;
    HintMode     rmode          = column->hint_mode;
    FT_Bool      warping        = column->warping;
    FT_Bool      have_0x0A      = 0;
    FT_Bool      have_0x0D      = 0;


    /* no need to check for errors: the values used here are always valid */
    FT_Property_Set( state->library,
                     "cff",
                     "hinting-engine",
                     &column->cff_hinting_engine );
    FT_Property_Set( state->library,
                     "type1",
                     "hinting-engine",
                     &column->type1_hinting_engine );
    FT_Property_Set( state->library,
                     "t1cid",
                     "hinting-engine",
                     &column->t1cid_hinting_engine );

    FT_Property_Set( state->library,
                     "truetype",
                     "interpreter-version",
                     &column->tt_interpreter_versions
                       [column->tt_interpreter_version_idx] );
    FT_Property_Set( state->library,
                     "autofitter",
                     "warping",
                     &column->warping );

    /* changing a property is in most cases a global operation; */
    /* we are on the safe side if we reload the face completely */
    /* (this is something a normal program doesn't need to do)  */
    render_state_set_file( state );
    _render_state_rescale( state );

    face = state->face;

    if ( column->use_lcd_filter )
      FT_Library_SetLcdFilter( face->glyph->library, column->lcd_filter );

    if ( column->use_custom_lcd_filter )
      FT_Library_SetLcdFilterWeights( face->glyph->library,
                                      column->filter_weights );

    y          += state->size->metrics.ascender / 64;
    line_height = state->size->metrics.height / 64;

    if ( rmode == HINT_MODE_AUTOHINT )
      load_flags = FT_LOAD_FORCE_AUTOHINT;

    if ( rmode == HINT_MODE_AUTOHINT_LIGHT          ||
         rmode == HINT_MODE_AUTOHINT_LIGHT_SUBPIXEL )
      load_flags = FT_LOAD_TARGET_LIGHT;

    if ( rmode == HINT_MODE_UNHINTED )
      load_flags |= FT_LOAD_NO_HINTING | FT_LOAD_NO_BITMAP;

    while ( 1 )
    {
      int           ch;
      FT_UInt       gindex;
      FT_GlyphSlot  slot = face->glyph;
      FT_Bitmap*    map  = &slot->bitmap;
      FT_Long       xmax;


      ch = utf8_next( &p, p_end );
      if ( ch < 0 )
      {
        p  = text;
        ch = utf8_next( &p, p_end );
      }

      /* handle newlines */
      if ( ch == 0x0A )
      {
        if ( have_0x0D )
        {
          have_0x0A = 0;
          have_0x0D = 0;
        }
        else
        {
          have_0x0A = 1;

          x_origin = left << 6;
          y       += line_height;
          prev_rsb_delta = 0;
          if ( y >= bottom )
            break;
        }

        continue;
      }
      else if ( ch == 0x0D )
      {
        if ( have_0x0A )
        {
          have_0x0A = 0;
          have_0x0D = 0;
        }
        else
        {
          have_0x0D = 1;

          x_origin = left << 6;
          y       += line_height;
          prev_rsb_delta = 0;
          if ( y >= bottom )
            break;
        }

        continue;
      }
      else
      {
        have_0x0A = 0;
        have_0x0D = 0;
      }

      gindex = FT_Get_Char_Index( state->face, (FT_ULong)ch );
      error  = FT_Load_Glyph( face, gindex, load_flags );

      if ( error )
        continue;

      if ( column->use_kerning && gindex != 0 && prev_glyph != 0 )
      {
        FT_Vector  vec;
        FT_UInt    kerning_mode = FT_KERNING_DEFAULT;


        if ( rmode == HINT_MODE_UNHINTED )
          kerning_mode = FT_KERNING_UNFITTED;

        FT_Get_Kerning( face, prev_glyph, gindex, kerning_mode, &vec );

        x_origin += vec.x;
      }

      if ( rmode != HINT_MODE_AUTOHINT_LIGHT_SUBPIXEL &&
           column->use_deltas                         )
      {
        if ( prev_rsb_delta - face->glyph->lsb_delta > 32 )
          x_origin -= 64;
        else if ( prev_rsb_delta - face->glyph->lsb_delta < -31 )
          x_origin += 64;
      }
      prev_rsb_delta = face->glyph->rsb_delta;

      /* implement sub-pixel positioning for       */
      /* un-hinted and (second) light hinting mode */
      if ( ( rmode == HINT_MODE_UNHINTED                ||
             rmode == HINT_MODE_AUTOHINT_LIGHT_SUBPIXEL ) &&
           slot->format == FT_GLYPH_FORMAT_OUTLINE        )
      {
        FT_Pos  shift = x_origin & 63;


        FT_Outline_Translate( &slot->outline, shift, 0 );
      }

      if ( column->use_cboxes )
      {
        if ( slot->format == FT_GLYPH_FORMAT_OUTLINE )
        {
          FT_BBox  cbox;


          FT_Outline_Get_CBox( &slot->outline, &cbox );
          xmax = ( x_origin + cbox.xMax + 63 ) >> 6;
        }
        else
          xmax = ( x_origin >> 6 ) +
                 slot->bitmap_left + (FT_Long)slot->bitmap.width;
      }
      else
      {
        if ( rmode == HINT_MODE_UNHINTED                ||
             rmode == HINT_MODE_AUTOHINT_LIGHT_SUBPIXEL )
          xmax = slot->linearHoriAdvance >> 10;
        else
          xmax = slot->advance.x;

        xmax  += x_origin;
        xmax >>= 6;
        xmax  -= 1;
      }

      if ( slot->format == FT_GLYPH_FORMAT_OUTLINE )
        FT_Render_Glyph( slot,
                         column->use_lcd_filter ? FT_RENDER_MODE_LCD
                                                : FT_RENDER_MODE_NORMAL );

      if ( xmax >= right )
      {
        x  = left;
        y += line_height;
        if ( y >= bottom )
          break;

        x_origin       = x << 6;
        prev_rsb_delta = 0;
      }

      {
        DisplayMode  mode = DISPLAY_MODE_MONO;


        if ( slot->bitmap.pixel_mode == FT_PIXEL_MODE_GRAY )
          mode = DISPLAY_MODE_GRAY;
        else if ( slot->bitmap.pixel_mode == FT_PIXEL_MODE_LCD )
          mode = DISPLAY_MODE_LCD;

        state->display.disp_draw( state->display.disp, mode,
                                  ( x_origin >> 6 ) + slot->bitmap_left,
                                  y - slot->bitmap_top,
                                  (int)map->width, (int)map->rows,
                                  map->pitch, map->buffer );
      }
      if ( rmode == HINT_MODE_UNHINTED                ||
           rmode == HINT_MODE_AUTOHINT_LIGHT_SUBPIXEL )
        x_origin += slot->linearHoriAdvance >> 10;
      else
        x_origin += slot->advance.x;

      prev_glyph = gindex;
    }

    /* display footer on this column */
    {
      FT_Module    module = &state->face->driver->root;
      void*        disp   = state->display.disp;

      const char*  extra;
      const char*  msg;
      char         temp[64];


      extra = "";
      if ( rmode == HINT_MODE_BYTECODE )
      {
        if ( !strcmp( module->clazz->module_name, "cff" ) )
        {
          switch ( column->cff_hinting_engine )
          {
          case FT_HINTING_FREETYPE:
            extra = " (CFF FT)";
            break;
          case FT_HINTING_ADOBE:
            extra = " (CFF Adobe)";
            break;
          }
        }

        else if ( !strcmp( module->clazz->module_name, "type1" ) )
        {
          switch ( column->type1_hinting_engine )
          {
          case FT_HINTING_FREETYPE:
            extra = " (T1 FT)";
            break;
          case FT_HINTING_ADOBE:
            extra = " (T1 Adobe)";
            break;
          }
        }

        else if ( !strcmp( module->clazz->module_name, "t1cid" ) )
        {
          switch ( column->t1cid_hinting_engine )
          {
          case FT_HINTING_FREETYPE:
            extra = " (CID FT)";
            break;
          case FT_HINTING_ADOBE:
            extra = " (CID Adobe)";
            break;
          }
        }

        else if ( !strcmp( module->clazz->module_name, "truetype" ) )
        {
          switch ( column->tt_interpreter_versions[
                     column->tt_interpreter_version_idx] )
          {
          case TT_INTERPRETER_VERSION_35:
            extra = " (TT v35)";
            break;
          case TT_INTERPRETER_VERSION_38:
            extra = " (TT v38)";
            break;
          case TT_INTERPRETER_VERSION_40:
            extra = " (TT v40)";
            break;
          }
        }
      }
      else if ( rmode == HINT_MODE_AUTOHINT )
        extra = warping ? " (+warp)" : " (-warp)";

      sprintf( temp, "%s%s",
               render_mode_names[column->hint_mode], extra );
      state->display.disp_text( disp, left,
                                bottom + 5, temp );

      if ( column->use_lcd_filter )
        msg = "LCD rendering";
      else
        msg = "gray rendering";
      state->display.disp_text( disp, left,
                                bottom + HEADER_HEIGHT + 5, msg );

      if ( column->use_lcd_filter )
      {
        if ( column->use_custom_lcd_filter )
        {
          int             fwi = column->fw_index;
          unsigned char*  fw  = column->filter_weights;


          sprintf( temp,
                   "%s0x%02X%s0x%02X%s0x%02X%s0x%02X%s0x%02X%s",
                   fwi == 0 ? "[" : " ",
                     fw[0],
                   fwi == 0 ? "]" : ( fwi == 1 ? "[" : " " ),
                     fw[1],
                   fwi == 1 ? "]" : ( fwi == 2 ? "[" : " " ),
                     fw[2],
                   fwi == 2 ? "]" : ( fwi == 3 ? "[" : " " ),
                     fw[3],
                   fwi == 3 ? "]" : ( fwi == 4 ? "[" : " " ),
                     fw[4],
                   fwi == 4 ? "]" : " " );
          state->display.disp_text( disp, left,
                                    bottom + 2 * HEADER_HEIGHT + 5, temp );
        }
        else
        {
          switch ( column->lcd_filter )
          {
          case FT_LCD_FILTER_NONE:
            msg = "LCD without filtering";
            break;
          case FT_LCD_FILTER_DEFAULT:
            msg = "default LCD filter";
            break;
          case FT_LCD_FILTER_LIGHT:
            msg = "light LCD filter";
            break;
          default:
            msg = "legacy LCD filter";
          }
          state->display.disp_text( disp, left,
                                    bottom + 2 * HEADER_HEIGHT + 5, msg );
        }
      }
      else
      {
        msg = "";
        state->display.disp_text( disp, left,
                                  bottom + 2 * HEADER_HEIGHT + 5, msg );
      }

      sprintf( temp, "%s %s %s",
               column->use_kerning ? "+kern"
                                   : "-kern",
               column->use_deltas ? "+delta"
                                  : "-delta",
               column->use_cboxes ? "glyph boxes"
                                  : "adv. widths" );
      state->display.disp_text( disp, left,
                                bottom + 3 * HEADER_HEIGHT + 5, temp );

      if ( state->col == idx )
        state->display.disp_text( disp, left,
                                  bottom + 4 * HEADER_HEIGHT + 5,
                                  "************************" );
    }
  }


  /***********************************************************************/
  /***********************************************************************/
  /*****                                                             *****/
  /*****                D I S P L A Y                                *****/
  /*****                                                             *****/
  /***********************************************************************/
  /***********************************************************************/

#include "graph.h"
#include "grobjs.h"
#include "grfont.h"

  typedef struct  ADisplayRec_
  {
    int         width;
    int         height;
    grSurface*  surface;
    grBitmap*   bitmap;
    grColor     fore_color;
    grColor     back_color;
    double      gamma;

  } ADisplayRec, *ADisplay;


  static int
  adisplay_init( ADisplay     display,
                 grPixelMode  mode,
                 int          width,
                 int          height )
  {
    grSurface*  surface;
    grBitmap    bit;


    if ( mode != gr_pixel_mode_gray  &&
         mode != gr_pixel_mode_rgb24 )
      return -1;

    grInitDevices();

    bit.mode  = mode;
    bit.width = width;
    bit.rows  = height;
    bit.grays = 256;

    surface = grNewSurface( 0, &bit );

    if ( !surface )
      return -1;

    display->width   = width;
    display->height  = height;
    display->surface = surface;
    display->bitmap  = &surface->bitmap;
    display->gamma   = GAMMA;

    grSetGlyphGamma( display->gamma );

    memset( &display->fore_color, 0, sizeof( grColor ) );
    memset( &display->back_color, 0xff, sizeof( grColor ) );

    return 0;
  }


  static void
  adisplay_clear( ADisplay  display )
  {
    grBitmap*  bit   = display->bitmap;
    int        pitch = bit->pitch;


    if ( pitch < 0 )
      pitch = -pitch;

    if ( bit->mode == gr_pixel_mode_gray )
      memset( bit->buffer,
              display->back_color.value,
              (unsigned int)( pitch * bit->rows ) );
    else
    {
      unsigned char*  p = bit->buffer;
      int             i, j;


      for ( i = 0; i < bit->rows; i++ )
      {
        for ( j = 0; j < bit->width; j++ )
          memcpy( p + 3 * j, display->back_color.chroma, 3 );

        p += pitch;
      }
    }
  }


  static void
  adisplay_done( ADisplay  display )
  {
    grDoneBitmap( display->bitmap );
    grDoneSurface( display->surface );

    display->bitmap  = NULL;
    display->surface = NULL;

    grDoneDevices();
  }


  static void
  adisplay_draw_glyph( void*        _display,
                       DisplayMode  mode,
                       int          x,
                       int          y,
                       int          width,
                       int          height,
                       int          pitch,
                       void*        buffer )
  {
    ADisplay  display = (ADisplay)_display;
    grBitmap  glyph;


    glyph.width  = width;
    glyph.rows   = height;
    glyph.pitch  = pitch;
    glyph.buffer = (unsigned char*)buffer;
    glyph.grays  = 256;
    glyph.mode   = gr_pixel_mode_mono;

    if ( mode == DISPLAY_MODE_GRAY )
      glyph.mode = gr_pixel_mode_gray;
    else if ( mode == DISPLAY_MODE_LCD )
      glyph.mode = gr_pixel_mode_lcd;

    grBlitGlyphToBitmap( display->bitmap, &glyph,
                         x, y, display->fore_color );
  }


  static void
  adisplay_draw_text( void*        _display,
                      int          x,
                      int          y,
                      const char*  msg )
  {
    ADisplay  adisplay = (ADisplay)_display;


    grWriteCellString( adisplay->bitmap, x, y, msg,
                       adisplay->fore_color );
  }


  static void
  adisplay_change_gamma( ADisplay  display,
                         double    delta )
  {
    /* use epsilons */
    display->gamma += delta;
    if ( display->gamma > 2.9999 )
      display->gamma = 3.0;
    else if ( display->gamma < 0.0001 )
      display->gamma = 0.0;

    grSetGlyphGamma( display->gamma );
  }


  static void
  event_help( RenderState  state )
  {
    char  buf[256];
    char  version[64];

    const char*  format;
    FT_Int       major, minor, patch;

    ADisplay  display = (ADisplay)state->display.disp;
    grEvent   dummy_event;


    FT_Library_Version( state->library, &major, &minor, &patch );

    format = patch ? "%d.%d.%d" : "%d.%d";
    sprintf( version, format, major, minor, patch );

    adisplay_clear( display );
    grSetLineHeight( 10 );
    grGotoxy( 0, 0 );
    grSetMargin( 2, 1 );
    grGotobitmap( display->bitmap );

    sprintf( buf,
             "FreeType Hinting Mode Comparator - part of the FreeType %s test suite",
             version );

    grWriteln( buf );
    grLn();
    grWriteln( "Use the following keys:" );
    grLn();
    /*          |----------------------------------|    |----------------------------------| */
    grWriteln( "F1, ?       display this help screen                                        " );
    grWriteln( "                                                                            " );
    grWriteln( "global parameters:                                                          " );
    grWriteln( "  p, n        previous/next font        1, 2, 3      select column          " );
    grWriteln( "  Up, Down    adjust size by 0.5pt      Left, Right  switch between columns " );
    grWriteln( "  PgUp, PgDn  adjust size by 5pt        g, v         adjust gamma value     " );
    grWriteln( "                                                                            " );
    grWriteln( " per-column parameters:                                                     " );
    grWriteln( "  d           toggle lsb/rsb deltas     hinting modes:                      " );
    grWriteln( "  h           cycle hinting mode          A          unhinted               " );
    grWriteln( "  H           cycle hinting engine        B          auto-hinter            " );
    grWriteln( "               (if CFF or TTF)            C          light auto-hinter      " );
    grWriteln( "  w           toggle warping (if          D          light auto-hinter      " );
    grWriteln( "               normal auto-hinting)                   (subpixel)            " );
    grWriteln( "  k           toggle kerning (only        E          native hinter          " );
    grWriteln( "               from `kern' table)                                           " );
    grWriteln( "  r           toggle rendering mode                                         " );
    grWriteln( "  x           toggle layout mode                                            " );
    grWriteln( "                                                                            " );
    grWriteln( "  l           cycle LCD filtering                                           " );
    grWriteln( "  [, ]        select custom LCD                                             " );
    grWriteln( "               filter weight                                                " );
    grWriteln( "  -, +(=)     adjust selected custom                                        " );
    grWriteln( "               LCD filter weight                                            " );
    /*          |----------------------------------|    |----------------------------------| */
    grLn();
    grLn();
    grWriteln( "press any key to exit this help screen" );

    grRefreshSurface( display->surface );
    grListenSurface( display->surface, gr_event_key, &dummy_event );
  }


  static void
  event_change_gamma( RenderState  state,
                      double       delta )
  {
    ADisplay  display = (ADisplay)state->display.disp;


    adisplay_change_gamma( display, delta );
  }


  static void
  event_change_size( RenderState  state,
                     double       delta )
  {
    double  char_size = state->char_size;


    char_size += delta;
    if ( char_size < 6.0 )
      char_size = 6.0;
    else if ( char_size > 300.0 )
      char_size = 300.0;

    render_state_set_size( state, char_size );
  }


  static void
  event_change_face_index( RenderState  state,
                           int          idx )
  {
    if ( idx < 0 )
      idx = 0;

    if ( idx >= (int)state->num_faces )
      idx = (int)state->num_faces - 1;

    render_state_set_face_index( state, idx );
  }


  static int
  process_event( RenderState  state,
                 grEvent*     event )
  {
    int          ret    = 0;
    ColumnState  column = &state->columns[state->col];


    if ( event->key >= 'A'                &&
         event->key < 'A' + HINT_MODE_MAX )
    {
      column->hint_mode = (HintMode)( event->key - 'A' );
      return ret;
    }

    switch ( event->key )
    {
    case grKeyEsc:
    case grKEY( 'q' ):
      ret = 1;
      break;

    case grKeyF1:
    case grKEY( '?' ):
      event_help( state );
      break;

    case grKeyLeft:
      if ( --state->col < 0 )
        state->col = 2;
      break;

    case grKeyRight:
      if ( ++state->col > 2 )
        state->col = 0;
      break;

    case grKeyUp:
      event_change_size( state, 0.5 );
      break;

    case grKeyDown:
      event_change_size( state, -0.5 );
      break;

    case grKeyPageUp:
      event_change_size( state, 5. );
      break;

    case grKeyPageDown:
      event_change_size( state, -5. );
      break;

    case grKEY( '1' ):
      state->col = 0;
      break;

    case grKEY( '2' ):
      state->col = 1;
      break;

    case grKEY( '3' ):
      state->col = 2;
      break;

    case grKEY( 'd' ):
      column->use_deltas = !column->use_deltas;
      break;

    case grKEY( 'g' ):
      event_change_gamma( state, 0.1 );
      break;

    case grKEY( 'h' ):
      column->hint_mode =
        (HintMode)( ( column->hint_mode + 1 ) % HINT_MODE_MAX );
      break;

    case grKEY( 'H' ):
      {
        FT_Module  module = &state->face->driver->root;


        if ( column->hint_mode == HINT_MODE_BYTECODE )
        {
          if ( !strcmp( module->clazz->module_name, "cff" ) )
          {
            FTDemo_Event_Cff_Hinting_Engine_Change(
              state->library,
              &column->cff_hinting_engine,
              1 );
          }
          else if ( !strcmp( module->clazz->module_name, "type1" ) )
          {
            FTDemo_Event_Type1_Hinting_Engine_Change(
              state->library,
              &column->type1_hinting_engine,
              1 );
          }
          else if ( !strcmp( module->clazz->module_name, "t1cid" ) )
          {
            FTDemo_Event_T1cid_Hinting_Engine_Change(
              state->library,
              &column->t1cid_hinting_engine,
              1 );
          }
          else if ( !strcmp( module->clazz->module_name, "truetype" ) )
          {
            column->tt_interpreter_version_idx += 1;
            column->tt_interpreter_version_idx %=
              column->num_tt_interpreter_versions;

            FT_Property_Set( state->library,
                             "truetype",
                             "interpreter-version",
                             &column->tt_interpreter_versions[
                               column->tt_interpreter_version_idx] );
          }
        }
      }
      break;

    case grKEY( 'w' ):
      {
        FT_Bool  new_warping_state = !column->warping;


        error = FT_Property_Set( state->library,
                                 "autofitter",
                                 "warping",
                                 &new_warping_state );
        if ( !error )
          column->warping = new_warping_state;
      }
      break;

    case grKEY( 'k' ):
      column->use_kerning = !column->use_kerning;
      break;

    case grKEY( 'l' ):
      switch ( column->lcd_filter )
      {
      case FT_LCD_FILTER_NONE:
        column->lcd_filter = FT_LCD_FILTER_DEFAULT;
        break;

      case FT_LCD_FILTER_DEFAULT:
        if ( !column->use_custom_lcd_filter )
          column->use_custom_lcd_filter = 1;
        else
        {
          column->use_custom_lcd_filter = 0;
          column->lcd_filter            = FT_LCD_FILTER_LIGHT;
        }
        break;

      case FT_LCD_FILTER_LIGHT:
        column->lcd_filter = FT_LCD_FILTER_LEGACY;
        break;

      case FT_LCD_FILTER_LEGACY:
        column->lcd_filter = FT_LCD_FILTER_NONE;
        break;

      default:  /* to satisfy picky compilers */
        break;
      }
      break;

    case grKEY( 'n' ):
      event_change_face_index( state, state->face_index + 1 );
      break;

    case grKEY( 'p' ):
      event_change_face_index( state, state->face_index - 1 );
      break;

    case grKEY( 'r' ):
      column->use_lcd_filter = !column->use_lcd_filter;
      break;

    case grKEY( 'v' ):
      event_change_gamma( state, -0.1 );
      break;

    case grKEY( 'x' ):
      column->use_cboxes = !column->use_cboxes;
      break;

    case grKEY( '[' ):
      if ( !column->use_custom_lcd_filter )
        break;

      column->fw_index--;
      if ( column->fw_index < 0 )
        column->fw_index = 4;
      break;

    case grKEY( ']' ):
      if ( !column->use_custom_lcd_filter )
        break;

      column->fw_index++;
      if ( column->fw_index > 4 )
        column->fw_index = 0;
      break;

    case grKEY( '-' ):
      if ( !column->use_custom_lcd_filter )
        break;

      column->filter_weights[column->fw_index]--;
      break;

    case grKEY( '+' ):
    case grKEY( '=' ):
      if ( !column->use_custom_lcd_filter )
        break;

      column->filter_weights[column->fw_index]++;
      break;

    default:
      break;
    }

    return ret;
  }


  static void
  write_global_info( RenderState  state )
  {
    ADisplay  adisplay = (ADisplay)state->display.disp;
    char      gamma[] = "sRGB";
    char      buf[256];

    FontFace     face = &state->faces[state->face_index];
    const char*  basename;


    basename = ft_basename( state->filename );
    sprintf( buf, "%.50s %.50s (file `%.100s')",
                  face->family_name,
                  face->style_name,
                  basename );
    grWriteCellString( adisplay->bitmap, 0, 5,
                       buf, adisplay->fore_color );

    if ( adisplay->gamma != 0.0 )
      sprintf( gamma, "%.1f", adisplay->gamma );
    sprintf( buf, "%.1fpt (%dppem) at %ddpi, gamma: %s",
                  state->char_size,
                  (int)(state->char_size * state->resolution / 72 + 0.5),
                  state->resolution,
                  gamma );
    grWriteCellString( adisplay->bitmap, 0, 5 + HEADER_HEIGHT,
                       buf, adisplay->fore_color );

  }


  int
  main( int     argc,
        char**  argv )
  {
    FT_Library  library;

    ADisplayRec     adisplay[1];
    RenderStateRec  state[1];
    DisplayRec      display[1];
    int             width      = 640;
    int             height     = 480;
    int             resolution = -1;
    double          size       = -1;
    const char*     textfile   = NULL;
    char*           text       = (char*)default_text;

    char*  execname;
    int    option;


    execname  = ft_basename( argv[0] );

    if ( FT_Init_FreeType( &library ) != 0 )
      panic( "could not initialize FreeType\n" );

    while ( 1 )
    {
      option = getopt( argc, argv, "f:h:r:s:vw:" );

      if ( option == -1 )
        break;

      switch ( option )
      {
      case 'f':
        textfile = optarg;
        break;

      case 'h':
        height = atoi( optarg );
        if ( height < 1 )
          usage( execname );
        break;

      case 'r':
        resolution = atoi( optarg );
        break;

      case 's':
        size = atof( optarg );
        break;

      case 'v':
        {
          FT_Int  major, minor, patch;


          FT_Library_Version( library, &major, &minor, &patch );

          printf( "ftdiff (FreeType) %d.%d", major, minor );
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

    if ( argc < 1 )
      usage( execname );

    /* Read Text File, if any */
    if ( textfile != NULL )
    {
      FILE*  tfile = fopen( textfile, "r" );


      if ( tfile == NULL )
        fprintf( stderr, "could not read textfile '%s'\n", textfile );
      else
      {
        int  tsize;


        fseek( tfile, 0, SEEK_END );
        tsize = ftell( tfile );

        fseek( tfile, 0, SEEK_SET );
        text = (char*)malloc( (unsigned int)( tsize + 1 ) );

        if ( text )
        {
          if ( !fread( text, (unsigned int)tsize, 1, tfile ) )
          {
            fprintf( stderr, "read error\n" );
            text = (char *)default_text;
          }
          else
            text[tsize] = '\0';
        }
        else
        {
          fprintf( stderr, "not enough memory to read `%s'\n", textfile );
          text = (char *)default_text;
        }

        fclose( tfile );
      }
    }

    /* Initialize display */
    if ( adisplay_init( adisplay, gr_pixel_mode_rgb24,
                        width, height ) < 0 )
    {
      fprintf( stderr, "could not initialize display!  Aborting.\n" );
      exit( 1 );
    }
    display->disp      = adisplay;
    display->disp_draw = adisplay_draw_glyph;
    display->disp_text = adisplay_draw_text;

    render_state_init( state, display, library );

    if ( resolution > 0 )
      render_state_set_resolution( state, (unsigned int)resolution );

    if ( size > 0.0 )
      render_state_set_size( state, size );

    render_state_set_files( state, argv, execname );

    grSetTitle( adisplay->surface, "FreeType Text Proofer, press ? for help" );

    for (;;)
    {
      grEvent  event;

      int  border_width;

      int  column_x_start[3];
      int  column_y_start;

      int  column_height;
      int  column_width;


      adisplay_clear( adisplay );

      /* We have this layout:                                */
      /*                                                     */
      /*  | n ----x---- n  n ----x---- n  n ----x---- n |    */
      /*                                                     */
      /* w = 6 * n + 3 * x                                   */

      border_width = 10;                                /* n */
      column_width = ( width - 6 * border_width ) / 3;  /* x */

      column_x_start[0] =     border_width;
      column_x_start[1] = 3 * border_width +     column_width;
      column_x_start[2] = 5 * border_width + 2 * column_width;

      column_y_start = 10 + 2 * HEADER_HEIGHT;
      column_height  = height - 8 * HEADER_HEIGHT - 5;

      render_state_draw( state, text, 0,
                         column_x_start[0], column_y_start,
                         column_width, column_height );
      render_state_draw( state, text, 1,
                         column_x_start[1], column_y_start,
                         column_width, column_height );
      render_state_draw( state, text, 2,
                         column_x_start[2], column_y_start,
                         column_width, column_height );

      write_global_info( state );
      grRefreshSurface( adisplay->surface );
      grListenSurface( adisplay->surface, 0, &event );
      if ( process_event( state, &event ) )
        break;
    }

    render_state_done( state );
    adisplay_done( adisplay );
    exit( 0 );  /* for safety reasons */

    /* return 0; */  /* never reached */
  }


/* End */
