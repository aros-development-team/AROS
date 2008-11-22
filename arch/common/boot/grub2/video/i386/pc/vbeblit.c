/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2006,2007,2008  Free Software Foundation, Inc.
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

/* SPECIAL NOTES!

   Please note following when reading the code below:

   - In this driver we assume that every memory can be accessed by same memory
   bus.  If there are different address spaces do not use this code as a base
   code for other archs.

   - Every function in this code assumes that bounds checking has been done in
   previous phase and they are opted out in here.  */

#include <grub/machine/vbe.h>
#include <grub/machine/vbeblit.h>
#include <grub/machine/vbeutil.h>
#include <grub/misc.h>
#include <grub/types.h>
#include <grub/video.h>

/* Generic replacing blitter (slow).  Works for every supported format.  */
void
grub_video_i386_vbeblit_replace (struct grub_video_i386_vbeblit_info *dst,
				 struct grub_video_i386_vbeblit_info *src,
				 int x, int y, int width, int height,
				 int offset_x, int offset_y)
{
  int i;
  int j;
  grub_uint8_t src_red;
  grub_uint8_t src_green;
  grub_uint8_t src_blue;
  grub_uint8_t src_alpha;
  grub_video_color_t src_color;
  grub_video_color_t dst_color;

  for (j = 0; j < height; j++)
    {
      for (i = 0; i < width; i++)
	{
	  src_color = get_pixel (src, i + offset_x, j + offset_y);

	  grub_video_vbe_unmap_color_int (src, src_color, &src_red, &src_green,
					  &src_blue, &src_alpha);

	  dst_color = grub_video_vbe_map_rgba (src_red, src_green,
					       src_blue, src_alpha);

	  set_pixel (dst, x + i, y + j, dst_color);
	}
    }
}

/* Block copy replacing blitter.  Works with modes multiple of 8 bits.  */
void
grub_video_i386_vbeblit_replace_directN (struct grub_video_i386_vbeblit_info *dst,
					 struct grub_video_i386_vbeblit_info *src,
					 int x, int y, int width, int height,
					 int offset_x, int offset_y)
{
  int j;
  grub_uint32_t *srcptr;
  grub_uint32_t *dstptr;
  int bpp;

  bpp = src->mode_info->bytes_per_pixel;

  for (j = 0; j < height; j++)
    {
      srcptr = (grub_uint32_t *)get_data_ptr (src, offset_x, j + offset_y);
      dstptr = (grub_uint32_t *)get_data_ptr (dst, x, y + j);

      grub_memmove (dstptr, srcptr, width * bpp);
    }
}

/* Optimized replacing blitter for RGBX8888 to BGRX8888.  */
void
grub_video_i386_vbeblit_replace_BGRX8888_RGBX8888 (struct grub_video_i386_vbeblit_info *dst,
						   struct grub_video_i386_vbeblit_info *src,
						   int x, int y,
						   int width, int height,
						   int offset_x, int offset_y)
{
  int i;
  int j;
  grub_uint8_t *srcptr;
  grub_uint8_t *dstptr;
  unsigned int srcrowskip;
  unsigned int dstrowskip;

  /* Calculate the number of bytes to advance from the end of one line
     to the beginning of the next line.  */
  srcrowskip = src->mode_info->pitch - src->mode_info->bytes_per_pixel * width;
  dstrowskip = dst->mode_info->pitch - dst->mode_info->bytes_per_pixel * width;

  srcptr = (grub_uint8_t *) get_data_ptr (src, offset_x, offset_y);
  dstptr = (grub_uint8_t *) get_data_ptr (dst, x, y);

  for (j = 0; j < height; j++)
    {
      for (i = 0; i < width; i++)
        {
          grub_uint8_t r = *srcptr++;
          grub_uint8_t g = *srcptr++;
          grub_uint8_t b = *srcptr++;
          grub_uint8_t a = *srcptr++;

          *dstptr++ = b;
          *dstptr++ = g;
          *dstptr++ = r;
          *dstptr++ = a;
        }

      srcptr += srcrowskip;
      dstptr += dstrowskip;
    }
}

/* Optimized replacing blitter for RGB888 to BGRX8888.  */
void
grub_video_i386_vbeblit_replace_BGRX8888_RGB888 (struct grub_video_i386_vbeblit_info *dst,
						 struct grub_video_i386_vbeblit_info *src,
						 int x, int y,
						 int width, int height,
						 int offset_x, int offset_y)
{
  int i;
  int j;
  grub_uint8_t *srcptr;
  grub_uint8_t *dstptr;
  unsigned int srcrowskip;
  unsigned int dstrowskip;

  /* Calculate the number of bytes to advance from the end of one line
     to the beginning of the next line.  */
  srcrowskip = src->mode_info->pitch - src->mode_info->bytes_per_pixel * width;
  dstrowskip = dst->mode_info->pitch - dst->mode_info->bytes_per_pixel * width;

  srcptr = (grub_uint8_t *) get_data_ptr (src, offset_x, offset_y);
  dstptr = (grub_uint8_t *) get_data_ptr (dst, x, y);

  for (j = 0; j < height; j++)
    {
      for (i = 0; i < width; i++)
        {
          grub_uint8_t r = *srcptr++;
          grub_uint8_t g = *srcptr++;
          grub_uint8_t b = *srcptr++;

          *dstptr++ = b;
          *dstptr++ = g;
          *dstptr++ = r;

          /* Set alpha component as opaque.  */
          *dstptr++ = 255;
        }

      srcptr += srcrowskip;
      dstptr += dstrowskip;
    }
}

/* Optimized replacing blitter for RGBX8888 to BGR888.  */
void
grub_video_i386_vbeblit_replace_BGR888_RGBX8888 (struct grub_video_i386_vbeblit_info *dst,
						 struct grub_video_i386_vbeblit_info *src,
						 int x, int y,
						 int width, int height,
						 int offset_x, int offset_y)
{
  grub_uint32_t *srcptr;
  grub_uint8_t *dstptr;
  unsigned int srcrowskip;
  unsigned int dstrowskip;
  int i;
  int j;

  /* Calculate the number of bytes to advance from the end of one line
     to the beginning of the next line.  */
  srcrowskip = src->mode_info->pitch - src->mode_info->bytes_per_pixel * width;
  dstrowskip = dst->mode_info->pitch - dst->mode_info->bytes_per_pixel * width;

  srcptr = (grub_uint32_t *) get_data_ptr (src, offset_x, offset_y);
  dstptr = (grub_uint8_t *) get_data_ptr (dst, x, y);

  for (j = 0; j < height; j++)
    {
      for (i = 0; i < width; i++)
        {
          grub_uint32_t color;
          grub_uint8_t sr;
          grub_uint8_t sg;
          grub_uint8_t sb;

          color = *srcptr++;

          sr = (color >> 0) & 0xFF;
          sg = (color >> 8) & 0xFF;
          sb = (color >> 16) & 0xFF;

          *dstptr++ = sb;
          *dstptr++ = sg;
          *dstptr++ = sr;
        }

      srcptr = (grub_uint32_t *) (((grub_uint8_t *) srcptr) + srcrowskip);
      dstptr += dstrowskip;
    }
}

/* Optimized replacing blitter for RGB888 to BGR888.  */
void
grub_video_i386_vbeblit_replace_BGR888_RGB888 (struct grub_video_i386_vbeblit_info *dst,
					       struct grub_video_i386_vbeblit_info *src,
					       int x, int y,
					       int width, int height,
					       int offset_x, int offset_y)
{
  int i;
  int j;
  grub_uint8_t *srcptr;
  grub_uint8_t *dstptr;
  unsigned int srcrowskip;
  unsigned int dstrowskip;

  /* Calculate the number of bytes to advance from the end of one line
     to the beginning of the next line.  */
  srcrowskip = src->mode_info->pitch - src->mode_info->bytes_per_pixel * width;
  dstrowskip = dst->mode_info->pitch - dst->mode_info->bytes_per_pixel * width;

  srcptr = (grub_uint8_t *) get_data_ptr (src, offset_x, offset_y);
  dstptr = (grub_uint8_t *) get_data_ptr (dst, x, y);

  for (j = 0; j < height; j++)
    {
      for (i = 0; i < width; i++)
        {
          grub_uint8_t r = *srcptr++;
          grub_uint8_t g = *srcptr++;
          grub_uint8_t b = *srcptr++;

          *dstptr++ = b;
          *dstptr++ = g;
          *dstptr++ = r;
        }

      srcptr += srcrowskip;
      dstptr += dstrowskip;
    }
}

/* Optimized replacing blitter for RGB888 to RGBX8888.  */
void
grub_video_i386_vbeblit_replace_RGBX8888_RGB888 (struct grub_video_i386_vbeblit_info *dst,
						 struct grub_video_i386_vbeblit_info *src,
						 int x, int y,
						 int width, int height,
						 int offset_x, int offset_y)
{
  grub_uint32_t color;
  int i;
  int j;
  grub_uint8_t *srcptr;
  grub_uint32_t *dstptr;
  unsigned int sr;
  unsigned int sg;
  unsigned int sb;

  for (j = 0; j < height; j++)
    {
      srcptr = (grub_uint8_t *)get_data_ptr (src, offset_x, j + offset_y);
      dstptr = (grub_uint32_t *)get_data_ptr (dst, x, y + j);

      for (i = 0; i < width; i++)
        {
          sr = *srcptr++;
          sg = *srcptr++;
          sb = *srcptr++;

          /* Set alpha as opaque.  */
          color = 0xFF000000 | (sb << 16) | (sg << 8) | sr;

          *dstptr++ = color;
        }
    }
}

/* Optimized replacing blitter for RGBX8888 to RGB888.  */
void
grub_video_i386_vbeblit_replace_RGB888_RGBX8888 (struct grub_video_i386_vbeblit_info *dst,
						 struct grub_video_i386_vbeblit_info *src,
						 int x, int y,
						 int width, int height,
						 int offset_x, int offset_y)
{
  grub_uint32_t color;
  int i;
  int j;
  grub_uint32_t *srcptr;
  grub_uint8_t *dstptr;
  unsigned int sr;
  unsigned int sg;
  unsigned int sb;

  for (j = 0; j < height; j++)
    {
      srcptr = (grub_uint32_t *)get_data_ptr (src, offset_x, j + offset_y);
      dstptr = (grub_uint8_t *)get_data_ptr (dst, x, y + j);

      for (i = 0; i < width; i++)
	{
	  color = *srcptr++;

	  sr = (color >> 0) & 0xFF;
	  sg = (color >> 8) & 0xFF;
	  sb = (color >> 16) & 0xFF;

	  *dstptr++ = sr;
	  *dstptr++ = sg;
	  *dstptr++ = sb;
	}
    }
}

/* Optimized replacing blitter for RGBX8888 to indexed color.  */
void
grub_video_i386_vbeblit_replace_index_RGBX8888 (struct grub_video_i386_vbeblit_info *dst,
						struct grub_video_i386_vbeblit_info *src,
						int x, int y,
						int width, int height,
						int offset_x, int offset_y)
{
  grub_uint32_t color;
  int i;
  int j;
  grub_uint32_t *srcptr;
  grub_uint8_t *dstptr;
  unsigned int sr;
  unsigned int sg;
  unsigned int sb;

  for (j = 0; j < height; j++)
    {
      srcptr = (grub_uint32_t *)get_data_ptr (src, offset_x, j + offset_y);
      dstptr = (grub_uint8_t *)get_data_ptr (dst, x, y + j);

      for (i = 0; i < width; i++)
	{
	  color = *srcptr++;

	  sr = (color >> 0) & 0xFF;
	  sg = (color >> 8) & 0xFF;
	  sb = (color >> 16) & 0xFF;

	  color = grub_video_vbe_map_rgb(sr, sg, sb);
	  *dstptr++ = color & 0xFF;
	}
    }
}

/* Optimized replacing blitter for RGB888 to indexed color.  */
void
grub_video_i386_vbeblit_replace_index_RGB888 (struct grub_video_i386_vbeblit_info *dst,
					      struct grub_video_i386_vbeblit_info *src,
					      int x, int y,
					      int width, int height,
					      int offset_x, int offset_y)
{
  grub_uint32_t color;
  int i;
  int j;
  grub_uint8_t *srcptr;
  grub_uint8_t *dstptr;
  unsigned int sr;
  unsigned int sg;
  unsigned int sb;

  for (j = 0; j < height; j++)
    {
      srcptr = (grub_uint8_t *)get_data_ptr (src, offset_x, j + offset_y);
      dstptr = (grub_uint8_t *)get_data_ptr (dst, x, y + j);

      for (i = 0; i < width; i++)
        {
          sr = *srcptr++;
          sg = *srcptr++;
          sb = *srcptr++;

          color = grub_video_vbe_map_rgb(sr, sg, sb);

          *dstptr++ = color & 0xFF;
        }
    }
}

/* Generic blending blitter.  Works for every supported format.  */
void
grub_video_i386_vbeblit_blend (struct grub_video_i386_vbeblit_info *dst,
                               struct grub_video_i386_vbeblit_info *src,
                               int x, int y, int width, int height,
                               int offset_x, int offset_y)
{
  int i;
  int j;

  for (j = 0; j < height; j++)
    {
      for (i = 0; i < width; i++)
        {
          grub_uint8_t src_red;
          grub_uint8_t src_green;
          grub_uint8_t src_blue;
          grub_uint8_t src_alpha;
          grub_uint8_t dst_red;
          grub_uint8_t dst_green;
          grub_uint8_t dst_blue;
          grub_uint8_t dst_alpha;
          grub_video_color_t src_color;
          grub_video_color_t dst_color;

          src_color = get_pixel (src, i + offset_x, j + offset_y);
          grub_video_vbe_unmap_color_int (src, src_color, &src_red, &src_green,
                                      &src_blue, &src_alpha);

          if (src_alpha == 0)
            continue;

          if (src_alpha == 255)
            {
              dst_color = grub_video_vbe_map_rgba (src_red, src_green,
                                                   src_blue, src_alpha);
              set_pixel (dst, x + i, y + j, dst_color);
              continue;
            }

          dst_color = get_pixel (dst, x + i, y + j);

          grub_video_vbe_unmap_color_int (dst, dst_color, &dst_red,
                                      &dst_green, &dst_blue, &dst_alpha);

          dst_red = (((src_red * src_alpha)
                      + (dst_red * (255 - src_alpha))) / 255);
          dst_green = (((src_green * src_alpha)
                        + (dst_green * (255 - src_alpha))) / 255);
          dst_blue = (((src_blue * src_alpha)
                       + (dst_blue * (255 - src_alpha))) / 255);

          dst_alpha = src_alpha;
          dst_color = grub_video_vbe_map_rgba (dst_red, dst_green, dst_blue,
                                               dst_alpha);

          set_pixel (dst, x + i, y + j, dst_color);
        }
    }
}

/* Optimized blending blitter for RGBA8888 to BGRA8888.  */
void
grub_video_i386_vbeblit_blend_BGRA8888_RGBA8888 (struct grub_video_i386_vbeblit_info *dst,
						 struct grub_video_i386_vbeblit_info *src,
						 int x, int y,
						 int width, int height,
						 int offset_x, int offset_y)
{
  grub_uint32_t *srcptr;
  grub_uint32_t *dstptr;
  unsigned int srcrowskip;
  unsigned int dstrowskip;
  int i;
  int j;

  /* Calculate the number of bytes to advance from the end of one line
     to the beginning of the next line.  */
  srcrowskip = src->mode_info->pitch - src->mode_info->bytes_per_pixel * width;
  dstrowskip = dst->mode_info->pitch - dst->mode_info->bytes_per_pixel * width;

  srcptr = (grub_uint32_t *) get_data_ptr (src, offset_x, offset_y);
  dstptr = (grub_uint32_t *) get_data_ptr (dst, x, y);

  for (j = 0; j < height; j++)
    {
      for (i = 0; i < width; i++)
        {
          grub_uint32_t color;
          unsigned int sr;
          unsigned int sg;
          unsigned int sb;
          unsigned int a;
          unsigned int dr;
          unsigned int dg;
          unsigned int db;

          color = *srcptr++;

          a = color >> 24;

          if (a == 0)
            {
              /* Skip transparent source pixels.  */
              dstptr++;
              continue;
            }

          sr = (color >> 0) & 0xFF;
          sg = (color >> 8) & 0xFF;
          sb = (color >> 16) & 0xFF;

          if (a == 255)
            {
              /* Opaque pixel shortcut.  */
              dr = sr;
              dg = sg;
              db = sb;
            }
          else
            {
              /* General pixel color blending.  */
              color = *dstptr;

              dr = (color >> 16) & 0xFF;
              dr = (dr * (255 - a) + sr * a) / 255;
              dg = (color >> 8) & 0xFF;
              dg = (dg * (255 - a) + sg * a) / 255;
              db = (color >> 0) & 0xFF;
              db = (db * (255 - a) + sb * a) / 255;
            }

          color = (a << 24) | (dr << 16) | (dg << 8) | db;

          *dstptr++ = color;
        }

      srcptr = (grub_uint32_t *) (((grub_uint8_t *) srcptr) + srcrowskip);
      dstptr = (grub_uint32_t *) (((grub_uint8_t *) dstptr) + dstrowskip);
    }
}

/* Optimized blending blitter for RGBA8888 to BGR888.  */
void
grub_video_i386_vbeblit_blend_BGR888_RGBA8888 (struct grub_video_i386_vbeblit_info *dst,
					       struct grub_video_i386_vbeblit_info *src,
					       int x, int y,
					       int width, int height,
					       int offset_x, int offset_y)
{
  grub_uint32_t *srcptr;
  grub_uint8_t *dstptr;
  unsigned int srcrowskip;
  unsigned int dstrowskip;
  int i;
  int j;

  /* Calculate the number of bytes to advance from the end of one line
     to the beginning of the next line.  */
  srcrowskip = src->mode_info->pitch - src->mode_info->bytes_per_pixel * width;
  dstrowskip = dst->mode_info->pitch - dst->mode_info->bytes_per_pixel * width;

  srcptr = (grub_uint32_t *) get_data_ptr (src, offset_x, offset_y);
  dstptr = (grub_uint8_t *) get_data_ptr (dst, x, y);

  for (j = 0; j < height; j++)
    {
      for (i = 0; i < width; i++)
        {
          grub_uint32_t color;
          unsigned int sr;
          unsigned int sg;
          unsigned int sb;
          unsigned int a;
          unsigned int dr;
          unsigned int dg;
          unsigned int db;

          color = *srcptr++;

          a = color >> 24;

          if (a == 0)
            {
              /* Skip transparent source pixels.  */
              dstptr += 3;
              continue;
            }

          sr = (color >> 0) & 0xFF;
          sg = (color >> 8) & 0xFF;
          sb = (color >> 16) & 0xFF;

          if (a == 255)
            {
              /* Opaque pixel shortcut.  */
              dr = sr;
              dg = sg;
              db = sb;
            }
          else
            {
              /* General pixel color blending.  */
              color = *dstptr;

              db = dstptr[0];
              db = (db * (255 - a) + sb * a) / 255;
              dg = dstptr[1];
              dg = (dg * (255 - a) + sg * a) / 255;
              dr = dstptr[2];
              dr = (dr * (255 - a) + sr * a) / 255;
            }

          *dstptr++ = db;
          *dstptr++ = dg;
          *dstptr++ = dr;
        }

      srcptr = (grub_uint32_t *) (((grub_uint8_t *) srcptr) + srcrowskip);
      dstptr += dstrowskip;
    }
}

/* Optimized blending blitter for RGBA888 to RGBA8888.  */
void
grub_video_i386_vbeblit_blend_RGBA8888_RGBA8888 (struct grub_video_i386_vbeblit_info *dst,
						 struct grub_video_i386_vbeblit_info *src,
						 int x, int y,
						 int width, int height,
						 int offset_x, int offset_y)
{
  grub_uint32_t color;
  int i;
  int j;
  grub_uint32_t *srcptr;
  grub_uint32_t *dstptr;
  unsigned int sr;
  unsigned int sg;
  unsigned int sb;
  unsigned int a;
  unsigned int dr;
  unsigned int dg;
  unsigned int db;

  for (j = 0; j < height; j++)
    {
      srcptr = (grub_uint32_t *)get_data_ptr (src, offset_x, j + offset_y);
      dstptr = (grub_uint32_t *)get_data_ptr (dst, x, y + j);

      for (i = 0; i < width; i++)
        {
          color = *srcptr++;

          a = color >> 24;

          if (a == 0)
            {
              dstptr++;
              continue;
            }

          if (a == 255)
            {
              *dstptr++ = color;
              continue;
            }

          sr = (color >> 0) & 0xFF;
          sg = (color >> 8) & 0xFF;
          sb = (color >> 16) & 0xFF;

          color = *dstptr;

          dr = (color >> 0) & 0xFF;
          dg = (color >> 8) & 0xFF;
          db = (color >> 16) & 0xFF;

          dr = (dr * (255 - a) + sr * a) / 255;
          dg = (dg * (255 - a) + sg * a) / 255;
          db = (db * (255 - a) + sb * a) / 255;

          color = (a << 24) | (db << 16) | (dg << 8) | dr;

          *dstptr++ = color;
        }
    }
}

/* Optimized blending blitter for RGBA8888 to RGB888.  */
void
grub_video_i386_vbeblit_blend_RGB888_RGBA8888 (struct grub_video_i386_vbeblit_info *dst,
					       struct grub_video_i386_vbeblit_info *src,
					       int x, int y,
					       int width, int height,
					       int offset_x, int offset_y)
{
  grub_uint32_t color;
  int i;
  int j;
  grub_uint32_t *srcptr;
  grub_uint8_t *dstptr;
  unsigned int sr;
  unsigned int sg;
  unsigned int sb;
  unsigned int a;
  unsigned int dr;
  unsigned int dg;
  unsigned int db;

  for (j = 0; j < height; j++)
    {
      srcptr = (grub_uint32_t *)get_data_ptr (src, offset_x, j + offset_y);
      dstptr = (grub_uint8_t *)get_data_ptr (dst, x, y + j);

      for (i = 0; i < width; i++)
        {
          color = *srcptr++;

          a = color >> 24;

          if (a == 0)
            {
              dstptr += 3;
              continue;
            }

          sr = (color >> 0) & 0xFF;
          sg = (color >> 8) & 0xFF;
          sb = (color >> 16) & 0xFF;

          if (a == 255)
            {
              *dstptr++ = sr;
              *dstptr++ = sg;
              *dstptr++ = sb;

              continue;
            }

          dr = dstptr[0];
          dg = dstptr[1];
          db = dstptr[2];

          dr = (dr * (255 - a) + sr * a) / 255;
          dg = (dg * (255 - a) + sg * a) / 255;
          db = (db * (255 - a) + sb * a) / 255;

          *dstptr++ = dr;
          *dstptr++ = dg;
          *dstptr++ = db;
        }
    }
}

/* Optimized blending blitter for RGBA8888 to indexed color.  */
void
grub_video_i386_vbeblit_blend_index_RGBA8888 (struct grub_video_i386_vbeblit_info *dst,
					      struct grub_video_i386_vbeblit_info *src,
					      int x, int y,
					      int width, int height,
					      int offset_x, int offset_y)
{
  grub_uint32_t color;
  int i;
  int j;
  grub_uint32_t *srcptr;
  grub_uint8_t *dstptr;
  unsigned int sr;
  unsigned int sg;
  unsigned int sb;
  unsigned int a;
  unsigned char dr;
  unsigned char dg;
  unsigned char db;
  unsigned char da;

  for (j = 0; j < height; j++)
    {
      srcptr = (grub_uint32_t *)get_data_ptr (src, offset_x, j + offset_y);
      dstptr = (grub_uint8_t *)get_data_ptr (dst, x, y + j);

      for (i = 0; i < width; i++)
        {
          color = *srcptr++;

          a = color >> 24;

          if (a == 0)
            {
              dstptr++;
              continue;
            }

          sr = (color >> 0) & 0xFF;
          sg = (color >> 8) & 0xFF;
          sb = (color >> 16) & 0xFF;

          if (a == 255)
            {
              color = grub_video_vbe_map_rgb(sr, sg, sb);
              *dstptr++ = color & 0xFF;
              continue;
            }

          grub_video_vbe_unmap_color_int (dst, *dstptr, &dr, &dg, &db, &da);

          dr = (dr * (255 - a) + sr * a) / 255;
          dg = (dg * (255 - a) + sg * a) / 255;
          db = (db * (255 - a) + sb * a) / 255;

          color = grub_video_vbe_map_rgb(dr, dg, db);

          *dstptr++ = color & 0xFF;
        }
    }
}
