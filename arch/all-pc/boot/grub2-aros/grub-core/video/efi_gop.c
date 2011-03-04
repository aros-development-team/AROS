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
#include <grub/efi/api.h>
#include <grub/efi/efi.h>
#include <grub/efi/graphics_output.h>

static grub_efi_guid_t graphics_output_guid = GRUB_EFI_GOP_GUID;
static struct grub_efi_gop *gop;
static unsigned old_mode;
static int restore_needed;

static struct
{
  struct grub_video_mode_info mode_info;
  struct grub_video_render_target *render_target;
  grub_uint8_t *ptr;
} framebuffer;


static int
check_protocol (void)
{
  gop = grub_efi_locate_protocol (&graphics_output_guid, 0);
  if (gop)
    return 1;

  return 0;
}

static grub_err_t
grub_video_gop_init (void)
{
  grub_memset (&framebuffer, 0, sizeof(framebuffer));
  return grub_video_fb_init ();
}

static grub_err_t
grub_video_gop_fini (void)
{
  if (restore_needed)
    {
      efi_call_2 (gop->set_mode, gop, old_mode);
      restore_needed = 0;
    }
  return grub_video_fb_fini ();
}

static int
grub_video_gop_get_bpp (struct grub_efi_gop_mode_info *in)
{
  grub_uint32_t total_mask;
  int i;
  switch (in->pixel_format)
    {
    case GRUB_EFI_GOT_BGRA8:
    case GRUB_EFI_GOT_RGBA8:
      return 32;

    case GRUB_EFI_GOT_BITMASK:
      /* Check overlaps.  */
      if ((in->pixel_bitmask.r & in->pixel_bitmask.g)
	  || (in->pixel_bitmask.r & in->pixel_bitmask.b)
	  || (in->pixel_bitmask.g & in->pixel_bitmask.b)
	  || (in->pixel_bitmask.r & in->pixel_bitmask.a)
	  || (in->pixel_bitmask.g & in->pixel_bitmask.a)
	  || (in->pixel_bitmask.b & in->pixel_bitmask.a))
	return 0;

      total_mask = in->pixel_bitmask.r | in->pixel_bitmask.g
	| in->pixel_bitmask.b | in->pixel_bitmask.a;

      for (i = 31; i >= 0; i--)
	if (total_mask & (1 << i))
	  return i + 1;

      /* Fall through.  */
    default:
      return 0;
    }
}

static void
grub_video_gop_get_bitmask (grub_uint32_t mask, unsigned int *mask_size,
			    unsigned int *field_pos)
{
  int i;
  int last_p;
  for (i = 31; i >= 0; i--)
    if (mask & (1 << i))
      break;
  if (i == -1)
    {
      *mask_size = *field_pos = 0;
      return;
    }
  last_p = i;
  for (; i >= 0; i--)
    if (!(mask & (1 << i)))
      break;
  *field_pos = i + 1;
  *mask_size = last_p - *field_pos + 1;
}

static grub_err_t
grub_video_gop_fill_mode_info (unsigned mode,
			       struct grub_efi_gop_mode_info *in,
			       struct grub_video_mode_info *out)
{
  out->mode_number = mode;
  out->number_of_colors = 256;
  out->width = in->width;
  out->height = in->height;
  out->mode_type = GRUB_VIDEO_MODE_TYPE_RGB;
  out->bpp = grub_video_gop_get_bpp (in);
  out->bytes_per_pixel = out->bpp >> 3;
  if (!out->bpp)
    return grub_error (GRUB_ERR_IO, "unsupported video mode");
  out->pitch = in->pixels_per_scanline * out->bytes_per_pixel;

  switch (in->pixel_format)
    {
    case GRUB_EFI_GOT_RGBA8:
      out->red_mask_size = 8;
      out->red_field_pos = 0;
      out->green_mask_size = 8;
      out->green_field_pos = 8;
      out->blue_mask_size = 8;
      out->blue_field_pos = 16;
      out->reserved_mask_size = 8;
      out->reserved_field_pos = 24;
      break;

    case GRUB_EFI_GOT_BGRA8:
      out->red_mask_size = 8;
      out->red_field_pos = 16;
      out->green_mask_size = 8;
      out->green_field_pos = 8;
      out->blue_mask_size = 8;
      out->blue_field_pos = 0;
      out->reserved_mask_size = 8;
      out->reserved_field_pos = 24;
      break;

    case GRUB_EFI_GOT_BITMASK:
      grub_video_gop_get_bitmask (in->pixel_bitmask.r, &out->red_mask_size,
				  &out->red_field_pos);
      grub_video_gop_get_bitmask (in->pixel_bitmask.g, &out->green_mask_size,
				  &out->green_field_pos);
      grub_video_gop_get_bitmask (in->pixel_bitmask.b, &out->blue_mask_size,
				  &out->blue_field_pos);
      grub_video_gop_get_bitmask (in->pixel_bitmask.a, &out->reserved_mask_size,
				  &out->reserved_field_pos);
      break;

    default:
      return grub_error (GRUB_ERR_IO, "unsupported video mode");
    }

  out->blit_format = grub_video_get_blit_format (out);
  return GRUB_ERR_NONE;
}

static int
grub_video_gop_iterate (int (*hook) (const struct grub_video_mode_info *info))
{
  unsigned mode;

  for (mode = 0; mode < gop->mode->max_mode; mode++)
    {
      grub_efi_uintn_t size;
      grub_efi_status_t status;
      struct grub_efi_gop_mode_info *info = NULL;
      grub_err_t err;
      struct grub_video_mode_info mode_info;
	 
      status = efi_call_4 (gop->query_mode, gop, mode, &size, &info);

      if (status)
	{
	  info = 0;
	  continue;
	}

      err = grub_video_gop_fill_mode_info (mode, info, &mode_info);
      if (err)
	{
	  grub_errno = GRUB_ERR_NONE;
	  continue;
	}
      if (hook (&mode_info))
	return 1;
    }
  return 0;
}

static grub_err_t
grub_video_gop_setup (unsigned int width, unsigned int height,
		      unsigned int mode_type,
		      unsigned int mode_mask __attribute__ ((unused)))
{
  unsigned int depth;
  struct grub_efi_gop_mode_info *info = NULL;
  unsigned best_mode = 0;
  grub_err_t err;
  unsigned bpp;
  int found = 0;
  unsigned long long best_volume = 0;

  depth = (mode_type & GRUB_VIDEO_MODE_TYPE_DEPTH_MASK)
    >> GRUB_VIDEO_MODE_TYPE_DEPTH_POS;

  /* Keep current mode if possible.  */
  if (gop->mode->info)
    {
      bpp = grub_video_gop_get_bpp (gop->mode->info);
      if (bpp && ((width == gop->mode->info->width
		   && height == gop->mode->info->height)
		  || (width == 0 && height == 0))
	  && (depth == bpp || depth == 0))
	{
	  grub_dprintf ("video", "GOP: keeping mode %d\n", gop->mode->mode);
	  best_mode = gop->mode->mode;
	  found = 1;
	}
    }
 
  if (!found)
    {
      unsigned mode;
      grub_dprintf ("video", "GOP: %d modes detected\n", gop->mode->max_mode);
      for (mode = 0; mode < gop->mode->max_mode; mode++)
	{
	  grub_efi_uintn_t size;
	  grub_efi_status_t status;
	 
	  status = efi_call_4 (gop->query_mode, gop, mode, &size, &info);
	  if (status)
	    {
	      info = 0;
	      continue;
	    }

	  grub_dprintf ("video", "GOP: mode %d: %dx%d\n", mode, info->width,
			info->height);

	  bpp = grub_video_gop_get_bpp (info);
	  if (!bpp)
	    {
	      grub_dprintf ("video", "GOP: mode %d: incompatible pixel mode\n",
			    mode);
	      continue;
	    }

	  grub_dprintf ("video", "GOP: mode %d: depth %d\n", mode, bpp);

	  if (!(((info->width == width && info->height == height)
		|| (width == 0 && height == 0))
		&& (bpp == depth || depth == 0)))
	    {
	      grub_dprintf ("video", "GOP: mode %d: rejected\n", mode);
	      continue;
	    }

	  if (best_volume < ((unsigned long long) info->width)
	      * ((unsigned long long) info->height)
	      * ((unsigned long long) bpp))
	    {
	      best_volume = ((unsigned long long) info->width)
		* ((unsigned long long) info->height)
		* ((unsigned long long) bpp);
	      best_mode = mode;
	    }
	  found = 1;
	}
    }

  if (!found)
    {
      grub_dprintf ("video", "GOP: no mode found\n");
      return grub_error (GRUB_ERR_UNKNOWN_DEVICE, "no matching mode found");
    }

  if (best_mode != gop->mode->mode)
    {
      if (!restore_needed)
	{
	  old_mode = gop->mode->mode;
	  restore_needed = 1;
	}
      efi_call_2 (gop->set_mode, gop, best_mode);
    }

  info = gop->mode->info;

  err = grub_video_gop_fill_mode_info (gop->mode->mode, info,
				       &framebuffer.mode_info);
  if (err)
    {
      grub_dprintf ("video", "GOP: couldn't fill mode info\n");
      return err;
    }

  framebuffer.ptr = (void *) (grub_addr_t) gop->mode->fb_base;

  grub_dprintf ("video", "GOP: initialising FB @ %p %dx%dx%d\n",
		framebuffer.ptr, framebuffer.mode_info.width,
		framebuffer.mode_info.height, framebuffer.mode_info.bpp);
 
  err = grub_video_fb_create_render_target_from_pointer
    (&framebuffer.render_target, &framebuffer.mode_info, framebuffer.ptr);

  if (err)
    {
      grub_dprintf ("video", "GOP: Couldn't create FB target\n");
      return err;
    }
 
  err = grub_video_fb_set_active_render_target (framebuffer.render_target);
 
  if (err)
    {
      grub_dprintf ("video", "GOP: Couldn't set FB target\n");
      return err;
    }
 
  err = grub_video_fb_set_palette (0, GRUB_VIDEO_FBSTD_NUMCOLORS,
				   grub_video_fbstd_colors);

  if (err)
    grub_dprintf ("video", "GOP: Couldn't set palette\n");
  else
    grub_dprintf ("video", "GOP: Success\n");
 
  return err;
}

static grub_err_t
grub_video_gop_swap_buffers (void)
{
  /* TODO: Implement buffer swapping.  */
  return GRUB_ERR_NONE;
}

static grub_err_t
grub_video_gop_set_active_render_target (struct grub_video_render_target *target)
{
  if (target == GRUB_VIDEO_RENDER_TARGET_DISPLAY)
    target = framebuffer.render_target;

  return grub_video_fb_set_active_render_target (target);
}

static grub_err_t
grub_video_gop_get_info_and_fini (struct grub_video_mode_info *mode_info,
				  void **framebuf)
{
  grub_memcpy (mode_info, &(framebuffer.mode_info), sizeof (*mode_info));
  *framebuf = (char *) framebuffer.ptr;

  grub_video_fb_fini ();

  return GRUB_ERR_NONE;
}

static struct grub_video_adapter grub_video_gop_adapter =
  {
    .name = "EFI GOP driver",
    .id = GRUB_VIDEO_DRIVER_EFI_GOP,

    .prio = GRUB_VIDEO_ADAPTER_PRIO_FIRMWARE,

    .init = grub_video_gop_init,
    .fini = grub_video_gop_fini,
    .setup = grub_video_gop_setup,
    .get_info = grub_video_fb_get_info,
    .get_info_and_fini = grub_video_gop_get_info_and_fini,
    .set_palette = grub_video_fb_set_palette,
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
    .swap_buffers = grub_video_gop_swap_buffers,
    .create_render_target = grub_video_fb_create_render_target,
    .delete_render_target = grub_video_fb_delete_render_target,
    .set_active_render_target = grub_video_gop_set_active_render_target,
    .get_active_render_target = grub_video_fb_get_active_render_target,
    .iterate = grub_video_gop_iterate,

    .next = 0
  };

GRUB_MOD_INIT(efi_gop)
{
  if (check_protocol ())
    grub_video_register (&grub_video_gop_adapter);
}

GRUB_MOD_FINI(efi_gop)
{
  if (restore_needed)
    {
      efi_call_2 (gop->set_mode, gop, old_mode);
      restore_needed = 0;
    }
  if (gop)
    grub_video_unregister (&grub_video_gop_adapter);
}
