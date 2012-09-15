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

#define grub_video_render_target grub_video_fbrender_target

#include <grub/err.h>
#include <grub/types.h>
#include <grub/dl.h>
#include <grub/misc.h>
#include <grub/mm.h>
#include <grub/video.h>
#include <grub/video_fb.h>
#include <grub/ieee1275/ieee1275.h>

GRUB_MOD_LICENSE ("GPLv3+");

/* Only 8-bit indexed color is supported for now.  */

static unsigned old_width, old_height;
static int restore_needed;
static char *display;
static grub_ieee1275_ihandle_t stdout_ihandle;
static int have_setcolors = 0;

static struct
{
  struct grub_video_mode_info mode_info;
  grub_uint8_t *ptr;
} framebuffer;

static grub_err_t
grub_video_ieee1275_set_palette (unsigned int start, unsigned int count,
				 struct grub_video_palette_data *palette_data);

static void
set_video_mode (unsigned width __attribute__ ((unused)),
		unsigned height __attribute__ ((unused)))
{
  /* TODO */
}

static void
find_display (void)
{
  auto int hook (struct grub_ieee1275_devalias *alias);
  int hook (struct grub_ieee1275_devalias *alias)
  {
    if (grub_strcmp (alias->type, "display") == 0)
      {
	grub_dprintf ("video", "Found display %s\n", alias->path);
	display = grub_strdup (alias->path);
	return 1;
      }
    return 0;
  }
  
  grub_ieee1275_devices_iterate (hook);
}

static grub_err_t
grub_video_ieee1275_init (void)
{
  grub_ssize_t actual;

  grub_memset (&framebuffer, 0, sizeof(framebuffer));

  if (! grub_ieee1275_test_flag (GRUB_IEEE1275_FLAG_CANNOT_SET_COLORS)
      && !grub_ieee1275_get_integer_property (grub_ieee1275_chosen,
					      "stdout", &stdout_ihandle,
					      sizeof (stdout_ihandle), &actual)
      && actual == sizeof (stdout_ihandle))
    have_setcolors = 1;

  return grub_video_fb_init ();
}

static grub_err_t
grub_video_ieee1275_fini (void)
{
  if (restore_needed)
    {
      set_video_mode (old_width, old_height);
      restore_needed = 0;
    }
  return grub_video_fb_fini ();
}

static grub_err_t
grub_video_ieee1275_fill_mode_info (grub_ieee1275_phandle_t dev,
				    struct grub_video_mode_info *out)
{
  grub_uint32_t tmp;

  grub_memset (out, 0, sizeof (*out));

  if (grub_ieee1275_get_integer_property (dev, "width", &tmp,
					  sizeof (tmp), 0))
    return grub_error (GRUB_ERR_IO, "Couldn't retrieve display width.");
  out->width = tmp;

  if (grub_ieee1275_get_integer_property (dev, "height", &tmp,
					  sizeof (tmp), 0))
    return grub_error (GRUB_ERR_IO, "Couldn't retrieve display height.");
  out->height = tmp;

  if (grub_ieee1275_get_integer_property (dev, "linebytes", &tmp,
					  sizeof (tmp), 0))
    return grub_error (GRUB_ERR_IO, "Couldn't retrieve display pitch.");
  out->pitch = tmp;

  out->mode_type = GRUB_VIDEO_MODE_TYPE_INDEX_COLOR;
  out->bpp = 8;
  out->bytes_per_pixel = 1;
  out->number_of_colors = 256;

  out->blit_format = grub_video_get_blit_format (out);
  return GRUB_ERR_NONE;
}

static grub_err_t
grub_video_ieee1275_setup (unsigned int width, unsigned int height,
			   unsigned int mode_type __attribute__ ((unused)),
			   unsigned int mode_mask __attribute__ ((unused)))
{
  grub_uint32_t current_width, current_height, address;
  grub_err_t err;
  grub_ieee1275_phandle_t dev;

  if (!display)
    return grub_error (GRUB_ERR_IO, "Couldn't find display device.");

  if (grub_ieee1275_finddevice (display, &dev))
    return grub_error (GRUB_ERR_IO, "Couldn't open display device.");

  if (grub_ieee1275_get_integer_property (dev, "width", &current_width,
					  sizeof (current_width), 0))
    return grub_error (GRUB_ERR_IO, "Couldn't retrieve display width.");

  if (grub_ieee1275_get_integer_property (dev, "height", &current_height,
					  sizeof (current_width), 0))
    return grub_error (GRUB_ERR_IO, "Couldn't retrieve display height.");

  if ((width == current_width && height == current_height)
      || (width == 0 && height == 0))
    {
      grub_dprintf ("video", "IEEE1275: keeping current mode %dx%d\n",
		    current_width, current_height);
    }
  else
    {
      grub_dprintf ("video", "IEEE1275: Setting mode %dx%d\n", width, height);
      /* TODO. */
      return grub_error (GRUB_ERR_IO, "can't set mode %dx%d", width, height);
    }
  
  err = grub_video_ieee1275_fill_mode_info (dev, &framebuffer.mode_info);
  if (err)
    {
      grub_dprintf ("video", "IEEE1275: couldn't fill mode info\n");
      return err;
    }

  if (grub_ieee1275_get_integer_property (dev, "address", (void *) &address,
					  sizeof (address), 0))
    return grub_error (GRUB_ERR_IO, "Couldn't retrieve display address.");

  /* For some reason sparc64 uses 32-bit pointer too.  */
  framebuffer.ptr = (void *) (grub_addr_t) address;

  grub_dprintf ("video", "IEEE1275: initialising FB @ %p %dx%dx%d\n",
		framebuffer.ptr, framebuffer.mode_info.width,
		framebuffer.mode_info.height, framebuffer.mode_info.bpp);

  err = grub_video_fb_setup (mode_type, mode_mask,
			     &framebuffer.mode_info,
			     framebuffer.ptr, NULL, NULL);
  if (err)
    return err;

  grub_video_ieee1275_set_palette (0, GRUB_VIDEO_FBSTD_NUMCOLORS,
				   grub_video_fbstd_colors);
    
  return err;
}

static grub_err_t
grub_video_ieee1275_get_info_and_fini (struct grub_video_mode_info *mode_info,
				  void **framebuf)
{
  grub_memcpy (mode_info, &(framebuffer.mode_info), sizeof (*mode_info));
  *framebuf = (char *) framebuffer.ptr;

  grub_video_fb_fini ();

  return GRUB_ERR_NONE;
}

static grub_err_t
grub_video_ieee1275_set_palette (unsigned int start, unsigned int count,
				 struct grub_video_palette_data *palette_data)
{
  grub_err_t err;
  struct grub_video_palette_data fb_palette_data[256];

  err = grub_video_fb_set_palette (start, count, palette_data);
  if (err)
    return err;

  grub_video_fb_get_palette (0, ARRAY_SIZE (fb_palette_data), fb_palette_data);

  /* Set colors.  */
  if (have_setcolors)
    {
      unsigned col;
      for (col = 0; col < ARRAY_SIZE (fb_palette_data); col++)
	grub_ieee1275_set_color (stdout_ihandle, col, fb_palette_data[col].r,
				 fb_palette_data[col].g,
				 fb_palette_data[col].b);
    }

  return GRUB_ERR_NONE;
}

static struct grub_video_adapter grub_video_ieee1275_adapter =
  {
    .name = "IEEE1275 video driver",

    .prio = GRUB_VIDEO_ADAPTER_PRIO_FIRMWARE,

    .init = grub_video_ieee1275_init,
    .fini = grub_video_ieee1275_fini,
    .setup = grub_video_ieee1275_setup,
    .get_info = grub_video_fb_get_info,
    .get_info_and_fini = grub_video_ieee1275_get_info_and_fini,
    .set_palette = grub_video_ieee1275_set_palette,
    .get_palette = grub_video_fb_get_palette,
    .set_viewport = grub_video_fb_set_viewport,
    .get_viewport = grub_video_fb_get_viewport,
    .map_color = grub_video_fb_map_color,
    .map_rgb = grub_video_fb_map_rgb,
    .map_rgba = grub_video_fb_map_rgba,
    .unmap_color = grub_video_fb_unmap_color,
    .fill_rect = grub_video_fb_fill_rect,
    .blit_bitmap = grub_video_fb_blit_bitmap,
    .blit_render_target = grub_video_fb_blit_render_target,
    .scroll = grub_video_fb_scroll,
    .swap_buffers = grub_video_fb_swap_buffers,
    .create_render_target = grub_video_fb_create_render_target,
    .delete_render_target = grub_video_fb_delete_render_target,
    .set_active_render_target = grub_video_fb_set_active_render_target,
    .get_active_render_target = grub_video_fb_get_active_render_target,

    .next = 0
  };

GRUB_MOD_INIT(ieee1275_fb)
{
  find_display ();
  if (display)
    grub_video_register (&grub_video_ieee1275_adapter);
}

GRUB_MOD_FINI(ieee1275_fb)
{
  if (restore_needed)
    {
      set_video_mode (old_width, old_height);
      restore_needed = 0;
    }
  if (display)
    grub_video_unregister (&grub_video_ieee1275_adapter);
  grub_free (display);
}
