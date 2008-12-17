#include "grobjs.h"
#include <stdlib.h>
#include <string.h>

  int  grError = 0;


  /* values must be in 0..255 range */
  grColor
  grFindColor( grBitmap*  target,
               int        red,
               int        green,
               int        blue,
               int        alpha )
  {
    grColor  color;

    color.value = 0;

    switch (target->mode)
    {
      case gr_pixel_mode_mono:
        if ( (red|green|blue) )
          color.value = 1;
        break;

      case gr_pixel_mode_gray:
        color.value = (3*red + 6*green + blue)/10;
        break;

      case gr_pixel_mode_rgb565:
        color.value = ((red & 0xF8) << 8) | ((green & 0xFC) << 3) | ((blue & 0xF8) >> 3);
        break;

      case gr_pixel_mode_rgb24:
        color.chroma[0] = red;
        color.chroma[1] = green;
        color.chroma[2] = blue;
        break;

      case gr_pixel_mode_rgb32:
        color.chroma[0] = red;
        color.chroma[1] = green;
        color.chroma[2] = blue;
        color.chroma[3] = alpha;
        break;

      default:
        ;
    }
    return color;
  }

 /********************************************************************
  *
  * <Function>
  *   grRealloc
  *
  * <Description>
  *   Simple memory re-allocation.
  *
  * <Input>
  *   block :: original memory block address
  *   size  :: new requested block size in bytes
  *
  * <Return>
  *   the memory block address. 0 in case of error
  *
  ********************************************************************/

  unsigned char*  grAlloc( long size )
  {
    unsigned char*  p;

    p = (unsigned char*)malloc(size);
    if (!p && size > 0)
    {
      grError = gr_err_memory;
    }

    if (p)
      memset( p, 0, size );

    return p;
  }


 /********************************************************************
  *
  * <Function>
  *   grRealloc
  *
  * <Description>
  *   Simple memory re-allocation.
  *
  * <Input>
  *   block :: original memory block address
  *   size  :: new requested block size in bytes
  *
  * <Return>
  *   the memory block address. 0 in case of error
  *
  ********************************************************************/

  unsigned char*  grRealloc( const unsigned char*  block, long size )
  {
    unsigned char*  p;

    p = (unsigned char *)realloc( (void*)block, size );
    if (!p && size > 0)
    {
      grError = gr_err_memory;
    }
    return p;
  }


 /********************************************************************
  *
  * <Function>
  *   grFree
  *
  * <Description>
  *   Simple memory release
  *
  * <Input>
  *   block :: target block
  *
  ********************************************************************/

  void  grFree( const void*  block )
  {
    if (block)
      free( (void *)block );
  }



  static
  int  check_mode( grPixelMode  pixel_mode,
                   int          num_grays )
  {
    if ( pixel_mode <= gr_pixel_mode_none ||
         pixel_mode >= gr_pixel_mode_max  )
      goto Fail;

    if ( pixel_mode != gr_pixel_mode_gray       ||
         ( num_grays >= 2 && num_grays <= 256 ) )
      return 0;

  Fail:
    grError = gr_err_bad_argument;
    return grError;
  }


 /**********************************************************************
  *
  * <Function>
  *    grNewBitmap
  *
  * <Description>
  *    creates a new bitmap
  *
  * <Input>
  *    pixel_mode   :: the target surface's pixel_mode
  *    num_grays    :: number of grays levels for PAL8 pixel mode
  *    width        :: width in pixels
  *    height       :: height in pixels
  *
  * <Output>
  *    bit  :: descriptor of the new bitmap
  *
  * <Return>
  *    Error code. 0 means success.
  *
  **********************************************************************/

  extern  int  grNewBitmap( grPixelMode  pixel_mode,
                            int          num_grays,
                            int          width,
                            int          height,
                            grBitmap    *bit )
  {
    int  pitch;

    /* check mode */
    if (check_mode(pixel_mode,num_grays))
      goto Fail;

    /* check dimensions */
    if (width < 0 || height < 0)
    {
      grError = gr_err_bad_argument;
      goto Fail;
    }

    bit->width = width;
    bit->rows  = height;
    bit->mode  = pixel_mode;
    bit->grays = num_grays;

    pitch = width;

    switch (pixel_mode)
    {
      case gr_pixel_mode_mono  : pitch = (width+7) >> 3; break;
      case gr_pixel_mode_pal4  : pitch = (width+3) >> 2; break;

      case gr_pixel_mode_pal8  :
      case gr_pixel_mode_gray  : pitch = width; break;

      case gr_pixel_mode_rgb555:
      case gr_pixel_mode_rgb565: pitch = width*2; break;

      case gr_pixel_mode_rgb24 : pitch = width*3; break;

      case gr_pixel_mode_rgb32 : pitch = width*4; break;

      default:
        grError = gr_err_bad_target_depth;
        return 0;
    }

    bit->pitch  = pitch;
    bit->buffer = grAlloc( (long)bit->pitch * bit->rows );
    if (!bit->buffer) goto Fail;

    return 0;

  Fail:
    return grError;
  }

 /**********************************************************************
  *
  * <Function>
  *    grDoneBitmap
  *
  * <Description>
  *    destroys a bitmap
  *
  * <Input>
  *    bitmap :: handle to bitmap descriptor
  *
  * <Note>
  *    This function does NOT release the bitmap descriptor, only
  *    the pixel buffer.
  *
  **********************************************************************/

  extern  void  grDoneBitmap( grBitmap*  bit )
  {
    grFree( bit->buffer );
    bit->buffer = 0;
  }



