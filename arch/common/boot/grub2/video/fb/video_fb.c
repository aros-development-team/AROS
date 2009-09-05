/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2005,2006,2007,2008,2009  Free Software Foundation, Inc.
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

#include <grub/video.h>
#include <grub/video_fb.h>
#include <grub/misc.h>
#include <grub/mm.h>
#include <grub/fbblit.h>
#include <grub/fbfill.h>
#include <grub/fbutil.h>
#include <grub/bitmap.h>

static struct grub_video_fbrender_target *render_target;
struct grub_video_palette_data *palette;
static unsigned int palette_size;

/* Specify "standard" VGA palette, some video cards may
   need this and this will also be used when using RGB modes.  */
struct grub_video_palette_data grub_video_fbstd_colors[GRUB_VIDEO_FBSTD_NUMCOLORS] =
  {
    // {R, G, B, A}
    {0x00, 0x00, 0x00, 0xFF}, // 0 = black
    {0x00, 0x00, 0xA8, 0xFF}, // 1 = blue
    {0x00, 0xA8, 0x00, 0xFF}, // 2 = green
    {0x00, 0xA8, 0xA8, 0xFF}, // 3 = cyan
    {0xA8, 0x00, 0x00, 0xFF}, // 4 = red
    {0xA8, 0x00, 0xA8, 0xFF}, // 5 = magenta
    {0xA8, 0x54, 0x00, 0xFF}, // 6 = brown
    {0xA8, 0xA8, 0xA8, 0xFF}, // 7 = light gray

    {0x54, 0x54, 0x54, 0xFF}, // 8 = dark gray
    {0x54, 0x54, 0xFE, 0xFF}, // 9 = bright blue
    {0x54, 0xFE, 0x54, 0xFF}, // 10 = bright green
    {0x54, 0xFE, 0xFE, 0xFF}, // 11 = bright cyan
    {0xFE, 0x54, 0x54, 0xFF}, // 12 = bright red
    {0xFE, 0x54, 0xFE, 0xFF}, // 13 = bright magenta
    {0xFE, 0xFE, 0x54, 0xFF}, // 14 = yellow
    {0xFE, 0xFE, 0xFE, 0xFF}  // 15 = white
  };

grub_err_t
grub_video_fb_init (void)
{
  grub_free (palette);
  render_target = 0;
  palette = 0;
  palette_size = 0;
  return GRUB_ERR_NONE;
}

grub_err_t
grub_video_fb_fini (void)
{
  grub_free (palette);
  render_target = 0;
  palette = 0;
  palette_size = 0;
  return GRUB_ERR_NONE;
}

grub_err_t
grub_video_fb_get_info (struct grub_video_mode_info *mode_info)
{
  /* Copy mode info from active render target.  */
  grub_memcpy (mode_info, &render_target->mode_info,
               sizeof (struct grub_video_mode_info));

  return GRUB_ERR_NONE;
}

grub_err_t
grub_video_fb_get_palette (unsigned int start, unsigned int count,
			   struct grub_video_palette_data *palette_data)
{
  unsigned int i;

  /* Assume that we know everything from index color palette.  */
  for (i = 0; (i < count) && ((i + start) < palette_size); i++)
    palette_data[i] = palette[start + i];

  return GRUB_ERR_NONE;
}

grub_err_t
grub_video_fb_set_palette (unsigned int start, unsigned int count,
			   struct grub_video_palette_data *palette_data)
{
  unsigned i;
  if (start + count > palette_size)
    {
      palette_size = start + count;
      palette = grub_realloc (palette, sizeof (palette[0]) * palette_size);
      if (!palette)
	{
	  grub_video_fb_fini ();
	  return grub_errno;
	}
    }
  for (i = 0; (i < count) && ((i + start) < palette_size); i++)
    palette[start + i] = palette_data[i];
  return GRUB_ERR_NONE;
}

grub_err_t
grub_video_fb_set_viewport (unsigned int x, unsigned int y,
			    unsigned int width, unsigned int height)
{
  /* Make sure viewport is withing screen dimensions.  If viewport was set
     to be out of the region, mark its size as zero.  */
  if (x > render_target->mode_info.width)
    {
      x = 0;
      width = 0;
    }

  if (y > render_target->mode_info.height)
    {
      y = 0;
      height = 0;
    }

  if (x + width > render_target->mode_info.width)
    width = render_target->mode_info.width - x;

  if (y + height > render_target->mode_info.height)
    height = render_target->mode_info.height - y;

  render_target->viewport.x = x;
  render_target->viewport.y = y;
  render_target->viewport.width = width;
  render_target->viewport.height = height;

  return GRUB_ERR_NONE;
}

grub_err_t
grub_video_fb_get_viewport (unsigned int *x, unsigned int *y,
			    unsigned int *width, unsigned int *height)
{
  if (x) *x = render_target->viewport.x;
  if (y) *y = render_target->viewport.y;
  if (width) *width = render_target->viewport.width;
  if (height) *height = render_target->viewport.height;

  return GRUB_ERR_NONE;
}

/* Maps color name to target optimized color format.  */
grub_video_color_t
grub_video_fb_map_color (grub_uint32_t color_name)
{
  /* TODO: implement color theme mapping code.  */

  if (color_name < palette_size)
    {
      if ((render_target->mode_info.mode_type
           & GRUB_VIDEO_MODE_TYPE_INDEX_COLOR) != 0)
        return color_name;
      else
        {
          grub_video_color_t color;

          color = grub_video_fb_map_rgb (palette[color_name].r,
					 palette[color_name].g,
					 palette[color_name].b);

          return color;
        }
    }

  return 0;
}

/* Maps RGB to target optimized color format.  */
grub_video_color_t
grub_video_fb_map_rgb (grub_uint8_t red, grub_uint8_t green,
		       grub_uint8_t blue)
{
  if ((render_target->mode_info.mode_type
       & GRUB_VIDEO_MODE_TYPE_INDEX_COLOR) != 0)
    {
      int minindex = 0;
      int delta = 0;
      int tmp;
      int val;
      unsigned i;

      /* Find best matching color.  */
      for (i = 0; i < palette_size; i++)
        {
          val = palette[i].r - red;
          tmp = val * val;
          val = palette[i].g - green;
          tmp += val * val;
          val = palette[i].b - blue;
          tmp += val * val;

          if (i == 0)
            delta = tmp;

          if (tmp < delta)
            {
              delta = tmp;
              minindex = i;
              if (tmp == 0)
                break;
            }
        }

      return minindex;
    }
  else if ((render_target->mode_info.mode_type
            & GRUB_VIDEO_MODE_TYPE_1BIT_BITMAP) != 0)
    {
       if (red == render_target->mode_info.fg_red
           && green == render_target->mode_info.fg_green
           && blue == render_target->mode_info.fg_blue)
         return 1;
       else
         return 0;
    }
  else
    {
      grub_uint32_t value;
      grub_uint8_t alpha = 255; /* Opaque color.  */

      red >>= 8 - render_target->mode_info.red_mask_size;
      green >>= 8 - render_target->mode_info.green_mask_size;
      blue >>= 8 - render_target->mode_info.blue_mask_size;
      alpha >>= 8 - render_target->mode_info.reserved_mask_size;

      value = red << render_target->mode_info.red_field_pos;
      value |= green << render_target->mode_info.green_field_pos;
      value |= blue << render_target->mode_info.blue_field_pos;
      value |= alpha << render_target->mode_info.reserved_field_pos;

      return value;
    }

}

/* Maps RGBA to target optimized color format.  */
grub_video_color_t
grub_video_fb_map_rgba (grub_uint8_t red, grub_uint8_t green,
			grub_uint8_t blue, grub_uint8_t alpha)
{
  if ((render_target->mode_info.mode_type
       & GRUB_VIDEO_MODE_TYPE_INDEX_COLOR) != 0)
    /* No alpha available in index color modes, just use
       same value as in only RGB modes.  */
    return grub_video_fb_map_rgb (red, green, blue);
  else if ((render_target->mode_info.mode_type
            & GRUB_VIDEO_MODE_TYPE_1BIT_BITMAP) != 0)
    {
      if (red == render_target->mode_info.fg_red
          && green == render_target->mode_info.fg_green
          && blue == render_target->mode_info.fg_blue
          && alpha == render_target->mode_info.fg_alpha)
        return 1;
      else
        return 0;
    }
  else
    {
      grub_uint32_t value;

      red >>= 8 - render_target->mode_info.red_mask_size;
      green >>= 8 - render_target->mode_info.green_mask_size;
      blue >>= 8 - render_target->mode_info.blue_mask_size;
      alpha >>= 8 - render_target->mode_info.reserved_mask_size;

      value = red << render_target->mode_info.red_field_pos;
      value |= green << render_target->mode_info.green_field_pos;
      value |= blue << render_target->mode_info.blue_field_pos;
      value |= alpha << render_target->mode_info.reserved_field_pos;

      return value;
    }
}

/* Splits target optimized format to components.  */
grub_err_t
grub_video_fb_unmap_color (grub_video_color_t color,
			   grub_uint8_t *red, grub_uint8_t *green,
			   grub_uint8_t *blue, grub_uint8_t *alpha)
{
  struct grub_video_fbblit_info target_info;

  target_info.mode_info = &render_target->mode_info;
  target_info.data = render_target->data;

  grub_video_fb_unmap_color_int (&target_info, color, red, green, blue, alpha);

  return GRUB_ERR_NONE;
}

/* Splits color in source format to components.  */
void
grub_video_fb_unmap_color_int (struct grub_video_fbblit_info * source,
			       grub_video_color_t color,
			       grub_uint8_t *red, grub_uint8_t *green,
			       grub_uint8_t *blue, grub_uint8_t *alpha)
{
  struct grub_video_mode_info *mode_info;
  mode_info = source->mode_info;

  if ((mode_info->mode_type
       & GRUB_VIDEO_MODE_TYPE_INDEX_COLOR) != 0)
    {
      /* If we have an out-of-bounds color, return transparent black.  */
      if (color > 255)
        {
          *red = 0;
          *green = 0;
          *blue = 0;
          *alpha = 0;
          return;
        }

      *red = palette[color].r;
      *green = palette[color].g;
      *blue = palette[color].b;
      *alpha = palette[color].a;
      return;
    }
  else if ((mode_info->mode_type
            & GRUB_VIDEO_MODE_TYPE_1BIT_BITMAP) != 0)
    {
      if (color & 1)
        {
          *red = mode_info->fg_red;
          *green = mode_info->fg_green;
          *blue = mode_info->fg_blue;
          *alpha = mode_info->fg_alpha;
        }
      else
        {
          *red = mode_info->bg_red;
          *green = mode_info->bg_green;
          *blue = mode_info->bg_blue;
          *alpha = mode_info->bg_alpha;
        }
    }
  else
    {
      grub_uint32_t tmp;

      /* Get red component.  */
      tmp = color >> mode_info->red_field_pos;
      tmp &= (1 << mode_info->red_mask_size) - 1;
      tmp <<= 8 - mode_info->red_mask_size;
      tmp |= (1 << (8 - mode_info->red_mask_size)) - 1;
      *red = tmp & 0xFF;

      /* Get green component.  */
      tmp = color >> mode_info->green_field_pos;
      tmp &= (1 << mode_info->green_mask_size) - 1;
      tmp <<= 8 - mode_info->green_mask_size;
      tmp |= (1 << (8 - mode_info->green_mask_size)) - 1;
      *green = tmp & 0xFF;

      /* Get blue component.  */
      tmp = color >> mode_info->blue_field_pos;
      tmp &= (1 << mode_info->blue_mask_size) - 1;
      tmp <<= 8 - mode_info->blue_mask_size;
      tmp |= (1 << (8 - mode_info->blue_mask_size)) - 1;
      *blue = tmp & 0xFF;

      /* Get alpha component.  */
      if (source->mode_info->reserved_mask_size > 0)
        {
          tmp = color >> mode_info->reserved_field_pos;
          tmp &= (1 << mode_info->reserved_mask_size) - 1;
          tmp <<= 8 - mode_info->reserved_mask_size;
          tmp |= (1 << (8 - mode_info->reserved_mask_size)) - 1;
        }
      else
        /* If there is no alpha component, assume it opaque.  */
        tmp = 255;

      *alpha = tmp & 0xFF;
    }
}

grub_err_t
grub_video_fb_fill_rect (grub_video_color_t color, int x, int y,
			 unsigned int width, unsigned int height)
{
  struct grub_video_fbblit_info target;

  /* Make sure there is something to do.  */
  if ((x >= (int)render_target->viewport.width) || (x + (int)width < 0))
    return GRUB_ERR_NONE;
  if ((y >= (int)render_target->viewport.height) || (y + (int)height < 0))
    return GRUB_ERR_NONE;

  /* Do not allow drawing out of viewport.  */
  if (x < 0)
    {
      width += x;
      x = 0;
    }
  if (y < 0)
    {
      height += y;
      y = 0;
    }

  if ((x + width) > render_target->viewport.width)
    width = render_target->viewport.width - x;
  if ((y + height) > render_target->viewport.height)
    height = render_target->viewport.height - y;

  /* Add viewport offset.  */
  x += render_target->viewport.x;
  y += render_target->viewport.y;

  /* Use fbblit_info to encapsulate rendering.  */
  target.mode_info = &render_target->mode_info;
  target.data = render_target->data;

  /* Try to figure out more optimized version.  Note that color is already
     mapped to target format so we can make assumptions based on that.  */
  if (target.mode_info->blit_format == GRUB_VIDEO_BLIT_FORMAT_BGRA_8888)
    {
      grub_video_fbfill_direct32 (&target, color, x, y,
                                        width, height);
      return GRUB_ERR_NONE;
    }
  else if (target.mode_info->blit_format == GRUB_VIDEO_BLIT_FORMAT_RGBA_8888)
    {
      grub_video_fbfill_direct32 (&target, color, x, y,
                                        width, height);
      return GRUB_ERR_NONE;
    }
  else if (target.mode_info->blit_format == GRUB_VIDEO_BLIT_FORMAT_RGB_888)
    {
      grub_video_fbfill_direct24 (&target, color, x, y,
                                        width, height);
      return GRUB_ERR_NONE;
    }
  else if (target.mode_info->blit_format == GRUB_VIDEO_BLIT_FORMAT_RGB_565)
    {
      grub_video_fbfill_direct16 (&target, color, x, y,
                                        width, height);
      return GRUB_ERR_NONE;
    }
  else if (target.mode_info->blit_format == GRUB_VIDEO_BLIT_FORMAT_BGR_565)
    {
      grub_video_fbfill_direct16 (&target, color, x, y,
                                        width, height);
      return GRUB_ERR_NONE;
    }
  else if (target.mode_info->blit_format == GRUB_VIDEO_BLIT_FORMAT_INDEXCOLOR)
    {
      grub_video_fbfill_direct8 (&target, color, x, y,
				       width, height);
      return GRUB_ERR_NONE;
    }

  /* No optimized version found, use default (slow) filler.  */
  grub_video_fbfill (&target, color, x, y, width, height);

  return GRUB_ERR_NONE;
}

/* NOTE: This function assumes that given coordinates are within bounds of
   handled data.  */
static void
common_blitter (struct grub_video_fbblit_info *target,
                struct grub_video_fbblit_info *source,
                enum grub_video_blit_operators oper, int x, int y,
                unsigned int width, unsigned int height,
                int offset_x, int offset_y)
{
  if (oper == GRUB_VIDEO_BLIT_REPLACE)
    {
      /* Try to figure out more optimized version for replace operator.  */
      if (source->mode_info->blit_format == GRUB_VIDEO_BLIT_FORMAT_RGBA_8888)
	{
	  if (target->mode_info->blit_format == GRUB_VIDEO_BLIT_FORMAT_RGBA_8888)
	    {
	      grub_video_fbblit_replace_directN (target, source,
						       x, y, width, height,
						       offset_x, offset_y);
	      return;
	    }
	  else if (target->mode_info->blit_format == GRUB_VIDEO_BLIT_FORMAT_BGRA_8888)
	    {
	      grub_video_fbblit_replace_BGRX8888_RGBX8888 (target, source,
								 x, y, width, height,
								 offset_x, offset_y);
	      return;
	    }
	  else if (target->mode_info->blit_format == GRUB_VIDEO_BLIT_FORMAT_BGR_888)
	    {
	      grub_video_fbblit_replace_BGR888_RGBX8888 (target, source,
							       x, y, width, height,
							       offset_x, offset_y);
	      return;
	    }
	  else if (target->mode_info->blit_format == GRUB_VIDEO_BLIT_FORMAT_RGB_888)
	    {
	      grub_video_fbblit_replace_RGB888_RGBX8888 (target, source,
							       x, y, width, height,
							       offset_x, offset_y);
	      return;
	    }
	  else if (target->mode_info->blit_format == GRUB_VIDEO_BLIT_FORMAT_INDEXCOLOR)
	    {
	      grub_video_fbblit_replace_index_RGBX8888 (target, source,
							      x, y, width, height,
							      offset_x, offset_y);
	      return;
	    }
	}
      else if (source->mode_info->blit_format == GRUB_VIDEO_BLIT_FORMAT_RGB_888)
	{
	  if (target->mode_info->blit_format == GRUB_VIDEO_BLIT_FORMAT_BGRA_8888)
	    {
	      grub_video_fbblit_replace_BGRX8888_RGB888 (target, source,
							       x, y, width, height,
							       offset_x, offset_y);
	      return;
	    }
	  else if (target->mode_info->blit_format == GRUB_VIDEO_BLIT_FORMAT_RGBA_8888)
	    {
	      grub_video_fbblit_replace_RGBX8888_RGB888 (target, source,
							       x, y, width, height,
							       offset_x, offset_y);
	      return;
	    }
	  else if (target->mode_info->blit_format == GRUB_VIDEO_BLIT_FORMAT_BGR_888)
	    {
	      grub_video_fbblit_replace_BGR888_RGB888 (target, source,
							     x, y, width, height,
							     offset_x, offset_y);
	      return;
	    }
	  else if (target->mode_info->blit_format == GRUB_VIDEO_BLIT_FORMAT_RGB_888)
	    {
	      grub_video_fbblit_replace_directN (target, source,
						       x, y, width, height,
						       offset_x, offset_y);
	      return;
	    }
	  else if (target->mode_info->blit_format == GRUB_VIDEO_BLIT_FORMAT_INDEXCOLOR)
	    {
	      grub_video_fbblit_replace_index_RGB888 (target, source,
							    x, y, width, height,
							    offset_x, offset_y);
	      return;
	    }
	}
      else if (source->mode_info->blit_format == GRUB_VIDEO_BLIT_FORMAT_BGRA_8888)
	{
	  if (target->mode_info->blit_format == GRUB_VIDEO_BLIT_FORMAT_BGRA_8888)
	    {
	      grub_video_fbblit_replace_directN (target, source,
						       x, y, width, height,
						       offset_x, offset_y);
	      return;
	    }
	}
      else if (source->mode_info->blit_format == GRUB_VIDEO_BLIT_FORMAT_INDEXCOLOR)
	{
	  if (target->mode_info->blit_format == GRUB_VIDEO_BLIT_FORMAT_INDEXCOLOR)
	    {
	      grub_video_fbblit_replace_directN (target, source,
						       x, y, width, height,
						       offset_x, offset_y);
	      return;
	    }
	}
      else if (source->mode_info->blit_format == GRUB_VIDEO_BLIT_FORMAT_1BIT_PACKED)
	{
	  if (target->mode_info->bpp == 32)
	    {
	      grub_video_fbblit_replace_32bit_1bit (target, source,
						    x, y, width, height,
						    offset_x, offset_y);
	      return;
	    }
	  else if (target->mode_info->bpp == 24)
	    {
	      grub_video_fbblit_replace_24bit_1bit (target, source,
						    x, y, width, height,
						    offset_x, offset_y);
	      return;
	    }
	  else if (target->mode_info->bpp == 16)
	    {
	      grub_video_fbblit_replace_16bit_1bit (target, source,
						    x, y, width, height,
						    offset_x, offset_y);
	      return;
	    }
	  else if (target->mode_info->bpp == 8)
	    {
	      grub_video_fbblit_replace_8bit_1bit (target, source,
						   x, y, width, height,
						   offset_x, offset_y);
	      return;
	    }
	}

      /* No optimized replace operator found, use default (slow) blitter.  */
      grub_video_fbblit_replace (target, source, x, y, width, height,
				       offset_x, offset_y);
    }
  else
    {
      /* Try to figure out more optimized blend operator.  */
      if (source->mode_info->blit_format == GRUB_VIDEO_BLIT_FORMAT_RGBA_8888)
	{
	  if (target->mode_info->blit_format == GRUB_VIDEO_BLIT_FORMAT_BGRA_8888)
	    {
	      grub_video_fbblit_blend_BGRA8888_RGBA8888 (target, source,
							       x, y, width, height,
							       offset_x, offset_y);
	      return;
	    }
	  else if (target->mode_info->blit_format == GRUB_VIDEO_BLIT_FORMAT_RGBA_8888)
	    {
	      grub_video_fbblit_blend_RGBA8888_RGBA8888 (target, source,
							       x, y, width, height,
							       offset_x, offset_y);
	      return;
	    }
	  else if (target->mode_info->blit_format == GRUB_VIDEO_BLIT_FORMAT_BGR_888)
	    {
	      grub_video_fbblit_blend_BGR888_RGBA8888 (target, source,
							     x, y, width, height,
							     offset_x, offset_y);
	      return;
	    }
	  else if (target->mode_info->blit_format == GRUB_VIDEO_BLIT_FORMAT_RGB_888)
	    {
	      grub_video_fbblit_blend_RGB888_RGBA8888 (target, source,
							     x, y, width, height,
							     offset_x, offset_y);
	      return;
	    }
	  else if (target->mode_info->blit_format == GRUB_VIDEO_BLIT_FORMAT_INDEXCOLOR)
	    {
	      grub_video_fbblit_blend_index_RGBA8888 (target, source,
							    x, y, width, height,
							    offset_x, offset_y);
	      return;
	    }
	}
      else if (source->mode_info->blit_format == GRUB_VIDEO_BLIT_FORMAT_RGB_888)
	{
	  /* Note: There is really no alpha information here, so blend is
	     changed to replace.  */

	  if (target->mode_info->blit_format == GRUB_VIDEO_BLIT_FORMAT_BGRA_8888)
	    {
	      grub_video_fbblit_replace_BGRX8888_RGB888 (target, source,
							       x, y, width, height,
							       offset_x, offset_y);
	      return;
	    }
	  else if (target->mode_info->blit_format == GRUB_VIDEO_BLIT_FORMAT_RGBA_8888)
	    {
	      grub_video_fbblit_replace_RGBX8888_RGB888 (target, source,
							       x, y, width, height,
							       offset_x, offset_y);
	      return;
	    }
	  else if (target->mode_info->blit_format == GRUB_VIDEO_BLIT_FORMAT_BGR_888)
	    {
	      grub_video_fbblit_replace_BGR888_RGB888 (target, source,
							     x, y, width, height,
							     offset_x, offset_y);
	      return;
	    }
	  else if (target->mode_info->blit_format == GRUB_VIDEO_BLIT_FORMAT_RGB_888)
	    {
	      grub_video_fbblit_replace_directN (target, source,
						       x, y, width, height,
						       offset_x, offset_y);
	      return;
	    }
	  else if (target->mode_info->blit_format == GRUB_VIDEO_BLIT_FORMAT_INDEXCOLOR)
	    {
	      grub_video_fbblit_replace_index_RGB888 (target, source,
							    x, y, width, height,
							    offset_x, offset_y);
	      return;
	    }
	}
      else if (source->mode_info->blit_format == GRUB_VIDEO_BLIT_FORMAT_1BIT_PACKED)
	{
	  if (target->mode_info->blit_format
	      == GRUB_VIDEO_BLIT_FORMAT_BGRA_8888
	      || target->mode_info->blit_format
	      == GRUB_VIDEO_BLIT_FORMAT_RGBA_8888)
	    {
	      grub_video_fbblit_blend_XXXA8888_1bit (target, source,
						     x, y, width, height,
						     offset_x, offset_y);
	      return;
	    }
	  else if (target->mode_info->blit_format
		   == GRUB_VIDEO_BLIT_FORMAT_BGR_888
		   || target->mode_info->blit_format
		   == GRUB_VIDEO_BLIT_FORMAT_RGB_888)
	    {
	      grub_video_fbblit_blend_XXX888_1bit (target, source,
						   x, y, width, height,
						   offset_x, offset_y);
	      return;
	    }
	  else if (target->mode_info->blit_format
		   == GRUB_VIDEO_BLIT_FORMAT_BGR_565
		   || target->mode_info->blit_format
		   == GRUB_VIDEO_BLIT_FORMAT_RGB_565)
	    {
	      grub_video_fbblit_blend_XXX565_1bit (target, source,
						   x, y, width, height,
						   offset_x, offset_y);
	      return;
	    }

	}


      /* No optimized blend operation found, use default (slow) blitter.  */
      grub_video_fbblit_blend (target, source, x, y, width, height,
				     offset_x, offset_y);
    }
}

grub_err_t
grub_video_fb_blit_bitmap (struct grub_video_bitmap *bitmap,
			   enum grub_video_blit_operators oper, int x, int y,
			   int offset_x, int offset_y,
			   unsigned int width, unsigned int height)
{
  struct grub_video_fbblit_info source;
  struct grub_video_fbblit_info target;

  /* Make sure there is something to do.  */
  if ((width == 0) || (height == 0))
    return GRUB_ERR_NONE;
  if ((x >= (int)render_target->viewport.width) || (x + (int)width < 0))
    return GRUB_ERR_NONE;
  if ((y >= (int)render_target->viewport.height) || (y + (int)height < 0))
    return GRUB_ERR_NONE;
  if ((x + (int)bitmap->mode_info.width) < 0)
    return GRUB_ERR_NONE;
  if ((y + (int)bitmap->mode_info.height) < 0)
    return GRUB_ERR_NONE;
  if ((offset_x >= (int)bitmap->mode_info.width)
      || (offset_x + (int)width < 0))
    return GRUB_ERR_NONE;
  if ((offset_y >= (int)bitmap->mode_info.height)
      || (offset_y + (int)height < 0))
    return GRUB_ERR_NONE;

  /* If we have negative coordinates, optimize drawing to minimum.  */
  if (offset_x < 0)
    {
      width += offset_x;
      x -= offset_x;
      offset_x = 0;
    }

  if (offset_y < 0)
    {
      height += offset_y;
      y -= offset_y;
      offset_y = 0;
    }

  if (x < 0)
    {
      width += x;
      offset_x -= x;
      x = 0;
    }

  if (y < 0)
    {
      height += y;
      offset_y -= y;
      y = 0;
    }

  /* Do not allow drawing out of viewport.  */
  if ((x + width) > render_target->viewport.width)
    width = render_target->viewport.width - x;
  if ((y + height) > render_target->viewport.height)
    height = render_target->viewport.height - y;

  if ((offset_x + width) > bitmap->mode_info.width)
    width = bitmap->mode_info.width - offset_x;
  if ((offset_y + height) > bitmap->mode_info.height)
    height = bitmap->mode_info.height - offset_y;

  /* Limit drawing to source render target dimensions.  */
  if (width > bitmap->mode_info.width)
    width = bitmap->mode_info.width;

  if (height > bitmap->mode_info.height)
    height = bitmap->mode_info.height;

  /* Add viewport offset.  */
  x += render_target->viewport.x;
  y += render_target->viewport.y;

  /* Use fbblit_info to encapsulate rendering.  */
  source.mode_info = &bitmap->mode_info;
  source.data = bitmap->data;
  target.mode_info = &render_target->mode_info;
  target.data = render_target->data;

  /* Do actual blitting.  */
  common_blitter (&target, &source, oper, x, y, width, height,
                  offset_x, offset_y);

  return GRUB_ERR_NONE;
}

grub_err_t
grub_video_fb_blit_render_target (struct grub_video_fbrender_target *source,
                                   enum grub_video_blit_operators oper,
                                   int x, int y, int offset_x, int offset_y,
                                   unsigned int width, unsigned int height)
{
  struct grub_video_fbblit_info source_info;
  struct grub_video_fbblit_info target_info;

  /* Make sure there is something to do.  */
  if ((width == 0) || (height == 0))
    return GRUB_ERR_NONE;
  if ((x >= (int)render_target->viewport.width) || (x + (int)width < 0))
    return GRUB_ERR_NONE;
  if ((y >= (int)render_target->viewport.height) || (y + (int)height < 0))
    return GRUB_ERR_NONE;
  if ((x + (int)source->mode_info.width) < 0)
    return GRUB_ERR_NONE;
  if ((y + (int)source->mode_info.height) < 0)
    return GRUB_ERR_NONE;
  if ((offset_x >= (int)source->mode_info.width)
      || (offset_x + (int)width < 0))
    return GRUB_ERR_NONE;
  if ((offset_y >= (int)source->mode_info.height)
      || (offset_y + (int)height < 0))
    return GRUB_ERR_NONE;

  /* If we have negative coordinates, optimize drawing to minimum.  */
  if (offset_x < 0)
    {
      width += offset_x;
      x -= offset_x;
      offset_x = 0;
    }

  if (offset_y < 0)
    {
      height += offset_y;
      y -= offset_y;
      offset_y = 0;
    }

  if (x < 0)
    {
      width += x;
      offset_x -= x;
      x = 0;
    }

  if (y < 0)
    {
      height += y;
      offset_y -= y;
      y = 0;
    }

  /* Do not allow drawing out of viewport.  */
  if ((x + width) > render_target->viewport.width)
    width = render_target->viewport.width - x;
  if ((y + height) > render_target->viewport.height)
    height = render_target->viewport.height - y;

  if ((offset_x + width) > source->mode_info.width)
    width = source->mode_info.width - offset_x;
  if ((offset_y + height) > source->mode_info.height)
    height = source->mode_info.height - offset_y;

  /* Limit drawing to source render target dimensions.  */
  if (width > source->mode_info.width)
    width = source->mode_info.width;

  if (height > source->mode_info.height)
    height = source->mode_info.height;

  /* Add viewport offset.  */
  x += render_target->viewport.x;
  y += render_target->viewport.y;

  /* Use fbblit_info to encapsulate rendering.  */
  source_info.mode_info = &source->mode_info;
  source_info.data = source->data;
  target_info.mode_info = &render_target->mode_info;
  target_info.data = render_target->data;

  /* Do actual blitting.  */
  common_blitter (&target_info, &source_info, oper, x, y, width, height,
                  offset_x, offset_y);

  return GRUB_ERR_NONE;
}

grub_err_t
grub_video_fb_scroll (grub_video_color_t color, int dx, int dy)
{
  int width;
  int height;
  int src_x;
  int src_y;
  int dst_x;
  int dst_y;

  /* 1. Check if we have something to do.  */
  if ((dx == 0) && (dy == 0))
    return GRUB_ERR_NONE;

  width = render_target->viewport.width - grub_abs (dx);
  height = render_target->viewport.height - grub_abs (dy);

  if (dx < 0)
    {
      src_x = render_target->viewport.x - dx;
      dst_x = render_target->viewport.x;
    }
  else
    {
      src_x = render_target->viewport.x;
      dst_x = render_target->viewport.x + dx;
    }

  if (dy < 0)
    {
      src_y = render_target->viewport.y - dy;
      dst_y = render_target->viewport.y;
    }
  else
    {
      src_y = render_target->viewport.y;
      dst_y = render_target->viewport.y + dy;
    }

  /* 2. Check if there is need to copy data.  */
  if ((grub_abs (dx) < render_target->viewport.width)
       && (grub_abs (dy) < render_target->viewport.height))
    {
      /* 3. Move data in render target.  */
      struct grub_video_fbblit_info target;
      grub_uint8_t *src;
      grub_uint8_t *dst;
      int j;

      target.mode_info = &render_target->mode_info;
      target.data = render_target->data;

      /* Check vertical direction of the move.  */
      if (dy <= 0)
	/* 3a. Move data upwards.  */
	for (j = 0; j < height; j++)
	  {
	    dst = grub_video_fb_get_video_ptr (&target, dst_x, dst_y + j);
	    src = grub_video_fb_get_video_ptr (&target, src_x, src_y + j);
	    grub_memmove (dst, src,
			  width * target.mode_info->bytes_per_pixel);
	  }
      else
	/* 3b. Move data downwards.  */
	for (j = (height - 1); j >= 0; j--)
	  {
	    dst = grub_video_fb_get_video_ptr (&target, dst_x, dst_y + j);
	    src = grub_video_fb_get_video_ptr (&target, src_x, src_y + j);
	    grub_memmove (dst, src,
			  width * target.mode_info->bytes_per_pixel);
	  }
    }

  /* 4. Fill empty space with specified color.  In this implementation
     there might be colliding areas but at the moment there is no need
     to optimize this.  */

  /* 4a. Fill top & bottom parts.  */
  if (dy > 0)
    grub_video_fb_fill_rect (color, 0, 0, render_target->viewport.width, dy);
  else if (dy < 0)
    {
      if (render_target->viewport.height < grub_abs (dy))
        dy = -render_target->viewport.height;

      grub_video_fb_fill_rect (color, 0, render_target->viewport.height + dy,
                                render_target->viewport.width, -dy);
    }

  /* 4b. Fill left & right parts.  */
  if (dx > 0)
    grub_video_fb_fill_rect (color, 0, 0,
                              dx, render_target->viewport.height);
  else if (dx < 0)
    {
      if (render_target->viewport.width < grub_abs (dx))
        dx = -render_target->viewport.width;

      grub_video_fb_fill_rect (color, render_target->viewport.width + dx, 0,
                                -dx, render_target->viewport.height);
    }

  return GRUB_ERR_NONE;
}


grub_err_t
grub_video_fb_create_render_target (struct grub_video_fbrender_target **result,
				    unsigned int width, unsigned int height,
				    unsigned int mode_type __attribute__ ((unused)))
{
  struct grub_video_fbrender_target *target;
  unsigned int size;

  /* Validate arguments.  */
  if ((! result)
      || (width == 0)
      || (height == 0))
    return grub_error (GRUB_ERR_BAD_ARGUMENT,
                       "invalid argument given.");

  /* Allocate memory for render target.  */
  target = grub_malloc (sizeof (struct grub_video_fbrender_target));
  if (! target)
    return grub_errno;

  /* TODO: Implement other types too.
     Currently only 32bit render targets are supported.  */

  /* Mark render target as allocated.  */
  target->is_allocated = 1;

  /* Maximize viewport.  */
  target->viewport.x = 0;
  target->viewport.y = 0;
  target->viewport.width = width;
  target->viewport.height = height;

  /* Setup render target format.  */
  target->mode_info.width = width;
  target->mode_info.height = height;
  target->mode_info.mode_type = GRUB_VIDEO_MODE_TYPE_RGB
                                | GRUB_VIDEO_MODE_TYPE_ALPHA;
  target->mode_info.bpp = 32;
  target->mode_info.bytes_per_pixel = 4;
  target->mode_info.pitch = target->mode_info.bytes_per_pixel * width;
  target->mode_info.number_of_colors = palette_size; /* Emulated palette.  */
  target->mode_info.red_mask_size = 8;
  target->mode_info.red_field_pos = 0;
  target->mode_info.green_mask_size = 8;
  target->mode_info.green_field_pos = 8;
  target->mode_info.blue_mask_size = 8;
  target->mode_info.blue_field_pos = 16;
  target->mode_info.reserved_mask_size = 8;
  target->mode_info.reserved_field_pos = 24;

  target->mode_info.blit_format = grub_video_get_blit_format (&target->mode_info);

  /* Calculate size needed for the data.  */
  size = (width * target->mode_info.bytes_per_pixel) * height;

  target->data = grub_malloc (size);
  if (! target->data)
    {
      grub_free (target);
      return grub_errno;
    }

  /* Clear render target with black and maximum transparency.  */
  grub_memset (target->data, 0, size);

  /* TODO: Add render target to render target list.  */

  /* Save result to caller.  */
  *result = target;

  return GRUB_ERR_NONE;
}

grub_err_t
grub_video_fb_create_render_target_from_pointer (struct grub_video_fbrender_target **result,
						 const struct grub_video_mode_info *mode_info,
						 void *ptr)
{
  struct grub_video_fbrender_target *target;
  unsigned y;

  /* Allocate memory for render target.  */
  target = grub_malloc (sizeof (struct grub_video_fbrender_target));
  if (! target)
    return grub_errno;

  /* Mark framebuffer memory as non allocated.  */
  target->is_allocated = 0;
  target->data = ptr;

  grub_memcpy (&(target->mode_info), mode_info, sizeof (target->mode_info));

  /* Reset viewport to match new mode.  */
  target->viewport.x = 0;
  target->viewport.y = 0;
  target->viewport.width = mode_info->width;
  target->viewport.height = mode_info->height;

  /* Clear render target with black and maximum transparency.  */
  for (y = 0; y < mode_info->height; y++)
    grub_memset (target->data + mode_info->pitch * y, 0,
		 mode_info->bytes_per_pixel * mode_info->width);

  /* Save result to caller.  */
  *result = target;

  return GRUB_ERR_NONE;
}

grub_err_t
grub_video_fb_delete_render_target (struct grub_video_fbrender_target *target)
{
  /* If there is no target, then just return without error.  */
  if (! target)
    return GRUB_ERR_NONE;

  /* TODO: Delist render target from render target list.  */

  /* If this is software render target, free it's memory.  */
  if (target->is_allocated)
    grub_free (target->data);

  /* Free render target.  */
  grub_free (target);

  return GRUB_ERR_NONE;
}

grub_err_t
grub_video_fb_set_active_render_target (struct grub_video_fbrender_target *target)
{
  if (! target->data)
    return grub_error (GRUB_ERR_BAD_ARGUMENT,
                       "invalid render target given.");

  render_target = target;

  return GRUB_ERR_NONE;
}

grub_err_t
grub_video_fb_get_active_render_target (struct grub_video_fbrender_target **target)
{
  *target = render_target;

  return GRUB_ERR_NONE;
}
