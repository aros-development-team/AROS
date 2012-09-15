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
#include <grub/dl.h>

GRUB_MOD_LICENSE ("GPLv3+");

typedef grub_err_t (*grub_video_fb_doublebuf_update_screen_t) (void);
typedef volatile void *framebuf_t;

struct dirty
{
  int first_line;
  int last_line;
};

static struct
{
  struct grub_video_fbrender_target *render_target;
  struct grub_video_fbrender_target *back_target;
  struct grub_video_palette_data *palette;
  framebuf_t pages[2];

  unsigned int palette_size;

  struct dirty current_dirty;
  struct dirty previous_dirty;

  /* For page flipping strategy.  */
  int displayed_page;           /* The page # that is the front buffer.  */
  int render_page;              /* The page # that is the back buffer.  */
  grub_video_fb_set_page_t set_page;
  char *offscreen_buffer;
  grub_video_fb_doublebuf_update_screen_t update_screen;
} framebuffer;

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
  grub_free (framebuffer.palette);
  framebuffer.render_target = 0;
  framebuffer.back_target = 0;
  framebuffer.palette = 0;
  framebuffer.palette_size = 0;
  framebuffer.set_page = 0;
  return GRUB_ERR_NONE;
}

grub_err_t
grub_video_fb_fini (void)
{
  /* TODO: destroy render targets.  */

  grub_free (framebuffer.offscreen_buffer);
  grub_free (framebuffer.palette);
  framebuffer.render_target = 0;
  framebuffer.back_target = 0;
  framebuffer.palette = 0;
  framebuffer.palette_size = 0;
  framebuffer.set_page = 0;
  framebuffer.offscreen_buffer = 0;
  return GRUB_ERR_NONE;
}

grub_err_t
grub_video_fb_get_info (struct grub_video_mode_info *mode_info)
{
  /* Copy mode info from active render target.  */
  grub_memcpy (mode_info, &framebuffer.render_target->mode_info,
               sizeof (struct grub_video_mode_info));

  return GRUB_ERR_NONE;
}

grub_err_t
grub_video_fb_get_palette (unsigned int start, unsigned int count,
			   struct grub_video_palette_data *palette_data)
{
  unsigned int i;

  /* Assume that we know everything from index color palette.  */
  for (i = 0; (i < count) && ((i + start) < framebuffer.palette_size); i++)
    palette_data[i] = framebuffer.palette[start + i];

  return GRUB_ERR_NONE;
}

grub_err_t
grub_video_fb_set_palette (unsigned int start, unsigned int count,
			   struct grub_video_palette_data *palette_data)
{
  unsigned i;
  if (start + count > framebuffer.palette_size)
    {
      framebuffer.palette_size = start + count;
      framebuffer.palette = grub_realloc (framebuffer.palette,
					  sizeof (framebuffer.palette[0])
					  * framebuffer.palette_size);
      if (!framebuffer.palette)
	{
	  grub_video_fb_fini ();
	  return grub_errno;
	}
    }
  for (i = 0; (i < count) && ((i + start) < framebuffer.palette_size); i++)
    framebuffer.palette[start + i] = palette_data[i];
  return GRUB_ERR_NONE;
}

grub_err_t
grub_video_fb_set_viewport (unsigned int x, unsigned int y,
			    unsigned int width, unsigned int height)
{
  /* Make sure viewport is withing screen dimensions.  If viewport was set
     to be out of the region, mark its size as zero.  */
  if (x > framebuffer.render_target->mode_info.width)
    {
      x = 0;
      width = 0;
    }

  if (y > framebuffer.render_target->mode_info.height)
    {
      y = 0;
      height = 0;
    }

  if (x + width > framebuffer.render_target->mode_info.width)
    width = framebuffer.render_target->mode_info.width - x;

  if (y + height > framebuffer.render_target->mode_info.height)
    height = framebuffer.render_target->mode_info.height - y;

  framebuffer.render_target->viewport.x = x;
  framebuffer.render_target->viewport.y = y;
  framebuffer.render_target->viewport.width = width;
  framebuffer.render_target->viewport.height = height;

  return GRUB_ERR_NONE;
}

grub_err_t
grub_video_fb_get_viewport (unsigned int *x, unsigned int *y,
			    unsigned int *width, unsigned int *height)
{
  if (x) *x = framebuffer.render_target->viewport.x;
  if (y) *y = framebuffer.render_target->viewport.y;
  if (width) *width = framebuffer.render_target->viewport.width;
  if (height) *height = framebuffer.render_target->viewport.height;

  return GRUB_ERR_NONE;
}

/* Maps color name to target optimized color format.  */
grub_video_color_t
grub_video_fb_map_color (grub_uint32_t color_name)
{
  /* TODO: implement color theme mapping code.  */

  if (color_name < framebuffer.palette_size)
    {
      if ((framebuffer.render_target->mode_info.mode_type
           & GRUB_VIDEO_MODE_TYPE_INDEX_COLOR) != 0)
        return color_name;
      else
        {
          grub_video_color_t color;

          color = grub_video_fb_map_rgb (framebuffer.palette[color_name].r,
					 framebuffer.palette[color_name].g,
					 framebuffer.palette[color_name].b);

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
  if ((framebuffer.render_target->mode_info.mode_type
       & GRUB_VIDEO_MODE_TYPE_INDEX_COLOR) != 0)
    {
      int minindex = 0;
      int delta = 0;
      int tmp;
      int val;
      unsigned i;

      /* Find best matching color.  */
      for (i = 0; i < framebuffer.palette_size; i++)
        {
          val = framebuffer.palette[i].r - red;
          tmp = val * val;
          val = framebuffer.palette[i].g - green;
          tmp += val * val;
          val = framebuffer.palette[i].b - blue;
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
  else if ((framebuffer.render_target->mode_info.mode_type
            & GRUB_VIDEO_MODE_TYPE_1BIT_BITMAP) != 0)
    {
       if (red == framebuffer.render_target->mode_info.fg_red
           && green == framebuffer.render_target->mode_info.fg_green
           && blue == framebuffer.render_target->mode_info.fg_blue)
         return 1;
       else
         return 0;
    }
  else
    {
      grub_uint32_t value;
      grub_uint8_t alpha = 255; /* Opaque color.  */

      red >>= 8 - framebuffer.render_target->mode_info.red_mask_size;
      green >>= 8 - framebuffer.render_target->mode_info.green_mask_size;
      blue >>= 8 - framebuffer.render_target->mode_info.blue_mask_size;
      alpha >>= 8 - framebuffer.render_target->mode_info.reserved_mask_size;

      value = red << framebuffer.render_target->mode_info.red_field_pos;
      value |= green << framebuffer.render_target->mode_info.green_field_pos;
      value |= blue << framebuffer.render_target->mode_info.blue_field_pos;
      value |= alpha << framebuffer.render_target->mode_info.reserved_field_pos;

      return value;
    }

}

/* Maps RGBA to target optimized color format.  */
grub_video_color_t
grub_video_fb_map_rgba (grub_uint8_t red, grub_uint8_t green,
			grub_uint8_t blue, grub_uint8_t alpha)
{
  if ((framebuffer.render_target->mode_info.mode_type
       & GRUB_VIDEO_MODE_TYPE_INDEX_COLOR) != 0)
    /* No alpha available in index color modes, just use
       same value as in only RGB modes.  */
    return grub_video_fb_map_rgb (red, green, blue);
  else if ((framebuffer.render_target->mode_info.mode_type
            & GRUB_VIDEO_MODE_TYPE_1BIT_BITMAP) != 0)
    {
      if (red == framebuffer.render_target->mode_info.fg_red
          && green == framebuffer.render_target->mode_info.fg_green
          && blue == framebuffer.render_target->mode_info.fg_blue
          && alpha == framebuffer.render_target->mode_info.fg_alpha)
        return 1;
      else
        return 0;
    }
  else
    {
      grub_uint32_t value;

      red >>= 8 - framebuffer.render_target->mode_info.red_mask_size;
      green >>= 8 - framebuffer.render_target->mode_info.green_mask_size;
      blue >>= 8 - framebuffer.render_target->mode_info.blue_mask_size;
      alpha >>= 8 - framebuffer.render_target->mode_info.reserved_mask_size;

      value = red << framebuffer.render_target->mode_info.red_field_pos;
      value |= green << framebuffer.render_target->mode_info.green_field_pos;
      value |= blue << framebuffer.render_target->mode_info.blue_field_pos;
      value |= alpha << framebuffer.render_target->mode_info.reserved_field_pos;

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

  target_info.mode_info = &framebuffer.render_target->mode_info;
  target_info.data = framebuffer.render_target->data;

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

      *red = framebuffer.palette[color].r;
      *green = framebuffer.palette[color].g;
      *blue = framebuffer.palette[color].b;
      *alpha = framebuffer.palette[color].a;
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

static void
dirty (int y, int height)
{
  if (framebuffer.render_target != framebuffer.back_target)
    return;
  if (framebuffer.current_dirty.first_line > y)
    framebuffer.current_dirty.first_line = y;
  if (framebuffer.current_dirty.last_line < y + height)
    framebuffer.current_dirty.last_line = y + height;
}

grub_err_t
grub_video_fb_fill_rect (grub_video_color_t color, int x, int y,
			 unsigned int width, unsigned int height)
{
  struct grub_video_fbblit_info target;

  /* Make sure there is something to do.  */
  if ((x >= (int)framebuffer.render_target->viewport.width) || (x + (int)width < 0))
    return GRUB_ERR_NONE;
  if ((y >= (int)framebuffer.render_target->viewport.height) || (y + (int)height < 0))
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

  if ((x + width) > framebuffer.render_target->viewport.width)
    width = framebuffer.render_target->viewport.width - x;
  if ((y + height) > framebuffer.render_target->viewport.height)
    height = framebuffer.render_target->viewport.height - y;

  /* Add viewport offset.  */
  x += framebuffer.render_target->viewport.x;
  y += framebuffer.render_target->viewport.y;

  dirty (y, height);

  /* Use fbblit_info to encapsulate rendering.  */
  target.mode_info = &framebuffer.render_target->mode_info;
  target.data = framebuffer.render_target->data;

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
  dirty (y, height);

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
#ifdef GRUB_HAVE_UNALIGNED_ACCESS
	  else if (target->mode_info->bpp == 24)
	    {
	      grub_video_fbblit_replace_24bit_1bit (target, source,
						    x, y, width, height,
						    offset_x, offset_y);
	      return;
	    }
#endif
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
#ifdef GRUB_HAVE_UNALIGNED_ACCESS
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
#endif
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
  if ((x >= (int)framebuffer.render_target->viewport.width) || (x + (int)width < 0))
    return GRUB_ERR_NONE;
  if ((y >= (int)framebuffer.render_target->viewport.height) || (y + (int)height < 0))
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
  if ((x + width) > framebuffer.render_target->viewport.width)
    width = framebuffer.render_target->viewport.width - x;
  if ((y + height) > framebuffer.render_target->viewport.height)
    height = framebuffer.render_target->viewport.height - y;

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
  x += framebuffer.render_target->viewport.x;
  y += framebuffer.render_target->viewport.y;

  /* Use fbblit_info to encapsulate rendering.  */
  source.mode_info = &bitmap->mode_info;
  source.data = bitmap->data;
  target.mode_info = &framebuffer.render_target->mode_info;
  target.data = framebuffer.render_target->data;

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
  if ((x >= (int)framebuffer.render_target->viewport.width) || (x + (int)width < 0))
    return GRUB_ERR_NONE;
  if ((y >= (int)framebuffer.render_target->viewport.height) || (y + (int)height < 0))
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
  if ((x + width) > framebuffer.render_target->viewport.width)
    width = framebuffer.render_target->viewport.width - x;
  if ((y + height) > framebuffer.render_target->viewport.height)
    height = framebuffer.render_target->viewport.height - y;

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
  x += framebuffer.render_target->viewport.x;
  y += framebuffer.render_target->viewport.y;

  /* Use fbblit_info to encapsulate rendering.  */
  source_info.mode_info = &source->mode_info;
  source_info.data = source->data;
  target_info.mode_info = &framebuffer.render_target->mode_info;
  target_info.data = framebuffer.render_target->data;

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

  width = framebuffer.render_target->viewport.width - grub_abs (dx);
  height = framebuffer.render_target->viewport.height - grub_abs (dy);

  dirty (framebuffer.render_target->viewport.y,
	 framebuffer.render_target->viewport.height);

  if (dx < 0)
    {
      src_x = framebuffer.render_target->viewport.x - dx;
      dst_x = framebuffer.render_target->viewport.x;
    }
  else
    {
      src_x = framebuffer.render_target->viewport.x;
      dst_x = framebuffer.render_target->viewport.x + dx;
    }

  if (dy < 0)
    {
      src_y = framebuffer.render_target->viewport.y - dy;
      dst_y = framebuffer.render_target->viewport.y;
    }
  else
    {
      src_y = framebuffer.render_target->viewport.y;
      dst_y = framebuffer.render_target->viewport.y + dy;
    }

  /* 2. Check if there is need to copy data.  */
  if ((grub_abs (dx) < framebuffer.render_target->viewport.width)
       && (grub_abs (dy) < framebuffer.render_target->viewport.height))
    {
      /* 3. Move data in render target.  */
      struct grub_video_fbblit_info target;
      int i, j;
      int linedelta, linelen;

      target.mode_info = &framebuffer.render_target->mode_info;
      target.data = framebuffer.render_target->data;

      linedelta = target.mode_info->pitch
	- width * target.mode_info->bytes_per_pixel;
      linelen = width * target.mode_info->bytes_per_pixel;
#define DO_SCROLL                                                    \
      /* Check vertical direction of the move.  */                   \
      if (dy < 0 || (dy == 0 && dx < 0))                             \
	{                                                            \
	  dst = (void *) grub_video_fb_get_video_ptr (&target,       \
						      dst_x, dst_y); \
	  src = (void *) grub_video_fb_get_video_ptr (&target,	     \
						      src_x, src_y); \
	  /* 3a. Move data upwards.  */                              \
	  for (j = 0; j < height; j++)                               \
	    {                                                        \
	      for (i = 0; i < linelen; i++)                          \
		*(dst++) = *(src++);	                             \
	      dst += linedelta;                                      \
	      src += linedelta;                                      \
	    }							     \
	}                                                            \
      else                                                           \
	{                                                            \
	  /* 3b. Move data downwards.  */                            \
	  dst = (void *) grub_video_fb_get_video_ptr (&target,	     \
					              dst_x + width, \
					     dst_y + height - 1);    \
	  src = (void *) grub_video_fb_get_video_ptr (&target,	     \
					              src_x + width, \
					     src_y + height - 1);    \
	  dst--;                                                     \
          src--;                                                     \
	  for (j = 0; j < height; j++)                               \
	    {                                                        \
	      for (i = 0; i < linelen; i++)                          \
		*(dst--) = *(src--);                                 \
	      dst -= linedelta;                                      \
	      src -= linedelta;                                      \
	    }                                                        \
	}

      /* If everything is aligned on 32-bit use 32-bit copy.  */
      if ((grub_addr_t) grub_video_fb_get_video_ptr (&target, src_x, src_y)
	  % sizeof (grub_uint32_t) == 0
	  && (grub_addr_t) grub_video_fb_get_video_ptr (&target, dst_x, dst_y) 
	  % sizeof (grub_uint32_t) == 0
	  && linelen % sizeof (grub_uint32_t) == 0
	  && linedelta % sizeof (grub_uint32_t) == 0)
	{
	  grub_uint32_t *src, *dst;
	  linelen /= sizeof (grub_uint32_t);
	  linedelta /= sizeof (grub_uint32_t);
	  DO_SCROLL
	}
      /* If everything is aligned on 16-bit use 16-bit copy.  */
      else if ((grub_addr_t) grub_video_fb_get_video_ptr (&target, src_x, src_y)
	       % sizeof (grub_uint16_t) == 0
	       && (grub_addr_t) grub_video_fb_get_video_ptr (&target,
							     dst_x, dst_y) 
	       % sizeof (grub_uint16_t) == 0
	       && linelen % sizeof (grub_uint16_t) == 0
	       && linedelta % sizeof (grub_uint16_t) == 0)
	{
	  grub_uint16_t *src, *dst;
	  linelen /= sizeof (grub_uint16_t);
	  linedelta /= sizeof (grub_uint16_t);
	  DO_SCROLL
	}
      /* If not aligned at all use 8-bit copy.  */
      else
	{
	  grub_uint8_t *src, *dst;
	  DO_SCROLL
	}	
    }

  /* 4. Fill empty space with specified color.  In this implementation
     there might be colliding areas but at the moment there is no need
     to optimize this.  */

  /* 4a. Fill top & bottom parts.  */
  if (dy > 0)
    grub_video_fb_fill_rect (color, 0, 0, framebuffer.render_target->viewport.width, dy);
  else if (dy < 0)
    {
      if (framebuffer.render_target->viewport.height < grub_abs (dy))
        dy = -framebuffer.render_target->viewport.height;

      grub_video_fb_fill_rect (color, 0, framebuffer.render_target->viewport.height + dy,
                                framebuffer.render_target->viewport.width, -dy);
    }

  /* 4b. Fill left & right parts.  */
  if (dx > 0)
    grub_video_fb_fill_rect (color, 0, 0,
                              dx, framebuffer.render_target->viewport.height);
  else if (dx < 0)
    {
      if (framebuffer.render_target->viewport.width < grub_abs (dx))
        dx = -framebuffer.render_target->viewport.width;

      grub_video_fb_fill_rect (color, framebuffer.render_target->viewport.width + dx, 0,
                                -dx, framebuffer.render_target->viewport.height);
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
    return grub_error (GRUB_ERR_BUG,
                       "invalid argument given");

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
  target->mode_info.number_of_colors = framebuffer.palette_size; /* Emulated palette.  */
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

#ifndef GRUB_HAVE_UNALIGNED_ACCESS
  if (!(mode_info->bytes_per_pixel & (mode_info->bytes_per_pixel - 1))
      && ((grub_addr_t) ptr & (mode_info->bytes_per_pixel - 1)))
    return grub_error (GRUB_ERR_BAD_ARGUMENT, "unaligned pointer");
  if (!(mode_info->bytes_per_pixel & (mode_info->bytes_per_pixel - 1))
      && (mode_info->pitch & (mode_info->bytes_per_pixel - 1)))
    return grub_error (GRUB_ERR_BAD_ARGUMENT, "unaligned pitch");
#endif

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
  if (target == (struct grub_video_fbrender_target *)
      GRUB_VIDEO_RENDER_TARGET_DISPLAY)
    target = framebuffer.back_target;

  if (! target->data)
    return grub_error (GRUB_ERR_BUG,
                       "invalid render target given");

  framebuffer.render_target = target;

  return GRUB_ERR_NONE;
}

grub_err_t
grub_video_fb_get_active_render_target (struct grub_video_fbrender_target **target)
{
  *target = framebuffer.render_target;

  if (*target == framebuffer.back_target)
    *target = (struct grub_video_fbrender_target *) GRUB_VIDEO_RENDER_TARGET_DISPLAY;

  return GRUB_ERR_NONE;
}

static grub_err_t
doublebuf_blit_update_screen (void)
{
  if (framebuffer.current_dirty.first_line
      <= framebuffer.current_dirty.last_line)
    grub_memcpy ((char *) framebuffer.pages[0]
		 + framebuffer.current_dirty.first_line
		 * framebuffer.back_target->mode_info.pitch,
		 (char *) framebuffer.back_target->data
		 + framebuffer.current_dirty.first_line
		 * framebuffer.back_target->mode_info.pitch,
		 framebuffer.back_target->mode_info.pitch
		 * (framebuffer.current_dirty.last_line
		    - framebuffer.current_dirty.first_line));
  framebuffer.current_dirty.first_line
    = framebuffer.back_target->mode_info.height;
  framebuffer.current_dirty.last_line = 0;

  return GRUB_ERR_NONE;
}

static grub_err_t
grub_video_fb_doublebuf_blit_init (struct grub_video_fbrender_target **back,
				   struct grub_video_mode_info mode_info,
				   volatile void *framebuf)
{
  grub_err_t err;
  grub_size_t page_size = mode_info.pitch * mode_info.height;

  framebuffer.offscreen_buffer = grub_zalloc (page_size);
  if (! framebuffer.offscreen_buffer)
    return grub_errno;

  err = grub_video_fb_create_render_target_from_pointer (&framebuffer.back_target,
							 &mode_info,
							 framebuffer.offscreen_buffer);

  if (err)
    {
      grub_free (framebuffer.offscreen_buffer);
      framebuffer.offscreen_buffer = 0;
      return grub_errno;
    }
  (*back)->is_allocated = 1;

  framebuffer.update_screen = doublebuf_blit_update_screen;
  framebuffer.pages[0] = framebuf;
  framebuffer.displayed_page = 0;
  framebuffer.render_page = 0;
  framebuffer.current_dirty.first_line = mode_info.height;
  framebuffer.current_dirty.last_line = 0;

  return GRUB_ERR_NONE;
}

static grub_err_t
doublebuf_pageflipping_update_screen (void)
{
  int new_displayed_page;
  grub_err_t err;
  int first_line, last_line;

  first_line = framebuffer.current_dirty.first_line;
  last_line = framebuffer.current_dirty.last_line;
  if (first_line > framebuffer.previous_dirty.first_line)
    first_line = framebuffer.previous_dirty.first_line;
  if (last_line < framebuffer.previous_dirty.last_line)
    last_line = framebuffer.previous_dirty.last_line;

  if (first_line <= last_line)
    grub_memcpy ((char *) framebuffer.pages[framebuffer.render_page]
		 + first_line * framebuffer.back_target->mode_info.pitch,
		 (char *) framebuffer.back_target->data
		 + first_line * framebuffer.back_target->mode_info.pitch,
		 framebuffer.back_target->mode_info.pitch
		 * (last_line - first_line));
  framebuffer.previous_dirty = framebuffer.current_dirty;
  framebuffer.current_dirty.first_line
    = framebuffer.back_target->mode_info.height;
  framebuffer.current_dirty.last_line = 0;

  /* Swap the page numbers in the framebuffer struct.  */
  new_displayed_page = framebuffer.render_page;
  framebuffer.render_page = framebuffer.displayed_page;
  framebuffer.displayed_page = new_displayed_page;

  err = framebuffer.set_page (framebuffer.displayed_page);
  if (err)
    {
      /* Restore previous state.  */
      framebuffer.render_page = framebuffer.displayed_page;
      framebuffer.displayed_page = new_displayed_page;
      return err;
    }

  return GRUB_ERR_NONE;
}

static grub_err_t
doublebuf_pageflipping_init (struct grub_video_mode_info *mode_info,
			     volatile void *page0_ptr,
			     grub_video_fb_set_page_t set_page_in,
			     volatile void *page1_ptr)
{
  grub_err_t err;
  grub_size_t page_size = mode_info->pitch * mode_info->height;

  framebuffer.offscreen_buffer = grub_malloc (page_size);
  if (! framebuffer.offscreen_buffer)
    {
      return grub_errno;
    }

  err = grub_video_fb_create_render_target_from_pointer (&framebuffer.back_target,
							 mode_info,
							 framebuffer.offscreen_buffer);

  if (err)
    {
      grub_free (framebuffer.offscreen_buffer);
      framebuffer.offscreen_buffer = 0;
      return grub_errno;
    }
  framebuffer.back_target->is_allocated = 1;

  framebuffer.displayed_page = 0;
  framebuffer.render_page = 1;

  framebuffer.update_screen = doublebuf_pageflipping_update_screen;
  framebuffer.pages[0] = page0_ptr;
  framebuffer.pages[1] = page1_ptr;

  framebuffer.current_dirty.first_line
    = framebuffer.back_target->mode_info.height;
  framebuffer.current_dirty.last_line = 0;
  framebuffer.previous_dirty.first_line
    = framebuffer.back_target->mode_info.height;
  framebuffer.previous_dirty.last_line = 0;

  /* Set the framebuffer memory data pointer and display the right page.  */
  err = set_page_in (framebuffer.displayed_page);
  if (err)
    {
      grub_video_fb_delete_render_target (framebuffer.back_target);
      return err;
    }
  framebuffer.set_page = set_page_in;

  return GRUB_ERR_NONE;
}

/* Select the best double buffering mode available.  */
grub_err_t
grub_video_fb_setup (unsigned int mode_type, unsigned int mode_mask,
		     struct grub_video_mode_info *mode_info,
		     volatile void *page0_ptr,
		     grub_video_fb_set_page_t set_page_in,
		     volatile void *page1_ptr)
{
  grub_err_t err;

  /* Do double buffering only if it's either requested or efficient.  */
  if (set_page_in && grub_video_check_mode_flag (mode_type, mode_mask,
						 GRUB_VIDEO_MODE_TYPE_DOUBLE_BUFFERED,
						 1))
    {
      mode_info->mode_type |= GRUB_VIDEO_MODE_TYPE_DOUBLE_BUFFERED;
      mode_info->mode_type |= GRUB_VIDEO_MODE_TYPE_UPDATING_SWAP;

      err = doublebuf_pageflipping_init (mode_info, page0_ptr,
					 set_page_in,
					 page1_ptr);
      if (!err)
	{
	  framebuffer.render_target = framebuffer.back_target;
	  return GRUB_ERR_NONE;
	}
      
      mode_info->mode_type &= ~(GRUB_VIDEO_MODE_TYPE_DOUBLE_BUFFERED
				| GRUB_VIDEO_MODE_TYPE_UPDATING_SWAP);

      grub_errno = GRUB_ERR_NONE;
    }

  if (grub_video_check_mode_flag (mode_type, mode_mask,
				  GRUB_VIDEO_MODE_TYPE_DOUBLE_BUFFERED,
				  1))
    {
      /* It was much nicer with the cast directly at function call but
	 some older gcc versions don't accept it properly.*/
      void *tmp = (void *) page0_ptr;
      mode_info->mode_type |= (GRUB_VIDEO_MODE_TYPE_DOUBLE_BUFFERED
			       | GRUB_VIDEO_MODE_TYPE_UPDATING_SWAP);

      err = grub_video_fb_doublebuf_blit_init (&framebuffer.back_target,
					       *mode_info,
					       tmp);

      if (!err)
	{
	  framebuffer.render_target = framebuffer.back_target;
	  return GRUB_ERR_NONE;
	}

      mode_info->mode_type &= ~(GRUB_VIDEO_MODE_TYPE_DOUBLE_BUFFERED
				| GRUB_VIDEO_MODE_TYPE_UPDATING_SWAP);

      grub_errno = GRUB_ERR_NONE;
    }

  /* Fall back to no double buffering.  */
  err = grub_video_fb_create_render_target_from_pointer (&framebuffer.back_target,
							 mode_info,
							 (void *) page0_ptr);

  if (err)
    return err;

  framebuffer.update_screen = 0;
  framebuffer.pages[0] = page0_ptr;
  framebuffer.displayed_page = 0;
  framebuffer.render_page = 0;
  framebuffer.set_page = 0;
  framebuffer.current_dirty.first_line
    = framebuffer.back_target->mode_info.height;
  framebuffer.current_dirty.last_line = 0;

  mode_info->mode_type &= ~GRUB_VIDEO_MODE_TYPE_DOUBLE_BUFFERED;

  framebuffer.render_target = framebuffer.back_target;

  return GRUB_ERR_NONE;
}


grub_err_t
grub_video_fb_swap_buffers (void)
{
  grub_err_t err;
  if (!framebuffer.update_screen)
    return GRUB_ERR_NONE;

  err = framebuffer.update_screen ();
  if (err)
    return err;

  return GRUB_ERR_NONE;
}

grub_err_t
grub_video_fb_get_info_and_fini (struct grub_video_mode_info *mode_info,
				 void **framebuf)
{
  grub_memcpy (mode_info, &(framebuffer.back_target->mode_info),
	       sizeof (*mode_info));

  /* We are about to load a kernel.  Switch back to page zero, since some
     kernel drivers expect that.  */
  if (framebuffer.set_page && framebuffer.displayed_page != 0)
    {
      framebuffer.update_screen ();
    }

  *framebuf = (void *) framebuffer.pages[framebuffer.displayed_page];

  grub_video_fb_fini ();

  return GRUB_ERR_NONE;
}
