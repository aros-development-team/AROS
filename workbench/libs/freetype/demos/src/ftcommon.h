/****************************************************************************/
/*                                                                          */
/*  The FreeType project -- a free and portable quality TrueType renderer.  */
/*                                                                          */
/*  Copyright 2005, 2006 by                                                 */
/*  D. Turner, R.Wilhelm, and W. Lemberg                                    */
/*                                                                          */
/*                                                                          */
/*  ftcommon.h - common routines for the FreeType demo programs.            */
/*                                                                          */
/****************************************************************************/

#ifndef _FT_COMMON_H_
#define _FT_COMMON_H_


#include <ft2build.h>
#include FT_FREETYPE_H

#include FT_CACHE_H
#include FT_CACHE_MANAGER_H

#include FT_GLYPH_H
#include FT_BITMAP_H

#include <stdlib.h>
#include <stdarg.h>

  extern FT_Error   error;

  /* forward declarations */
  extern void  PanicZ( const char*  message );

#undef  NODEBUG

#ifndef NODEBUG

#define LOG( x )  LogMessage##x


  void
  LogMessage( const char*  fmt, ... );

#else /* !DEBUG */

#define LOG( x )  /* */

#endif


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                 DISPLAY-SPECIFIC DEFINITIONS                  *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/


#include "graph.h"
#include "grobjs.h"
#include "grfont.h"

  typedef struct
  {
    grSurface*  surface;
    grBitmap*   bitmap;
    grColor     fore_color;
    grColor     back_color;

  } FTDemo_Display;


  FTDemo_Display*
  FTDemo_Display_New( grPixelMode  mode );


  void
  FTDemo_Display_Done( FTDemo_Display*  display );


  /* fill the bitmap with background color */
  void
  FTDemo_Display_Clear( FTDemo_Display*  display );


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****               FREETYPE-SPECIFIC DEFINITIONS                   *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/

#define MAX_GLYPHS 512            /* at most 512 glyphs in the string */
#define MAX_GLYPH_BYTES  150000   /* 150kB for the glyph image cache */


  typedef struct  TGlyph_
  {
    FT_UInt    glyph_index;
    FT_Glyph   image;    /* the glyph image */

    FT_Pos     delta;    /* delta caused by hinting */
    FT_Vector  vvector;  /* vert. origin => hori. origin */
    FT_Vector  vadvance; /* vertical advance */

  } TGlyph, *PGlyph;

  /* this simple record is used to model a given `installed' face */
  typedef struct  TFont_
  {
    const char*  filepathname;
    int          face_index;
    int          cmap_index;
    int          num_indices;
    void*        file_address;  /* for preloaded files */
    size_t       file_size;

  } TFont, *PFont;

  enum {
    LCD_MODE_AA = 0,
    LCD_MODE_LIGHT,
    LCD_MODE_RGB,
    LCD_MODE_BGR,
    LCD_MODE_VRGB,
    LCD_MODE_VBGR,
    N_LCD_MODES
  };

  enum {
    KERNING_DEGREE_NONE = 0,
    KERNING_DEGREE_LIGHT,
    KERNING_DEGREE_MEDIUM,
    KERNING_DEGREE_TIGHT,
    N_KERNING_DEGREES
  };

  enum {
    KERNING_MODE_NONE = 0,      /* 0: no kerning;                  */
    KERNING_MODE_NORMAL,        /* 1: `kern' values                */
    KERNING_MODE_SMART,         /* 2: `kern' + side bearing errors */
    N_KERNING_MODES
  };

  typedef struct
  {
    int         kerning_mode;
    int         kerning_degree;
    FT_Fixed    center;            /* 0..1 */
    int         vertical;          /* displayed vertically? */
    FT_Matrix*  matrix;            /* string transformation */
    FT_Byte*    gamma_ramp;        /* an array of size 256 */

  } FTDemo_String_Context;

  typedef struct
  {
    FT_Library        library;           /* the FreeType library            */
    FTC_Manager       cache_manager;     /* the cache manager               */
    FTC_ImageCache    image_cache;       /* the glyph image cache           */
    FTC_SBitCache     sbits_cache;       /* the glyph small bitmaps cache   */
    FTC_CMapCache     cmap_cache;        /* the charmap cache..             */

    PFont*            fonts;             /* installed fonts */
    int               num_fonts;
    int               max_fonts;

    int               use_sbits_cache;   /* toggle sbits cache */

    /* use FTDemo_Set_Current_XXX to set the following two fields */
    PFont             current_font;      /* selected font */
    FTC_ScalerRec     scaler;
    FT_ULong          load_flags;

    /* call FTDemo_Update_Current_Flags after setting any of the following fields */
    int               hinted;            /* is glyph hinting active?    */
    int               antialias;         /* is anti-aliasing active?    */
    int               use_sbits;         /* do we use embedded bitmaps? */
    int               low_prec;          /* force low precision         */
    int               autohint;          /* force auto-hinting          */
    int               lcd_mode;
    int               preload;           /* force font file preloading */

    /* don't touch the following fields! */

    /* used for string rendering */
    TGlyph            string[MAX_GLYPHS];
    int               string_length;
    int               string_reload;

    FT_Encoding       encoding;
    FT_Bitmap         bitmap;            /* used as bitmap conversion buffer */

  } FTDemo_Handle;


  FTDemo_Handle*
  FTDemo_New( FT_Encoding  encoding );


  void
  FTDemo_Done( FTDemo_Handle*  handle );


  /* install a font */
  FT_Error
  FTDemo_Install_Font( FTDemo_Handle*  handle,
                       const char*     filepath );


  void
  FTDemo_Set_Preload( FTDemo_Handle*  handle,
                      int             preload );

  void
  FTDemo_Set_Current_Font( FTDemo_Handle*  handle,
                           PFont           font );

  void
  FTDemo_Set_Current_Size( FTDemo_Handle*  handle,
                           int             pixel_size );

  void
  FTDemo_Set_Current_Pointsize( FTDemo_Handle*  handle,
                                int             point_size,
                                int             res );

  void
  FTDemo_Set_Current_Charsize( FTDemo_Handle*  handle,
                               int             point_size,
                               int             res );

  void
  FTDemo_Update_Current_Flags( FTDemo_Handle*  handle );


  /* charcode => glyph index of current font */
  FT_UInt
  FTDemo_Get_Index( FTDemo_Handle*  handle,
                    FT_UInt32       charcode );


  /* get FT_Size of current font */
  FT_Error
  FTDemo_Get_Size( FTDemo_Handle*  handle,
                   FT_Size*        asize );


  /* convert a FT_Glyph to a grBitmap (don't free target->buffer) */
  /* if aglyf != NULL, you should FT_Glyph_Done the aglyf */
  FT_Error
  FTDemo_Glyph_To_Bitmap( FTDemo_Handle*  handle,
                          FT_Glyph        glyf,
                          grBitmap*       target,
                          int*            left,
                          int*            top,
                          int*            x_advance,
                          int*            y_advance,
                          FT_Glyph*       aglyf );

  /* get a grBitmap from glyph index (don't free target->buffer) */
  /* if aglyf != NULL, you should FT_Glyph_Done the aglyf */
  FT_Error
  FTDemo_Index_To_Bitmap( FTDemo_Handle*  handle,
                          FT_ULong        Index,
                          grBitmap*       target,
                          int*            left,
                          int*            top,
                          int*            x_advance,
                          int*            y_advance,
                          FT_Glyph*       aglyf );


  /* given glyph index, draw a glyph on the display */
  FT_Error
  FTDemo_Draw_Index( FTDemo_Handle*   handle,
                     FTDemo_Display*  display,
                     int              gindex,
                     int*             pen_x,
                     int*             pen_y);


  /* given FT_Glyph, draw a glyph on the display */
  FT_Error
  FTDemo_Draw_Glyph( FTDemo_Handle*   handle,
                     FTDemo_Display*  display,
                     FT_Glyph         glyph,
                     int*             pen_x,
                     int*             pen_y);

  FT_Error
  FTDemo_Draw_Glyph_Color( FTDemo_Handle*   handle,
                           FTDemo_Display*  display,
                           FT_Glyph         glyph,
                           int*             pen_x,
                           int*             pen_y,
                           grColor          color );

  /* given FT_GlyphSlot, draw a glyph on the display */
  FT_Error
  FTDemo_Draw_Slot( FTDemo_Handle*   handle,
                    FTDemo_Display*  display,
                    FT_GlyphSlot     slot,
                    int*             pen_x,
                    int*             pen_y);


  /* set the string to be drawn */
  void
  FTDemo_String_Set( FTDemo_Handle*        handle,
                     const unsigned char*  string );


  /* draw a string centered at (center_x, center_y) --  */
  /* note that handle->use_sbits_cache is not supported */
  FT_Error
  FTDemo_String_Draw( FTDemo_Handle*          handle,
                      FTDemo_Display*         display,
                      FTDemo_String_Context*  sc,
                      int                     center_x,
                      int                     center_y );


  /* make a FT_Encoding tag from a string */
  FT_Encoding
  FTDemo_Make_Encoding_Tag( const char*  s );


#endif /* _FTCOMMON_H_ */

/* End */
