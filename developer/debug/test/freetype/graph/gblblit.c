#include "gblblit.h"

/* blitting gray glyphs
 */

/* generic macros
 */
#define  GRGB_PACK(r,g,b)      ( ((GBlenderPixel)(r) << 16) | \
                                 ((GBlenderPixel)(g) <<  8) | \
                                  (GBlenderPixel)(b)        )

#define  GDST_STORE3(d,r,g,b)  (d)[0] = (unsigned char)(r); \
                               (d)[1] = (unsigned char)(g); \
                               (d)[2] = (unsigned char)(b)

/* */

#define  GRGB_TO_RGB565(r,g,b)   ((unsigned short)( (((r) << 8) & 0xF800) |  \
                                                    (((g) << 3) & 0x07E0) |  \
                                                    (((b) >> 3) & 0x001F) ) )

#define  GRGB565_TO_RGB24(p)   ( ( ((p) << 8) & 0xF80000 ) |             \
                                 ( ((p) << 3) & 0x0700F8 ) |             \
                                 ( ((p) << 5) & 0x00FC00 ) |             \
                                 ( ((p) >> 1) & 0x000300 ) |             \
                                 ( ((p) >> 2) & 0x000007 ) )

#define  GRGB24_TO_RGB565(p)   ( (unsigned short)( (((p) >> 8) & 0xF800 ) |   \
                                                   (((p) >> 5) & 0x07E0 ) |   \
                                                   (((p) >> 3) & 0x001F ) ) )

/* */

#define  GRGB_TO_BGR565(r,g,b)    GRGB_TO_RGB565(b,g,r)

#define  GBGR565_TO_RGB24(p)   ( ( ((p) << 19) & 0xF80000 ) |             \
                                 ( ((p) << 12) & 0x070000 ) |             \
                                 ( ((p) <<  5) & 0x00FC00 ) |             \
                                 ( ((p) >>  1) & 0x000300 ) |             \
                                 ( ((p) >>  8) & 0x0000F8 ) |             \
                                 ( ((p) >> 13) & 0x000007 ) )

#define  GRGB24_TO_BGR565(p)   ( (unsigned short)( (((p) <<  8) & 0xF800 ) |  \
                                                   (((p) >>  5) & 0x07E0 ) |  \
                                                   (((p) >> 19) & 0x001F ) ) )

/* */

/* Rgb32 blitting routines
 */

#define  GDST_TYPE                rgb32
#define  GDST_INCR                4
#define  GDST_READ(d,p)           (p) = *(GBlenderPixel*)(d) & 0xFFFFFF
#define  GDST_COPY(d)             *(GBlenderPixel*)(d) = color
#define  GDST_STOREP(d,cells,a)   *(GBlenderPixel*)(d) = (cells)[(a)]
#define  GDST_STOREB(d,cells,a)              \
  {                                          \
    GBlenderCell*  _g = (cells) + (a)*3;     \
                                             \
    GDST_STOREC(d,_g[0],_g[1],_g[2]);        \
  }
#define  GDST_STOREC(d,r,g,b)     *(GBlenderPixel*)(d) = GRGB_PACK(r,g,b)
#define  GDST_COPY_VAR            /* nothing */

#include "gblany.h"

/* Rgb24 blitting routines
 */

#define  GDST_TYPE                 rgb24
#define  GDST_INCR                 3
#define  GDST_READ(d,p)            (p) = GRGB_PACK((d)[0],(d)[1],(d)[2])
#define  GDST_COPY(d)              GDST_STORE3(d,r,g,b)
#define  GDST_STOREC(d,r,g,b)      GDST_STORE3(d,r,g,b)

#define  GDST_STOREB(d,cells,a)                \
    {                                          \
      GBlenderCell*  _g = (cells) + (a)*3;     \
                                               \
      (d)[0] = _g[0];                          \
      (d)[1] = _g[1];                          \
      (d)[2] = _g[2];                          \
    }

#define  GDST_STOREP(d,cells,a)                 \
    {                                           \
      GBlenderPixel  _pix = (cells)[(a)];       \
                                                \
      GDST_STORE3(d,_pix >> 16,_pix >> 8,_pix); \
    }

#define  GDST_COPY_VAR            /* nothing */

#include "gblany.h"

/* Rgb565 blitting routines
 */

#define  GDST_TYPE               rgb565
#define  GDST_INCR               2

#define  GDST_READ(d,p)          p = (GBlenderPixel)*(unsigned short*)(d);  \
                                 p = GRGB565_TO_RGB24(p)

#define  GDST_COPY_VAR           unsigned short  pix = GRGB_TO_RGB565(r,g,b);
#define  GDST_COPY(d)            *(unsigned short*)(d) = pix

#define  GDST_STOREB(d,cells,a)                                   \
    {                                                             \
      GBlenderCell*  _g = (cells) + (a)*3;                        \
                                                                  \
      *(unsigned short*)(d) = GRGB_TO_RGB565(_g[0],_g[1],_g[2]);  \
    }

#define  GDST_STOREP(d,cells,a)                         \
    {                                                   \
      GBlenderPixel  _pix = (cells)[(a)];               \
                                                        \
      *(unsigned short*)(d) = GRGB24_TO_RGB565(_pix);   \
    }

#define  GDST_STOREC(d,r,g,b)   *(unsigned short*)(d) = GRGB_TO_RGB565(r,g,b)

#include "gblany.h"

/* Bgr565 blitting routines
 */
#define  GDST_TYPE               bgr565
#define  GDST_INCR               2

#define  GDST_READ(d,p)          p = (GBlenderPixel)*(unsigned short*)(d);  \
                                 p = GBGR565_TO_RGB24(p)

#define  GDST_COPY_VAR           unsigned short  pix = GRGB_TO_BGR565(r,g,b);
#define  GDST_COPY(d)            *(d) = (unsigned char)pix

#define  GDST_STOREB(d,cells,a)                                   \
    {                                                             \
      GBlenderCell*  _g = (cells) + (a)*3;                        \
                                                                  \
      *(unsigned short*)(d) = GRGB_TO_BGR565(_g[0],_g[1],_g[2]);  \
    }

#define  GDST_STOREP(d,cells,a)                         \
    {                                                   \
      GBlenderPixel  _pix = (cells)[(a)];               \
                                                        \
      *(unsigned short*)(d) = GRGB24_TO_BGR565(_pix);   \
    }

#define  GDST_STOREC(d,r,g,b)   *(unsigned short*)(d) = GRGB_TO_BGR565(r,g,b)

#include "gblany.h"

/* */

static void
_gblender_blit_dummy( GBlenderBlit   blit,
                      GBlenderPixel  color )
{
  (void)blit;
  (void)color;
}


GBLENDER_APIDEF( int )
gblender_blit_init( GBlenderBlit           blit,
                    GBlender               blender,
                    int                    dst_x,
                    int                    dst_y,
                    GBlenderSourceFormat   src_format,
                    const unsigned char*   src_buffer,
                    int                    src_pitch,
                    int                    src_width,
                    int                    src_height,
                    GBlenderTargetFormat   dst_format,
                    unsigned char*         dst_buffer,
                    int                    dst_pitch,
                    int                    dst_width,
                    int                    dst_height )
{
  int               src_x = 0;
  int               src_y = 0;
  int               delta;
  GBlenderBlitFunc  blit_func = 0;

  switch ( src_format )
  {
  case GBLENDER_SOURCE_GRAY8:
      switch ( dst_format )
      {
      case GBLENDER_TARGET_RGB32:  blit_func = _gblender_blit_gray8_rgb32; break;
      case GBLENDER_TARGET_RGB24:  blit_func = _gblender_blit_gray8_rgb24; break;
      case GBLENDER_TARGET_RGB565: blit_func = _gblender_blit_gray8_rgb565; break;
      case GBLENDER_TARGET_BGR565: blit_func = _gblender_blit_gray8_bgr565; break;
      default:
          ;
      }
      break;

  case GBLENDER_SOURCE_HRGB:
      switch ( dst_format )
      {
      case GBLENDER_TARGET_RGB32:  blit_func = _gblender_blit_hrgb_rgb32; break;
      case GBLENDER_TARGET_RGB24:  blit_func = _gblender_blit_hrgb_rgb24; break;
      case GBLENDER_TARGET_RGB565: blit_func = _gblender_blit_hrgb_rgb565; break;
      case GBLENDER_TARGET_BGR565: blit_func = _gblender_blit_hrgb_bgr565; break;
      default:
          ;
      }
      break;

  case GBLENDER_SOURCE_HBGR:
      switch ( dst_format )
      {
      case GBLENDER_TARGET_RGB32:  blit_func = _gblender_blit_hbgr_rgb32; break;
      case GBLENDER_TARGET_RGB24:  blit_func = _gblender_blit_hbgr_rgb24; break;
      case GBLENDER_TARGET_RGB565: blit_func = _gblender_blit_hbgr_rgb565; break;
      case GBLENDER_TARGET_BGR565: blit_func = _gblender_blit_hbgr_bgr565; break;
      default:
          ;
      }
      break;

  case GBLENDER_SOURCE_VRGB:
      switch ( dst_format )
      {
      case GBLENDER_TARGET_RGB32:  blit_func = _gblender_blit_vrgb_rgb32; break;
      case GBLENDER_TARGET_RGB24:  blit_func = _gblender_blit_vrgb_rgb24; break;
      case GBLENDER_TARGET_RGB565: blit_func = _gblender_blit_vrgb_rgb565; break;
      case GBLENDER_TARGET_BGR565: blit_func = _gblender_blit_vrgb_bgr565; break;
      default:
          ;
      }
      break;

  case GBLENDER_SOURCE_VBGR:
      switch ( dst_format )
      {
      case GBLENDER_TARGET_RGB32:  blit_func = _gblender_blit_vbgr_rgb32; break;
      case GBLENDER_TARGET_RGB24:  blit_func = _gblender_blit_vbgr_rgb24; break;
      case GBLENDER_TARGET_RGB565: blit_func = _gblender_blit_vbgr_rgb565; break;
      case GBLENDER_TARGET_BGR565: blit_func = _gblender_blit_vbgr_bgr565; break;
      default:
          ;
      }
      break;

  case GBLENDER_SOURCE_BGRA:
      switch ( dst_format )
      {
      case GBLENDER_TARGET_RGB32:  blit_func = _gblender_blit_bgra_rgb32; break;
      case GBLENDER_TARGET_RGB24:  blit_func = _gblender_blit_bgra_rgb24; break;
      case GBLENDER_TARGET_RGB565: blit_func = _gblender_blit_bgra_rgb565; break;
      case GBLENDER_TARGET_BGR565: blit_func = _gblender_blit_bgra_bgr565; break;
      default:
          ;
      }
      break;

  default:
    ;
  }

  blit->blender   = blender;
  blit->blit_func = blit_func;

  if ( blit_func == 0 )
  {
   /* unsupported blit mode
    */
    blit->blit_func = _gblender_blit_dummy;
    return -2;
  }

  if ( dst_x < 0 )
  {
    src_width += dst_x;
    src_x     -= dst_x;
    dst_x      = 0;
  }

  delta = dst_x + src_width - dst_width;
  if ( delta > 0 )
    src_width -= delta;

  if ( dst_y < 0 )
  {
    src_height += dst_y;
    src_y      -= dst_y;
    dst_y       = 0;
  }

  delta = dst_y + src_height - dst_height;
  if ( delta > 0 )
    src_height -= delta;

 /* nothing to blit
  */
  if ( src_width <= 0 || src_height <= 0 )
  {
    blit->blit_func = _gblender_blit_dummy;
    return -1;
  }

  blit->width      = src_width;
  blit->height     = src_height;
  blit->src_format = src_format;
  blit->dst_format = dst_format;

  blit->src_x     = src_x;
  blit->src_y     = src_y;
  blit->src_line  = src_buffer + src_pitch*src_y;
  blit->src_pitch = src_pitch;
  if ( src_pitch < 0 )
    blit->src_line -= (src_height-1)*src_pitch;

  blit->dst_x     = dst_x;
  blit->dst_y     = dst_y;
  blit->dst_line  = dst_buffer + dst_pitch*dst_y;
  blit->dst_pitch = dst_pitch;
  if ( dst_pitch < 0 )
    blit->dst_line -= (dst_height-1)*dst_pitch;

  return 0;
}

