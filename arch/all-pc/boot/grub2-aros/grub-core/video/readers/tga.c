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

#include <grub/bitmap.h>
#include <grub/types.h>
#include <grub/normal.h>
#include <grub/dl.h>
#include <grub/mm.h>
#include <grub/misc.h>
#include <grub/bufio.h>

GRUB_MOD_LICENSE ("GPLv3+");

/* Uncomment following define to enable TGA debug.  */
//#define TGA_DEBUG

#if defined(TGA_DEBUG)
#define dump_int_field(x) grub_printf( #x " = %d (0x%04x)\n", x, x);
static grub_command_t cmd;
#endif

enum
{
  GRUB_TGA_IMAGE_TYPE_NONE = 0,
  GRUB_TGA_IMAGE_TYPE_UNCOMPRESSED_INDEXCOLOR = 1,
  GRUB_TGA_IMAGE_TYPE_UNCOMPRESSED_TRUECOLOR = 2,
  GRUB_TGA_IMAGE_TYPE_UNCOMPRESSED_BLACK_AND_WHITE = 3,
  GRUB_TGA_IMAGE_TYPE_RLE_INDEXCOLOR = 9,
  GRUB_TGA_IMAGE_TYPE_RLE_TRUECOLOR = 10,
  GRUB_TGA_IMAGE_TYPE_RLE_BLACK_AND_WHITE = 11,
};

enum
{
  GRUB_TGA_COLOR_MAP_TYPE_NONE = 0,
  GRUB_TGA_COLOR_MAP_TYPE_INCLUDED = 1
};

enum
{
  GRUB_TGA_IMAGE_ORIGIN_RIGHT = 0x10,
  GRUB_TGA_IMAGE_ORIGIN_TOP   = 0x20
};

struct grub_tga_header
{
  grub_uint8_t id_length;
  grub_uint8_t color_map_type;
  grub_uint8_t image_type;

  /* Color Map Specification.  */
  grub_uint16_t color_map_first_index;
  grub_uint16_t color_map_length;
  grub_uint8_t color_map_bpp;

  /* Image Specification.  */
  grub_uint16_t image_x_origin;
  grub_uint16_t image_y_origin;
  grub_uint16_t image_width;
  grub_uint16_t image_height;
  grub_uint8_t image_bpp;
  grub_uint8_t image_descriptor;
} __attribute__ ((packed));

static grub_err_t
tga_load_truecolor_rle_R8G8B8 (struct grub_video_bitmap *bitmap,
                               struct grub_tga_header *header,
                               grub_file_t file)
{
  unsigned int x;
  unsigned int y;
  grub_uint8_t type;
  grub_uint8_t *ptr;
  grub_uint8_t tmp[4]; /* Size should be max_bpp / 8.  */
  grub_uint8_t bytes_per_pixel;

  bytes_per_pixel = header->image_bpp / 8;

  for (y = 0; y < header->image_height; y++)
    {
      ptr = bitmap->data;
      if ((header->image_descriptor & GRUB_TGA_IMAGE_ORIGIN_TOP) != 0)
        ptr += y * bitmap->mode_info.pitch;
      else
        ptr += (header->image_height - 1 - y) * bitmap->mode_info.pitch;

      for (x = 0; x < header->image_width;)
        {
          if (grub_file_read (file, &type, sizeof (type)) != sizeof(type))
            return grub_errno;

          if (type & 0x80)
            {
              /* RLE-encoded packet.  */
              type &= 0x7f;
              type++;

              if (grub_file_read (file, &tmp[0], bytes_per_pixel)
                  != bytes_per_pixel)
                return grub_errno;

              while (type)
                {
                  if (x < header->image_width)
                    {
                      ptr[0] = tmp[2];
                      ptr[1] = tmp[1];
                      ptr[2] = tmp[0];
                      ptr += 3;
                    }

                  type--;
                  x++;
                }
            }
          else
            {
              /* RAW-encoded packet.  */
              type++;

              while (type)
                {
                  if (grub_file_read (file, &tmp[0], bytes_per_pixel)
                      != bytes_per_pixel)
                    return grub_errno;

                  if (x < header->image_width)
                    {
                      ptr[0] = tmp[2];
                      ptr[1] = tmp[1];
                      ptr[2] = tmp[0];
                      ptr += 3;
                    }

                  type--;
                  x++;
                }
            }
        }
    }
  return GRUB_ERR_NONE;
}

static grub_err_t
tga_load_truecolor_rle_R8G8B8A8 (struct grub_video_bitmap *bitmap,
                                 struct grub_tga_header *header,
                                 grub_file_t file)
{
  unsigned int x;
  unsigned int y;
  grub_uint8_t type;
  grub_uint8_t *ptr;
  grub_uint8_t tmp[4]; /* Size should be max_bpp / 8.  */
  grub_uint8_t bytes_per_pixel;

  bytes_per_pixel = header->image_bpp / 8;

  for (y = 0; y < header->image_height; y++)
    {
      ptr = bitmap->data;
      if ((header->image_descriptor & GRUB_TGA_IMAGE_ORIGIN_TOP) != 0)
        ptr += y * bitmap->mode_info.pitch;
      else
        ptr += (header->image_height - 1 - y) * bitmap->mode_info.pitch;

      for (x = 0; x < header->image_width;)
        {
          if (grub_file_read (file, &type, sizeof (type)) != sizeof(type))
            return grub_errno;

          if (type & 0x80)
            {
              /* RLE-encoded packet.  */
              type &= 0x7f;
              type++;

              if (grub_file_read (file, &tmp[0], bytes_per_pixel)
                  != bytes_per_pixel)
                return grub_errno;

              while (type)
                {
                  if (x < header->image_width)
                    {
                      ptr[0] = tmp[2];
                      ptr[1] = tmp[1];
                      ptr[2] = tmp[0];
                      ptr[3] = tmp[3];
                      ptr += 4;
                    }

                  type--;
                  x++;
                }
            }
          else
            {
              /* RAW-encoded packet.  */
              type++;

              while (type)
                {
                  if (grub_file_read (file, &tmp[0], bytes_per_pixel)
                      != bytes_per_pixel)
                    return grub_errno;

                  if (x < header->image_width)
                    {
                      ptr[0] = tmp[2];
                      ptr[1] = tmp[1];
                      ptr[2] = tmp[0];
                      ptr[3] = tmp[3];
                      ptr += 4;
                    }

                  type--;
                  x++;
                }
            }
        }
    }
  return GRUB_ERR_NONE;
}

static grub_err_t
tga_load_truecolor_R8G8B8 (struct grub_video_bitmap *bitmap,
                           struct grub_tga_header *header,
                           grub_file_t file)
{
  unsigned int x;
  unsigned int y;
  grub_uint8_t *ptr;
  grub_uint8_t tmp[4]; /* Size should be max_bpp / 8.  */
  grub_uint8_t bytes_per_pixel;

  bytes_per_pixel = header->image_bpp / 8;

  for (y = 0; y < header->image_height; y++)
    {
      ptr = bitmap->data;
      if ((header->image_descriptor & GRUB_TGA_IMAGE_ORIGIN_TOP) != 0)
        ptr += y * bitmap->mode_info.pitch;
      else
        ptr += (header->image_height - 1 - y) * bitmap->mode_info.pitch;

      for (x = 0; x < header->image_width; x++)
        {
          if (grub_file_read (file, &tmp[0], bytes_per_pixel)
              != bytes_per_pixel)
            return grub_errno;

          ptr[0] = tmp[2];
          ptr[1] = tmp[1];
          ptr[2] = tmp[0];

          ptr += 3;
        }
    }
  return GRUB_ERR_NONE;
}

static grub_err_t
tga_load_truecolor_R8G8B8A8 (struct grub_video_bitmap *bitmap,
                             struct grub_tga_header *header,
                             grub_file_t file)
{
  unsigned int x;
  unsigned int y;
  grub_uint8_t *ptr;
  grub_uint8_t tmp[4]; /* Size should be max_bpp / 8.  */
  grub_uint8_t bytes_per_pixel;

  bytes_per_pixel = header->image_bpp / 8;

  for (y = 0; y < header->image_height; y++)
    {
      ptr = bitmap->data;
      if ((header->image_descriptor & GRUB_TGA_IMAGE_ORIGIN_TOP) != 0)
        ptr += y * bitmap->mode_info.pitch;
      else
        ptr += (header->image_height - 1 - y) * bitmap->mode_info.pitch;

      for (x = 0; x < header->image_width; x++)
        {
          if (grub_file_read (file, &tmp[0], bytes_per_pixel)
              != bytes_per_pixel)
            return grub_errno;

          ptr[0] = tmp[2];
          ptr[1] = tmp[1];
          ptr[2] = tmp[0];
          ptr[3] = tmp[3];

          ptr += 4;
        }
    }
  return GRUB_ERR_NONE;
}

static grub_err_t
grub_video_reader_tga (struct grub_video_bitmap **bitmap,
                       const char *filename)
{
  grub_file_t file;
  grub_ssize_t pos;
  struct grub_tga_header header;
  int has_alpha;

  file = grub_buffile_open (filename, 0);
  if (! file)
    return grub_errno;

  /* TGA Specification states that we SHOULD start by reading
     ID from end of file, but we really don't care about that as we are
     not going to support developer area & extensions at this point.  */

  /* Read TGA header from beginning of file.  */
  if (grub_file_read (file, &header, sizeof (header))
      != sizeof (header))
    {
      grub_file_close (file);
      return grub_errno;
    }

  /* Skip ID field.  */
  pos = grub_file_tell (file);
  pos += header.id_length;
  grub_file_seek (file, pos);
  if (grub_errno != GRUB_ERR_NONE)
    {
      grub_file_close (file);
      return grub_errno;
    }

#if defined(TGA_DEBUG)
  grub_printf("tga: header\n");
  dump_int_field(header.id_length);
  dump_int_field(header.color_map_type);
  dump_int_field(header.image_type);
  dump_int_field(header.color_map_first_index);
  dump_int_field(header.color_map_length);
  dump_int_field(header.color_map_bpp);
  dump_int_field(header.image_x_origin);
  dump_int_field(header.image_y_origin);
  dump_int_field(header.image_width);
  dump_int_field(header.image_height);
  dump_int_field(header.image_bpp);
  dump_int_field(header.image_descriptor);
#endif

  /* Check that bitmap encoding is supported.  */
  switch (header.image_type)
    {
      case GRUB_TGA_IMAGE_TYPE_UNCOMPRESSED_TRUECOLOR:
      case GRUB_TGA_IMAGE_TYPE_RLE_TRUECOLOR:
        break;

      default:
        grub_file_close (file);
        return grub_error (GRUB_ERR_BAD_FILE_TYPE,
                           "unsupported bitmap format (unknown encoding)");
    }

  /* Check that bitmap depth is supported.  */
  switch (header.image_bpp)
    {
      case 24:
        has_alpha = 0;
        break;

      case 32:
        has_alpha = 1;
        break;

      default:
        grub_file_close (file);
        return grub_error (GRUB_ERR_BAD_FILE_TYPE,
                           "unsupported bitmap format (bpp=%d)",
                           header.image_bpp);
    }

  /* Allocate bitmap.  If there is alpha information store it too.  */
  if (has_alpha)
    {
      grub_video_bitmap_create (bitmap, header.image_width,
                                header.image_height,
                                GRUB_VIDEO_BLIT_FORMAT_RGBA_8888);
      if (grub_errno != GRUB_ERR_NONE)
        {
          grub_file_close (file);
          return grub_errno;
        }

      /* Load bitmap data.  */
      switch (header.image_type)
        {
          case GRUB_TGA_IMAGE_TYPE_UNCOMPRESSED_TRUECOLOR:
            tga_load_truecolor_R8G8B8A8 (*bitmap, &header, file);
            break;

          case GRUB_TGA_IMAGE_TYPE_RLE_TRUECOLOR:
            tga_load_truecolor_rle_R8G8B8A8 (*bitmap, &header, file);
            break;
        }
    }
  else
    {
      grub_video_bitmap_create (bitmap, header.image_width,
                                header.image_height,
                                GRUB_VIDEO_BLIT_FORMAT_RGB_888);
      if (grub_errno != GRUB_ERR_NONE)
        {
          grub_file_close (file);
          return grub_errno;
        }

      /* Load bitmap data.  */
      switch (header.image_type)
        {
          case GRUB_TGA_IMAGE_TYPE_UNCOMPRESSED_TRUECOLOR:
            tga_load_truecolor_R8G8B8 (*bitmap, &header, file);
            break;

          case GRUB_TGA_IMAGE_TYPE_RLE_TRUECOLOR:
            tga_load_truecolor_rle_R8G8B8 (*bitmap, &header, file);
            break;
        }
    }

  /* If there was a loading problem, destroy bitmap.  */
  if (grub_errno != GRUB_ERR_NONE)
    {
      grub_video_bitmap_destroy (*bitmap);
      *bitmap = 0;
    }

  grub_file_close (file);
  return grub_errno;
}

#if defined(TGA_DEBUG)
static grub_err_t
grub_cmd_tgatest (grub_command_t cmd __attribute__ ((unused)),
                  int argc, char **args)
{
  struct grub_video_bitmap *bitmap = 0;

  if (argc != 1)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, "file name required");

  grub_video_reader_tga (&bitmap, args[0]);
  if (grub_errno != GRUB_ERR_NONE)
    return grub_errno;

  grub_video_bitmap_destroy (bitmap);

  return GRUB_ERR_NONE;
}
#endif

static struct grub_video_bitmap_reader tga_reader = {
  .extension = ".tga",
  .reader = grub_video_reader_tga,
  .next = 0
};

GRUB_MOD_INIT(tga)
{
  grub_video_bitmap_reader_register (&tga_reader);
#if defined(TGA_DEBUG)
  cmd = grub_register_command ("tgatest", grub_cmd_tgatest,
                               "FILE", "Tests loading of TGA bitmap.");
#endif
}

GRUB_MOD_FINI(tga)
{
#if defined(TGA_DEBUG)
  grub_unregister_command (cmd);
#endif
  grub_video_bitmap_reader_unregister (&tga_reader);
}
