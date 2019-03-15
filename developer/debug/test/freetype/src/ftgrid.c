/****************************************************************************/
/*                                                                          */
/*  The FreeType project -- a free and portable quality TrueType renderer.  */
/*                                                                          */
/*  Copyright 1996-2018 by                                                  */
/*  D. Turner, R.Wilhelm, and W. Lemberg                                    */
/*                                                                          */
/*                                                                          */
/*  FTGrid - a simple viewer to show glyph outlines on a grid               */
/*                                                                          */
/*  Press ? when running this program to have a list of key-bindings        */
/*                                                                          */
/****************************************************************************/


#include "ftcommon.h"
#include "common.h"
#include "output.h"
#include "mlgetopt.h"
#include <stdlib.h>

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
#include FT_MULTIPLE_MASTERS_H
#include FT_SFNT_NAMES_H
#include FT_TRUETYPE_IDS_H
#include FT_TRIGONOMETRY_H

#define MAXPTSIZE    500                 /* dtp */
#define MAX_MM_AXES   32

#ifdef _WIN32
#define snprintf  _snprintf
#endif


#ifdef FT_DEBUG_AUTOFIT
  /* these variables, structures, and declarations are for  */
  /* communication with the debugger in the autofit module; */
  /* normal programs don't need this                        */
  struct  AF_GlyphHintsRec_;
  typedef struct AF_GlyphHintsRec_*  AF_GlyphHints;

  extern int            _af_debug_disable_horz_hints;
  extern int            _af_debug_disable_vert_hints;
  extern int            _af_debug_disable_blue_hints;
  extern AF_GlyphHints  _af_debug_hints;

#ifdef __cplusplus
  extern "C" {
#endif
  extern void
  af_glyph_hints_dump_segments( AF_GlyphHints  hints,
                                FT_Bool        to_stdout );
  extern void
  af_glyph_hints_dump_points( AF_GlyphHints  hints,
                              FT_Bool        to_stdout );
  extern void
  af_glyph_hints_dump_edges( AF_GlyphHints  hints,
                             FT_Bool        to_stdout );
  extern FT_Error
  af_glyph_hints_get_num_segments( AF_GlyphHints  hints,
                                   FT_Int         dimension,
                                   FT_Int*        num_segments );
  extern FT_Error
  af_glyph_hints_get_segment_offset( AF_GlyphHints  hints,
                                     FT_Int         dimension,
                                     FT_Int         idx,
                                     FT_Pos        *offset,
                                     FT_Bool       *is_blue,
                                     FT_Pos        *blue_offset );
#ifdef __cplusplus
  }
#endif

#endif /* FT_DEBUG_AUTOFIT */


#define BUFSIZE  256

#define DO_BITMAP      1
#define DO_OUTLINE     2
#define DO_DOTS        4
#define DO_DOTNUMBERS  8

  typedef struct  GridStatusRec_
  {
    int          width;
    int          height;

    int          ptsize;
    int          res;
    int          Num;  /* glyph index */
    int          font_index;

    int          scale;
    int          x_origin;
    int          y_origin;

    int          scale_0;
    int          x_origin_0;
    int          y_origin_0;

    int          disp_width;
    int          disp_height;
    grBitmap*    disp_bitmap;

    grColor      axis_color;
    grColor      grid_color;
    grColor      outline_color;
    grColor      on_color;
    grColor      off_color;
    grColor      segment_color;
    grColor      blue_color;

    int          work;
    int          do_horz_hints;
    int          do_vert_hints;
    int          do_blue_hints;
    int          do_segment;
    int          do_grid;
    int          do_alt_colors;

    FT_LcdFilter lcd_filter;
    const char*  header;
    char         header_buffer[BUFSIZE];

    FT_Stroker   stroker;

    unsigned int cff_hinting_engine;
    unsigned int type1_hinting_engine;
    unsigned int t1cid_hinting_engine;
    unsigned int tt_interpreter_versions[3];
    int          num_tt_interpreter_versions;
    int          tt_interpreter_version_idx;
    FT_Bool      warping;

    FT_MM_Var*   mm;
    char*        axis_name[MAX_MM_AXES];
    FT_Fixed     design_pos[MAX_MM_AXES];
    FT_Fixed     requested_pos[MAX_MM_AXES];
    FT_UInt      requested_cnt;
    FT_UInt      current_axis;
    FT_UInt      used_num_axis;

    int          no_named_instances;

  } GridStatusRec, *GridStatus;

  static GridStatusRec  status;


  static void
  grid_status_init( GridStatus  st )
  {
    st->width         = DIM_X;
    st->height        = DIM_Y;
    st->res           = 72;

    st->scale         = 64;
    st->x_origin      = 0;
    st->y_origin      = 0;

    st->work          = DO_BITMAP | DO_OUTLINE | DO_DOTS;
    st->do_horz_hints = 1;
    st->do_vert_hints = 1;
    st->do_blue_hints = 1;
    st->do_segment    = 0;
    st->do_grid       = 1;
    st->do_alt_colors = 0;

    st->Num           = 0;
    st->lcd_filter    = FT_LCD_FILTER_DEFAULT;
    st->header        = NULL;

    st->mm            = NULL;
    st->current_axis  = 0;

    st->no_named_instances = 0;
  }


  static void
  grid_status_display( GridStatus       st,
                       FTDemo_Display*  display )
  {
    st->disp_width    = display->bitmap->width;
    st->disp_height   = display->bitmap->rows;
    st->disp_bitmap   = display->bitmap;
  }


  static void
  grid_status_colors( GridStatus       st,
                      FTDemo_Display*  display )
  {
    st->axis_color    = grFindColor( display->bitmap,   0,   0,   0, 255 ); /* black       */
    st->grid_color    = grFindColor( display->bitmap, 192, 192, 192, 255 ); /* gray        */
    st->outline_color = grFindColor( display->bitmap, 255,   0,   0, 255 ); /* red         */
    st->on_color      = grFindColor( display->bitmap, 255,   0,   0, 255 ); /* red         */
    st->off_color     = grFindColor( display->bitmap,   0, 128,   0, 255 ); /* dark green  */
    st->segment_color = grFindColor( display->bitmap,  64, 255, 128,  64 ); /* light green */
    st->blue_color    = grFindColor( display->bitmap,  64,  64, 255,  64 ); /* light blue  */
  }


  static void
  grid_status_alt_colors( GridStatus       st,
                          FTDemo_Display*  display )
  {
    /* colours are adjusted for color-blind people, */
    /* cf. http://jfly.iam.u-tokyo.ac.jp/color      */
    st->axis_color    = grFindColor( display->bitmap,   0,   0,   0, 255 ); /* black          */
    st->grid_color    = grFindColor( display->bitmap, 192, 192, 192, 255 ); /* gray           */
    st->outline_color = grFindColor( display->bitmap, 230, 159,   0, 255 ); /* orange         */
    st->on_color      = grFindColor( display->bitmap, 230, 159,   0, 255 ); /* orange         */
    st->off_color     = grFindColor( display->bitmap,  86, 180, 233, 255 ); /* sky blue       */
    st->segment_color = grFindColor( display->bitmap, 204, 121, 167,  64 ); /* reddish purple */
    st->blue_color    = grFindColor( display->bitmap,   0, 114, 178,  64 ); /* blue           */
  }


  static void
  grid_status_rescale_initial( GridStatus      st,
                               FTDemo_Handle*  handle )
  {
    FT_Size     size;
    FT_Error    err    = FTDemo_Get_Size( handle, &size );
    FT_F26Dot6  margin = 4;


    if ( !err )
    {
      FT_Face  face = size->face;

      int  xmin = FT_MulFix( face->bbox.xMin, size->metrics.x_scale );
      int  ymin = FT_MulFix( face->bbox.yMin, size->metrics.y_scale );
      int  xmax = FT_MulFix( face->bbox.xMax, size->metrics.x_scale );
      int  ymax = FT_MulFix( face->bbox.yMax, size->metrics.y_scale );

      FT_F26Dot6  x_scale, y_scale;


      xmin &= ~63;
      ymin &= ~63;
      xmax  = ( xmax + 63 ) & ~63;
      ymax  = ( ymax + 63 ) & ~63;

      if ( xmax - xmin )
        x_scale = st->disp_width  * ( 64 - 2 * margin ) / ( xmax - xmin );
      else
        x_scale = 64;

      if ( ymax - ymin )
        y_scale = st->disp_height * ( 64 - 2 * margin ) / ( ymax - ymin );
      else
        y_scale = 64;

      if ( x_scale <= y_scale )
        st->scale = x_scale;
      else
        st->scale = y_scale;

      st->x_origin = st->disp_width  * margin          - xmin * st->scale;
      st->y_origin = st->disp_height * ( 64 - margin ) + ymin * st->scale;
    }
    else
    {
      st->scale    = 64;
      st->x_origin = st->disp_width  * margin;
      st->y_origin = st->disp_height * ( 64 - margin );
    }

    st->x_origin >>= 6;
    st->y_origin >>= 6;

    st->scale_0    = st->scale;
    st->x_origin_0 = st->x_origin;
    st->y_origin_0 = st->y_origin;
  }


  static void
  grid_status_draw_grid( GridStatus  st )
  {
    int  x_org   = st->x_origin;
    int  y_org   = st->y_origin;
    int  xy_incr = st->scale;


    if ( xy_incr >= 2 )
    {
      int  x2 = x_org;
      int  y2 = y_org;


      for ( ; x2 < st->disp_width; x2 += xy_incr )
        grFillVLine( st->disp_bitmap, x2, 0,
                     st->disp_height, st->grid_color );

      for ( x2 = x_org - xy_incr; x2 >= 0; x2 -= xy_incr )
        grFillVLine( st->disp_bitmap, x2, 0,
                     st->disp_height, st->grid_color );

      for ( ; y2 < st->disp_height; y2 += xy_incr )
        grFillHLine( st->disp_bitmap, 0, y2,
                     st->disp_width, st->grid_color );

      for ( y2 = y_org - xy_incr; y2 >= 0; y2 -= xy_incr )
        grFillHLine( st->disp_bitmap, 0, y2,
                     st->disp_width, st->grid_color );
    }

    grFillVLine( st->disp_bitmap, x_org, 0,
                 st->disp_height, st->axis_color );
    grFillHLine( st->disp_bitmap, 0, y_org,
                 st->disp_width,  st->axis_color );
  }


#ifdef FT_DEBUG_AUTOFIT

  static void
  grid_hint_draw_segment( GridStatus     st,
                          FT_Size        size,
                          AF_GlyphHints  hints )
  {
    FT_Fixed  x_scale = size->metrics.x_scale;
    FT_Fixed  y_scale = size->metrics.y_scale;

    FT_Int  dimension;
    int     x_org = st->x_origin;
    int     y_org = st->y_origin;


    for ( dimension = 1; dimension >= 0; dimension-- )
    {
      FT_Int  num_seg;
      FT_Int  count;


      af_glyph_hints_get_num_segments( hints, dimension, &num_seg );

      for ( count = 0; count < num_seg; count++ )
      {
        int      pos;
        FT_Pos   offset;
        FT_Bool  is_blue;
        FT_Pos   blue_offset;


        af_glyph_hints_get_segment_offset( hints, dimension,
                                           count, &offset,
                                           &is_blue, &blue_offset);

        if ( dimension == 0 ) /* AF_DIMENSION_HORZ is 0 */
        {
          offset = FT_MulFix( offset, x_scale );
          pos    = x_org + ( ( offset * st->scale ) >> 6 );
          grFillVLine( st->disp_bitmap, pos, 0,
                       st->disp_height, st->segment_color );
        }
        else
        {
          offset = FT_MulFix( offset, y_scale );
          pos    = y_org - ( ( offset * st->scale ) >> 6 );

          if ( is_blue )
          {
            int  blue_pos;


            blue_offset = FT_MulFix( blue_offset, y_scale );
            blue_pos    = y_org - ( ( blue_offset * st->scale ) >> 6 );

            if ( blue_pos == pos )
              grFillHLine( st->disp_bitmap, 0, blue_pos,
                           st->disp_width, st->blue_color );
            else
            {
              grFillHLine( st->disp_bitmap, 0, blue_pos,
                           st->disp_width, st->blue_color );
              grFillHLine( st->disp_bitmap, 0, pos,
                           st->disp_width, st->segment_color );
            }
          }
          else
            grFillHLine( st->disp_bitmap, 0, pos,
                         st->disp_width, st->segment_color );
        }
      }
    }
  }

#endif /* FT_DEBUG_AUTOFIT */


  static void
  ft_bitmap_draw( FT_Bitmap*       bitmap,
                  int              x,
                  int              y,
                  FTDemo_Display*  display,
                  grColor          color )
  {
    grBitmap  gbit;


    gbit.width  = (int)bitmap->width;
    gbit.rows   = (int)bitmap->rows;
    gbit.pitch  = bitmap->pitch;
    gbit.buffer = bitmap->buffer;

    switch ( bitmap->pixel_mode )
    {
    case FT_PIXEL_MODE_GRAY:
      gbit.mode  = gr_pixel_mode_gray;
      gbit.grays = 256;
      break;

    case FT_PIXEL_MODE_MONO:
      gbit.mode  = gr_pixel_mode_mono;
      gbit.grays = 2;
      break;

    case FT_PIXEL_MODE_LCD:
      gbit.mode  = gr_pixel_mode_lcd;
      gbit.grays = 256;
      break;

    case FT_PIXEL_MODE_LCD_V:
      gbit.mode  = gr_pixel_mode_lcdv;
      gbit.grays = 256;
      break;

    default:
      return;
    }

    grBlitGlyphToBitmap( display->bitmap, &gbit, x, y, color );
  }


  static void
  ft_outline_draw( FT_Outline*      outline,
                   double           scale,
                   int              pen_x,
                   int              pen_y,
                   FTDemo_Handle*   handle,
                   FTDemo_Display*  display,
                   grColor          color )
  {
    FT_Outline  transformed;
    FT_BBox     cbox;
    FT_Bitmap   bitm;


    FT_Outline_New( handle->library,
                    (FT_UInt)outline->n_points,
                    outline->n_contours,
                    &transformed );

    FT_Outline_Copy( outline, &transformed );

    if ( scale != 1. )
    {
      int  nn;


      for ( nn = 0; nn < transformed.n_points; nn++ )
      {
        FT_Vector*  vec = &transformed.points[nn];


        vec->x = (FT_F26Dot6)( vec->x * scale );
        vec->y = (FT_F26Dot6)( vec->y * scale );
      }
    }

    FT_Outline_Get_CBox( &transformed, &cbox );
    cbox.xMin &= ~63;
    cbox.yMin &= ~63;
    cbox.xMax  = ( cbox.xMax + 63 ) & ~63;
    cbox.yMax  = ( cbox.yMax + 63 ) & ~63;

    bitm.width      = (unsigned int)( ( cbox.xMax - cbox.xMin ) >> 6 );
    bitm.rows       = (unsigned int)( ( cbox.yMax - cbox.yMin ) >> 6 );
    bitm.pitch      = (int)bitm.width;
    bitm.num_grays  = 256;
    bitm.pixel_mode = FT_PIXEL_MODE_GRAY;
    bitm.buffer     = (unsigned char*)calloc( (unsigned int)bitm.pitch,
                                              bitm.rows );

    FT_Outline_Translate( &transformed, -cbox.xMin, -cbox.yMin );
    FT_Outline_Get_Bitmap( handle->library, &transformed, &bitm );

    ft_bitmap_draw( &bitm,
                    pen_x + ( cbox.xMin >> 6 ),
                    pen_y - ( cbox.yMax >> 6 ),
                    display,
                    color );

    free( bitm.buffer );
    FT_Outline_Done( handle->library, &transformed );
  }


  static void
  ft_outline_new_circle( FT_Outline*     outline,
                         FT_F26Dot6      radius,
                         FTDemo_Handle*  handle )
  {
    char*       tag;
    FT_Vector*  vec;
    FT_F26Dot6  disp = (FT_F26Dot6)( radius * 0.5523 );
    /* so that Bézier curve touches circle at 0, 45, and 90 degrees */


    FT_Outline_New( handle->library, 12, 1, outline );
    outline->n_points    = 12;
    outline->n_contours  = 1;
    outline->contours[0] = outline->n_points - 1;

    vec = outline->points;
    tag = outline->tags;

    vec->x =  radius; vec->y =       0; vec++; *tag++ = FT_CURVE_TAG_ON;
    vec->x =  radius; vec->y =    disp; vec++; *tag++ = FT_CURVE_TAG_CUBIC;
    vec->x =    disp; vec->y =  radius; vec++; *tag++ = FT_CURVE_TAG_CUBIC;
    vec->x =       0; vec->y =  radius; vec++; *tag++ = FT_CURVE_TAG_ON;
    vec->x =   -disp; vec->y =  radius; vec++; *tag++ = FT_CURVE_TAG_CUBIC;
    vec->x = -radius; vec->y =    disp; vec++; *tag++ = FT_CURVE_TAG_CUBIC;
    vec->x = -radius; vec->y =       0; vec++; *tag++ = FT_CURVE_TAG_ON;
    vec->x = -radius; vec->y =   -disp; vec++; *tag++ = FT_CURVE_TAG_CUBIC;
    vec->x =   -disp; vec->y = -radius; vec++; *tag++ = FT_CURVE_TAG_CUBIC;
    vec->x =       0; vec->y = -radius; vec++; *tag++ = FT_CURVE_TAG_ON;
    vec->x =    disp; vec->y = -radius; vec++; *tag++ = FT_CURVE_TAG_CUBIC;
    vec->x =  radius; vec->y =   -disp; vec++; *tag++ = FT_CURVE_TAG_CUBIC;
  }


  static void
  circle_draw( FT_F26Dot6       center_x,
               FT_F26Dot6       center_y,
               FT_F26Dot6       radius,
               FTDemo_Handle*   handle,
               FTDemo_Display*  display,
               grColor          color )
  {
    FT_Outline  outline;


    ft_outline_new_circle( &outline, radius, handle );
    /* subpixel adjustment considering downward direction of y-axis */
    FT_Outline_Translate( &outline, center_x & 63, -( center_y & 63 ) );

    ft_outline_draw( &outline, 1., ( center_x >> 6 ), ( center_y >> 6 ),
                     handle, display, color );

    FT_Outline_Done( handle->library, &outline );
  }


  static void
  bitmap_scale( grBitmap*  bit,
                int        scale )
  {
    unsigned char*  s = bit->buffer;
    unsigned char*  t;
    unsigned char*  line;
    int             pitch;
    int             width;
    int             i, j, k;

    pitch = bit->pitch > 0 ?  bit->pitch
                           : -bit->pitch;
    width = bit->width;

    t = (unsigned char*)malloc( (size_t)( pitch * bit->rows *
                                          scale * scale ) );
    if ( !t )
      return;

    line = t;

    switch( bit->mode )
    {
      case gr_pixel_mode_mono:
        for ( i = 0; i < bit->rows; i++ )
        {
          for ( j = 0; j < pitch * scale * 8; j++ )
            if ( s[i * pitch + j / scale / 8] & ( 0x80 >> ( j / scale & 7 ) ) )
              line[j / 8] |= 0x80 >> ( j & 7 );
            else
              line[j / 8] &= ~( 0x80 >> ( j & 7 ) );

          for ( k = 1; k < scale; k++, line += pitch * scale )
            memcpy( line + pitch * scale, line, (size_t)( pitch * scale ) );
          line += pitch * scale;
        }
        break;

      case gr_pixel_mode_gray:
        for ( i = 0; i < bit->rows; i++ )
        {
          for ( j = 0; j < pitch; j++ )
            memset( line + j * scale, s[i * pitch + j], (size_t)scale );

          for ( k = 1; k < scale; k++, line += pitch * scale )
            memcpy( line + pitch * scale, line, (size_t)( pitch * scale ) );
          line += pitch * scale;
        }
        break;

      case gr_pixel_mode_lcd:
      case gr_pixel_mode_lcd2:
        for ( i = 0; i < bit->rows; i++ )
        {
          for ( j = 0; j < width; j += 3 )
            for ( k = 0; k < scale; k++ )
            {
              line[j * scale + 3 * k    ] = s[i * pitch + j    ];
              line[j * scale + 3 * k + 1] = s[i * pitch + j + 1];
              line[j * scale + 3 * k + 2] = s[i * pitch + j + 2];
            }

          for ( k = 1; k < scale; k++, line += pitch * scale )
            memcpy( line + pitch * scale, line, (size_t)( pitch * scale ) );
          line += pitch * scale;
        }
        break;

      case gr_pixel_mode_lcdv:
      case gr_pixel_mode_lcdv2:
        for ( i = 0; i < bit->rows; i += 3 )
        {
          for ( j = 0; j < pitch; j++ )
          {
            memset( line + j * scale,
                    s[i * pitch +             j], (size_t)scale );
            memset( line + j * scale +     pitch * scale,
                    s[i * pitch +     pitch + j], (size_t)scale );
            memset( line + j * scale + 2 * pitch * scale,
                    s[i * pitch + 2 * pitch + j], (size_t)scale );
          }

          for ( k = 1; k < scale; k++, line += 3 * pitch * scale )
            memcpy( line + 3 * pitch * scale,
                    line,
                    (size_t)( 3 * pitch * scale ) );
          line += 3 * pitch * scale;
        }
        break;

      default:
        return;
    }

    bit->buffer = t;
    bit->rows  *= scale;
    bit->width *= scale;
    bit->pitch *= scale;
  }


  static void
  grid_status_draw_outline( GridStatus       st,
                            FTDemo_Handle*   handle,
                            FTDemo_Display*  display )
  {
    FT_Error      err;
    FT_Size       size;
    FT_GlyphSlot  slot;
    FT_UInt       glyph_idx;
    int           scale = st->scale;
    int           ox    = st->x_origin;
    int           oy    = st->y_origin;


    err = FTDemo_Get_Size( handle, &size );
    if ( err )
      return;

#ifdef FT_DEBUG_AUTOFIT
    /* Draw segment before drawing glyph. */
    if ( status.do_segment )
    {
      /* Force hinting first in order to collect segment info. */
      _af_debug_disable_horz_hints = 0;
      _af_debug_disable_vert_hints = 0;

      if ( !FT_Load_Glyph( size->face, (FT_UInt)st->Num,
                           FT_LOAD_DEFAULT        |
                           FT_LOAD_NO_BITMAP      |
                           FT_LOAD_FORCE_AUTOHINT |
                           FT_LOAD_TARGET_NORMAL ) )
        grid_hint_draw_segment( &status, size, _af_debug_hints );
    }

    _af_debug_disable_horz_hints = !st->do_horz_hints;
    _af_debug_disable_vert_hints = !st->do_vert_hints;
    _af_debug_disable_blue_hints = !st->do_blue_hints;
#endif

    if ( handle->encoding == FT_ENCODING_ORDER )
      glyph_idx = (FT_UInt)st->Num;
    else
      glyph_idx = FTDemo_Get_Index( handle, (FT_UInt32)st->Num );


    if ( FT_Load_Glyph( size->face, glyph_idx,
                        handle->load_flags | FT_LOAD_NO_BITMAP ) )
      return;

    if ( st->do_grid )
    {
      /* show advance width */
      grFillVLine( st->disp_bitmap,
                   st->x_origin +
                     ( ( size->face->glyph->metrics.horiAdvance +
                         size->face->glyph->lsb_delta           -
                         size->face->glyph->rsb_delta           ) *
                       scale >> 6 ),
                   0,
                   st->disp_height,
                   st->axis_color );

      /* show ascender and descender */
      grFillHLine( st->disp_bitmap,
                   0,
                   st->y_origin - ( size->metrics.ascender  * scale >> 6 ),
                   st->disp_width,
                   st->axis_color );
      grFillHLine( st->disp_bitmap,
                   0,
                   st->y_origin - ( size->metrics.descender * scale >> 6 ),
                   st->disp_width,
                   st->axis_color );
    }

    slot = size->face->glyph;
    if ( slot->format == FT_GLYPH_FORMAT_OUTLINE )
    {
      FT_Glyph     glyph;
      FT_Outline*  gimage = &slot->outline;
      int          nn;


      /* render scaled bitmap */
      if ( st->work & DO_BITMAP )
      {
        int             left, top, x_advance, y_advance;
        grBitmap        bitg;
        FT_Glyph        glyf;


        FT_Get_Glyph( slot, &glyph );
        error  = FTDemo_Glyph_To_Bitmap( handle, glyph, &bitg, &left, &top,
                                         &x_advance, &y_advance, &glyf);

        if ( !error )
        {
          bitmap_scale( &bitg, scale );

          grBlitGlyphToBitmap( display->bitmap, &bitg,
                               ox + left * scale, oy - top * scale,
                               st->axis_color );

          free( bitg.buffer );

          if ( glyf )
            FT_Done_Glyph( glyf );
        }

        FT_Done_Glyph( glyph );
      }

      /* scale the outline */
      for ( nn = 0; nn < gimage->n_points; nn++ )
      {
        FT_Vector*  vec = &gimage->points[nn];


        /* half-pixel shift hints the stroked path */
        vec->x = vec->x * scale + 32;
        vec->y = vec->y * scale - 32;
      }

      /* stroke then draw it */
      if ( st->work & DO_OUTLINE )
      {
        FT_Get_Glyph( slot, &glyph );
        FT_Glyph_Stroke( &glyph, st->stroker, 1 );

        error = FTDemo_Draw_Glyph_Color( handle, display, glyph, &ox, &oy,
                                         st->outline_color );
        if ( !error )
          FT_Done_Glyph( glyph );
      }

      /* draw the points... */
      if ( st->work & DO_DOTS )
      {
        for ( nn = 0; nn < gimage->n_points; nn++ )
          circle_draw(
            st->x_origin * 64 + gimage->points[nn].x,
            st->y_origin * 64 - gimage->points[nn].y,
            128,
            handle,
            display,
            ( gimage->tags[nn] & FT_CURVE_TAG_ON ) ? st->on_color
                                                   : st->off_color );
      }

      /* ... and point numbers */
      if ( st->work & DO_DOTNUMBERS )
      {
        FT_Vector*  points   = gimage->points;
        FT_Short*   contours = gimage->contours;
        char*       tags     = gimage->tags;
        short       c, n;
        char        number_string[10];
        size_t      number_string_len = sizeof ( number_string );

        FT_Long  octant_x[8] = { 1024, 724, 0, -724, -1024, -724, 0, 724 };
        FT_Long  octant_y[8] = { 0, 724, 1024, 724, 0, -724, -1024, -724 };


        c = 0;
        n = 0;
        for ( ; c < gimage->n_contours; c++ )
        {
          for (;;)
          {
            short      prev, next;
            FT_Vector  in, out, middle;
            FT_Fixed   in_len, out_len, middle_len;
            int        num_digits;


            /* find previous and next point in outline */
            if ( c == 0 )
            {
              if ( contours[c] == 0 )
              {
                prev = 0;
                next = 0;
              }
              else
              {
                prev = n > 0 ? n - 1
                             : contours[c];
                next = n < contours[c] ? n + 1
                                       : 0;
              }
            }
            else
            {
              prev = n > ( contours[c - 1] + 1 ) ? n - 1
                                                 : contours[c];
              next = n < contours[c] ? n + 1
                                     : contours[c - 1] + 1;
            }

            /* get vectors to previous and next point and normalize them; */
            /* we use 16.16 format to improve the computation precision   */
            in.x = ( points[prev].x - points[n].x ) * 1024;
            in.y = ( points[prev].y - points[n].y ) * 1024;

            out.x = ( points[next].x - points[n].x ) * 1024;
            out.y = ( points[next].y - points[n].y ) * 1024;

            in_len  = FT_Vector_Length( &in );
            out_len = FT_Vector_Length( &out );

            if ( in_len )
            {
              in.x = FT_DivFix( in.x, in_len );
              in.y = FT_DivFix( in.y, in_len );
            }
            if ( out_len )
            {
              out.x = FT_DivFix( out.x, out_len );
              out.y = FT_DivFix( out.y, out_len );
            }

            middle.x = in.x + out.x;
            middle.y = in.y + out.y;
            /* we use a delta of 1 << 13 (corresponding to 1/8px) */
            if ( ( middle.x < 4096 ) && ( middle.x > -4096 ) &&
                 ( middle.y < 4096 ) && ( middle.y > -4096 ) )
            {
              /* in case of vectors in almost exactly opposite directions, */
              /* use a vector orthogonal to them                           */
              middle.x =  out.y;
              middle.y = -out.x;

              if ( ( middle.x < 4096 ) && ( middle.x > -4096 ) &&
                   ( middle.y < 4096 ) && ( middle.y > -4096 ) )
              {
                /* use direction based on point index for the offset */
                /* if we still don't have a good value               */
                middle.x = octant_x[n % 8];
                middle.y = octant_y[n % 8];
              }
            }

            /* normalize `middle' vector (which is never zero) and */
            /* convert it back to 26.6 format, this time using a   */
            /* length of 8 pixels to get some distance between the */
            /* point and the number                                */
            middle_len = FT_Vector_Length( &middle );
            middle.x   = FT_DivFix( middle.x, middle_len ) >> 7;
            middle.y   = FT_DivFix( middle.y, middle_len ) >> 7;

            num_digits = snprintf( number_string,
                                   number_string_len,
                                   "%d", n );

            /* we now position the point number in the opposite       */
            /* direction of the `middle' vector, adding some offset   */
            /* since the string drawing function expects the upper    */
            /* left corner of the number string (the font size is 8x8 */
            /* pixels)                                                */
            grWriteCellString( display->bitmap,
                               st->x_origin +
                                 ( ( points[n].x - middle.x ) >> 6 ) -
                                 ( middle.x > 0 ? ( num_digits - 1 ) * 8 + 2
                                                : 2 ),
                               st->y_origin -
                                 ( ( ( points[n].y - middle.y ) >> 6 ) +
                                   8 / 2 ),
                               number_string,
                               ( tags[n] & FT_CURVE_TAG_ON )
                                 ? st->on_color
                                 : st->off_color );

            n++;
            if ( n > contours[c] )
              break;
          }
        }
      }
    }
  }


  static FTDemo_Display*  display;
  static FTDemo_Handle*   handle;

#if 0
  static const unsigned char*  Text = (unsigned char*)
    "The quick brown fox jumps over the lazy dog 0123456789 "
    "\342\352\356\373\364\344\353\357\366\374\377\340\371\351\350\347 "
    "&#~\"\'(-`_^@)=+\260 ABCDEFGHIJKLMNOPQRSTUVWXYZ "
    "$\243^\250*\265\371%!\247:/;.,?<>";
#endif


  static void
  Fatal( const char*  message )
  {
    FTDemo_Display_Done( display );
    FTDemo_Done( handle );
    PanicZ( message );
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
    char  buf[BUFSIZE];
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
            "FreeType Glyph Grid Viewer - part of the FreeType %s test suite",
             version );

    grWriteln( buf );
    grLn();
    grWriteln( "Use the following keys:" );
    grLn();
    /*          |----------------------------------|    |----------------------------------| */
#ifdef FT_DEBUG_AUTOFIT
    grWriteln( "F1, ?       display this help screen    if autohinting:                     " );
    grWriteln( "                                          H         toggle horiz. hinting   " );
    grWriteln( "i, k        move grid up/down             V         toggle vert. hinting    " );
    grWriteln( "j, l        move grid left/right          B         toggle blue zone hinting" );
    grWriteln( "PgUp, PgDn  zoom in/out grid              s         toggle segment drawing  " );
    grWriteln( "SPC         reset zoom and position                  (unfitted, with blues) " );
    grWriteln( "                                          1         dump edge hints         " );
    grWriteln( "p, n        previous/next font            2         dump segment hints      " );
    grWriteln( "                                          3         dump point hints        " );
#else
    grWriteln( "F1, ?       display this help screen    i, k        move grid up/down       " );
    grWriteln( "                                        j, l        move grid left/right    " );
    grWriteln( "p, n        previous/next font          PgUp, PgDn  zoom in/out grid        " );
    grWriteln( "                                        SPC         reset zoom and position " );
#endif /* FT_DEBUG_AUTOFIT */
    grWriteln( "Up, Down    adjust size by 0.5pt        if not auto-hinting:                " );
    grWriteln( "                                          H         cycle through hinting   " );
    grWriteln( "Left, Right adjust index by 1                        engines (if available) " );
    grWriteln( "F7, F8      adjust index by 16          if normal auto-hinting:             " );
    grWriteln( "F9, F10     adjust index by 256           w         toggle warping          " );
    grWriteln( "F11, F12    adjust index by 4096                      (if available)        " );
    grWriteln( "                                                                            " );
    grWriteln( "h           toggle hinting              b           toggle bitmap           " );
    grWriteln( "f           toggle forced auto-         d           toggle dot display      " );
    grWriteln( "             hinting (if hinting)       o           toggle outline display  " );
    grWriteln( "G           toggle grid display         D           toggle dotnumber display" );
    grWriteln( "C           change color palette                                            " );
    grWriteln( "                                        if Multiple Master or GX font:      " );
    grWriteln( "F5, F6      cycle through                 F2        cycle through axes      " );
    grWriteln( "             anti-aliasing modes          F3, F4    adjust current axis by  " );
    grWriteln( "L           cycle through LCD                        1/50th of its range    " );
    grWriteln( "             filters                                                        " );
    grWriteln( "g, v        adjust gamma value          q, ESC      quit ftgrid             " );
    /*          |----------------------------------|    |----------------------------------| */
    grLn();
    grLn();
    grWriteln( "press any key to exit this help screen" );

    grRefreshSurface( display->surface );
    grListenSurface( display->surface, gr_event_key, &dummy_event );
  }


  static void
  event_font_change( int  delta )
  {
    FT_Error         err;
    FT_Size          size;
    FT_UInt          n, num_names;
    FT_Int           instance_index;
    FT_Multi_Master  dummy;
    int              num_indices, is_GX;


    if ( status.font_index + delta >= handle->num_fonts ||
         status.font_index + delta < 0                  )
      return;

    status.font_index += delta;

    FTDemo_Set_Current_Font( handle, handle->fonts[status.font_index] );
    FTDemo_Set_Current_Charsize( handle, status.ptsize, status.res );
    FTDemo_Update_Current_Flags( handle );

    num_indices = handle->current_font->num_indices;

    if ( status.Num >= num_indices )
      status.Num = num_indices - 1;

    err = FTDemo_Get_Size( handle, &size );
    if ( err )
      return;

    free( status.mm );
    status.mm = NULL;

    err = FT_Get_MM_Var( size->face, &status.mm );
    if ( err )
      return;

    if ( status.mm->num_axis >= MAX_MM_AXES )
    {
      fprintf( stderr, "only handling first %d GX axes (of %d)\n",
                       MAX_MM_AXES, status.mm->num_axis );
      status.used_num_axis = MAX_MM_AXES;
    }
    else
      status.used_num_axis = status.mm->num_axis;

    err   = FT_Get_Multi_Master( size->face, &dummy );
    is_GX = err ? 1 : 0;

    num_names = FT_Get_Sfnt_Name_Count( size->face );

    /* in `face_index', the instance index starts with value 1 */
    instance_index = ( size->face->face_index >> 16 ) - 1;

    for ( n = 0; n < MAX_MM_AXES; n++ )
    {
      free( status.axis_name[n] );
      status.axis_name[n] = NULL;
    }

    for ( n = 0; n < status.used_num_axis; n++ )
    {
      if ( status.requested_cnt )
      {
        status.design_pos[n] = n < status.requested_cnt
                                 ? status.requested_pos[n]
                                 : status.mm->axis[n].def;
        if ( status.design_pos[n] < status.mm->axis[n].minimum )
          status.design_pos[n] = status.mm->axis[n].minimum;
        else if ( status.design_pos[n] > status.mm->axis[n].maximum )
          status.design_pos[n] = status.mm->axis[n].maximum;
      }
      else if ( FT_IS_NAMED_INSTANCE( size->face ) )
        status.design_pos[n] = status.mm->namedstyle[instance_index].
                                          coords[n];
      else
        status.design_pos[n] = status.mm->axis[n].def;

      if ( is_GX )
      {
        FT_SfntName  name;
        FT_UInt      strid, j;


        name.string = NULL;
        strid       = status.mm->axis[n].strid;

        /* iterate over all name entries        */
        /* to find an English entry for `strid' */

        for ( j = 0; j < num_names; j++ )
        {
          error = FT_Get_Sfnt_Name( size->face, j, &name );
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

        if ( name.string )
        {
          FT_UInt  len;
          char*    s;


          if ( name.platform_id == TT_PLATFORM_MACINTOSH )
          {
            len = put_ascii_string_size( name.string, name.string_len, 0 );
            s   = (char*)malloc( len );
            if ( s )
            {
              put_ascii_string( s, name.string, name.string_len, 0 );
              status.axis_name[n] = s;
            }
          }
          else
          {
            len = put_unicode_be16_string_size( name.string,
                                                name.string_len,
                                                0,
                                                0 );
            s   = (char*)malloc( len );
            if ( s )
            {
              put_unicode_be16_string( s,
                                       name.string,
                                       name.string_len,
                                       0,
                                       0 );
              status.axis_name[n] = s;
            }
          }
        }
      }
    }

    (void)FT_Set_Var_Design_Coordinates( size->face,
                                         status.used_num_axis,
                                         status.design_pos );
  }


  static void
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
    {
      /* Resetting the cache is perhaps a bit harsh, but I'm too  */
      /* lazy to walk over all loaded fonts to check whether they */
      /* are of type TTF, then unloading them explicitly.         */
      FTC_Manager_Reset( handle->cache_manager );
      event_font_change( 0 );
    }

    sprintf( status.header_buffer,
             "TrueType engine changed to version %d",
             status.tt_interpreter_versions[
               status.tt_interpreter_version_idx]);

    status.header = (const char *)status.header_buffer;
  }


  static void
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
        event_font_change( 0 );
      }

      status.header = status.warping ? "warping enabled"
                                     : "warping disabled";
    }
    else
      status.header = "need normal anti-aliasing mode to toggle warping";
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


  static void
  event_grid_reset( GridStatus  st )
  {
    st->x_origin = st->x_origin_0;
    st->y_origin = st->y_origin_0;
    st->scale    = st->scale_0;
  }


  static void
  event_grid_translate( int  dx,
                        int  dy )
  {
    status.x_origin += 32 * dx;
    status.y_origin += 32 * dy;
  }


  static void
  event_grid_zoom( double  zoom )
  {
    int  scale_old = status.scale;


    status.scale *= zoom;

    /* avoid same zoom value due to truncation */
    /* to integer in above multiplication      */
    if ( status.scale == scale_old && zoom > 1.0 )
      status.scale++;

    sprintf( status.header_buffer, "zoom scale %d:1", status.scale );

    status.header = (const char *)status.header_buffer;
  }


  static void
  event_lcd_mode_change( int  delta )
  {
    const char*  lcd_mode = NULL;


    handle->lcd_mode = ( handle->lcd_mode +
                         delta            +
                         N_LCD_MODES      ) % N_LCD_MODES;

    switch ( handle->lcd_mode )
    {
    case LCD_MODE_MONO:
      lcd_mode = "monochrome";
      break;
    case LCD_MODE_AA:
      lcd_mode = "normal AA";
      break;
    case LCD_MODE_LIGHT:
      lcd_mode = "light AA";
      break;
    case LCD_MODE_LIGHT_SUBPIXEL:
      lcd_mode = "light AA (subpixel positioning)";
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
    }

    if ( delta )
    {
      FTC_Manager_Reset( handle->cache_manager );
      event_font_change( 0 );
    }

    sprintf( status.header_buffer, "rendering mode changed to %s",
             lcd_mode );

    status.header = (const char *)status.header_buffer;

    FTDemo_Update_Current_Flags( handle );
  }


  static void
  event_lcd_filter_change( void )
  {
    if ( handle->lcd_mode >= LCD_MODE_RGB )
    {
      const char*  lcd_filter = NULL;


      switch( status.lcd_filter )
      {
      case FT_LCD_FILTER_DEFAULT:
        status.lcd_filter = FT_LCD_FILTER_LIGHT;
        break;
      case FT_LCD_FILTER_LIGHT:
        status.lcd_filter = FT_LCD_FILTER_LEGACY1;
        break;
      case FT_LCD_FILTER_LEGACY1:
        status.lcd_filter = FT_LCD_FILTER_NONE;
        break;
      case FT_LCD_FILTER_NONE:
      default:
        status.lcd_filter = FT_LCD_FILTER_DEFAULT;
        break;
      }

      switch ( status.lcd_filter )
      {
      case FT_LCD_FILTER_DEFAULT:
        lcd_filter = "default";
        break;
      case FT_LCD_FILTER_LIGHT:
        lcd_filter = "light";
        break;
      case FT_LCD_FILTER_LEGACY1:
        lcd_filter = "legacy";
        break;
      case FT_LCD_FILTER_NONE:
      default:
        lcd_filter = "none";
        break;
      }

      sprintf( status.header_buffer, "LCD filter changed to %s",
               lcd_filter );

      status.header = (const char *)status.header_buffer;

      FT_Library_SetLcdFilter( handle->library, status.lcd_filter );
    }
    else
      status.header = "need LCD mode to change filter";
  }


  static void
  event_size_change( int  delta )
  {
    status.ptsize += delta;

    if ( status.ptsize < 1 * 64 )
      status.ptsize = 1 * 64;
    else if ( status.ptsize > MAXPTSIZE * 64 )
      status.ptsize = MAXPTSIZE * 64;

    FTDemo_Set_Current_Charsize( handle, status.ptsize, status.res );
  }


  static void
  event_index_change( int  delta )
  {
    int  num_indices = handle->current_font->num_indices;


    status.Num += delta;

    if ( status.Num < 0 )
      status.Num = 0;
    else if ( status.Num >= num_indices )
      status.Num = num_indices - 1;
  }


  static void
  event_axis_change( int  delta )
  {
    FT_Error      err;
    FT_Size       size;
    FT_Var_Axis*  a;
    FT_Fixed      pos;


    err = FTDemo_Get_Size( handle, &size );
    if ( err )
      return;

    if ( !status.mm )
      return;

    a   = status.mm->axis + status.current_axis;
    pos = status.design_pos[status.current_axis];

    /*
     * Normalize i.  Changing by 20 is all very well for PostScript fonts,
     * which tend to have a range of ~1000 per axis, but it's not useful
     * for mac fonts, which have a range of ~3.  And it's rather extreme
     * for optical size even in PS.
     */
    pos += FT_MulDiv( delta, a->maximum - a->minimum, 1000 );
    if ( pos < a->minimum )
      pos = a->minimum;
    if ( pos > a->maximum )
      pos = a->maximum;

    status.design_pos[status.current_axis] = pos;

    (void)FT_Set_Var_Design_Coordinates( size->face,
                                         status.used_num_axis,
                                         status.design_pos );
  }


  static int
  Process_Event( grEvent*  event )
  {
    int  ret = 0;


    status.header = NULL;

    switch ( event->key )
    {
    case grKeyEsc:
    case grKEY( 'q' ):
      ret = 1;
      break;

    case grKeyF1:
    case grKEY( '?' ):
      event_help();
      break;

    case grKEY( 'f' ):
      handle->autohint = !handle->autohint;
      status.header    = handle->autohint ? "forced auto-hinting is now on"
                                          : "forced auto-hinting is now off";

      FTDemo_Update_Current_Flags( handle );
      break;

#ifdef FT_DEBUG_AUTOFIT
    case grKEY( '1' ):
      if ( handle->hinted                                  &&
           ( handle->autohint                            ||
             handle->lcd_mode == LCD_MODE_LIGHT          ||
             handle->lcd_mode == LCD_MODE_LIGHT_SUBPIXEL ) )
      {
        status.header = "dumping glyph edges to stdout";
        af_glyph_hints_dump_edges( _af_debug_hints, 1 );
      }
      break;

    case grKEY( '2' ):
      if ( handle->hinted                                  &&
           ( handle->autohint                            ||
             handle->lcd_mode == LCD_MODE_LIGHT          ||
             handle->lcd_mode == LCD_MODE_LIGHT_SUBPIXEL ) )
      {
        status.header = "dumping glyph segments to stdout";
        af_glyph_hints_dump_segments( _af_debug_hints, 1 );
      }
      break;

    case grKEY( '3' ):
      if ( handle->hinted                                  &&
           ( handle->autohint                            ||
             handle->lcd_mode == LCD_MODE_LIGHT          ||
             handle->lcd_mode == LCD_MODE_LIGHT_SUBPIXEL ) )
      {
        status.header = "dumping glyph points to stdout";
        af_glyph_hints_dump_points( _af_debug_hints, 1 );
      }
      break;
#endif /* FT_DEBUG_AUTOFIT */

    case grKEY( 'C' ):
      status.do_alt_colors = !status.do_alt_colors;
      if ( status.do_alt_colors )
      {
        status.header = "use alternative colors";
        grid_status_alt_colors( &status, display );
      }
      else
      {
        status.header = "use default colors";
        grid_status_colors( &status, display );
      }
      break;

    case grKEY( 'L' ):
      event_lcd_filter_change();
      break;

    case grKEY( 'g' ):
      event_gamma_change( 0.1 );
      break;

    case grKEY( 'v' ):
      event_gamma_change( -0.1 );
      break;

    case grKEY( 'n' ):
      event_font_change( 1 );
      break;

    case grKEY( 'h' ):
      handle->hinted = !handle->hinted;
      status.header  = handle->hinted ? "glyph hinting is now active"
                                      : "glyph hinting is now ignored";

      FTC_Manager_Reset( handle->cache_manager );
      event_font_change( 0 );
      break;

    case grKEY( 'G' ):
      status.do_grid = !status.do_grid;
      status.header = status.do_grid ? "grid drawing enabled"
                                     : "grid drawing disabled";
      break;

    case grKEY( 'd' ):
      status.work ^= DO_DOTS;
      break;

    case grKEY( 'D' ):
      status.work ^= DO_DOTNUMBERS;
      break;

    case grKEY( 'o' ):
      status.work ^= DO_OUTLINE;
      break;

    case grKEY( 'b' ):
      status.work ^= DO_BITMAP;
      break;

    case grKEY( 'p' ):
      event_font_change( -1 );
      break;

    case grKEY( 'H' ):
      if ( !( handle->autohint                            ||
              handle->lcd_mode == LCD_MODE_LIGHT          ||
              handle->lcd_mode == LCD_MODE_LIGHT_SUBPIXEL ) )
      {
        FT_Face    face;
        FT_Module  module;


        error = FTC_Manager_LookupFace( handle->cache_manager,
                                        handle->scaler.face_id, &face );
        if ( !error )
        {
          module = &face->driver->root;

          if ( !strcmp( module->clazz->module_name, "cff" ) )
          {
            if ( FTDemo_Event_Cff_Hinting_Engine_Change(
                   handle->library,
                   &status.cff_hinting_engine,
                   1 ) )
            {
              /* Resetting the cache is perhaps a bit harsh, but I'm too  */
              /* lazy to walk over all loaded fonts to check whether they */
              /* are of type CFF, then unloading them explicitly.         */
              FTC_Manager_Reset( handle->cache_manager );
              event_font_change( 0 );
            }

            sprintf( status.header_buffer, "CFF engine changed to %s",
                     status.cff_hinting_engine == FT_HINTING_FREETYPE
                       ? "FreeType" : "Adobe" );

            status.header = (const char *)status.header_buffer;
          }
          else if ( !strcmp( module->clazz->module_name, "type1" ) )
          {
            if ( FTDemo_Event_Type1_Hinting_Engine_Change(
                   handle->library,
                   &status.type1_hinting_engine,
                   1 ) )
            {
              /* Resetting the cache is perhaps a bit harsh, but I'm too  */
              /* lazy to walk over all loaded fonts to check whether they */
              /* are of type Type1, then unloading them explicitly.       */
              FTC_Manager_Reset( handle->cache_manager );
              event_font_change( 0 );
            }

            sprintf( status.header_buffer, "Type 1 engine changed to %s",
                     status.type1_hinting_engine == FT_HINTING_FREETYPE
                       ? "FreeType" : "Adobe" );

            status.header = (const char *)status.header_buffer;
          }
          else if ( !strcmp( module->clazz->module_name, "t1cid" ) )
          {
            if ( FTDemo_Event_T1cid_Hinting_Engine_Change(
                   handle->library,
                   &status.t1cid_hinting_engine,
                   1 ) )
            {
              /* Resetting the cache is perhaps a bit harsh, but I'm too  */
              /* lazy to walk over all loaded fonts to check whether they */
              /* are of type CID, then unloading them explicitly.         */
              FTC_Manager_Reset( handle->cache_manager );
              event_font_change( 0 );
            }

            sprintf( status.header_buffer, "CID engine changed to %s",
                     status.t1cid_hinting_engine == FT_HINTING_FREETYPE
                       ? "FreeType" : "Adobe" );

            status.header = (const char *)status.header_buffer;
          }
          else if ( !strcmp( module->clazz->module_name, "truetype" ) )
            event_tt_interpreter_version_change();
        }
      }
#ifdef FT_DEBUG_AUTOFIT
      else
      {
        status.do_horz_hints = !status.do_horz_hints;
        status.header = status.do_horz_hints ? "horizontal hinting enabled"
                                             : "horizontal hinting disabled";
      }
#endif
      break;

    case grKEY( 'w' ):
      event_warping_change();
      break;

#ifdef FT_DEBUG_AUTOFIT
    case grKEY( 'V' ):
      if ( handle->autohint                            ||
           handle->lcd_mode == LCD_MODE_LIGHT          ||
           handle->lcd_mode == LCD_MODE_LIGHT_SUBPIXEL )
      {
        status.do_vert_hints = !status.do_vert_hints;
        status.header = status.do_vert_hints ? "vertical hinting enabled"
                                             : "vertical hinting disabled";
      }
      else
        status.header = "need autofit mode to toggle vertical hinting";
      break;

    case grKEY( 'B' ):
      if ( handle->autohint                            ||
           handle->lcd_mode == LCD_MODE_LIGHT          ||
           handle->lcd_mode == LCD_MODE_LIGHT_SUBPIXEL )
      {
        status.do_blue_hints = !status.do_blue_hints;
        status.header = status.do_blue_hints ? "blue zone hinting enabled"
                                             : "blue zone hinting disabled";
      }
      else
        status.header = "need autofit mode to toggle blue zone hinting";
      break;

    case grKEY( 's' ):
      status.do_segment = !status.do_segment;
      status.header = status.do_segment ? "segment drawing enabled"
                                        : "segment drawing disabled";
      break;
#endif /* FT_DEBUG_AUTOFIT */

    case grKeyLeft:     event_index_change(      -1 ); break;
    case grKeyRight:    event_index_change(       1 ); break;
    case grKeyF7:       event_index_change(   -0x10 ); break;
    case grKeyF8:       event_index_change(    0x10 ); break;
    case grKeyF9:       event_index_change(  -0x100 ); break;
    case grKeyF10:      event_index_change(   0x100 ); break;
    case grKeyF11:      event_index_change( -0x1000 ); break;
    case grKeyF12:      event_index_change(  0x1000 ); break;

    case grKeyUp:       event_size_change(  32 ); break;
    case grKeyDown:     event_size_change( -32 ); break;

    case grKEY( ' ' ):  event_grid_reset( &status );
#if 0
                        status.do_horz_hints = 1;
                        status.do_vert_hints = 1;
                        status.do_blue_hints = 1;
#endif
                        break;

    case grKEY( 'i' ):  event_grid_translate(  0, -1 ); break;
    case grKEY( 'k' ):  event_grid_translate(  0,  1 ); break;
    case grKEY( 'j' ):  event_grid_translate( -1,  0 ); break;
    case grKEY( 'l' ):  event_grid_translate(  1,  0 ); break;

    case grKeyPageUp:   event_grid_zoom( 1.25     ); break;
    case grKeyPageDown: event_grid_zoom( 1 / 1.25 ); break;

    case grKeyF2:       if ( status.mm )
                        {
                          status.current_axis++;
                          status.current_axis %= status.used_num_axis;
                        }
                        break;

    case grKeyF3:       event_axis_change( -20 ); break;
    case grKeyF4:       event_axis_change(  20 ); break;

    case grKeyF5:       event_lcd_mode_change( -1 ); break;
    case grKeyF6:       event_lcd_mode_change(  1 ); break;

    default:
      ;
    }

    return ret;
  }


  static void
  write_header( FT_Error  error_code )
  {
    FTDemo_Draw_Header( handle, display, status.ptsize, status.res,
                        status.Num, error_code );

    if ( status.header )
      grWriteCellString( display->bitmap, 0, 3 * HEADER_HEIGHT,
                         status.header, display->fore_color );

    if ( status.mm )
    {
      const char*  format = "%s axis: %.02f";


      snprintf( status.header_buffer, BUFSIZE, format,
                status.axis_name[status.current_axis]
                  ? status.axis_name[status.current_axis]
                  : status.mm->axis[status.current_axis].name,
                status.design_pos[status.current_axis] / 65536.0 );

      status.header = (const char *)status.header_buffer;
      grWriteCellString( display->bitmap, 0, 4 * HEADER_HEIGHT,
                         status.header, display->fore_color );
    }

    grRefreshSurface( display->surface );
  }


  static void
  usage( char*  execname )
  {
    fprintf( stderr,
      "\n"
      "ftgrid: simple glyph grid viewer -- part of the FreeType project\n"
      "----------------------------------------------------------------\n"
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
      "            For Type 1 font files, ftgrid also tries to attach\n"
      "            the corresponding metrics file (with extension\n"
      "            `.afm' or `.pfm').\n"
      "\n" );
    fprintf( stderr,
      "  -w W      Set the window width to W pixels (default: %dpx).\n"
      "  -h H      Set the window height to H pixels (default: %dpx).\n"
      "\n",
             DIM_X, DIM_Y );
    fprintf( stderr,
      "  -r R      Use resolution R dpi (default: 72dpi).\n"
      "  -f index  Specify first index to display (default: 0).\n"
      "  -e enc    Specify encoding tag (default: no encoding).\n"
      "            Common values: `unic' (Unicode), `symb' (symbol),\n"
      "            `ADOB' (Adobe standard), `ADBC' (Adobe custom).\n"
      "  -d \"axis1 axis2 ...\"\n"
      "            Specify the design coordinates for each\n"
      "            Multiple Master axis at start-up.  Implies `-n'.\n"
      "  -n        Don't display named instances of variation fonts.\n"
      "\n"
      "  -v        Show version."
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
      option = getopt( *argc, *argv, "d:e:f:h:nr:vw:" );

      if ( option == -1 )
        break;

      switch ( option )
      {
      case 'd':
        {
          FT_UInt    cnt;
          FT_Fixed*  pos = status.requested_pos;
          char*      s   = optarg;


          for ( cnt = 0; cnt < MAX_MM_AXES && *s; cnt++ )
          {
            pos[cnt] = (FT_Fixed)( strtod( s, &s ) * 65536.0 );

            while ( *s == ' ' )
              ++s;
          }

          status.requested_cnt      = cnt;
          status.no_named_instances = 1;
        }
        break;

      case 'e':
        handle->encoding = FTDemo_Make_Encoding_Tag( optarg );
        status.Num       = 0x20;
        break;

      case 'f':
        status.Num = atoi( optarg );
        break;

      case 'h':
        status.height = atoi( optarg );
        if ( status.height < 1 )
          usage( execname );
        break;

      case 'n':
        status.no_named_instances = 1;
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

          printf( "ftgrid (FreeType) %d.%d", major, minor );
          if ( patch )
            printf( ".%d", patch );
          printf( "\n" );
          exit( 0 );
        }
        /* break; */

      case 'w':
        status.width = atoi( optarg );
        if ( status.width < 1 )
          usage( execname );
        break;

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
    grEvent       event;
    int           n;
    unsigned int  dflt_tt_interpreter_version;
    unsigned int  versions[3] = { TT_INTERPRETER_VERSION_35,
                                  TT_INTERPRETER_VERSION_38,
                                  TT_INTERPRETER_VERSION_40 };


    /* initialize engine */
    handle = FTDemo_New();

    grid_status_init( &status );
    parse_cmdline( &argc, &argv );

    /* get the default value as compiled into FreeType */
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
    for ( n = 0; n < 3; n++ )
    {
      error = FT_Property_Set( handle->library,
                               "truetype",
                               "interpreter-version", &versions[n] );
      if ( !error )
        status.tt_interpreter_versions[
          status.num_tt_interpreter_versions++] = versions[n];
      if ( versions[n] == dflt_tt_interpreter_version )
        status.tt_interpreter_version_idx = n;
    }
    FT_Property_Set( handle->library,
                     "truetype",
                     "interpreter-version", &dflt_tt_interpreter_version );

    FT_Property_Get( handle->library,
                     "autofitter",
                     "warping", &status.warping );

    FT_Library_SetLcdFilter( handle->library, status.lcd_filter );

    FT_Stroker_New( handle->library, &status.stroker );

    FT_Stroker_Set( status.stroker, 32, FT_STROKER_LINECAP_BUTT,
                      FT_STROKER_LINEJOIN_BEVEL, 0x20000 );

    display = FTDemo_Display_New( gr_pixel_mode_rgb24,
                                  status.width, status.height );
    if ( !display )
      Fatal( "could not allocate display surface" );

    grid_status_display( &status, display );
    grid_status_colors(  &status, display );

    grSetTitle( display->surface,
                "FreeType Glyph Grid Viewer - press ? for help" );

    for ( ; argc > 0; argc--, argv++ )
    {
      error = FTDemo_Install_Font( handle, argv[0], 1,
                                   status.no_named_instances ? 1 : 0 );
      if ( error == FT_Err_Invalid_Argument )
        fprintf( stderr, "skipping font `%s' without outlines\n",
                         argv[0] );
    }

    if ( handle->num_fonts == 0 )
      Fatal( "could not find/open any font file" );

    event_font_change( 0 );

    grid_status_rescale_initial( &status, handle );

    for ( ;; )
    {
      FTDemo_Display_Clear( display );

      if ( status.do_grid )
        grid_status_draw_grid( &status );

      if ( status.work )
        grid_status_draw_outline( &status, handle, display );

      write_header( 0 );

      grListenSurface( display->surface, 0, &event );
      if ( Process_Event( &event ) )
        break;
    }

    printf( "Execution completed successfully.\n" );

    for ( n = 0; n < MAX_MM_AXES; n++ )
      free( status.axis_name[n] );
    FT_Done_MM_Var( handle->library, status.mm );

    FT_Stroker_Done( status.stroker );
    FTDemo_Display_Done( display );
    FTDemo_Done( handle );

    exit( 0 );      /* for safety reasons */
    /* return 0; */ /* never reached */
  }


/* End */
