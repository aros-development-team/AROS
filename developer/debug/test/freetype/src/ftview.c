/****************************************************************************/
/*                                                                          */
/*  The FreeType project -- a free and portable quality TrueType renderer.  */
/*                                                                          */
/*  Copyright (C) 1996-2019 by                                              */
/*  D. Turner, R.Wilhelm, and W. Lemberg                                    */
/*                                                                          */
/*                                                                          */
/*  FTView - a simple font viewer.                                          */
/*                                                                          */
/*  This is a new version using the MiGS graphics subsystem for             */
/*  blitting and display.                                                   */
/*                                                                          */
/*  Press ? when running this program to have a list of key-bindings.       */
/*                                                                          */
/****************************************************************************/


#include "ftcommon.h"
#include "common.h"
#include "mlgetopt.h"
#include <stdio.h>

  /* the following header shouldn't be used in normal programs */
#include FT_INTERNAL_DEBUG_H

  /* showing driver name */
#include FT_MODULE_H
#include FT_INTERNAL_OBJECTS_H
#include FT_INTERNAL_DRIVER_H

#include FT_STROKER_H
#include FT_SYNTHESIS_H
#include FT_LCD_FILTER_H
#include FT_DRIVER_H

#include FT_COLOR_H
#include FT_BITMAP_H


#define MAXPTSIZE  500                 /* dtp */

#ifdef CEIL
#undef CEIL
#endif
#define CEIL( x )  ( ( (x) + 63 ) >> 6 )

#define START_X  18 * 8
#define START_Y  3 * HEADER_HEIGHT

#define INIT_SIZE( size, start_x, start_y, step_y, x, y )        \
          do {                                                   \
            start_x = START_X;                                   \
            start_y = CEIL( size->metrics.ascender -             \
                            size->metrics.descender ) + START_Y; \
            step_y  = CEIL( size->metrics.height ) + 4;          \
                                                                 \
            x = start_x;                                         \
            y = start_y;                                         \
          } while ( 0 )

#define X_TOO_LONG( x, display )                 \
          ( (x) >= (display)->bitmap->width - 3 )
#define Y_TOO_LONG( y, display )                \
          ( (y) >= (display)->bitmap->rows - 3 )

#ifdef _WIN32
#define snprintf  _snprintf
#endif


  /* omit LCD_MODE_LIGHT_SUBPIXEL; we don't need it in this application */
  static int  lcd_modes[] =
  {
    LCD_MODE_MONO,
    LCD_MODE_AA,
    LCD_MODE_LIGHT,
    LCD_MODE_RGB,
    LCD_MODE_BGR,
    LCD_MODE_VRGB,
    LCD_MODE_VBGR
  };

#define N_LCD_IDXS  ( (int)( sizeof ( lcd_modes ) / sizeof ( int ) ) )


  enum
  {
    RENDER_MODE_ALL = 0,
    RENDER_MODE_FANCY,
    RENDER_MODE_STROKE,
    RENDER_MODE_TEXT,
    RENDER_MODE_WATERFALL,
    N_RENDER_MODES
  };

  static struct  status_
  {
    int            update;

    const char*    dims;
    int            render_mode;

    int            res;
    int            ptsize;            /* current point size, 26.6 format */
    int            lcd_idx;
    double         xbold_factor;
    double         ybold_factor;
    double         radius;
    double         slant;

    unsigned int   cff_hinting_engine;
    unsigned int   type1_hinting_engine;
    unsigned int   t1cid_hinting_engine;
    unsigned int   tt_interpreter_versions[3];
    int            num_tt_interpreter_versions;
    int            tt_interpreter_version_idx;
    FT_Bool        warping;

    int            font_idx;
    int            offset;            /* as selected by the user */
    int            topleft;           /* as displayed by ftview  */
    int            num_fails;
    int            preload;

    int            lcd_filter;
    unsigned char  filter_weights[5];
    int            fw_idx;

  } status = { 1,
               DIM, RENDER_MODE_ALL,
               72, 48, 1, 0.04, 0.04, 0.02, 0.22,
               0, 0, 0, { 0 }, 0, 0, 0, /* default values are set at runtime */
               0, 0, 0, 0, 0,
               FT_LCD_FILTER_DEFAULT, { 0x08, 0x4D, 0x56, 0x4D, 0x08 }, 2 };


  static FTDemo_Display*  display;
  static FTDemo_Handle*   handle;


  /*
     In UTF-8 encoding:

       The quick brown fox jumps over the lazy dog
       0123456789
       âêîûôäëïöüÿàùéèç
       &#~"'(-`_^@)=+°
       ABCDEFGHIJKLMNOPQRSTUVWXYZ
       $£^¨*µù%!§:/;.,?<>

     The trailing space is for `looping' in case `Text' gets displayed more
     than once.
   */
  static const char*  Text =
    "The quick brown fox jumps over the lazy dog"
    " 0123456789"
    " \303\242\303\252\303\256\303\273\303\264"
     "\303\244\303\253\303\257\303\266\303\274\303\277"
     "\303\240\303\271\303\251\303\250\303\247"
    " &#~\"\'(-`_^@)=+\302\260"
    " ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    " $\302\243^\302\250*\302\265\303\271%!\302\247:/;.,?<> ";


  static void
  Fatal( const char*  message )
  {
    FTDemo_Display_Done( display );
    FTDemo_Done( handle );
    PanicZ( message );
  }


  static FT_Error
  Render_Stroke( int  num_indices,
                 int  offset )
  {
    int           start_x, start_y, step_y, x, y, width;
    int           i, have_topleft;
    FT_Size       size;
    FT_Face       face;
    FT_GlyphSlot  slot;

    FT_Fixed  radius;


    error = FTDemo_Get_Size( handle, &size );
    if ( error )
    {
      /* probably a non-existent bitmap font size */
      return error;
    }

    INIT_SIZE( size, start_x, start_y, step_y, x, y );
    face = size->face;
    slot = face->glyph;

    radius = (FT_Fixed)( size->metrics.y_ppem * 64 * status.radius );

    FT_Stroker_Set( handle->stroker, radius,
                    FT_STROKER_LINECAP_ROUND,
                    FT_STROKER_LINEJOIN_ROUND,
                    0 );

    have_topleft = 0;

    for ( i = offset; i < num_indices; i++ )
    {
      FT_UInt  glyph_idx;


      glyph_idx = FTDemo_Get_Index( handle, (FT_UInt32)i );

      error = FT_Load_Glyph( face, glyph_idx,
                             handle->load_flags | FT_LOAD_NO_BITMAP );

      if ( !error && slot->format == FT_GLYPH_FORMAT_OUTLINE )
      {
        FT_Glyph  glyph;


        error = FT_Get_Glyph( slot, &glyph );
        if ( error )
          goto Next;

        error = FT_Glyph_Stroke( &glyph, handle->stroker, 1 );
        if ( error )
        {
          FT_Done_Glyph( glyph );
          goto Next;
        }

        width = slot->advance.x ? slot->advance.x >> 6
                                : size->metrics.y_ppem / 2;

        if ( X_TOO_LONG( x + width, display ) )
        {
          x  = start_x;
          y += step_y;

          if ( Y_TOO_LONG( y, display ) )
          {
            FT_Done_Glyph( glyph );
            break;
          }
        }

        /* extra space between glyphs */
        x++;
        if ( slot->advance.x == 0 )
        {
          grFillRect( display->bitmap, x, y - width, width, width,
                      display->warn_color );
          x += width;
        }

        error = FTDemo_Draw_Glyph( handle, display, glyph, &x, &y );

        if ( error )
          goto Next;
        else
          FT_Done_Glyph( glyph );

        if ( !have_topleft )
        {
          have_topleft   = 1;
          status.topleft = i;
        }
      }
      else
    Next:
      status.num_fails++;
    }

    return error;
  }


  static FT_Error
  Render_Fancy( int  num_indices,
                int  offset )
  {
    int           start_x, start_y, step_y, x, y, width;
    int           i, have_topleft;
    FT_Size       size;
    FT_Face       face;
    FT_GlyphSlot  slot;

    FT_Matrix  shear;
    FT_Pos     xstr, ystr;


    error = FTDemo_Get_Size( handle, &size );
    if ( error )
    {
      /* probably a non-existent bitmap font size */
      return error;
    }

    INIT_SIZE( size, start_x, start_y, step_y, x, y );
    face = size->face;
    slot = face->glyph;

    /***************************************************************/
    /*                                                             */
    /*  2*2 affine transformation matrix, 16.16 fixed float format */
    /*                                                             */
    /*  Shear matrix:                                              */
    /*                                                             */
    /*         | x' |     | 1  k |   | x |          x' = x + ky    */
    /*         |    |  =  |      | * |   |   <==>                  */
    /*         | y' |     | 0  1 |   | y |          y' = y         */
    /*                                                             */
    /*        outline'     shear    outline                        */
    /*                                                             */
    /***************************************************************/

    shear.xx = 1 << 16;
    shear.xy = (FT_Fixed)( status.slant * ( 1 << 16 ) );
    shear.yx = 0;
    shear.yy = 1 << 16;

    xstr = (FT_Pos)( size->metrics.y_ppem * 64 * status.xbold_factor );
    ystr = (FT_Pos)( size->metrics.y_ppem * 64 * status.ybold_factor );

    have_topleft = 0;

    for ( i = offset; i < num_indices; i++ )
    {
      FT_UInt  glyph_idx;


      glyph_idx = FTDemo_Get_Index( handle, (FT_UInt32)i );

      error = FT_Load_Glyph( face, glyph_idx, handle->load_flags );
      if ( error )
        goto Next;

      /* this is essentially the code of function */
      /* `FT_GlyphSlot_Embolden'                  */

      if ( slot->format == FT_GLYPH_FORMAT_OUTLINE )
      {
        FT_Outline_Transform( &slot->outline, &shear );

        error = FT_Outline_EmboldenXY( &slot->outline, xstr, ystr );
        /* ignore error */
      }
      else if ( slot->format == FT_GLYPH_FORMAT_BITMAP )
      {
        /* round to full pixels */
        xstr &= ~63;
        ystr &= ~63;

        error = FT_GlyphSlot_Own_Bitmap( slot );
        if ( error )
          goto Next;

        error = FT_Bitmap_Embolden( slot->library, &slot->bitmap,
                                    xstr, ystr );
        if ( error )
          goto Next;
      }
      else
        goto Next;

      if ( slot->advance.x )
        slot->advance.x += xstr;

      if ( slot->advance.y )
        slot->advance.y += ystr;

      slot->metrics.width        += xstr;
      slot->metrics.height       += ystr;
      slot->metrics.horiAdvance  += xstr;
      slot->metrics.vertAdvance  += ystr;

      if ( slot->format == FT_GLYPH_FORMAT_BITMAP )
        slot->bitmap_top += ystr >> 6;

      width = slot->advance.x ? slot->advance.x >> 6
                              : size->metrics.y_ppem / 2;

      if ( X_TOO_LONG( x + width, display ) )
      {
        x  = start_x;
        y += step_y;

        if ( Y_TOO_LONG( y, display ) )
          break;
      }

      /* extra space between glyphs */
      x++;
      if ( slot->advance.x == 0 )
      {
        grFillRect( display->bitmap, x, y - width, width, width,
                    display->warn_color );
        x += width;
      }

      error = FTDemo_Draw_Slot( handle, display, slot, &x, &y );

      if ( error )
        goto Next;

      if ( !have_topleft )
      {
        have_topleft   = 1;
        status.topleft = i;
      }

      continue;

    Next:
      status.num_fails++;
    }

    return error;
  }


  static FT_Error
  Render_All( int  num_indices,
              int  offset )
  {
    int           start_x, start_y, step_y, x, y, width;
    int           i, have_topleft;
    FT_Size       size;
    FT_Face       face;
    FT_GlyphSlot  slot;
    FT_Color*     palette;


    error = FTDemo_Get_Size( handle, &size );
    if ( error )
    {
      /* probably a non-existent bitmap font size */
      return error;
    }

    INIT_SIZE( size, start_x, start_y, step_y, x, y );
    face = size->face;
    slot = face->glyph;

    error = FT_Palette_Select( face,
                               handle->current_font->palette_index,
                               &palette );
    if ( error )
      palette = NULL;

    have_topleft = 0;

    for ( i = offset; i < num_indices; i++ )
    {
      FT_LayerIterator  iterator;
      FT_UInt           glyph_idx;

      FT_Bool  have_layers;
      FT_UInt  layer_glyph_idx;
      FT_UInt  layer_color_idx;


      glyph_idx = FTDemo_Get_Index( handle, (FT_UInt32)i );

      /* check whether we have glyph color layers */
      iterator.p  = NULL;
      have_layers = FT_Get_Color_Glyph_Layer( face,
                                              glyph_idx,
                                              &layer_glyph_idx,
                                              &layer_color_idx,
                                              &iterator );

      if ( palette && have_layers && handle->use_layers )
      {
        FT_Int32  load_flags = handle->load_flags;

        FT_Bitmap  bitmap;
        FT_Vector  bitmap_offset = { 0, 0 };


        /*
         * We want to handle glyph layers manually, thus switching off
         * `FT_LOAD_COLOR' and ensuring normal AA render mode.
         */
        load_flags &= ~FT_LOAD_COLOR;
        load_flags |=  FT_LOAD_RENDER;

        load_flags &= ~FT_LOAD_TARGET_( 0xF );
        load_flags |=  FT_LOAD_TARGET_NORMAL;

        FT_Bitmap_Init( &bitmap );

        do
        {
          FT_Vector  slot_offset;


          error = FT_Load_Glyph( face, layer_glyph_idx, load_flags );
          if ( error )
            break;

          slot_offset.x = slot->bitmap_left * 64;
          slot_offset.y = slot->bitmap_top * 64;

          error = FT_Bitmap_Blend( handle->library,
                                   &slot->bitmap,
                                   slot_offset,
                                   &bitmap,
                                   &bitmap_offset,
                                   palette[layer_color_idx] );

        } while ( FT_Get_Color_Glyph_Layer( face,
                                            glyph_idx,
                                            &layer_glyph_idx,
                                            &layer_color_idx,
                                            &iterator ) );

        if ( error )
        {
          FT_Bitmap_Done( handle->library, &bitmap );
          goto Next;
        }
        else
        {
          FT_Bitmap_Done( handle->library, &slot->bitmap );

          slot->bitmap      = bitmap;
          slot->bitmap_left = bitmap_offset.x / 64;
          slot->bitmap_top  = bitmap_offset.y / 64;
        }
      }
      else
      {
        error = FT_Load_Glyph( face, glyph_idx, handle->load_flags );
        if ( error )
          goto Next;
      }

      width = slot->advance.x ? slot->advance.x >> 6
                              : size->metrics.y_ppem / 2;

      if ( X_TOO_LONG( x + width, display ) )
      {
        x  = start_x;
        y += step_y;

        if ( Y_TOO_LONG( y, display ) )
          break;
      }

      /* extra space between glyphs */
      x++;
      if ( slot->advance.x == 0 )
      {
        grFillRect( display->bitmap, x, y - width, width, width,
                    display->warn_color );
        x += width;
      }

      error = FTDemo_Draw_Slot( handle, display, slot, &x, &y );

      if ( error )
        goto Next;

      if ( !have_topleft )
      {
        have_topleft   = 1;
        status.topleft = i;
      }

      continue;

    Next:
      status.num_fails++;
    }

    return FT_Err_Ok;
  }


  static FT_Error
  Render_Text( int  num_indices,
               int  offset )
  {
    int      start_x, start_y, step_y, x, y;
    FT_Size  size;
    int      have_topleft;

    const char*  p;
    const char*  pEnd;
    int          ch;


    error = FTDemo_Get_Size( handle, &size );
    if ( error )
    {
      /* probably a non-existent bitmap font size */
      return error;
    }

    INIT_SIZE( size, start_x, start_y, step_y, x, y );

    p    = Text;
    pEnd = p + strlen( Text );

    while ( offset-- )
    {
      ch = utf8_next( &p, pEnd );
      if ( ch < 0 )
      {
        p  = Text;
        ch = utf8_next( &p, pEnd );
      }
    }

    have_topleft = 0;

    while ( num_indices-- )
    {
      FT_UInt  glyph_idx;


      ch = utf8_next( &p, pEnd );
      if ( ch < 0 )
      {
        p  = Text;
        ch = utf8_next( &p, pEnd );

        /* not a single character of the text string could be displayed */
        if ( !have_topleft )
          return error;
      }

      glyph_idx = FTDemo_Get_Index( handle, (FT_UInt32)ch );

      error = FTDemo_Draw_Index( handle, display, glyph_idx, &x, &y );

      if ( error )
        goto Next;

      if ( !have_topleft )
      {
        have_topleft   = 1;
        status.topleft = ch;
      }

      if ( X_TOO_LONG( x + ( size->metrics.max_advance >> 6 ), display ) )
      {
        x  = start_x;
        y += step_y;

        if ( Y_TOO_LONG( y, display ) )
          break;
      }

      continue;

    Next:
      status.num_fails++;
    }

    return FT_Err_Ok;
  }


  static FT_Error
  Render_Waterfall( int  mid_size,
                    int  offset )
  {
    int      start_x, start_y, step_y, x, y;
    int      pt_size, step, pt_height;
    FT_Size  size;
    int      have_topleft, start;

    char         text[256];
    const char*  p;
    const char*  pEnd;


    start_x = START_X;
    start_y = START_Y;

    have_topleft = 0;

    pt_height = 64 * 72 * display->bitmap->rows / status.res;
    step      = ( mid_size * mid_size / pt_height + 64 ) & ~63;
    pt_size   = mid_size - step * ( mid_size / step );  /* remainder */

    while ( 1 )
    {
      int first = offset;
      int ch;


      pt_size += step;

      FTDemo_Set_Current_Charsize( handle, pt_size, status.res );

      error = FTDemo_Get_Size( handle, &size );
      if ( error )
      {
        /* probably a non-existent bitmap font size */
        continue;
      }

      step_y = ( size->metrics.height >> 6 ) + 1;

      x = start_x;
      y = start_y + ( size->metrics.ascender >> 6 );

      start_y += step_y;

      if ( y >= display->bitmap->rows )
        break;

      p    = Text;
      pEnd = p + strlen( Text );

      while ( first-- )
      {
        ch = utf8_next( &p, pEnd );
        if ( ch < 0 )
        {
          p  = Text;
          ch = utf8_next( &p, pEnd );
        }
      }

      start = snprintf( text, 256, "%g: ", pt_size / 64.0 );
      snprintf( text + start, (unsigned int)( 256 - start ), "%s", p );

      p    = text;
      pEnd = p + strlen( text );

      while ( 1 )
      {
        FT_UInt      glyph_idx;
        const char*  oldp;


        oldp = p;
        ch   = utf8_next( &p, pEnd );
        if ( ch < 0 )
        {
          /* end of the text (or invalid UTF-8); continue to next size */
          break;
        }

        glyph_idx = FTDemo_Get_Index( handle, (FT_UInt32)ch );

        error = FTDemo_Draw_Index( handle, display, glyph_idx, &x, &y );

        if ( error )
          goto Next;

        /* `topleft' should be the first character after the size string */
        if ( oldp - text == start && !have_topleft )
        {
          have_topleft   = 1;
          status.topleft = ch;
        }

        if ( X_TOO_LONG( x + ( size->metrics.max_advance >> 6 ), display ) )
          break;

        continue;

      Next:
        status.num_fails++;
      }
    }

    FTDemo_Set_Current_Charsize( handle, mid_size, status.res );
    FTDemo_Get_Size( handle, &size );

    return FT_Err_Ok;
  }


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                REST OF THE APPLICATION/PROGRAM                *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/

  static void
  event_help( void )
  {
    char  buf[256];
    char  version[64];

    const char*  format;
    FT_Int       major, minor, patch;

    grEvent  dummy_event;


    FT_Library_Version( handle->library, &major, &minor, &patch );

    format = patch ? "%d.%d.%d" : "%d.%d";
    sprintf( version, format, major, minor, patch );

    FTDemo_Display_Clear( display );
    grSetLineHeight( 10 );
    grGotoxy( 0, 0 );
    grSetMargin( 2, 1 );
    grGotobitmap( display->bitmap );

    sprintf( buf,
             "FreeType Glyph Viewer - part of the FreeType %s test suite",
             version );

    grWriteln( buf );
    grLn();
    grWriteln( "Use the following keys:" );
    grLn();
    /*          |----------------------------------|    |----------------------------------| */
    grWriteln( "F1, ?       display this help screen                                        " );
    grWriteln( "                                                                            " );
    grWriteln( "render modes:                           anti-aliasing modes:                " );
    grWriteln( "  1         all glyphs                    A         monochrome              " );
    grWriteln( "  2         all glyphs fancy              B         normal                  " );
    grWriteln( "             (emboldened / slanted)       C         light                   " );
    grWriteln( "  3         all glyphs stroked            D         horizontal RGB (LCD)    " );
    grWriteln( "  4         text string                   E         horizontal BGR (LCD)    " );
    grWriteln( "  5         waterfall                     F         vertical RGB (LCD)      " );
    grWriteln( "  space     cycle forwards                G         vertical BGR (LCD)      " );
    grWriteln( "  backspace cycle backwards               k, l      cycle back and forth    " );
    grWriteln( "                                                                            " );
    grWriteln( "b           toggle embedded bitmaps     i, I        cycle through color     " );
    grWriteln( "                                                      color palette         " );
    grWriteln( "c           toggle coloured bitmaps     x, X        adjust horizontal       " );
    grWriteln( "z           toggle colour-layered                    emboldening (in mode 2)" );
    grWriteln( "              glyphs                    y, Y        adjust vertical         " );
    grWriteln( "                                                     emboldening (in mode 2)" );
    grWriteln( "K           toggle cache modes          s, S        adjust slanting         " );
    grWriteln( "                                                     (in mode 2)            " );
    grWriteln( "p, n        previous/next font          r, R        adjust stroking radius  " );
    grWriteln( "                                                     (in mode 3)            " );
    grWriteln( "Up, Down    adjust size by 1 unit                                           " );
    grWriteln( "PgUp, PgDn  adjust size by 10 units     L           cycle through           " );
    grWriteln( "                                                     LCD filtering          " );
    grWriteln( "Left, Right adjust index by 1           [, ]        select custom LCD       " );
    grWriteln( "F7, F8      adjust index by 16                        filter weight         " );
    grWriteln( "F9, F10     adjust index by 256                       (if custom filtering) " );
    grWriteln( "F11, F12    adjust index by 4096        -, +(=)     adjust selected custom  " );
    grWriteln( "                                                     LCD filter weight      " );
    grWriteln( "h           toggle hinting                                                  " );
    grWriteln( "H           cycle through hinting       g, v        adjust gamma value      " );
    grWriteln( "             engines (if available)                                         " );
    grWriteln( "f           toggle forced auto-         Tab         cycle through charmaps  " );
    grWriteln( "             hinting (if hinting)                                           " );
    grWriteln( "w           toggle warping                                                  " );
    grWriteln( "             (if available)             q, ESC      quit ftview             " );
    /*          |----------------------------------|    |----------------------------------| */
    grLn();
    grLn();
    grWriteln( "press any key to exit this help screen" );

    grRefreshSurface( display->surface );
    grListenSurface( display->surface, gr_event_key, &dummy_event );
  }


  static int
  event_tt_interpreter_version_change( void )
  {
    status.tt_interpreter_version_idx += 1;
    status.tt_interpreter_version_idx %= status.num_tt_interpreter_versions;

    error = FT_Property_Set( handle->library,
                             "truetype",
                             "interpreter-version",
                             &status.tt_interpreter_versions[
                               status.tt_interpreter_version_idx] );

    if ( !error )
      return 1;

    return 0;
  }


  static int
  event_warping_change( void )
  {
    if ( handle->lcd_mode == LCD_MODE_AA && handle->autohint )
    {
      FT_Bool  new_warping_state = !status.warping;


      error = FT_Property_Set( handle->library,
                               "autofitter",
                               "warping",
                               &new_warping_state );

      if ( !error )
      {
        /* Resetting the cache is perhaps a bit harsh, but I'm too  */
        /* lazy to walk over all loaded fonts to check whether they */
        /* are auto-hinted, then unloading them explicitly.         */
        FTC_Manager_Reset( handle->cache_manager );
        status.warping = new_warping_state;
        return 1;
      }
    }

    return 0;
  }


  static void
  event_gamma_change( double  delta )
  {
    display->gamma += delta;

    if ( display->gamma > 3.0 )
      display->gamma = 3.0;
    else if ( display->gamma < 0.0 )
      display->gamma = 0.0;

    grSetGlyphGamma( display->gamma );
  }


  static int
  event_bold_change( double  xdelta,
                     double  ydelta )
  {
    double  old_xbold_factor = status.xbold_factor;
    double  old_ybold_factor = status.ybold_factor;


    status.xbold_factor += xdelta;
    status.ybold_factor += ydelta;

    if ( status.xbold_factor > 0.1 )
      status.xbold_factor = 0.1;
    else if ( status.xbold_factor < -0.1 )
      status.xbold_factor = -0.1;

    if ( status.ybold_factor > 0.1 )
      status.ybold_factor = 0.1;
    else if ( status.ybold_factor < -0.1 )
      status.ybold_factor = -0.1;

    return ( old_xbold_factor == status.xbold_factor &&
             old_ybold_factor == status.ybold_factor ) ? 0 : 1;
  }


  static int
  event_radius_change( double  delta )
  {
    double  old_radius = status.radius;


    status.radius += delta;

    if ( status.radius > 0.05 )
      status.radius = 0.05;
    else if ( status.radius < 0.0 )
      status.radius = 0.0;

    return old_radius == status.radius ? 0 : 1;
  }


  static int
  event_slant_change( double  delta )
  {
    double  old_slant = status.slant;


    status.slant += delta;

    if ( status.slant > 1.0 )
      status.slant = 1.0;
    else if ( status.slant < -1.0 )
      status.slant = -1.0;

    return old_slant == status.slant ? 0 : 1;
  }


  static int
  event_size_change( int  delta )
  {
    int  old_ptsize = status.ptsize;


    status.ptsize += delta;

    if ( status.ptsize < 64 * 1 )
      status.ptsize = 1 * 64;
    else if ( status.ptsize > MAXPTSIZE * 64 )
      status.ptsize = MAXPTSIZE * 64;

    FTDemo_Set_Current_Charsize( handle, status.ptsize, status.res );

    return old_ptsize == status.ptsize ? 0 : 1;
  }


  static int
  event_index_change( int  delta )
  {
    int  old_offset  = status.offset;
    int  num_indices = handle->current_font->num_indices;


    status.offset += delta;

    if ( status.offset < 0 )
      status.offset = 0;
    else if ( status.offset >= num_indices )
      status.offset = num_indices - 1;

    return old_offset == status.offset ? 0 : 1;
  }


  static void
  event_render_mode_change( int  delta )
  {
    if ( delta )
      status.render_mode = ( status.render_mode +
                             delta              +
                             N_RENDER_MODES     ) % N_RENDER_MODES;
  }


  static int
  event_encoding_change( void )
  {
    PFont    font = handle->current_font;


    if ( handle->encoding != FT_ENCODING_ORDER )
      font->cmap_index++;
    else
      font->cmap_index = 0;

    FTDemo_Set_Current_Font( handle, font );

    status.offset = handle->encoding == FT_ENCODING_ORDER ? 0 : 0x20;

    return 1;
  }


  static int
  event_font_change( int  delta )
  {
    int  num_indices;


    if ( status.font_idx + delta >= handle->num_fonts ||
         status.font_idx + delta < 0                  )
      return 0;

    status.font_idx += delta;

    FTDemo_Set_Current_Font( handle, handle->fonts[status.font_idx] );
    FTDemo_Set_Current_Charsize( handle, status.ptsize, status.res );
    FTDemo_Update_Current_Flags( handle );

    num_indices = handle->current_font->num_indices;

    if ( status.offset >= num_indices )
      status.offset = num_indices - 1;

    return 1;
  }


  static int
  event_palette_change( int  delta )
  {
    FT_Size  size;
    FT_Face  face;

    FT_Palette_Data  palette;

    int  palette_index     = handle->current_font->palette_index;
    int  old_palette_index = palette_index;


    error = FTDemo_Get_Size( handle, &size );
    if ( error )
    {
      /* probably a non-existent bitmap font size */
      return 0;
    }

    face = size->face;

    error = FT_Palette_Data_Get( face, &palette );
    if ( error || !palette.num_palettes )
      return 0;

    palette_index += delta;

    if ( palette_index < 0 )
      palette_index = palette.num_palettes - 1;
    else if ( palette_index >= palette.num_palettes )
      palette_index = 0;

    handle->current_font->palette_index = palette_index;

    return old_palette_index == palette_index ? 0 : 1;
  }


  static int
  Process_Event( grEvent*  event )
  {
    int  ret = 0;


    status.update = 0;

    if ( status.render_mode == (int)( event->key - '1' ) )
      return ret;
    if ( event->key >= '1' && event->key < '1' + N_RENDER_MODES )
    {
      status.render_mode = (int)( event->key - '1' );
      event_render_mode_change( 0 );
      status.update = 1;
      return ret;
    }

    if ( event->key >= 'A'             &&
         event->key < 'A' + N_LCD_IDXS )
    {
      int  lcd_idx = (int)( event->key - 'A' );


      if ( status.lcd_idx == lcd_idx )
        return ret;

      handle->lcd_mode = lcd_modes[lcd_idx];
      FTDemo_Update_Current_Flags( handle );
      status.update  = 1;
      status.lcd_idx = lcd_idx;
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
      event_help();
      status.update = 1;
      break;

    case grKEY( 'b' ):
      handle->use_sbits = !handle->use_sbits;
      FTDemo_Update_Current_Flags( handle );
      status.update = 1;
      break;

    case grKEY( 'c' ):
      handle->use_color = !handle->use_color;
      FTDemo_Update_Current_Flags( handle );
      status.update = 1;
      break;

    case grKEY( 'z' ):
      handle->use_layers = !handle->use_layers;
      FTDemo_Update_Current_Flags( handle );
      status.update = 1;
      break;

    case grKEY( 'i' ):
      status.update = event_palette_change( 1 );
      break;

    case grKEY( 'I' ):
      status.update = event_palette_change( -1 );
      break;

    case grKEY( 'K' ):
      handle->use_sbits_cache = !handle->use_sbits_cache;
      status.update = 1;
      break;

    case grKEY( 'f' ):
      if ( handle->hinted )
      {
        handle->autohint = !handle->autohint;
        FTDemo_Update_Current_Flags( handle );
        status.update = 1;
      }
      break;

    case grKEY( 'h' ):
      handle->hinted = !handle->hinted;
      FTDemo_Update_Current_Flags( handle );
      status.update = 1;
      break;

    case grKEY( 'H' ):
      if ( !handle->autohint                  &&
           handle->lcd_mode != LCD_MODE_LIGHT )
      {
        FT_Face    face;
        FT_Module  module;


        error = FTC_Manager_LookupFace( handle->cache_manager,
                                        handle->scaler.face_id, &face );
        if ( !error )
        {
          module = &face->driver->root;

          if ( !strcmp( module->clazz->module_name, "cff" ) )
            status.update = FTDemo_Event_Cff_Hinting_Engine_Change(
                              handle->library,
                              &status.cff_hinting_engine,
                              1 );
          else if ( !strcmp( module->clazz->module_name, "type1" ) )
            status.update = FTDemo_Event_Type1_Hinting_Engine_Change(
                              handle->library,
                              &status.type1_hinting_engine,
                              1 );
          else if ( !strcmp( module->clazz->module_name, "t1cid" ) )
            status.update = FTDemo_Event_T1cid_Hinting_Engine_Change(
                              handle->library,
                              &status.t1cid_hinting_engine,
                              1 );
          else if ( !strcmp( module->clazz->module_name, "truetype" ) )
            status.update = event_tt_interpreter_version_change();

          if ( status.update )
          {
            /* Resetting the cache is perhaps a bit harsh, but I'm too  */
            /* lazy to walk over all loaded fonts to check whether they */
            /* are of type TTF, then unloading them explicitly.         */
            FTC_Manager_Reset( handle->cache_manager );
          }
        }
      }
      break;

    case grKEY( 'l' ):
    case grKEY( 'k' ):
      status.lcd_idx =
        ( status.lcd_idx                          +
          ( event->key == grKEY( 'l' ) ? 1 : -1 ) +
          N_LCD_IDXS                              ) % N_LCD_IDXS;

      handle->lcd_mode = lcd_modes[status.lcd_idx];
      FTDemo_Update_Current_Flags( handle );
      status.update = 1;
      break;

    case grKEY( 'w' ):
      status.update = event_warping_change();
      break;

    case grKeySpace:
      event_render_mode_change( 1 );
      status.update = 1;
      break;

    case grKeyBackSpace:
      event_render_mode_change( -1 );
      status.update = 1;
      break;

    case grKeyTab:
      status.update = event_encoding_change();
      break;

    case grKEY( 's' ):
      if ( status.render_mode == RENDER_MODE_FANCY )
        status.update = event_slant_change( 0.02 );
      break;

    case grKEY( 'S' ):
      if ( status.render_mode == RENDER_MODE_FANCY )
        status.update = event_slant_change( -0.02 );
      break;

    case grKEY( 'r' ):
      if ( status.render_mode == RENDER_MODE_STROKE )
        status.update = event_radius_change( 0.005 );
      break;

    case grKEY( 'R' ):
      if ( status.render_mode == RENDER_MODE_STROKE )
        status.update = event_radius_change( -0.005 );
      break;

    case grKEY( 'x' ):
      if ( status.render_mode == RENDER_MODE_FANCY )
        status.update = event_bold_change( 0.005, 0.0 );
      break;

    case grKEY( 'X' ):
      if ( status.render_mode == RENDER_MODE_FANCY )
        status.update = event_bold_change( -0.005, 0.0 );
      break;

    case grKEY( 'y' ):
      if ( status.render_mode == RENDER_MODE_FANCY )
        status.update = event_bold_change( 0.0, 0.005 );
      break;

    case grKEY( 'Y' ):
      if ( status.render_mode == RENDER_MODE_FANCY )
        status.update = event_bold_change( 0.0, -0.005 );
      break;

    case grKEY( 'g' ):
      event_gamma_change( 0.1 );
      status.update = 1;
      break;

    case grKEY( 'v' ):
      event_gamma_change( -0.1 );
      status.update = 1;
      break;

    case grKEY( 'n' ):
      status.update = event_font_change( 1 );
      break;

    case grKEY( 'p' ):
      status.update = event_font_change( -1 );
      break;

    case grKeyUp:
      status.update = event_size_change( 64 );
      break;
    case grKeyDown:
      status.update = event_size_change( -64 );
      break;
    case grKeyPageUp:
      status.update = event_size_change( 640 );
      break;
    case grKeyPageDown:
      status.update = event_size_change( -640 );
      break;

    case grKeyLeft:
      status.update = event_index_change( -1 );
      break;
    case grKeyRight:
      status.update = event_index_change( 1 );
      break;
    case grKeyF7:
      status.update = event_index_change( -0x10 );
      break;
    case grKeyF8:
      status.update = event_index_change( 0x10 );
      break;
    case grKeyF9:
      status.update = event_index_change( -0x100 );
      break;
    case grKeyF10:
      status.update = event_index_change( 0x100 );
      break;
    case grKeyF11:
      status.update = event_index_change( -0x1000 );
      break;
    case grKeyF12:
      status.update = event_index_change( 0x1000 );
      break;

    default:
      break;
    }

    if ( FT_Library_SetLcdFilterWeights( NULL, NULL ) ==
                         FT_Err_Unimplemented_Feature    ||
         handle->lcd_mode < LCD_MODE_RGB                 )
      return ret;

    switch ( event->key )
    {
    case grKEY( 'L' ):
      FTC_Manager_RemoveFaceID( handle->cache_manager,
                                handle->scaler.face_id );

      status.lcd_filter++;
      switch ( status.lcd_filter )
      {
      case FT_LCD_FILTER_NONE:
      case FT_LCD_FILTER_DEFAULT:
      case FT_LCD_FILTER_LIGHT:
      case FT_LCD_FILTER_LEGACY1:
        FT_Library_SetLcdFilter( handle->library,
                                 (FT_LcdFilter)status.lcd_filter );
        break;
      default:
        FT_Library_SetLcdFilterWeights( handle->library,
                                        status.filter_weights );
        status.lcd_filter = -1;
      }

      status.update = 1;
      break;

    case grKEY( '[' ):
      if ( status.lcd_filter < 0 )
      {
        status.fw_idx--;
        if ( status.fw_idx < 0 )
          status.fw_idx = 4;
        status.update = 1;
      }
      break;

    case grKEY( ']' ):
      if ( status.lcd_filter < 0 )
      {
        status.fw_idx++;
        if ( status.fw_idx > 4 )
          status.fw_idx = 0;
        status.update = 1;
      }
      break;

    case grKEY( '-' ):
      if ( status.lcd_filter < 0 )
      {
        FTC_Manager_RemoveFaceID( handle->cache_manager,
                                  handle->scaler.face_id );

        status.filter_weights[status.fw_idx]--;
        FT_Library_SetLcdFilterWeights( handle->library,
                                        status.filter_weights );
        status.update = 1;
      }
      break;

    case grKEY( '+' ):
    case grKEY( '=' ):
      if ( status.lcd_filter < 0 )
      {
        FTC_Manager_RemoveFaceID( handle->cache_manager,
                                  handle->scaler.face_id );

        status.filter_weights[status.fw_idx]++;
        FT_Library_SetLcdFilterWeights( handle->library,
                                        status.filter_weights );
        status.update = 1;
      }
      break;

    default:
      break;
    }

    return ret;
  }


  static void
  write_header( FT_Error  error_code )
  {
    char  buf[256];
    int   line = 4;

    FT_Face  face;


    FTC_Manager_LookupFace( handle->cache_manager,
                            handle->scaler.face_id, &face );

    FTDemo_Draw_Header( handle, display, status.ptsize, status.res,
                        status.render_mode != RENDER_MODE_TEXT      &&
                        status.render_mode != RENDER_MODE_WATERFALL ?
                        status.topleft : -1, error_code );

    /* render mode */
    {
      const char*  render_mode = NULL;


      switch ( status.render_mode )
      {
      case RENDER_MODE_ALL:
        render_mode = "all glyphs";
        break;
      case RENDER_MODE_FANCY:
        render_mode = "fancy";
        break;
      case RENDER_MODE_STROKE:
        render_mode = "stroked";
        break;
      case RENDER_MODE_TEXT:
        render_mode = "text string";
        break;
      case RENDER_MODE_WATERFALL:
        render_mode = "waterfall";
        break;
      }
      sprintf( buf, "%d: %s",
                    status.render_mode + 1,
                    render_mode );
      grWriteCellString( display->bitmap, 0, (line++) * HEADER_HEIGHT,
                         buf, display->fore_color );
    }

    if ( status.render_mode == RENDER_MODE_FANCY )
    {
      /* x emboldening */
      sprintf( buf, " x: % .3f",
                    status.xbold_factor );
      grWriteCellString( display->bitmap, 0, (line++) * HEADER_HEIGHT,
                         buf, display->fore_color );

      /* y emboldening */
      sprintf( buf, " y: % .3f",
                    status.ybold_factor );
      grWriteCellString( display->bitmap, 0, (line++) * HEADER_HEIGHT,
                         buf, display->fore_color );

      /* slanting */
      sprintf( buf, " s: % .3f",
                    status.slant );
      grWriteCellString( display->bitmap, 0, (line++) * HEADER_HEIGHT,
                         buf, display->fore_color );
    }

    if ( status.render_mode == RENDER_MODE_STROKE )
    {
      /* stroking radius */
      sprintf( buf, " radius: %.3f",
                    status.radius );
      grWriteCellString( display->bitmap, 0, (line++) * HEADER_HEIGHT,
                         buf, display->fore_color );
    }

    line++;

    /* anti-aliasing */
    {
      const char*  lcd_mode;


      switch ( handle->lcd_mode )
      {
      case LCD_MODE_AA:
        lcd_mode = "normal AA";
        break;
      case LCD_MODE_LIGHT:
        lcd_mode = "light AA";
        break;
      case LCD_MODE_RGB:
        lcd_mode = "LCD (horiz. RGB)";
        break;
      case LCD_MODE_BGR:
        lcd_mode = "LCD (horiz. BGR)";
        break;
      case LCD_MODE_VRGB:
        lcd_mode = "LCD (vert. RGB)";
        break;
      case LCD_MODE_VBGR:
        lcd_mode = "LCD (vert. BGR)";
        break;
      default:
        handle->lcd_mode = 0;
        lcd_mode = "monochrome";
      }
      grWriteCellString( display->bitmap, 0, (line++) * HEADER_HEIGHT,
                         lcd_mode, display->fore_color );
    }

    /* hinting */
    sprintf( buf, "hinting: %s",
                  handle->hinted ? "on" : "off" );
    grWriteCellString( display->bitmap, 0, (line++) * HEADER_HEIGHT,
                       buf, display->fore_color );

    if ( handle->hinted )
    {
      /* auto-hinting */
      sprintf( buf, " forced auto: %s",
                    ( handle->autohint                   ||
                      handle->lcd_mode == LCD_MODE_LIGHT ) ? "on" : "off" );
      grWriteCellString( display->bitmap, 0, (line++) * HEADER_HEIGHT,
                         buf, display->fore_color );
    }

    if ( !handle->autohint                  &&
         handle->lcd_mode != LCD_MODE_LIGHT )
    {
      /* hinting engine */
      FT_Module    module;
      const char*  hinting_engine = NULL;


      module = &face->driver->root;

      if ( !strcmp( module->clazz->module_name, "cff" ) )
      {
        switch ( status.cff_hinting_engine )
        {
        case FT_HINTING_FREETYPE:
          hinting_engine = "FreeType";
          break;
        case FT_HINTING_ADOBE:
          hinting_engine = "Adobe";
          break;
        }
      }

      else if ( !strcmp( module->clazz->module_name, "type1" ) )
      {
        switch ( status.type1_hinting_engine )
        {
        case FT_HINTING_FREETYPE:
          hinting_engine = "FreeType";
          break;
        case FT_HINTING_ADOBE:
          hinting_engine = "Adobe";
          break;
        }
      }

      else if ( !strcmp( module->clazz->module_name, "t1cid" ) )
      {
        switch ( status.t1cid_hinting_engine )
        {
        case FT_HINTING_FREETYPE:
          hinting_engine = "FreeType";
          break;
        case FT_HINTING_ADOBE:
          hinting_engine = "Adobe";
          break;
        }
      }

      else if ( !strcmp( module->clazz->module_name, "truetype" ) )
      {
        switch ( status.tt_interpreter_versions[
                   status.tt_interpreter_version_idx] )
        {
        case TT_INTERPRETER_VERSION_35:
          hinting_engine = "v35";
          break;
        case TT_INTERPRETER_VERSION_38:
          hinting_engine = "v38";
          break;
        case TT_INTERPRETER_VERSION_40:
          hinting_engine = "v40";
          break;
        }
      }

      if ( hinting_engine )
      {
        sprintf( buf, "engine: %s",
                      hinting_engine );
        grWriteCellString( display->bitmap, 0, (line++) * HEADER_HEIGHT,
                           buf, display->fore_color );
      }
    }

    if ( handle->lcd_mode == LCD_MODE_AA && handle->autohint )
    {
      sprintf( buf, "warping: %s",
                    status.warping ? "on" : "off" );
      grWriteCellString( display->bitmap, 0, (line++) * HEADER_HEIGHT,
                         buf, display->fore_color );
    }

    line++;

    /* embedded bitmaps */
    sprintf( buf, "bitmaps: %s",
                  handle->use_sbits ? "on" : "off" );
    grWriteCellString( display->bitmap, 0, (line++) * HEADER_HEIGHT,
                       buf, display->fore_color );

    if ( FT_HAS_COLOR( face ) )
    {
      sprintf( buf, "color:" );
      grWriteCellString( display->bitmap, 0, (line++) * HEADER_HEIGHT,
                         buf, display->fore_color );

      /* color bitmaps */
      sprintf( buf, "  bitmaps: %s",
                    handle->use_color ? "on" : "off" );
      grWriteCellString( display->bitmap, 0, (line++) * HEADER_HEIGHT,
                         buf, display->fore_color );

      /* color-layered glyphs */
      sprintf( buf, "  outlines: %s",
                    handle->use_layers ? "on" : "off" );
      grWriteCellString( display->bitmap, 0, (line++) * HEADER_HEIGHT,
                         buf, display->fore_color );

      /* color palette */
      sprintf( buf, "  palette idx: %d",
                    handle->current_font->palette_index );
      grWriteCellString( display->bitmap, 0, (line++) * HEADER_HEIGHT,
                         buf, display->fore_color );
    }

    /* cache */
    sprintf( buf, "cache: %s",
                  handle->use_sbits_cache ? "on" : "off" );
    grWriteCellString( display->bitmap, 0, (line++) * HEADER_HEIGHT,
                       buf, display->fore_color );

    line++;

    /* LCD filtering */
    if ( FT_Library_SetLcdFilterWeights( NULL, NULL ) !=
                         FT_Err_Unimplemented_Feature    &&
         handle->lcd_mode >= LCD_MODE_RGB                )
    {
      sprintf( buf, "filter: %s",
                    status.lcd_filter == 0 ? "none" :
                    status.lcd_filter == 1 ? "default" :
                    status.lcd_filter == 2 ? "light" :
                    status.lcd_filter == 3 ? "legacy" : "custom" );
      grWriteCellString( display->bitmap, 0, (line++) * HEADER_HEIGHT,
                         buf, display->fore_color );

      /* custom LCD filter settings */
      if ( status.lcd_filter < 0 )
      {
        int             fwi = status.fw_idx;
        unsigned char*  fw  = status.filter_weights;
        int             i;


        for ( i = 0; i < 5; i++ )
        {
          sprintf( buf,
                   " %s0x%02X%s",
                   fwi == i ? "[" : " ",
                   fw[i],
                   fwi == i ? "]" : " " );
          grWriteCellString( display->bitmap, 0, (line++) * HEADER_HEIGHT,
                             buf, display->fore_color );
        }
      }
    }

    grRefreshSurface( display->surface );
  }


  static void
  usage( char*  execname )
  {
    fprintf( stderr,
      "\n"
      "ftview: simple glyph viewer -- part of the FreeType project\n"
      "-----------------------------------------------------------\n"
      "\n" );
    fprintf( stderr,
      "Usage: %s [options] pt font ...\n"
      "\n",
             execname );
    fprintf( stderr,
      "  pt        The point size for the given resolution.\n"
      "            If resolution is 72dpi, this directly gives the\n"
      "            ppem value (pixels per EM).\n" );
    fprintf( stderr,
      "  font      The font file(s) to display.\n"
      "            For Type 1 font files, ftview also tries to attach\n"
      "            the corresponding metrics file (with extension\n"
      "            `.afm' or `.pfm').\n"
      "\n" );
    fprintf( stderr,
      "  -d WxHxD  Set the window width, height, and color depth\n"
      "            (default: 640x480x24).\n"
      "  -r R      Use resolution R dpi (default: 72dpi).\n"
      "  -f index  Specify first index to display (default: 0).\n"
      "  -e enc    Specify encoding tag (default: no encoding).\n"
      "            Common values: `unic' (Unicode), `symb' (symbol),\n"
      "            `ADOB' (Adobe standard), `ADBC' (Adobe custom).\n"
      "  -m text   Use `text' for rendering.\n" );
    fprintf( stderr,
      "  -l mode   Set start-up rendering mode (0 <= mode <= %d).\n",
             N_LCD_IDXS - 1 );
    fprintf( stderr,
      "  -L N,...  Set LCD filter or geometry by comma-separated values.\n"
      "  -p        Preload file in memory to simulate memory-mapping.\n"
      "\n"
      "  -v        Show version.\n"
      "\n" );

    exit( 1 );
  }


  static void
  parse_cmdline( int*    argc,
                 char**  argv[] )
  {
    char*  execname;
    int    option;


    execname = ft_basename( (*argv)[0] );

    while ( 1 )
    {
      option = getopt( *argc, *argv, "d:e:f:L:l:m:pr:v" );

      if ( option == -1 )
        break;

      switch ( option )
      {
      case 'd':
        status.dims = optarg;
        break;

      case 'e':
        handle->encoding = FTDemo_Make_Encoding_Tag( optarg );
        break;

      case 'f':
        status.offset = atoi( optarg );
        break;

      case 'l':
        status.lcd_idx = atoi( optarg );
        if ( status.lcd_idx < 0 || status.lcd_idx >= N_LCD_IDXS )
        {
          fprintf( stderr, "argument to `l' must be in the range [0;%d]\n",
                   N_LCD_IDXS - 1 );
          exit( 3 );
        }
        handle->lcd_mode = lcd_modes[status.lcd_idx];
        break;

      case 'L':
        {
          int i, buf[6];


          i = sscanf( optarg, "%d,%d,%d,%d,%d,%d",
                      buf, buf + 1, buf + 2, buf + 3, buf + 4, buf + 5 );
          if ( FT_Library_SetLcdFilterWeights( NULL, NULL ) !=
                               FT_Err_Unimplemented_Feature    &&
               i == 5                                          )
          {
            status.filter_weights[0] = (unsigned char)buf[0];
            status.filter_weights[1] = (unsigned char)buf[1];
            status.filter_weights[2] = (unsigned char)buf[2];
            status.filter_weights[3] = (unsigned char)buf[3];
            status.filter_weights[4] = (unsigned char)buf[4];

            FT_Library_SetLcdFilterWeights( handle->library,
                                            status.filter_weights );

            status.lcd_filter = -1;
          }
          else if ( FT_Library_SetLcdGeometry( NULL, NULL ) !=
                               FT_Err_Unimplemented_Feature    &&
                    i == 6                                     )
          {
            FT_Vector  sub[3] = { { buf[0], buf[1] },
                                  { buf[2], buf[3] },
                                  { buf[4], buf[5] } };


            FT_Library_SetLcdGeometry( handle->library, sub );
          }
        }
        break;

      case 'm':
        Text               = optarg;
        status.render_mode = RENDER_MODE_TEXT;
        break;

      case 'p':
        status.preload = 1;
        break;

      case 'r':
        status.res = atoi( optarg );
        if ( status.res < 1 )
          usage( execname );
        break;

      case 'v':
        {
          FT_Int  major, minor, patch;


          FT_Library_Version( handle->library, &major, &minor, &patch );

          printf( "ftview (FreeType) %d.%d", major, minor );
          if ( patch )
            printf( ".%d", patch );
          printf( "\n" );
          exit( 0 );
        }
        /* break; */

      default:
        usage( execname );
        break;
      }
    }

    *argc -= optind;
    *argv += optind;

    if ( *argc <= 1 )
      usage( execname );

    status.ptsize = (int)( atof( *argv[0] ) * 64.0 );
    if ( status.ptsize == 0 )
      status.ptsize = 64 * 10;

    (*argc)--;
    (*argv)++;
  }


  int
  main( int    argc,
        char*  argv[] )
  {
    grEvent  event;
    unsigned int  dflt_tt_interpreter_version;
    int           i;
    unsigned int  versions[3] = { TT_INTERPRETER_VERSION_35,
                                  TT_INTERPRETER_VERSION_38,
                                  TT_INTERPRETER_VERSION_40 };


    /* Initialize engine */
    handle = FTDemo_New();

    parse_cmdline( &argc, &argv );

    if ( status.lcd_filter != -1 )
      FT_Library_SetLcdFilter( handle->library,
                               (FT_LcdFilter)status.lcd_filter );

    /* get the default values as compiled into FreeType */
    FT_Property_Get( handle->library,
                     "cff",
                     "hinting-engine", &status.cff_hinting_engine );
    FT_Property_Get( handle->library,
                     "type1",
                     "hinting-engine", &status.type1_hinting_engine );
    FT_Property_Get( handle->library,
                     "t1cid",
                     "hinting-engine", &status.t1cid_hinting_engine );

    /* collect all available versions, then set again the default */
    FT_Property_Get( handle->library,
                     "truetype",
                     "interpreter-version", &dflt_tt_interpreter_version );
    for ( i = 0; i < 3; i++ )
    {
      error = FT_Property_Set( handle->library,
                               "truetype",
                               "interpreter-version", &versions[i] );
      if ( !error )
        status.tt_interpreter_versions[
          status.num_tt_interpreter_versions++] = versions[i];
      if ( versions[i] == dflt_tt_interpreter_version )
        status.tt_interpreter_version_idx = i;
    }
    FT_Property_Set( handle->library,
                     "truetype",
                     "interpreter-version", &dflt_tt_interpreter_version );

    FT_Property_Get( handle->library,
                     "autofitter",
                     "warping", &status.warping );

    if ( status.preload )
      FTDemo_Set_Preload( handle, 1 );

    for ( ; argc > 0; argc--, argv++ )
      FTDemo_Install_Font( handle, argv[0], 0, 0 );

    if ( handle->num_fonts == 0 )
      Fatal( "could not find/open any font file" );

    display = FTDemo_Display_New( status.dims );
    if ( !display )
      Fatal( "could not allocate display surface" );

    grSetTitle( display->surface,
                "FreeType Glyph Viewer - press ? for help" );

    status.num_fails = 0;

    event_font_change( 0 );

    do
    {
      if ( !status.update )
        goto Listen;

      FTDemo_Display_Clear( display );

      switch ( status.render_mode )
      {
      case RENDER_MODE_ALL:
        error = Render_All( handle->current_font->num_indices,
                            status.offset );
        break;

      case RENDER_MODE_FANCY:
        error = Render_Fancy( handle->current_font->num_indices,
                              status.offset );
        break;

      case RENDER_MODE_STROKE:
        error = Render_Stroke( handle->current_font->num_indices,
                               status.offset );
        break;

      case RENDER_MODE_TEXT:
        error = Render_Text( -1, status.offset );
        break;

      case RENDER_MODE_WATERFALL:
        error = Render_Waterfall( status.ptsize, status.offset );
        break;
      }

      write_header( error );

    Listen:
      grListenSurface( display->surface, 0, &event );
    } while ( Process_Event( &event ) == 0 );

    printf( "Execution completed successfully.\n" );
    printf( "Fails = %d\n", status.num_fails );

    FTDemo_Display_Done( display );
    FTDemo_Done( handle );
    exit( 0 );      /* for safety reasons */

    /* return 0; */ /* never reached */
  }


/* End */
