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

void
grub_video_i386_vbeblit_R8G8B8A8_R8G8B8A8 (struct grub_video_i386_vbeblit_info *dst,
                                           struct grub_video_i386_vbeblit_info *src,
                                           int x, int y, int width, int height,
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

  /* We do not need to worry about data being out of bounds
     as we assume that everything has been checked before.  */

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

void
grub_video_i386_vbeblit_R8G8B8X8_R8G8B8X8 (struct grub_video_i386_vbeblit_info *dst,
                                           struct grub_video_i386_vbeblit_info *src,
                                           int x, int y, int width, int height,
                                           int offset_x, int offset_y)
{
  int j;
  grub_uint32_t *srcptr;
  grub_uint32_t *dstptr;
  int pitch;

  pitch = src->mode_info->bytes_per_pixel;

  /* We do not need to worry about data being out of bounds
     as we assume that everything has been checked before.  */

  for (j = 0; j < height; j++)
    {
      srcptr = (grub_uint32_t *)get_data_ptr (src, offset_x, j + offset_y);
      dstptr = (grub_uint32_t *)get_data_ptr (dst, x, y + j);

      grub_memmove (dstptr, srcptr, width * pitch);
    }
}

void
grub_video_i386_vbeblit_R8G8B8_R8G8B8A8 (struct grub_video_i386_vbeblit_info *dst,
                                         struct grub_video_i386_vbeblit_info *src,
                                         int x, int y, int width, int height,
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

  /* We do not need to worry about data being out of bounds
     as we assume that everything has been checked before.  */

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

void
grub_video_i386_vbeblit_R8G8B8_R8G8B8X8 (struct grub_video_i386_vbeblit_info *dst,
                                         struct grub_video_i386_vbeblit_info *src,
                                         int x, int y, int width, int height,
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

  /* We do not need to worry about data being out of bounds
     as we assume that everything has been checked before.  */

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

void
grub_video_i386_vbeblit_index_R8G8B8A8 (struct grub_video_i386_vbeblit_info *dst,
                                        struct grub_video_i386_vbeblit_info *src,
                                        int x, int y, int width, int height,
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

  /* We do not need to worry about data being out of bounds
     as we assume that everything has been checked before.  */

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

void
grub_video_i386_vbeblit_index_R8G8B8X8 (struct grub_video_i386_vbeblit_info *dst,
                                        struct grub_video_i386_vbeblit_info *src,
                                        int x, int y, int width, int height,
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

  /* We do not need to worry about data being out of bounds
  as we assume that everything has been checked before.  */

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

void
grub_video_i386_vbeblit_R8G8B8A8_R8G8B8 (struct grub_video_i386_vbeblit_info *dst,
                                         struct grub_video_i386_vbeblit_info *src,
                                         int x, int y, int width, int height,
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

  /* We do not need to worry about data being out of bounds
     as we assume that everything has been checked before.  */

  for (j = 0; j < height; j++)
    {
      srcptr = (grub_uint8_t *)get_data_ptr (src, offset_x, j + offset_y);
      dstptr = (grub_uint32_t *)get_data_ptr (dst, x, y + j);

      for (i = 0; i < width; i++)
        {
          sr = *srcptr++;
          sg = *srcptr++;
          sb = *srcptr++;

          color = 0xFF000000 | (sb << 16) | (sg << 8) | sr;

          *dstptr++ = color;
        }
    }
}

void
grub_video_i386_vbeblit_R8G8B8_R8G8B8 (struct grub_video_i386_vbeblit_info *dst,
                                       struct grub_video_i386_vbeblit_info *src,
                                       int x, int y, int width, int height,
                                       int offset_x, int offset_y)
{
  int j;
  grub_uint8_t *srcptr;
  grub_uint8_t *dstptr;
  int pitch;

  pitch = src->mode_info->bytes_per_pixel;

  /* We do not need to worry about data being out of bounds
     as we assume that everything has been checked before.  */

  for (j = 0; j < height; j++)
    {
      srcptr = (grub_uint8_t *)get_data_ptr (src, offset_x, j + offset_y);
      dstptr = (grub_uint8_t *)get_data_ptr (dst, x, y + j);

      grub_memmove (dstptr, srcptr, width * pitch);
    }
}

void
grub_video_i386_vbeblit_index_R8G8B8 (struct grub_video_i386_vbeblit_info *dst,
                                      struct grub_video_i386_vbeblit_info *src,
                                      int x, int y, int width, int height,
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

  /* We do not need to worry about data being out of bounds
     as we assume that everything has been checked before.  */

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

void
grub_video_i386_vbeblit_index_index (struct grub_video_i386_vbeblit_info *dst,
                                     struct grub_video_i386_vbeblit_info *src,
                                     int x, int y, int width, int height,
                                     int offset_x, int offset_y)
{
  int j;
  grub_uint8_t *srcptr;
  grub_uint8_t *dstptr;
  int pitch;

  pitch = src->mode_info->bytes_per_pixel;

  /* We do not need to worry about data being out of bounds
     as we assume that everything has been checked before.  */

  for (j = 0; j < height; j++)
    {
      srcptr = (grub_uint8_t *)get_data_ptr (src, offset_x, j + offset_y);
      dstptr = (grub_uint8_t *)get_data_ptr (dst, x, y + j);

      grub_memmove (dstptr, srcptr, width * pitch);
    }
}

void
grub_video_i386_vbeblit_blend (struct grub_video_i386_vbeblit_info *dst,
                               struct grub_video_i386_vbeblit_info *src,
                               int x, int y, int width, int height,
                               int offset_x, int offset_y)
{
  int i;
  int j;

  /* We do not need to worry about data being out of bounds
     as we assume that everything has been checked before.  */

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

  /* We do not need to worry about data being out of bounds
     as we assume that everything has been checked before.  */

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
