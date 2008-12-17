/****************************************************************************/
/*                                                                          */
/*  The FreeType project -- a free and portable quality TrueType renderer.  */
/*                                                                          */
/*  Copyright 1996-2000, 2003, 2004, 2005, 2006, 2007 by                    */
/*  D. Turner, R.Wilhelm, and W. Lemberg                                    */
/*                                                                          */
/*                                                                          */
/*  FTGrid - a simple viewer to debug the auto-hinter                       */
/*                                                                          */
/*  Press F1 when running this program to have a list of key-bindings       */
/*                                                                          */
/****************************************************************************/


#include "ftcommon.h"
#include "common.h"
#include <math.h>

  /* the following header shouldn't be used in normal programs */
#include FT_INTERNAL_DEBUG_H
#include FT_STROKER_H
#include FT_SYNTHESIS_H

#define MAXPTSIZE      500                 /* dtp */
#define HEADER_HEIGHT  8

#ifdef CEIL
#undef CEIL
#endif
#define CEIL( x )   ( ( (x) + 63 ) >> 6 )

#define X_TOO_LONG( x, size, display) \
          ( ( x ) + ( ( size )->metrics.max_advance >> 6 ) > ( display )->bitmap->width )
#define Y_TOO_LONG( y, size, display) \
          ( ( y ) >= ( display )->bitmap->rows )

#ifdef _WIN32
#define snprintf  _snprintf
#endif


/* these variables, structures and declarations  are for  */
/* communication with the debugger in the autofit module; */
/* normal programs don't need this                        */
struct  AF_GlyphHintsRec_;
typedef struct AF_GlyphHintsRec_*  AF_GlyphHints;

int            _af_debug;
int            _af_debug_disable_horz_hints;
int            _af_debug_disable_vert_hints;
int            _af_debug_disable_blue_hints;
AF_GlyphHints  _af_debug_hints;

extern void af_glyph_hints_dump_segments( AF_GlyphHints  hints );
extern void af_glyph_hints_dump_points( AF_GlyphHints  hints );
extern void af_glyph_hints_dump_edges( AF_GlyphHints  hints );

typedef struct  status_
{
  int          ptsize;
  int          res;
  int          Num;  /* glyph index */
  int          font_index;

  double       scale;
  double       x_origin;
  double       y_origin;
  double       margin;

  double       scale_0;
  double       x_origin_0;
  double       y_origin_0;

  int          disp_width;
  int          disp_height;
  grBitmap*    disp_bitmap;

  grColor      axis_color;
  grColor      grid_color;
  grColor      outline_color;
  grColor      on_color;
  grColor      conic_color;
  grColor      cubic_color;

  int          do_horz_hints;
  int          do_vert_hints;
  int          do_blue_hints;
  int          do_outline;
  int          do_dots;

  double       gamma;
  const char*  header;
  char         header_buffer[256];

} GridStatusRec, *GridStatus;

static GridStatusRec  status;

static void
grid_status_init( GridStatus       st,
                  FTDemo_Display*  display )
{
  st->scale         = 1.0;
  st->x_origin      = display->bitmap->width / 4;
  st->y_origin      = display->bitmap->rows / 4;
  st->margin        = 0.05;
  st->axis_color    = grFindColor( display->bitmap,   0,   0,   0, 255 );
  st->grid_color    = grFindColor( display->bitmap, 192, 192, 192, 255 );
  st->outline_color = grFindColor( display->bitmap, 255,   0,   0, 255 );
  st->on_color      = grFindColor( display->bitmap,  64,  64, 255, 255 );
  st->conic_color   = grFindColor( display->bitmap,   0, 128,   0, 255 );
  st->cubic_color   = grFindColor( display->bitmap, 255,  64, 255, 255 );
  st->disp_width    = display->bitmap->width;
  st->disp_height   = display->bitmap->rows;
  st->disp_bitmap   = display->bitmap;

  st->do_horz_hints = 1;
  st->do_vert_hints = 1;
  st->do_blue_hints = 1;
  st->do_dots       = 1;
  st->do_outline    = 1;

  st->Num    = 0;
  st->gamma  = 1.0;
  st->header = "";
}


static void
grid_status_rescale_initial( GridStatus      st,
                             FTDemo_Handle*  handle )
{
  FT_Size   size;
  FT_Error  err = FTDemo_Get_Size( handle, &size );


  if ( !err )
  {
    FT_Face  face = size->face;

    int  xmin = FT_MulFix( face->bbox.xMin, size->metrics.x_scale );
    int  ymin = FT_MulFix( face->bbox.yMin, size->metrics.y_scale );
    int  xmax = FT_MulFix( face->bbox.xMax, size->metrics.x_scale );
    int  ymax = FT_MulFix( face->bbox.yMax, size->metrics.y_scale );

    double  x_scale, y_scale;


    xmin &= ~63;
    ymin &= ~63;
    xmax  = ( xmax + 63 ) & ~63;
    ymax  = ( ymax + 63 ) & ~63;

    printf( "XXX x_ppem=%d y_ppem=%d width=%d height=%d\n",
            size->metrics.x_ppem, size->metrics.y_ppem,
            xmax - xmin, ymax - ymin );

    x_scale = st->disp_width  * ( 1.0 - 2 * st->margin ) / ( xmax - xmin );
    y_scale = st->disp_height * ( 1.0 - 2 * st->margin ) / ( ymax - ymin );

    if ( x_scale <= y_scale )
      st->scale = x_scale;
    else
      st->scale = y_scale;

    st->x_origin = st->disp_width  * st->margin         - xmin * st->scale;
    st->y_origin = st->disp_height * ( 1 - st->margin ) + ymin * st->scale;
  }
  else
  {
    st->scale    = 1.;
    st->x_origin = st->disp_width  * st->margin;
    st->y_origin = st->disp_height * st->margin;
  }

  st->scale_0    = st->scale;
  st->x_origin_0 = st->x_origin;
  st->y_origin_0 = st->y_origin;
}


static void
grid_status_draw_grid( GridStatus  st )
{
  int     x_org   = (int)st->x_origin;
  int     y_org   = (int)st->y_origin;
  double  xy_incr = 64.0 * st->scale;

  if ( xy_incr >= 2. )
  {
    double  x2 = x_org;
    double  y2 = y_org;

    for ( ; x2 < st->disp_width; x2 += xy_incr )
      grFillVLine( st->disp_bitmap, (int)x2, 0,
                   st->disp_height, st->grid_color );

    for ( x2 = x_org - xy_incr; (int)x2 >= 0; x2 -= xy_incr )
      grFillVLine( st->disp_bitmap, (int)x2, 0,
                   st->disp_height, st->grid_color );

    for ( ; y2 < st->disp_height; y2 += xy_incr )
      grFillHLine( st->disp_bitmap, 0, (int)y2,
                   st->disp_width, st->grid_color );

    for ( y2 = y_org - xy_incr; (int)y2 >= 0; y2 -= xy_incr )
      grFillHLine( st->disp_bitmap, 0, (int)y2,
                   st->disp_width, st->grid_color );
  }

  grFillVLine( st->disp_bitmap, x_org, 0, st->disp_height, st->axis_color );
  grFillHLine( st->disp_bitmap, 0, y_org, st->disp_width,  st->axis_color );
}


static void
ft_bitmap_draw( FT_Bitmap*       bitmap,
                int              x,
                int              y,
                FTDemo_Display*  display,
                grColor          color)
{
  grBitmap  gbit;

  gbit.width  = bitmap->width;
  gbit.rows   = bitmap->rows;
  gbit.pitch  = bitmap->pitch;
  gbit.buffer = bitmap->buffer;

  switch ( bitmap->pixel_mode)
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
                  outline->n_points,
                  outline->n_contours,
                  &transformed );

  FT_Outline_Copy( outline, &transformed );

  if ( scale != 1. )
  {
    int  nn;

    for ( nn = 0; nn < transformed.n_points; nn++ )
    {
      FT_Vector*  vec = &transformed.points[nn];

      vec->x = (FT_F26Dot6)(vec->x*scale);
      vec->y = (FT_F26Dot6)(vec->y*scale);
    }
  }

  FT_Outline_Get_CBox( &transformed, &cbox );
  cbox.xMin &= ~63;
  cbox.yMin &= ~63;
  cbox.xMax  = (cbox.xMax + 63) & ~63;
  cbox.yMax  = (cbox.yMax + 63) & ~63;

  bitm.width      = (cbox.xMax - cbox.xMin) >> 6;
  bitm.rows       = (cbox.yMax - cbox.yMin) >> 6;
  bitm.pitch      = bitm.width;
  bitm.num_grays  = 256;
  bitm.pixel_mode = FT_PIXEL_MODE_GRAY;
  bitm.buffer     = (unsigned char*)calloc( bitm.pitch, bitm.rows );

  FT_Outline_Translate( &transformed, -cbox.xMin, -cbox.yMin );
  FT_Outline_Get_Bitmap( handle->library, &transformed, &bitm );

  ft_bitmap_draw( &bitm,
                  pen_x + (cbox.xMin >> 6),
                  pen_y - (cbox.yMax >> 6),
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
  FT_F26Dot6  disp = (FT_F26Dot6)( radius * 0.6781 );

  FT_Outline_New( handle->library, 12, 1, outline );
  outline->n_points   = 12;
  outline->n_contours = 1;
  outline->contours[0] = outline->n_points-1;

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
  FT_Outline_Translate( &outline, center_x & 63, center_y & 63 );

  ft_outline_draw( &outline, 1., (center_x >> 6), (center_y >> 6), handle, display, color );

  FT_Outline_Done( handle->library, &outline );
}


static void
grid_status_draw_outline( GridStatus       st,
                          FTDemo_Handle*   handle,
                          FTDemo_Display*  display )
{
  static FT_Stroker  stroker;
  FT_Size            size;
  FT_GlyphSlot       slot;
  double             scale = 64.0 * st->scale;
  int                ox    = (int)st->x_origin;
  int                oy    = (int)st->y_origin;


  if ( stroker == NULL )
  {
    FT_Stroker_New( handle->library, &stroker );

    FT_Stroker_Set( stroker, 32, FT_STROKER_LINECAP_BUTT,
                    FT_STROKER_LINEJOIN_ROUND, 0x20000 );
  }

  FTDemo_Get_Size( handle, &size );

  _af_debug_disable_horz_hints = !st->do_horz_hints;
  _af_debug_disable_vert_hints = !st->do_vert_hints;

  FT_Load_Glyph( size->face, st->Num,
                 handle->load_flags | FT_LOAD_NO_BITMAP );

  slot = size->face->glyph;
  if ( slot->format == FT_GLYPH_FORMAT_OUTLINE )
  {
    FT_Glyph     glyph;
    FT_Outline*  gimage = &slot->outline;
    int          nn;


    /* scale the outline */
    for (nn = 0; nn < gimage->n_points; nn++)
    {
      FT_Vector*  vec = &gimage->points[nn];


      vec->x = (FT_F26Dot6)( vec->x * scale );
      vec->y = (FT_F26Dot6)( vec->y * scale );
    }

    /* stroke then draw it */
    if ( st->do_outline )
    {
      FT_Get_Glyph( slot, &glyph );
      FT_Glyph_Stroke( &glyph, stroker, 1 );

      FTDemo_Draw_Glyph_Color( handle, display, glyph, &ox, &oy,
                               st->outline_color );
      FT_Done_Glyph( glyph );
    }

    /* now draw the points */
    if ( st->do_dots )
    {
      for (nn = 0; nn < gimage->n_points; nn++)
        circle_draw( (FT_F26Dot6)( st->x_origin * 64 + gimage->points[nn].x ),
                     (FT_F26Dot6)( st->y_origin * 64 - gimage->points[nn].y ),
                     128,
                     handle,
                     display,
                     ( gimage->tags[nn] & FT_CURVE_TAG_ON )
                       ? st->on_color
                       : st->conic_color );
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
  Fatal( const char* message )
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
    grEvent  dummy_event;


    FTDemo_Display_Clear( display );
    grGotoxy( 0, 0 );
    grSetMargin( 2, 1 );
    grGotobitmap( display->bitmap );

    grWriteln( "FreeType Glyph Viewer - part of the FreeType test suite" );
    grLn();
    grWriteln( "This program is used to display all glyphs from one or" );
    grWriteln( "several font files, with the FreeType library." );
    grLn();
    grWriteln( "Use the following keys:" );
    grLn();
    grWriteln( "  F1 or ?    : display this help screen" );
    grLn();
    grWriteln( "  a          : toggle anti-aliasing" );
    grWriteln( "  left/right : decrement/increment glyph index" );
    grWriteln( "  up/down    : change character size" );
    grLn();
    grWriteln( "  F7         : decrement index by 10" );
    grWriteln( "  F8         : increment index by 10" );
    grWriteln( "  F9         : decrement index by 100" );
    grWriteln( "  F10        : increment index by 100" );
    grWriteln( "  F11        : decrement index by 1000" );
    grWriteln( "  F12        : increment index by 1000" );
    grLn();
    grWriteln( "  i          : move grid up" );
    grWriteln( "  j          : move grid left" );
    grWriteln( "  k          : move grid down" );
    grWriteln( "  l          : move grid right" );
    grWriteln( "  Page up/dn : zoom in/out grid" );
    grWriteln( "  RETURN     : reset zoom and position" );
    grLn();
    grWriteln( "  H          : toggle horizontal hinting" );
    grWriteln( "  V          : toggle vertical hinting" );
    grWriteln( "  B          : toggle blue zone hinting" );
    grWriteln( "  d          : toggle dots display" );
    grWriteln( "  o          : toggle outline display" );
    grWriteln( "  g          : increase gamma by 0.1" );
    grWriteln( "  v          : decrease gamma by 0.1" );
    grLn();
    grWriteln( "  n          : next font" );
    grWriteln( "  p          : previous font" );
    grWriteln( "  q / ESC    : exit program" );
    grLn();
    grWriteln( "  1          : dump edge hints" );
    grWriteln( "  2          : dump segment hints" );
    grWriteln( "  3          : dump point hints" );
    grLn();
    grWriteln( "press any key to exit this help screen" );

    grRefreshSurface( display->surface );
    grListenSurface( display->surface, gr_event_key, &dummy_event );
  }


  static void
  event_gamma_change( double delta )
  {
    status.gamma += delta;

    if ( status.gamma > 3.0 )
      status.gamma = 3.0;
    else if ( status.gamma < 0.0 )
      status.gamma = 0.0;

    grSetGlyphGamma( status.gamma );

    sprintf( status.header_buffer, "gamma changed to %.1f%s",
             status.gamma, status.gamma == 0.0 ? " (sRGB mode)" : "" );

    status.header = (const char *)status.header_buffer;
  }


  static void
  event_grid_reset( GridStatus  st )
  {
    st->x_origin = st->x_origin_0;
    st->y_origin = st->y_origin_0;
    st->scale    = st->scale_0;
  }


  static void
  event_grid_translate( int  dx, int  dy )
  {
    status.x_origin += 32*dx;
    status.y_origin += 32*dy;
  }

  static void
  event_grid_zoom( double  zoom )
  {
    status.scale *= zoom;

    sprintf( status.header_buffer, "zoom level %.2f %%\n",
             status.scale / status.scale_0 );

    status.header = (const char *)status.header_buffer;
  }


  static void
  event_size_change( int delta )
  {
    status.ptsize += delta;

    if ( status.ptsize < 1*64 )
      status.ptsize = 1*64;
    else if ( status.ptsize > MAXPTSIZE*64 )
      status.ptsize = MAXPTSIZE*64;

    FTDemo_Set_Current_Charsize( handle, status.ptsize, status.res );
  }


  static void
  event_index_change( int delta )
  {
    int num_indices = handle->current_font->num_indices;


    status.Num += delta;

    if ( status.Num < 0 )
      status.Num = 0;
    else if ( status.Num >= num_indices )
      status.Num = num_indices - 1;
  }



  static void
  event_font_change( int  delta )
  {
    int      num_indices;


    if ( status.font_index + delta >= handle->num_fonts ||
         status.font_index + delta < 0 )
      return;

    status.font_index += delta;

    FTDemo_Set_Current_Font( handle, handle->fonts[status.font_index] );
    FTDemo_Set_Current_Charsize( handle, status.ptsize, status.res );
    FTDemo_Update_Current_Flags( handle );

    num_indices = handle->current_font->num_indices;

    if ( status.Num >= num_indices )
      status.Num = num_indices - 1;
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

    case grKEY( 'a' ):
      handle->antialias = !handle->antialias;
      status.header     = handle->antialias ? "anti-aliasing is now on"
                                            : "anti-aliasing is now off";

      FTDemo_Update_Current_Flags( handle );
      break;

    case grKEY( '1' ):
      af_glyph_hints_dump_edges( _af_debug_hints );
      break;

    case grKEY( '2' ):
      af_glyph_hints_dump_segments( _af_debug_hints );
      break;

    case grKEY( '3' ):
      af_glyph_hints_dump_points( _af_debug_hints );
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

      FTDemo_Update_Current_Flags( handle );
      break;

    case grKEY( 'd' ):
      status.do_dots = !status.do_dots;
      break;

    case grKEY( 'o' ):
      status.do_outline = !status.do_outline;
      break;

    case grKEY( 'p' ):
      event_font_change( -1 );
      break;

    case grKEY('H'):
      status.do_horz_hints = !status.do_horz_hints;
      status.header = status.do_horz_hints ? "horizontal hinting enabled"
                                           : "horizontal hinting disabled";
      break;

    case grKEY('V'):
      status.do_vert_hints = !status.do_vert_hints;
      status.header        = status.do_vert_hints
                             ? "vertical hinting enabled"
                             : "vertical hinting disabled";
      break;

    case grKEY('B'):
      status.do_blue_hints = !status.do_blue_hints;
      status.header        = status.do_blue_hints
                             ? "blue zone hinting enabled"
                             : "blue zone hinting disabled";
      break;


    case grKeyLeft:     event_index_change( -1 ); break;
    case grKeyRight:    event_index_change( +1 ); break;
    case grKeyF7:       event_index_change(   -10 ); break;
    case grKeyF8:       event_index_change(    10 ); break;
    case grKeyF9:       event_index_change(  -100 ); break;
    case grKeyF10:      event_index_change(   100 ); break;
    case grKeyF11:      event_index_change( -1000 ); break;
    case grKeyF12:      event_index_change(  1000 ); break;

    case grKeyUp:       event_size_change( +32 ); break;
    case grKeyDown:     event_size_change( -32 ); break;

    case grKEY( ' ' ):  event_grid_reset( &status );
                        status.do_horz_hints = 1;
                        status.do_vert_hints = 1;
                        break;

    case grKEY('i'):    event_grid_translate( 0, +1 ); break;
    case grKEY('k'):    event_grid_translate( 0, -1 ); break;
    case grKEY('l'):    event_grid_translate( +1, 0 ); break;
    case grKEY('j'):    event_grid_translate( -1, 0 ); break;

    case grKeyPageUp:   event_grid_zoom( 1.25 ); break;
    case grKeyPageDown: event_grid_zoom( 1/1.25 ); break;

    default:
      ;
    }

    return ret;
  }


  static void
  write_header( FT_Error error_code )
  {
    FT_Face      face;
    const char*  basename;
    const char*  format;


    error = FTC_Manager_LookupFace( handle->cache_manager,
                                    handle->scaler.face_id, &face );
    if ( error )
      Fatal( "can't access font file" );

    if ( !status.header )
    {
      basename = ft_basename( handle->current_font->filepathname );

      switch ( error_code )
      {
      case FT_Err_Ok:
        sprintf( status.header_buffer, "%s %s (file `%s')",
                 face->family_name, face->style_name, basename );
        break;

      case FT_Err_Invalid_Pixel_Size:
        sprintf( status.header_buffer, "Invalid pixel size (file `%s')",
                 basename );
        break;

      case FT_Err_Invalid_PPem:
        sprintf( status.header_buffer, "Invalid ppem value (file `%s')",
                 basename );
        break;

      default:
        sprintf( status.header_buffer, "File `%s': error 0x%04x",
                 basename, (FT_UShort)error_code );
        break;
      }

      status.header = (const char *)status.header_buffer;
    }

    grWriteCellString( display->bitmap, 0, 0, status.header,
                       display->fore_color );

    format = "at %g points, first glyph index = %d";

    snprintf( status.header_buffer, 256, format, status.ptsize/64., status.Num );

    if ( FT_HAS_GLYPH_NAMES( face ) )
    {
      char*  p;
      int    format_len, gindex, size;


      size = strlen( status.header_buffer );
      p    = status.header_buffer + size;
      size = 256 - size;

      format = ", name = ";
      format_len = strlen( format );

      if ( size >= format_len + 2 )
      {
        gindex = status.Num;

        strcpy( p, format );
        if ( FT_Get_Glyph_Name( face, gindex, p + format_len, size - format_len ) )
          *p = '\0';
      }
    }

    status.header = (const char *)status.header_buffer;
    grWriteCellString( display->bitmap, 0, HEADER_HEIGHT,
                       status.header_buffer, display->fore_color );

    grRefreshSurface( display->surface );
  }


  static void
  usage( char*  execname )
  {
    fprintf( stderr,  "\n" );
    fprintf( stderr,  "ftgrid: simple glyph grid viewer -- part of the FreeType project\n" );
    fprintf( stderr,  "-----------------------------------------------------------\n" );
    fprintf( stderr,  "\n" );
    fprintf( stderr,  "Usage: %s [status below] ppem fontname[.ttf|.ttc] ...\n",
             execname );
    fprintf( stderr,  "\n" );
    fprintf( stderr,  "  -r R      use resolution R dpi (default: 72 dpi)\n" );
    fprintf( stderr,  "  -f index  specify first index to display\n" );
    fprintf( stderr,  "\n" );

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
      option = getopt( *argc, *argv, "f:r:" );

      if ( option == -1 )
        break;

      switch ( option )
      {
      case 'f':
        status.Num  = atoi( optarg );
        break;

      case 'r':
        status.res = atoi( optarg );
        if ( status.res < 1 )
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

    status.ptsize = (int)(atof( *argv[0] ) * 64.0);
    if ( status.ptsize == 0 )
      status.ptsize = 64*10;

    if ( status.res <= 0 )
      status.res = 72;

    (*argc)--;
    (*argv)++;
  }


  int
  main( int    argc,
        char*  argv[] )
  {
    grEvent      event;

    display = FTDemo_Display_New( gr_pixel_mode_rgb24 );
    if ( !display )
      Fatal( "could not allocate display surface" );

    memset( display->fore_color.chroma, 0, 4 );
    memset( display->back_color.chroma, 0xff, 4 );
    grSetTitle( display->surface, "FreeType Glyph Grid Viewer - press F1 for help" );

    grid_status_init( &status, display );

    parse_cmdline( &argc, &argv );

    /* Initialize engine */
    handle = FTDemo_New( FT_ENCODING_NONE );

    for ( ; argc > 0; argc--, argv++ )
      FTDemo_Install_Font( handle, argv[0] );

    if ( handle->num_fonts == 0 )
      Fatal( "could not find/open any font file" );

    printf( "ptsize =%g \n", status.ptsize/64.0 );
    FTDemo_Set_Current_Charsize( handle, status.ptsize, status.res );
    FTDemo_Update_Current_Flags( handle );

    event_font_change( 0 );

    grid_status_rescale_initial( &status, handle );

    _af_debug = 1;

    for ( ;; )
    {
      FTDemo_Display_Clear( display );

      grid_status_draw_grid( &status );

      if ( status.do_outline || status.do_dots )
        grid_status_draw_outline( &status, handle, display );

      write_header( 0 );

      grListenSurface( display->surface, 0, &event );
      if ( Process_Event( &event ) )
        break;
    }

    printf( "Execution completed successfully.\n" );

    FTDemo_Display_Done( display );
    FTDemo_Done( handle );
    exit( 0 );      /* for safety reasons */

    return 0;       /* never reached */
  }


/* End */
