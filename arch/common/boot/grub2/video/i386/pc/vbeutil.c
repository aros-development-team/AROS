/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2006,2007  Free Software Foundation, Inc.
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

#include <grub/machine/vbeutil.h>
#include <grub/types.h>
#include <grub/video.h>

grub_uint8_t *
get_data_ptr (struct grub_video_i386_vbeblit_info *source,
              unsigned int x, unsigned int y)
{
  grub_uint8_t *ptr = 0;

  switch (source->mode_info->bpp)
    {
    case 32:
      ptr = (grub_uint8_t *)source->data
            + y * source->mode_info->pitch
            + x * 4;
      break;

    case 24:
      ptr = (grub_uint8_t *)source->data
            + y * source->mode_info->pitch
            + x * 3;
      break;

    case 16:
    case 15:
      ptr = (grub_uint8_t *)source->data
            + y * source->mode_info->pitch
            + x * 2;
      break;

    case 8:
      ptr = (grub_uint8_t *)source->data
            + y * source->mode_info->pitch
            + x;
      break;
    }

  return ptr;
}

grub_video_color_t
get_pixel (struct grub_video_i386_vbeblit_info *source,
           unsigned int x, unsigned int y)
{
  grub_video_color_t color = 0;

  switch (source->mode_info->bpp)
    {
    case 32:
      color = *(grub_uint32_t *)get_data_ptr (source, x, y);
      break;

    case 24:
      {
        grub_uint8_t *ptr;
        ptr = get_data_ptr (source, x, y);
        color = ptr[0] | (ptr[1] << 8) | (ptr[2] << 16);
      }
      break;

    case 16:
    case 15:
      color = *(grub_uint16_t *)get_data_ptr (source, x, y);
      break;

    case 8:
      color = *(grub_uint8_t *)get_data_ptr (source, x, y);
      break;

    default:
      break;
    }

  return color;
}

void
set_pixel (struct grub_video_i386_vbeblit_info *source,
           unsigned int x, unsigned int y, grub_video_color_t color)
{
  switch (source->mode_info->bpp)
    {
    case 32:
      {
        grub_uint32_t *ptr;

        ptr = (grub_uint32_t *)get_data_ptr (source, x, y);

        *ptr = color;
      }
      break;

    case 24:
      {
        grub_uint8_t *ptr;
        grub_uint8_t *colorptr = (grub_uint8_t *)&color;

        ptr = get_data_ptr (source, x, y);

        ptr[0] = colorptr[0];
        ptr[1] = colorptr[1];
        ptr[2] = colorptr[2];
      }
      break;

    case 16:
    case 15:
      {
        grub_uint16_t *ptr;

        ptr = (grub_uint16_t *)get_data_ptr (source, x, y);

        *ptr = (grub_uint16_t) (color & 0xFFFF);
      }
      break;

    case 8:
      {
        grub_uint8_t *ptr;

        ptr = (grub_uint8_t *)get_data_ptr (source, x, y);

        *ptr = (grub_uint8_t) (color & 0xFF);
      }
      break;

    default:
      break;
    }
}
