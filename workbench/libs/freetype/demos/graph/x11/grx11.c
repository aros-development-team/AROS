/*******************************************************************
 *
 *  grx11.c  graphics driver for X11.
 *
 *  This is the driver for displaying inside a window under X11,
 *  used by the graphics utility of the FreeType test suite.
 *
 *  Copyright 1999-2000, 2001, 2002 by Antoine Leca, David Turner
 *  David Turner, Robert Wilhelm, and Werner Lemberg.
 *
 *  This file is part of the FreeType project, and may only be used
 *  modified and distributed under the terms of the FreeType project
 *  license, LICENSE.TXT. By continuing to use, modify or distribute
 *  this file you indicate that you have read the license and
 *  understand and accept it fully.
 *
 ******************************************************************/

#include <grobjs.h>
#include <grdevice.h>

#define TEST

#ifdef TEST
#include "grfont.h"
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include <X11/keysym.h>


#if defined( __cplusplus ) || defined( c_plusplus )
#define Class  c_class
#else
#define Class  class
#endif

  /* old trick to determine 32-bit integer type */
#include <limits.h>

  /* The number of bytes in an `int' type.  */
#if   UINT_MAX == 0xFFFFFFFFUL
#define GR_SIZEOF_INT  4
#elif UINT_MAX == 0xFFFFU
#define GR_SIZEOF_INT  2
#elif UINT_MAX > 0xFFFFFFFFU && UINT_MAX == 0xFFFFFFFFFFFFFFFFU
#define GR_SIZEOF_INT  8
#else
#error "Unsupported number of bytes in `int' type!"
#endif

  /* The number of bytes in a `long' type.  */
#if   ULONG_MAX == 0xFFFFFFFFUL
#define GR_SIZEOF_LONG  4
#elif ULONG_MAX > 0xFFFFFFFFU && ULONG_MAX == 0xFFFFFFFFFFFFFFFFU
#define GR_SIZEOF_LONG  8
#else
#error "Unsupported number of bytes in `long' type!"
#endif

#if GR_SIZEOF_INT == 4
typedef  int             int32;
typedef  unsigned int    uint32;
#elif GR_SIZEOF_LONG == 4
typedef  long            int32;
typedef  unsigned long   uint32;
#else
#error  "could not find a 32-bit integer type"
#endif


  typedef struct  Translator
  {
    KeySym  xkey;
    grKey   grkey;

  } Translator;


  static
  Translator  key_translators[] =
  {
    { XK_BackSpace, grKeyBackSpace },
    { XK_Tab,       grKeyTab       },
    { XK_Return,    grKeyReturn    },
    { XK_Escape,    grKeyEsc       },
    { XK_Home,      grKeyHome      },
    { XK_Left,      grKeyLeft      },
    { XK_Up,        grKeyUp        },
    { XK_Right,     grKeyRight     },
    { XK_Down,      grKeyDown      },
    { XK_Page_Up,   grKeyPageUp    },
    { XK_Page_Down, grKeyPageDown  },
    { XK_End,       grKeyEnd       },
    { XK_Begin,     grKeyHome      },
    { XK_F1,        grKeyF1        },
    { XK_F2,        grKeyF2        },
    { XK_F3,        grKeyF3        },
    { XK_F4,        grKeyF4        },
    { XK_F5,        grKeyF5        },
    { XK_F6,        grKeyF6        },
    { XK_F7,        grKeyF7        },
    { XK_F8,        grKeyF8        },
    { XK_F9,        grKeyF9        },
    { XK_F10,       grKeyF10       },
    { XK_F11,       grKeyF11       },
    { XK_F12,       grKeyF12       }
  };

  typedef XPixmapFormatValues  XDepth;


#ifdef TEST
#define grAlloc  malloc
#endif


  /************************************************************************/
  /************************************************************************/
  /*****                                                              *****/
  /*****                   PIXEL BLITTING SUPPORT                     *****/
  /*****                                                              *****/
  /************************************************************************/
  /************************************************************************/

  typedef struct grX11Blitter_
  {
    unsigned char*  src_line;
    int             src_pitch;

    unsigned char*  dst_line;
    int             dst_pitch;

    int             x;
    int             y;
    int             width;
    int             height;

  } grX11Blitter;


  /* setup blitter; returns 1 if no drawing happens */
  static int
  gr_x11_blitter_reset( grX11Blitter*  blit,
                        grBitmap*      source,
                        grBitmap*      target,
                        int            x,
                        int            y,
                        int            width,
                        int            height )
  {
    long  pitch;
    int   delta;


    /* clip rectangle to source bitmap */
    if ( x < 0 )
    {
      width += x;
      x      = 0;
    }

    delta = x + width - source->width;
    if ( delta > 0 )
      width -= delta;

    if ( y < 0 )
    {
      height += y;
      y       = 0;
    }

    delta = y + height - source->rows;
    if ( delta > 0 )
      height -= delta;

    /* clip rectangle to target bitmap */
    delta = x + width - target->width;
    if ( delta > 0 )
      width -= delta;

    delta = y + height - target->rows;
    if ( delta > 0 )
      height -= delta;

    if ( width <= 0 || height <= 0 )
      return 1;

    /* now, setup the blitter */
    pitch = blit->src_pitch = source->pitch;

    blit->src_line  = source->buffer + y * pitch;
    if ( pitch < 0 )
      blit->src_line -= ( source->rows - 1 ) * pitch;

    pitch = blit->dst_pitch = target->pitch;

    blit->dst_line = target->buffer + y * pitch;
    if ( pitch < 0 )
      blit->dst_line -= ( target->rows - 1 ) * pitch;

    blit->x      = x;
    blit->y      = y;
    blit->width  = width;
    blit->height = height;

    return 0;
  }


  typedef void  (*grX11ConvertFunc)( grX11Blitter*  blit );

  typedef struct grX11FormatRec_
  {
    int             x_depth;
    int             x_bits_per_pixel;
    unsigned long   x_red_mask;
    unsigned long   x_green_mask;
    unsigned long   x_blue_mask;

    grX11ConvertFunc  rgb_convert;
    grX11ConvertFunc  gray_convert;

  } grX11Format;



  /************************************************************************/
  /************************************************************************/
  /*****                                                              *****/
  /*****                BLITTING ROUTINES FOR RGB565                  *****/
  /*****                                                              *****/
  /************************************************************************/
  /************************************************************************/

  static void
  gr_x11_convert_rgb_to_rgb565( grX11Blitter*  blit )
  {
    unsigned char*  line_read  = blit->src_line + blit->x * 3;
    unsigned char*  line_write = blit->dst_line + blit->x * 2;
    int             h          = blit->height;


    for ( ; h > 0; h-- )
    {
      unsigned char*   read  = line_read;
      unsigned short*  write = (unsigned short*)line_write;
      int              x     = blit->width;


      for ( ; x > 0; x--, read += 3, write++ )
      {
        unsigned int  r = read[0];
        unsigned int  g = read[1];
        unsigned int  b = read[2];


        write[0] = (unsigned short)( ( ( r << 8 ) & 0xF800U ) |
                                     ( ( g << 3 ) & 0x07E0  ) |
                                     ( ( b >> 3 ) & 0x001F  ) );
      }

      line_read  += blit->src_pitch;
      line_write += blit->dst_pitch;
    }
  }


  static void
  gr_x11_convert_gray_to_rgb565( grX11Blitter*  blit )
  {
    unsigned char*  line_read  = blit->src_line + blit->x;
    unsigned char*  line_write = blit->dst_line + blit->x * 2;
    int             h          = blit->height;


    for ( ; h > 0; h-- )
    {
      unsigned char*   read  = line_read;
      unsigned short*  write = (unsigned short*)line_write;
      int              x     = blit->width;


      for ( ; x > 0; x--, read++, write++ )
      {
        unsigned int  p = read[0];


        write[0] = (unsigned short)( ( ( p << 8 ) & 0xF800U ) |
                                     ( ( p << 3 ) & 0x07E0  ) |
                                     ( ( p >> 3 ) & 0x001F  ) );
      }

      line_read  += blit->src_pitch;
      line_write += blit->dst_pitch;
    }
  }


  static const grX11Format  gr_x11_format_rgb565 =
  {
    16, 16, 0xF800U, 0x07E0, 0x001F,
    gr_x11_convert_rgb_to_rgb565,
    gr_x11_convert_gray_to_rgb565
  };


  /************************************************************************/
  /************************************************************************/
  /*****                                                              *****/
  /*****                BLITTING ROUTINES FOR  BGR565                 *****/
  /*****                                                              *****/
  /************************************************************************/
  /************************************************************************/

  static void
  gr_x11_convert_rgb_to_bgr565( grX11Blitter*  blit )
  {
    unsigned char*  line_read  = blit->src_line + blit->x * 3;
    unsigned char*  line_write = blit->dst_line + blit->x * 2;
    int             h          = blit->height;


    for ( ; h > 0; h-- )
    {
      unsigned char*   read  = line_read;
      unsigned short*  write = (unsigned short*)line_write;
      int              x     = blit->width;


      for ( ; x > 0; x--, read += 3, write++ )
      {
        unsigned int  r = read[0];
        unsigned int  g = read[1];
        unsigned int  b = read[2];


        write[0] = (unsigned short)( ( ( b << 8 ) & 0xF800U ) |
                                     ( ( g << 3 ) & 0x07E0  ) |
                                     ( ( r >> 3 ) & 0x001F  ) );
      }

      line_read  += blit->src_pitch;
      line_write += blit->dst_pitch;
    }
  }


  static const grX11Format  gr_x11_format_bgr565 =
  {
    16, 16, 0x001F, 0x7E00, 0xF800U,
    gr_x11_convert_rgb_to_bgr565,
    gr_x11_convert_gray_to_rgb565  /* the same for bgr565! */
  };


  /************************************************************************/
  /************************************************************************/
  /*****                                                              *****/
  /*****                BLITTING ROUTINES FOR RGB555                  *****/
  /*****                                                              *****/
  /************************************************************************/
  /************************************************************************/

  static void
  gr_x11_convert_rgb_to_rgb555( grX11Blitter*  blit )
  {
    unsigned char*  line_read  = blit->src_line + blit->x * 3;
    unsigned char*  line_write = blit->dst_line + blit->x * 2;
    int             h          = blit->height;


    for ( ; h > 0; h-- )
    {
      unsigned char*   read  = line_read;
      unsigned short*  write = (unsigned short*)line_write;
      int              x     = blit->width;


      for ( ; x > 0; x--, read += 3, write++ )
      {
        unsigned int  r = read[0];
        unsigned int  g = read[1];
        unsigned int  b = read[2];


        write[0] = (unsigned short)( ( ( r << 7 ) & 0x7C00 ) |
                                     ( ( g << 2 ) & 0x03E0 ) |
                                     ( ( b >> 3 ) & 0x001F ) );
      }

      line_read  += blit->src_pitch;
      line_write += blit->dst_pitch;
    }
  }


  static void
  gr_x11_convert_gray_to_rgb555( grX11Blitter*  blit )
  {
    unsigned char*  line_read  = blit->src_line + blit->x;
    unsigned char*  line_write = blit->dst_line + blit->x * 2;
    int             h          = blit->height;


    for ( ; h > 0; h-- )
    {
      unsigned char*   read  = line_read;
      unsigned short*  write = (unsigned short*)line_write;
      int              x     = blit->width;


      for ( ; x > 0; x--, read++, write++ )
      {
        unsigned int  p = read[0];


        write[0] = (unsigned short)( ( ( p << 7 ) & 0x7C00 ) |
                                     ( ( p << 2 ) & 0x03E0 ) |
                                     ( ( p >> 3 ) & 0x001F ) );
      }

      line_read  += blit->src_pitch;
      line_write += blit->dst_pitch;
    }
  }


  static const grX11Format  gr_x11_format_rgb555 =
  {
    15, 16, 0x7C00, 0x3E00, 0x001F,
    gr_x11_convert_rgb_to_rgb555,
    gr_x11_convert_gray_to_rgb555
  };


  /************************************************************************/
  /************************************************************************/
  /*****                                                              *****/
  /*****                BLITTING ROUTINES FOR  BGR555                 *****/
  /*****                                                              *****/
  /************************************************************************/
  /************************************************************************/

  static void
  gr_x11_convert_rgb_to_bgr555( grX11Blitter*  blit )
  {
    unsigned char*  line_read  = blit->src_line + blit->x * 3;
    unsigned char*  line_write = blit->dst_line + blit->x * 2;
    int             h          = blit->height;


    for ( ; h > 0; h-- )
    {
      unsigned char*   read  = line_read;
      unsigned short*  write = (unsigned short*)line_write;
      int              x     = blit->width;


      for ( ; x > 0; x--, read += 3, write++ )
      {
        unsigned int  r = read[0];
        unsigned int  g = read[1];
        unsigned int  b = read[2];

        write[0] = (unsigned short)( ( (b << 7) & 0x7C00 ) |
                                     ( (g << 2) & 0x03E0 ) |
                                     ( (r >> 3) & 0x001F ) );
      }

      line_read  += blit->src_pitch;
      line_write += blit->dst_pitch;
    }
  }


  static const grX11Format gr_x11_format_bgr555 =
  {
    15, 16, 0x1F00, 0x3E00, 0x7C00,
    gr_x11_convert_rgb_to_bgr555,
    gr_x11_convert_gray_to_rgb555  /* the same for bgr555! */
  };


  /************************************************************************/
  /************************************************************************/
  /*****                                                              *****/
  /*****                BLITTING ROUTINES FOR RGB888                  *****/
  /*****                                                              *****/
  /************************************************************************/
  /************************************************************************/

  static void
  gr_x11_convert_rgb_to_rgb888( grX11Blitter*  blit )
  {
    unsigned char*  line_read  = blit->src_line + blit->x * 3;
    unsigned char*  line_write = blit->dst_line + blit->x * 3;
    int             h          = blit->height;


    for ( ; h > 0; h-- )
    {
      memcpy( line_write, line_read, blit->width * 3 );
      line_read  += blit->src_pitch;
      line_write += blit->dst_pitch;
    }
  }


  static void
  gr_x11_convert_gray_to_rgb888( grX11Blitter*  blit )
  {
    unsigned char*  line_read  = blit->src_line + blit->x;
    unsigned char*  line_write = blit->dst_line + blit->x * 3;
    int             h          = blit->height;


    for ( ; h > 0; h-- )
    {
      unsigned char*   read  = line_read;
      unsigned char*   write = line_write;
      int              x     = blit->width;


      for ( ; x > 0; x--, read++, write += 3 )
      {
        unsigned char  p = read[0];


        write[0] = p;
        write[1] = p;
        write[2] = p;
      }

      line_read  += blit->src_pitch;
      line_write += blit->dst_pitch;
    }
  }


  static const grX11Format  gr_x11_format_rgb888 =
  {
    24, 24, 0xFF0000L, 0x00FF00U, 0x0000FF,
    gr_x11_convert_rgb_to_rgb888,
    gr_x11_convert_gray_to_rgb888
  };


  /************************************************************************/
  /************************************************************************/
  /*****                                                              *****/
  /*****                BLITTING ROUTINES FOR  BGR888                 *****/
  /*****                                                              *****/
  /************************************************************************/
  /************************************************************************/

  static void
  gr_x11_convert_rgb_to_bgr888( grX11Blitter*  blit )
  {
    unsigned char*  line_read  = blit->src_line + blit->x * 3;
    unsigned char*  line_write = blit->dst_line + blit->x * 3;
    int             h          = blit->height;


    for ( ; h > 0; h-- )
    {
      unsigned char*   read  = line_read;
      unsigned char*   write = line_write;
      int              x     = blit->width;


      for ( ; x > 0; x--, read += 3, write += 3 )
      {
        write[0] = read[2];
        write[1] = read[1];
        write[2] = read[0];
      }

      line_read  += blit->src_pitch;
      line_write += blit->dst_pitch;
    }
  }


  static const grX11Format  gr_x11_format_bgr888 =
  {
    24, 24, 0x0000FF, 0x00FF00U, 0xFF0000L,
    gr_x11_convert_rgb_to_bgr888,
    gr_x11_convert_gray_to_rgb888   /* the same for bgr888 */
  };


  /************************************************************************/
  /************************************************************************/
  /*****                                                              *****/
  /*****                BLITTING ROUTINES FOR RGB8880                 *****/
  /*****                                                              *****/
  /************************************************************************/
  /************************************************************************/

  static void
  gr_x11_convert_rgb_to_rgb8880( grX11Blitter*  blit )
  {
    unsigned char*  line_read  = blit->src_line + blit->x * 3;
    unsigned char*  line_write = blit->dst_line + blit->x * 4;
    int             h          = blit->height;


    for ( ; h > 0; h-- )
    {
      unsigned char*   read  = line_read;
      unsigned char*   write = line_write;
      int              x     = blit->width;


      for ( ; x > 0; x--, read += 3, write += 4 )
      {
        uint32  r = read[0];
        uint32  g = read[1];
        uint32  b = read[2];

        *(uint32*)write = ( ( r << 24 ) | ( g << 16 ) | ( b << 8 ) );
      }

      line_read  += blit->src_pitch;
      line_write += blit->dst_pitch;
    }
  }


  static void
  gr_x11_convert_gray_to_rgb8880( grX11Blitter*  blit )
  {
    unsigned char*  line_read  = blit->src_line + blit->x;
    unsigned char*  line_write = blit->dst_line + blit->x*4;
    int             h          = blit->height;


    for ( ; h > 0; h-- )
    {
      unsigned char*  read  = line_read;
      unsigned char*  write = line_write;
      int             x     = blit->width;


      for ( ; x > 0; x--, read ++, write += 4 )
      {
        uint32  p = read[0];


        *(uint32*)write = ( ( p << 24 ) | ( p << 16 ) | ( p << 8 ) );
      }

      line_read  += blit->src_pitch;
      line_write += blit->dst_pitch;
    }
  }


  static const grX11Format  gr_x11_format_rgb8880 =
  {
    24, 32, 0xFF000000UL, 0x00FF0000L, 0x0000FF00U,
    gr_x11_convert_rgb_to_rgb8880,
    gr_x11_convert_gray_to_rgb8880
  };


  /************************************************************************/
  /************************************************************************/
  /*****                                                              *****/
  /*****                BLITTING ROUTINES FOR RGB0888                 *****/
  /*****                                                              *****/
  /************************************************************************/
  /************************************************************************/

  static void
  gr_x11_convert_rgb_to_rgb0888( grX11Blitter*  blit )
  {
    unsigned char*  line_read  = blit->src_line + blit->x * 3;
    unsigned char*  line_write = blit->dst_line + blit->x * 4;
    int             h          = blit->height;


    for ( ; h > 0; h-- )
    {
      unsigned char*  read  = line_read;
      unsigned char*  write = line_write;
      int             x     = blit->width;


      for ( ; x > 0; x--, read += 3, write += 4 )
      {
        uint32  r = read[0];
        uint32  g = read[1];
        uint32  b = read[2];

        *(uint32*)write = ( ( r << 16 ) | ( g << 8 ) | ( b << 0 ) );
      }

      line_read  += blit->src_pitch;
      line_write += blit->dst_pitch;
    }
  }


  static void
  gr_x11_convert_gray_to_rgb0888( grX11Blitter*  blit )
  {
    unsigned char*  line_read  = blit->src_line + blit->x;
    unsigned char*  line_write = blit->dst_line + blit->x * 4;
    int             h          = blit->height;


    for ( ; h > 0; h-- )
    {
      unsigned char*  read  = line_read;
      unsigned char*  write = line_write;
      int             x     = blit->width;


      for ( ; x > 0; x--, read ++, write += 4 )
      {
        uint32  p = read[0];


        *(uint32*)write = ( ( p << 16 ) | ( p << 8 ) | ( p << 0 ) );
      }

      line_read  += blit->src_pitch;
      line_write += blit->dst_pitch;
    }
  }


  static const grX11Format  gr_x11_format_rgb0888 =
  {
    24, 32, 0x00FF0000L, 0x0000FF00U, 0x000000FF,
    gr_x11_convert_rgb_to_rgb0888,
    gr_x11_convert_gray_to_rgb0888
  };


  /************************************************************************/
  /************************************************************************/
  /*****                                                              *****/
  /*****                BLITTING ROUTINES FOR BGR8880                 *****/
  /*****                                                              *****/
  /************************************************************************/
  /************************************************************************/

  static void
  gr_x11_convert_rgb_to_bgr8880( grX11Blitter*  blit )
  {
    unsigned char*  line_read  = blit->src_line + blit->x * 3;
    unsigned char*  line_write = blit->dst_line + blit->x * 4;
    int             h          = blit->height;


    for ( ; h > 0; h-- )
    {
      unsigned char*  read  = line_read;
      unsigned char*  write = line_write;
      int             x     = blit->width;


      for ( ; x > 0; x--, read += 3, write += 4 )
      {
        uint32  r = read[0];
        uint32  g = read[1];
        uint32  b = read[2];

        *(uint32*)write = ( ( r << 8 ) | ( g << 16 ) | ( b << 24 ) );
      }

      line_read  += blit->src_pitch;
      line_write += blit->dst_pitch;
    }
  }


  static const grX11Format  gr_x11_format_bgr8880 =
  {
    24, 32, 0x0000FF00U, 0x00FF0000L, 0xFF000000UL,
    gr_x11_convert_rgb_to_bgr8880,
    gr_x11_convert_gray_to_rgb8880  /* the same for bgr8880 */
  };


  /************************************************************************/
  /************************************************************************/
  /*****                                                              *****/
  /*****                BLITTING ROUTINES FOR BGR0888                 *****/
  /*****                                                              *****/
  /************************************************************************/
  /************************************************************************/

  static void
  gr_x11_convert_rgb_to_bgr0888( grX11Blitter*  blit )
  {
    unsigned char*  line_read  = blit->src_line + blit->x * 3;
    unsigned char*  line_write = blit->dst_line + blit->x * 4;
    int             h          = blit->height;


    for ( ; h > 0; h-- )
    {
      unsigned char*  read  = line_read;
      unsigned char*  write = line_write;
      int             x     = blit->width;


      for ( ; x > 0; x--, read += 3, write += 4 )
      {
        uint32  r = read[0];
        uint32  g = read[1];
        uint32  b = read[2];

        *(uint32*)write = ( ( r << 0 ) | ( g << 8 ) | ( b << 16 ) );
      }

      line_read  += blit->src_pitch;
      line_write += blit->dst_pitch;
    }
  }


  static const grX11Format  gr_x11_format_bgr0888 =
  {
    24, 32, 0x000000FF, 0x0000FF00U, 0x00FF0000L,
    gr_x11_convert_rgb_to_bgr0888,
    gr_x11_convert_gray_to_rgb0888  /* the same for bgr0888 */
  };


  /************************************************************************/
  /************************************************************************/
  /*****                                                              *****/
  /*****                   X11 DEVICE SUPPORT                         *****/
  /*****                                                              *****/
  /************************************************************************/
  /************************************************************************/

  static const grX11Format*  gr_x11_formats[] =
  {
    &gr_x11_format_rgb565,
    &gr_x11_format_bgr565,
    &gr_x11_format_rgb555,
    &gr_x11_format_bgr555,
    &gr_x11_format_rgb888,
    &gr_x11_format_bgr888,
    &gr_x11_format_rgb0888,
    &gr_x11_format_bgr0888,
    &gr_x11_format_rgb8880,
    &gr_x11_format_bgr8880,

    NULL,
  };

  typedef struct  grX11DeviceRec_
  {
    Display*            display;
    Cursor              idle;
    Cursor              busy;
    const grX11Format*  format;
    int                 scanline_pad;

  } grX11Device;


  static grX11Device  x11dev;


  static void
  gr_x11_device_done( void )
  {
    if ( x11dev.display )
    {
      XCloseDisplay( x11dev.display );
      x11dev.display = NULL;
    }
  }


  static int
  gr_x11_device_init( void )
  {
    grX11Device*  dev = &x11dev;


    memset( &x11dev, 0, sizeof ( x11dev ) );

    XrmInitialize();


    dev->display = XOpenDisplay( "" );
    if ( !dev->display )
    {
      fprintf( stderr, "cannot open X11 display\n" );
      return -1;
    }

    dev->idle = XCreateFontCursor( dev->display, XC_left_ptr );
    dev->busy = XCreateFontCursor( dev->display, XC_watch );

    {
      int          count;
      XDepth*      format;
      XDepth*      formats;
      XVisualInfo  templ;


      formats = XListPixmapFormats( dev->display, &count );

#ifdef TEST
      printf( "available pixmap formats\n" );
      printf( "depth  pixbits  scanpad\n" );
#endif /* TEST */

      for ( format = formats; count > 0; count--, format++ )
      {
#ifdef TEST
        printf( " %3d     %3d      %3d\n",
                format->depth,
                format->bits_per_pixel,
                format->scanline_pad );
#endif /* TEST */

        /* note, the 32-bit modes return a depth of 24, */
        /* and 32 bits per pixel                        */
        switch ( format->depth )
        {
        case 16:
        case 24:
        case 32:
          {
            int           count2;
            XVisualInfo*  visuals;
            XVisualInfo*  visual;


            templ.depth = format->depth;
            visuals     = XGetVisualInfo( dev->display,
                                          VisualDepthMask,
                                          &templ,
                                          &count2 );

            for ( visual = visuals; count2 > 0; count2--, visual++ )
            {
#ifdef TEST
              const char*  string = "unknown";


              switch ( visual->Class )
              {
              case TrueColor:
                string = "TrueColor";
                break;
              case DirectColor:
                string = "DirectColor";
                break;
              case PseudoColor:
                string = "PseudoColor";
                break;
              case StaticGray:
                string = "StaticGray";
                break;
              case StaticColor:
                string = "StaticColor";
                break;
              case GrayScale:
                string = "GrayScale";
                break;
              }

              printf( ">   RGB %04lx:%04lx:%04lx, colors %3d, bits %2d  %s\n",
                      visual->red_mask,
                      visual->green_mask,
                      visual->blue_mask,
                      visual->colormap_size,
                      visual->bits_per_rgb,
                      string );
#endif /* TEST */

              /* compare to the list of supported formats */
              {
                const grX11Format**  pcur_format = gr_x11_formats;
                const grX11Format*   cur_format;


                for (;;)
                {
                  cur_format = *pcur_format++;
                  if ( cur_format == NULL )
                    break;

                  if ( format->depth          == cur_format->x_depth          &&
                       format->bits_per_pixel == cur_format->x_bits_per_pixel &&
                       visual->red_mask       == cur_format->x_red_mask       &&
                       visual->green_mask     == cur_format->x_green_mask     &&
                       visual->blue_mask      == cur_format->x_blue_mask      )
                  {
                    dev->format       = cur_format;
                    dev->scanline_pad = format->scanline_pad;
                    return 0;
                  }
                }
              }
            } /* for visuals */
          }
          break;

        default:
          ;
        } /* switch format depth */
      } /* for formats */
    }

    fprintf( stderr, "unsupported X11 display depth!\n" );

    return -1;
  }


  /************************************************************************/
  /************************************************************************/
  /*****                                                              *****/
  /*****                   X11 SURFACE SUPPORT                        *****/
  /*****                                                              *****/
  /************************************************************************/
  /************************************************************************/

  typedef struct  grX11Surface_
  {
    grSurface           root;
    Display*            display;
    Window              win;
    Visual*             visual;
    Colormap            colormap;
    GC                  gc;
    int                 depth;
    XImage*             ximage;
    grBitmap            ximage_bitmap;

    const grX11Format*  format;
    grX11ConvertFunc    convert;

    int                 win_org_x,   win_org_y;
    int                 win_width,   win_height;
    int                 image_width, image_height;

    char                key_buffer[10];
    int                 key_cursor;
    int                 key_number;

  } grX11Surface;


  /* close a given window */
  static void
  gr_x11_surface_done( grX11Surface*  surface )
  {
    Display*  display = surface->display;


    if ( display )
    {
      if ( surface->ximage )
      {
        XDestroyImage( surface->ximage );
        surface->ximage = 0;
      }

      if ( surface->win )
      {
        XUnmapWindow( display, surface->win );
        surface->win = 0;
      }
    }
  }


  static void
  gr_x11_surface_refresh_rect( grX11Surface*  surface,
                               int            x,
                               int            y,
                               int            w,
                               int            h )
  {
    grX11Blitter  blit;


    if ( !gr_x11_blitter_reset( &blit, &surface->root.bitmap,
                                &surface->ximage_bitmap,
                                x, y, w, h ) )
    {
      surface->convert( &blit );

      XPutImage( surface->display,
                 surface->win,
                 surface->gc,
                 surface->ximage,
                 blit.x, blit.y, blit.x, blit.y, blit.width, blit.height );
    }
  }


  static void
  gr_x11_surface_refresh( grX11Surface*  surface )
  {
    gr_x11_surface_refresh_rect( surface, 0, 0,
                                 surface->root.bitmap.width,
                                 surface->root.bitmap.rows );
  }


  static void
  gr_x11_surface_set_title( grX11Surface*  surface,
                             const char*   title )
  {
    XStoreName( surface->display, surface->win, title );
  }


  static grKey
  KeySymTogrKey( KeySym  key )
  {
    grKey        k;
    int          count = sizeof ( key_translators ) /
                           sizeof( key_translators[0] );
    Translator*  trans = key_translators;
    Translator*  limit = trans + count;


    k = grKeyNone;

    while ( trans < limit )
    {
      if ( trans->xkey == key )
      {
        k = trans->grkey;
        break;
      }
      trans++;
    }

    return k;
  }


  static void
  gr_x11_surface_listen_event( grX11Surface*  surface,
                               int            event_mask,
                               grEvent*       grevent )
  {
    XEvent     x_event;
    KeySym     key;
    Display*   display = surface->display;

    int        bool_exit;
    grKey      grkey;

    XComposeStatus  compose;

    /* XXX: for now, ignore the event mask, and only exit when */
    /*      a key is pressed                                   */
    (void)event_mask;

    bool_exit = surface->key_cursor < surface->key_number;

    XDefineCursor( display, surface->win, x11dev.idle );

    while ( !bool_exit )
    {
      XNextEvent( display, &x_event );

      switch ( x_event.type )
      {
      case KeyPress:
        surface->key_number = XLookupString( &x_event.xkey,
                                             surface->key_buffer,
                                             sizeof ( surface->key_buffer ),
                                             &key,
                                             &compose );
        surface->key_cursor = 0;

        if ( surface->key_number == 0 ||
             key > 512       )
        {
          /* this may be a special key like F1, F2, etc. */
          grkey = KeySymTogrKey( key );
          if ( grkey != grKeyNone )
            goto Set_Key;
        }
        else
          bool_exit = 1;
        break;

      case MappingNotify:
        XRefreshKeyboardMapping( &x_event.xmapping );
        break;

      case Expose:
#if 1
        /* we don't need to convert the bits on each expose! */
        XPutImage( surface->display,
                   surface->win,
                   surface->gc,
                   surface->ximage,
                   x_event.xexpose.x,
                   x_event.xexpose.y,
                   x_event.xexpose.x,
                   x_event.xexpose.y,
                   x_event.xexpose.width,
                   x_event.xexpose.height );
#else
        gr_x11_surface_refresh_rectangle( surface,
                                          x_event.xexpose.x,
                                          x_event.xexpose.y,
                                          x_event.xexpose.width,
                                          x_event.xexpose.height );
#endif
        break;

      /* You should add more cases to handle mouse events, etc. */
      }
    }

    XDefineCursor( display, surface->win, x11dev.busy );
    XFlush       ( display );

    /* now, translate the keypress to a grKey; */
    /* if this wasn't part of the simple translated keys, */
    /* simply get the charcode from the character buffer  */
    grkey = grKEY( surface->key_buffer[surface->key_cursor++] );

  Set_Key:
    grevent->type = gr_key_down;
    grevent->key  = grkey;
  }


  static int
  gr_x11_surface_init( grX11Surface*  surface,
                       grBitmap*      bitmap )
  {
    Display*            display;
    int                 screen;
    grBitmap*           pximage = &surface->ximage_bitmap;
    const grX11Format*  format;


    surface->key_number = 0;
    surface->key_cursor = 0;
    surface->display    = display = x11dev.display;

    screen = DefaultScreen( display );

    surface->colormap = DefaultColormap( display, screen );
    surface->depth    = DefaultDepth( display, screen );
    surface->visual   = DefaultVisual( display, screen );

    surface->format      = format = x11dev.format;
    surface->root.bitmap = *bitmap;

    switch ( bitmap->mode )
    {
    case gr_pixel_mode_rgb24:
      surface->convert = format->rgb_convert;
      break;

    case gr_pixel_mode_gray:
      /* we only support 256-gray level 8-bit pixmaps */
      if ( bitmap->grays == 256 )
      {
        surface->convert = format->gray_convert;
        break;
      }

    default:
      /* we don't support other modes */
      return 0;
    }

    /* allocate surface image */
    {
      int  bits, over;


      bits = bitmap->width * format->x_bits_per_pixel;
      over = bits % x11dev.scanline_pad;

      if ( over )
        bits += x11dev.scanline_pad - over;

      pximage->pitch  = bits >> 3;
      pximage->width  = bitmap->width;
      pximage->rows   = bitmap->rows;
    }

    pximage->buffer = grAlloc( pximage->pitch * pximage->rows );
    if ( !pximage->buffer )
      return 0;

    /* create the bitmap */
    if ( grNewBitmap( bitmap->mode,
                      bitmap->grays,
                      bitmap->width,
                      bitmap->rows,
                      bitmap ) )
      return 0;

    surface->root.bitmap = *bitmap;

    /* Now create the surface X11 image */
    surface->ximage = XCreateImage( display,
                                    surface->visual,
                                    format->x_depth,
                                    ZPixmap,
                                    0,
                                    (char*)pximage->buffer,
                                    pximage->width,
                                    pximage->rows,
                                    8,
                                    0 );
    if ( !surface->ximage )
      return 0;

    {
      XTextProperty         xtp;
      XSizeHints            xsh;
      XSetWindowAttributes  xswa;


      xswa.border_pixel     = BlackPixel( display, screen );
      xswa.background_pixel = WhitePixel( display, screen );
      xswa.cursor           = x11dev.busy;

      xswa.event_mask = KeyPressMask | ExposureMask;

      surface->win = XCreateWindow( display,
                                    RootWindow( display, screen ),
                                    0,
                                    0,
                                    bitmap->width,
                                    bitmap->rows,
                                    10,
                                    format->x_depth,
                                    InputOutput,
                                    surface->visual,
                                    CWBackPixel | CWBorderPixel |
                                    CWEventMask | CWCursor,
                                    &xswa );

      XMapWindow( display, surface->win );

      surface->gc = XCreateGC( display, RootWindow( display, screen ),
                               0L, NULL );
      XSetForeground( display, surface->gc, xswa.border_pixel     );
      XSetBackground( display, surface->gc, xswa.background_pixel );

      /* make window manager happy :-) */
      xtp.value    = (unsigned char*)"FreeType";
      xtp.encoding = 31;
      xtp.format   = 8;
      xtp.nitems   = strlen( (char*)xtp.value );

      xsh.x = 0;
      xsh.y = 0;

      xsh.width  = bitmap->width;
      xsh.height = bitmap->rows;
      xsh.flags  = PPosition | PSize;
      xsh.flags  = 0;

      XSetWMProperties( display, surface->win, &xtp, &xtp,
                        NULL, 0, &xsh, NULL, NULL );
    }

    surface->root.done         = (grDoneSurfaceFunc)gr_x11_surface_done;
    surface->root.refresh_rect = (grRefreshRectFunc)gr_x11_surface_refresh_rect;
    surface->root.set_title    = (grSetTitleFunc)   gr_x11_surface_set_title;
    surface->root.listen_event = (grListenEventFunc)gr_x11_surface_listen_event;

    gr_x11_surface_refresh( surface );

    return 1;
  }


  grDevice  gr_x11_device =
  {
    sizeof( grX11Surface ),
    "x11",

    gr_x11_device_init,
    gr_x11_device_done,

    (grDeviceInitSurfaceFunc) gr_x11_surface_init,

    0,
    0
  };


#ifdef TEST

  typedef struct  grKeyName
  {
    grKey        key;
    const char*  name;

  } grKeyName;


  static const grKeyName  key_names[] =
  {
    { grKeyF1,   "F1"  },
    { grKeyF2,   "F2"  },
    { grKeyF3,   "F3"  },
    { grKeyF4,   "F4"  },
    { grKeyF5,   "F5"  },
    { grKeyF6,   "F6"  },
    { grKeyF7,   "F7"  },
    { grKeyF8,   "F8"  },
    { grKeyF9,   "F9"  },
    { grKeyF10,  "F10" },
    { grKeyF11,  "F11" },
    { grKeyF12,  "F12" },
    { grKeyEsc,  "Esc" },
    { grKeyHome, "Home" },
    { grKeyEnd,  "End"  },

    { grKeyPageUp,   "Page_Up" },
    { grKeyPageDown, "Page_Down" },
    { grKeyLeft,     "Left" },
    { grKeyRight,    "Right" },
    { grKeyUp,       "Up" },
    { grKeyDown,     "Down" },
    { grKeyBackSpace, "BackSpace" },
    { grKeyReturn,   "Return" }
  };


#if 0
  int
  main( void )
  {
    grSurface*  surface;
    int         n;


    grInit();
    surface = grNewScreenSurface( 0, gr_pixel_mode_gray, 320, 400, 128 );
    if ( !surface )
      Panic( "Could not create window\n" );
    else
    {
      grColor      color;
      grEvent      event;
      const char*  string;
      int          x;


      grSetSurfaceRefresh( surface, 1 );
      grSetTitle( surface, "X11 driver demonstration" );

      for ( x = -10; x < 10; x++ )
      {
        for ( n = 0; n < 128; n++ )
        {
          color.value = ( n * 3 ) & 127;
          grWriteCellChar( surface,
                           x + ( ( n % 60 ) << 3 ),
                           80 + ( x + 10 ) * 8 * 3 + ( ( n / 60 ) << 3 ),
                           n, color );
        }
      }

      color.value = 64;
      grWriteCellString( surface, 0, 0, "just an example", color );

      do
      {
        listen_event( (grXSurface*)surface, 0, &event );

        /* return if ESC was pressed */
        if ( event.key == grKeyEsc )
          return 0;

        /* otherwise, display key string */
        color.value = ( color.value + 8 ) & 127;
        {
          int          count = sizeof ( key_names ) / sizeof ( key_names[0] );
          grKeyName*   name  = key_names;
          grKeyName*   limit = name + count;
          const char*  kname = 0;
          char         kname_temp[16];


          while ( name < limit )
          {
            if ( name->key == event.key )
            {
              kname = name->name;
              break;
            }
            name++;
          }

          if ( !kname )
          {
            sprintf( kname_temp, "char '%c'", (char)event.key );
            kname = kname_temp;
          }

          grWriteCellString( surface, 30, 30, kname, color );
          grRefreshSurface( surface );
          paint_rectangle( surface, 0, 0,
                           surface->bitmap.width, surface->bitmap.rows );
        }
      } while ( 1 );
    }

    return 0;
  }
#endif /* O */
#endif /* TEST */


/* END */
