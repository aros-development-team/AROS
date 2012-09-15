/* bitmap_scale.c - Bitmap scaling. */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2006,2007,2008,2009  Free Software Foundation, Inc.
 *
 *  GRUB is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  GRUB is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GRUB.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <grub/mm.h>
#include <grub/misc.h>
#include <grub/video.h>
#include <grub/bitmap.h>
#include <grub/bitmap_scale.h>
#include <grub/types.h>
#include <grub/dl.h>

GRUB_MOD_LICENSE ("GPLv3+");

/* Prototypes for module-local functions.  */
static grub_err_t scale_nn (struct grub_video_bitmap *dst,
                            struct grub_video_bitmap *src);
static grub_err_t scale_bilinear (struct grub_video_bitmap *dst,
                                  struct grub_video_bitmap *src);

/* This function creates a new scaled version of the bitmap SRC.  The new
   bitmap has dimensions DST_WIDTH by DST_HEIGHT.  The scaling algorithm
   is given by SCALE_METHOD.  If an error is encountered, the return code is
   not equal to GRUB_ERR_NONE, and the bitmap DST is either not created, or
   it is destroyed before this function returns.

   Supports only direct color modes which have components separated
   into bytes (e.g., RGBA 8:8:8:8 or BGR 8:8:8 true color).
   But because of this simplifying assumption, the implementation is
   greatly simplified.  */
grub_err_t
grub_video_bitmap_create_scaled (struct grub_video_bitmap **dst,
                                 int dst_width, int dst_height,
                                 struct grub_video_bitmap *src,
                                 enum grub_video_bitmap_scale_method
                                 scale_method)
{
  *dst = 0;

  /* Verify the simplifying assumptions. */
  if (src == 0)
    return grub_error (GRUB_ERR_BUG,
                       "null src bitmap in grub_video_bitmap_create_scaled");
  if (src->mode_info.red_field_pos % 8 != 0
      || src->mode_info.green_field_pos % 8 != 0
      || src->mode_info.blue_field_pos % 8 != 0
      || src->mode_info.reserved_field_pos % 8 != 0)
    return grub_error (GRUB_ERR_NOT_IMPLEMENTED_YET,
                       "src format not supported for scale");
  if (src->mode_info.width == 0 || src->mode_info.height == 0)
    return grub_error (GRUB_ERR_BAD_ARGUMENT,
                       "source bitmap has a zero dimension");
  if (dst_width <= 0 || dst_height <= 0)
    return grub_error (GRUB_ERR_BUG,
                       "requested to scale to a size w/ a zero dimension");
  if (src->mode_info.bytes_per_pixel * 8 != src->mode_info.bpp)
    return grub_error (GRUB_ERR_BUG,
                       "bitmap to scale has inconsistent Bpp and bpp");

  /* Create the new bitmap. */
  grub_err_t ret;
  ret = grub_video_bitmap_create (dst, dst_width, dst_height,
                                  src->mode_info.blit_format);
  if (ret != GRUB_ERR_NONE)
    return ret;                 /* Error. */

  switch (scale_method)
    {
    case GRUB_VIDEO_BITMAP_SCALE_METHOD_FASTEST:
    case GRUB_VIDEO_BITMAP_SCALE_METHOD_NEAREST:
      ret = scale_nn (*dst, src);
      break;
    case GRUB_VIDEO_BITMAP_SCALE_METHOD_BEST:
    case GRUB_VIDEO_BITMAP_SCALE_METHOD_BILINEAR:
      ret = scale_bilinear (*dst, src);
      break;
    default:
      ret = grub_error (GRUB_ERR_BUG, "Invalid scale_method value");
      break;
    }

  if (ret == GRUB_ERR_NONE)
    {
      /* Success:  *dst is now a pointer to the scaled bitmap. */
      return GRUB_ERR_NONE;
    }
  else
    {
      /* Destroy the bitmap and return the error code. */
      grub_video_bitmap_destroy (*dst);
      *dst = 0;
      return ret;
    }
}

/* Nearest neighbor bitmap scaling algorithm.

   Copy the bitmap SRC to the bitmap DST, scaling the bitmap to fit the
   dimensions of DST.  This function uses the nearest neighbor algorithm to
   interpolate the pixels.

   Supports only direct color modes which have components separated
   into bytes (e.g., RGBA 8:8:8:8 or BGR 8:8:8 true color).
   But because of this simplifying assumption, the implementation is
   greatly simplified.  */
static grub_err_t
scale_nn (struct grub_video_bitmap *dst, struct grub_video_bitmap *src)
{
  /* Verify the simplifying assumptions. */
  if (dst == 0 || src == 0)
    return grub_error (GRUB_ERR_BUG, "null bitmap in scale_nn");
  if (dst->mode_info.red_field_pos % 8 != 0
      || dst->mode_info.green_field_pos % 8 != 0
      || dst->mode_info.blue_field_pos % 8 != 0
      || dst->mode_info.reserved_field_pos % 8 != 0)
    return grub_error (GRUB_ERR_NOT_IMPLEMENTED_YET,
		       "dst format not supported");
  if (src->mode_info.red_field_pos % 8 != 0
      || src->mode_info.green_field_pos % 8 != 0
      || src->mode_info.blue_field_pos % 8 != 0
      || src->mode_info.reserved_field_pos % 8 != 0)
    return grub_error (GRUB_ERR_NOT_IMPLEMENTED_YET,
		       "src format not supported");
  if (dst->mode_info.red_field_pos != src->mode_info.red_field_pos
      || dst->mode_info.red_mask_size != src->mode_info.red_mask_size
      || dst->mode_info.green_field_pos != src->mode_info.green_field_pos
      || dst->mode_info.green_mask_size != src->mode_info.green_mask_size
      || dst->mode_info.blue_field_pos != src->mode_info.blue_field_pos
      || dst->mode_info.blue_mask_size != src->mode_info.blue_mask_size
      || dst->mode_info.reserved_field_pos !=
      src->mode_info.reserved_field_pos
      || dst->mode_info.reserved_mask_size !=
      src->mode_info.reserved_mask_size)
    return grub_error (GRUB_ERR_NOT_IMPLEMENTED_YET,
		       "dst and src not compatible");
  if (dst->mode_info.bytes_per_pixel != src->mode_info.bytes_per_pixel)
    return grub_error (GRUB_ERR_NOT_IMPLEMENTED_YET,
		       "dst and src not compatible");
  if (dst->mode_info.width == 0 || dst->mode_info.height == 0
      || src->mode_info.width == 0 || src->mode_info.height == 0)
    return grub_error (GRUB_ERR_BUG, "bitmap has a zero dimension");

  grub_uint8_t *ddata = dst->data;
  grub_uint8_t *sdata = src->data;
  int dw = dst->mode_info.width;
  int dh = dst->mode_info.height;
  int sw = src->mode_info.width;
  int sh = src->mode_info.height;
  int dstride = dst->mode_info.pitch;
  int sstride = src->mode_info.pitch;
  /* bytes_per_pixel is the same for both src and dst. */
  int bytes_per_pixel = dst->mode_info.bytes_per_pixel;

  int dy;
  for (dy = 0; dy < dh; dy++)
    {
      int dx;
      for (dx = 0; dx < dw; dx++)
        {
          grub_uint8_t *dptr;
          grub_uint8_t *sptr;
          int sx;
          int sy;
          int comp;

          /* Compute the source coordinate that the destination coordinate
             maps to.  Note: sx/sw = dx/dw  =>  sx = sw*dx/dw. */
          sx = sw * dx / dw;
          sy = sh * dy / dh;

          /* Get the address of the pixels in src and dst. */
          dptr = ddata + dy * dstride + dx * bytes_per_pixel;
          sptr = sdata + sy * sstride + sx * bytes_per_pixel;

          /* Copy the pixel color value. */
          for (comp = 0; comp < bytes_per_pixel; comp++)
            dptr[comp] = sptr[comp];
        }
    }
  return GRUB_ERR_NONE;
}

/* Bilinear interpolation image scaling algorithm.

   Copy the bitmap SRC to the bitmap DST, scaling the bitmap to fit the
   dimensions of DST.  This function uses the bilinear interpolation algorithm
   to interpolate the pixels.

   Supports only direct color modes which have components separated
   into bytes (e.g., RGBA 8:8:8:8 or BGR 8:8:8 true color).
   But because of this simplifying assumption, the implementation is
   greatly simplified.  */
static grub_err_t
scale_bilinear (struct grub_video_bitmap *dst, struct grub_video_bitmap *src)
{
  /* Verify the simplifying assumptions. */
  if (dst == 0 || src == 0)
    return grub_error (GRUB_ERR_BUG, "null bitmap in scale func");
  if (dst->mode_info.red_field_pos % 8 != 0
      || dst->mode_info.green_field_pos % 8 != 0
      || dst->mode_info.blue_field_pos % 8 != 0
      || dst->mode_info.reserved_field_pos % 8 != 0)
    return grub_error (GRUB_ERR_NOT_IMPLEMENTED_YET, "dst format not supported");
  if (src->mode_info.red_field_pos % 8 != 0
      || src->mode_info.green_field_pos % 8 != 0
      || src->mode_info.blue_field_pos % 8 != 0
      || src->mode_info.reserved_field_pos % 8 != 0)
    return grub_error (GRUB_ERR_NOT_IMPLEMENTED_YET, "src format not supported");
  if (dst->mode_info.red_field_pos != src->mode_info.red_field_pos
      || dst->mode_info.red_mask_size != src->mode_info.red_mask_size
      || dst->mode_info.green_field_pos != src->mode_info.green_field_pos
      || dst->mode_info.green_mask_size != src->mode_info.green_mask_size
      || dst->mode_info.blue_field_pos != src->mode_info.blue_field_pos
      || dst->mode_info.blue_mask_size != src->mode_info.blue_mask_size
      || dst->mode_info.reserved_field_pos !=
      src->mode_info.reserved_field_pos
      || dst->mode_info.reserved_mask_size !=
      src->mode_info.reserved_mask_size)
    return grub_error (GRUB_ERR_NOT_IMPLEMENTED_YET, "dst and src not compatible");
  if (dst->mode_info.bytes_per_pixel != src->mode_info.bytes_per_pixel)
    return grub_error (GRUB_ERR_NOT_IMPLEMENTED_YET, "dst and src not compatible");
  if (dst->mode_info.width == 0 || dst->mode_info.height == 0
      || src->mode_info.width == 0 || src->mode_info.height == 0)
    return grub_error (GRUB_ERR_BUG, "bitmap has a zero dimension");

  grub_uint8_t *ddata = dst->data;
  grub_uint8_t *sdata = src->data;
  int dw = dst->mode_info.width;
  int dh = dst->mode_info.height;
  int sw = src->mode_info.width;
  int sh = src->mode_info.height;
  int dstride = dst->mode_info.pitch;
  int sstride = src->mode_info.pitch;
  /* bytes_per_pixel is the same for both src and dst. */
  int bytes_per_pixel = dst->mode_info.bytes_per_pixel;

  int dy;
  for (dy = 0; dy < dh; dy++)
    {
      int dx;
      for (dx = 0; dx < dw; dx++)
        {
          grub_uint8_t *dptr;
          grub_uint8_t *sptr;
          int sx;
          int sy;
          int comp;

          /* Compute the source coordinate that the destination coordinate
             maps to.  Note: sx/sw = dx/dw  =>  sx = sw*dx/dw. */
          sx = sw * dx / dw;
          sy = sh * dy / dh;

          /* Get the address of the pixels in src and dst. */
          dptr = ddata + dy * dstride + dx * bytes_per_pixel;
          sptr = sdata + sy * sstride + sx * bytes_per_pixel;

          /* If we have enough space to do so, use bilinear interpolation.
             Otherwise, fall back to nearest neighbor for this pixel. */
          if (sx < sw - 1 && sy < sh - 1)
            {
              /* Do bilinear interpolation. */

              /* Fixed-point .8 numbers representing the fraction of the
                 distance in the x (u) and y (v) direction within the
                 box of 4 pixels in the source. */
              int u = (256 * sw * dx / dw) - (sx * 256);
              int v = (256 * sh * dy / dh) - (sy * 256);

              for (comp = 0; comp < bytes_per_pixel; comp++)
                {
                  /* Get the component's values for the
                     four source corner pixels. */
                  grub_uint8_t f00 = sptr[comp];
                  grub_uint8_t f10 = sptr[comp + bytes_per_pixel];
                  grub_uint8_t f01 = sptr[comp + sstride];
                  grub_uint8_t f11 = sptr[comp + sstride + bytes_per_pixel];

                  /* Do linear interpolations along the top and bottom
                     rows of the box. */
                  grub_uint8_t f0y = (256 - v) * f00 / 256 + v * f01 / 256;
                  grub_uint8_t f1y = (256 - v) * f10 / 256 + v * f11 / 256;

                  /* Interpolate vertically. */
                  grub_uint8_t fxy = (256 - u) * f0y / 256 + u * f1y / 256;

                  dptr[comp] = fxy;
                }
            }
          else
            {
              /* Fall back to nearest neighbor interpolation. */
              /* Copy the pixel color value. */
              for (comp = 0; comp < bytes_per_pixel; comp++)
                dptr[comp] = sptr[comp];
            }
        }
    }
  return GRUB_ERR_NONE;
}
