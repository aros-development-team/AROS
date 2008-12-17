/****************************************************************************/
/*                                                                          */
/*  The FreeType project -- a free and portable quality font engine         */
/*                                                                          */
/*  Copyright 2006 by                                                       */
/*  D. Turner, R.Wilhelm, and W. Lemberg                                    */
/*                                                                          */
/*                                                                          */
/*  gbench is a small program used to benchmark a new algorithm             */
/*  performing gamma-corrected alpha-blending.                              */
/*                                                                          */
/****************************************************************************/


#ifndef __GBENCH_H__
#define __GBENCH_H__

  typedef enum
  {
    GBITMAP_FORMAT_NONE = 0,
    GBITMAP_FORMAT_RGB24,
    GBITMAP_FORMAT_GRAY,
    GBITMAP_FORMAT_RGB,
    GBITMAP_FORMAT_BGR,
    GBITMAP_FORMAT_RGBV,
    GBITMAP_FORMAT_BGRV,

    GBITMAP_FORMAT_MAX

  } GBitmapFormat;


  typedef struct GBitmapRec_
  {
    int             width;
    int             height;
    int             pitch;
    unsigned char*  buffer;
    GBitmapFormat   format;

  } GBitmapRec, *GBitmap;


  typedef struct GBlitterRec_*  GBlitter;

  typedef void  (*GBlitterFunc)( GBlitter   blitter,
                                 int        color );

  typedef struct GBlitterRec_
  {
    int              width;
    int              height;

    int              src_x;
    unsigned char*   src_line;
    int              src_incr;

    int              dst_x;
    unsigned char*   dst_line;
    int              dst_incr;

    GBlitterFunc     blit;

  } GBlitterRec;


  extern void
  ggamma_set( double  gamma );

  extern int
  gblitter_init_rgb24( GBlitter   blitter,
                       GBitmap    src,
                       int        dst_x,
                       int        dst_y,
                       int        dst_width,
                       int        dst_height,
                       void*      dst_buffer,
                       int        dst_pitch );

#define  gblitter_blit(b,c)   (b)->blit( (b), (c) )


#endif /* __GBENCH_H__ */


/* End */
