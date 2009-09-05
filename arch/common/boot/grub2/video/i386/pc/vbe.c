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
#include <grub/machine/memory.h>
#include <grub/machine/vga.h>
#include <grub/machine/vbe.h>
#include <grub/types.h>
#include <grub/dl.h>
#include <grub/misc.h>
#include <grub/mm.h>
#include <grub/video.h>

static int vbe_detected = -1;

static struct grub_vbe_info_block controller_info;
static struct grub_vbe_mode_info_block active_vbe_mode_info;

static struct
{
  struct grub_video_mode_info mode_info;
  struct grub_video_render_target *render_target;

  unsigned int bytes_per_scan_line;
  unsigned int bytes_per_pixel;
  grub_uint32_t active_vbe_mode;
  grub_uint8_t *ptr;
  int index_color_mode;
} framebuffer;

static grub_uint32_t initial_vbe_mode;
static grub_uint16_t *vbe_mode_list;

static void *
real2pm (grub_vbe_farptr_t ptr)
{
  return (void *) ((((unsigned long) ptr & 0xFFFF0000) >> 12UL)
                   + ((unsigned long) ptr & 0x0000FFFF));
}

grub_err_t
grub_vbe_probe (struct grub_vbe_info_block *info_block)
{
  struct grub_vbe_info_block *vbe_ib;
  grub_vbe_status_t status;

  /* Clear caller's controller info block.  */
  if (info_block)
    grub_memset (info_block, 0, sizeof (*info_block));

  /* Do not probe more than one time, if not necessary.  */
  if (vbe_detected == -1 || info_block)
    {
      /* Clear old copy of controller info block.  */
      grub_memset (&controller_info, 0, sizeof (controller_info));

      /* Mark VESA BIOS extension as undetected.  */
      vbe_detected = 0;

      /* Use low memory scratch area as temporary storage
         for VESA BIOS call.  */
      vbe_ib = (struct grub_vbe_info_block *) GRUB_MEMORY_MACHINE_SCRATCH_ADDR;

      /* Prepare info block.  */
      grub_memset (vbe_ib, 0, sizeof (*vbe_ib));

      vbe_ib->signature[0] = 'V';
      vbe_ib->signature[1] = 'B';
      vbe_ib->signature[2] = 'E';
      vbe_ib->signature[3] = '2';

      /* Try to get controller info block.  */
      status = grub_vbe_bios_get_controller_info (vbe_ib);
      if (status == GRUB_VBE_STATUS_OK)
        {
          /* Copy it for later usage.  */
          grub_memcpy (&controller_info, vbe_ib, sizeof (controller_info));

          /* Mark VESA BIOS extension as detected.  */
          vbe_detected = 1;
        }
    }

  if (! vbe_detected)
    return grub_error (GRUB_ERR_BAD_DEVICE, "VESA BIOS Extension not found");

  /* Make copy of controller info block to caller.  */
  if (info_block)
    grub_memcpy (info_block, &controller_info, sizeof (*info_block));

  return GRUB_ERR_NONE;
}

grub_err_t
grub_vbe_set_video_mode (grub_uint32_t vbe_mode,
			 struct grub_vbe_mode_info_block *vbe_mode_info)
{
  grub_vbe_status_t status;
  grub_uint32_t old_vbe_mode;
  struct grub_vbe_mode_info_block new_vbe_mode_info;
  grub_err_t err;

  /* Make sure that VBE is supported.  */
  grub_vbe_probe (0);
  if (grub_errno != GRUB_ERR_NONE)
    return grub_errno;

  /* Try to get mode info.  */
  grub_vbe_get_video_mode_info (vbe_mode, &new_vbe_mode_info);
  if (grub_errno != GRUB_ERR_NONE)
    return grub_errno;

  /* For all VESA BIOS modes, force linear frame buffer.  */
  if (vbe_mode >= 0x100)
    {
      /* We only want linear frame buffer modes.  */
      vbe_mode |= 1 << 14;

      /* Determine frame buffer pixel format.  */
      switch (new_vbe_mode_info.memory_model)
        {
        case GRUB_VBE_MEMORY_MODEL_PACKED_PIXEL:
          framebuffer.index_color_mode = 1;
          break;

        case GRUB_VBE_MEMORY_MODEL_DIRECT_COLOR:
          framebuffer.index_color_mode = 0;
          break;

        default:
          return grub_error (GRUB_ERR_NOT_IMPLEMENTED_YET,
                             "unsupported pixel format 0x%x",
                             new_vbe_mode_info.memory_model);
        }
    }

  /* Get current mode.  */
  grub_vbe_get_video_mode (&old_vbe_mode);
  if (grub_errno != GRUB_ERR_NONE)
    return grub_errno;

  /* Try to set video mode.  */
  status = grub_vbe_bios_set_mode (vbe_mode, 0);
  if (status != GRUB_VBE_STATUS_OK)
    return grub_error (GRUB_ERR_BAD_DEVICE, "cannot set VBE mode %x", vbe_mode);

  /* Save information for later usage.  */
  framebuffer.active_vbe_mode = vbe_mode;
  grub_memcpy (&active_vbe_mode_info, &new_vbe_mode_info, sizeof (active_vbe_mode_info));

  if (vbe_mode < 0x100)
    {
      /* If this is not a VESA mode, guess address.  */
      framebuffer.ptr = (grub_uint8_t *) GRUB_MEMORY_MACHINE_VGA_ADDR;
      framebuffer.index_color_mode = 1;
    }
  else
    {
      framebuffer.ptr = (grub_uint8_t *) new_vbe_mode_info.phys_base_addr;

      if (controller_info.version >= 0x300)
        framebuffer.bytes_per_scan_line = new_vbe_mode_info.lin_bytes_per_scan_line;
      else
        framebuffer.bytes_per_scan_line = new_vbe_mode_info.bytes_per_scan_line;
    }

  /* Check whether mode is text mode or graphics mode.  */
  if (new_vbe_mode_info.memory_model == GRUB_VBE_MEMORY_MODEL_TEXT)
    {
      /* Text mode.  */

      /* No special action needed for text mode as it is not supported for
         graphical support.  */
    }
  else
    {
      /* Graphics mode.  */

      /* Calculate bytes_per_pixel value.  */
      switch(new_vbe_mode_info.bits_per_pixel)
	{
	case 32: framebuffer.bytes_per_pixel = 4; break;
	case 24: framebuffer.bytes_per_pixel = 3; break;
	case 16: framebuffer.bytes_per_pixel = 2; break;
	case 15: framebuffer.bytes_per_pixel = 2; break;
	case 8: framebuffer.bytes_per_pixel = 1; break;
	default:
	  grub_vbe_bios_set_mode (old_vbe_mode, 0);
	  return grub_error (GRUB_ERR_BAD_DEVICE,
			     "cannot set VBE mode %x",
			     vbe_mode);
	  break;
	}

      /* If video mode is in indexed color, setup default VGA palette.  */
      if (framebuffer.index_color_mode)
	{
	  struct grub_vbe_palette_data *palette
	    = (struct grub_vbe_palette_data *) GRUB_MEMORY_MACHINE_SCRATCH_ADDR;
	  unsigned i;

	  /* Make sure that the BIOS can reach the palette.  */
	  for (i = 0; i < GRUB_VIDEO_FBSTD_NUMCOLORS; i++)
	    {
	      palette[i].red = grub_video_fbstd_colors[i].r;
	      palette[i].green = grub_video_fbstd_colors[i].g;
	      palette[i].blue = grub_video_fbstd_colors[i].b;
	      palette[i].alignment = 0;
	    }

	  status = grub_vbe_bios_set_palette_data (GRUB_VIDEO_FBSTD_NUMCOLORS,
						   0, palette);

	  /* Just ignore the status.  */
	  err = grub_video_fb_set_palette (0, GRUB_VIDEO_FBSTD_NUMCOLORS,
					   grub_video_fbstd_colors);
	  if (err)
	    return err;

	}
    }

  /* Copy mode info for caller.  */
  if (vbe_mode_info)
    grub_memcpy (vbe_mode_info, &new_vbe_mode_info, sizeof (*vbe_mode_info));

  return GRUB_ERR_NONE;
}

grub_err_t
grub_vbe_get_video_mode (grub_uint32_t *mode)
{
  grub_vbe_status_t status;

  /* Make sure that VBE is supported.  */
  grub_vbe_probe (0);
  if (grub_errno != GRUB_ERR_NONE)
    return grub_errno;

  /* Try to query current mode from VESA BIOS.  */
  status = grub_vbe_bios_get_mode (mode);
  if (status != GRUB_VBE_STATUS_OK)
    return grub_error (GRUB_ERR_BAD_DEVICE, "cannot get current VBE mode");

  return GRUB_ERR_NONE;
}

grub_err_t
grub_vbe_get_video_mode_info (grub_uint32_t mode,
                              struct grub_vbe_mode_info_block *mode_info)
{
  struct grub_vbe_mode_info_block *mi_tmp
    = (struct grub_vbe_mode_info_block *) GRUB_MEMORY_MACHINE_SCRATCH_ADDR;
  grub_vbe_status_t status;

  /* Make sure that VBE is supported.  */
  grub_vbe_probe (0);
  if (grub_errno != GRUB_ERR_NONE)
    return grub_errno;

  /* If mode is not VESA mode, skip mode info query.  */
  if (mode >= 0x100)
    {
      /* Try to get mode info from VESA BIOS.  */
      status = grub_vbe_bios_get_mode_info (mode, mi_tmp);
      if (status != GRUB_VBE_STATUS_OK)
        return grub_error (GRUB_ERR_BAD_DEVICE,
                           "cannot get information on the mode %x", mode);

      /* Make copy of mode info block.  */
      grub_memcpy (mode_info, mi_tmp, sizeof (*mode_info));

      /* Packed mode.  Query DAC Palette width for color sizes.  */
      if (mode_info->bits_per_pixel <= 8)
	{
	  int width = 8;
	  status = 0;

	  if (controller_info.capabilities & GRUB_VBE_CAPABILITY_DACWIDTH)
	    status = grub_vbe_bios_set_dac_palette_width (& width);

	  if (status != GRUB_VBE_STATUS_OK)
	    /* 6 is default after mode reset.  */
	    width = 6;

	  mode_info->red_mask_size = mode_info->green_mask_size
	    = mode_info->blue_mask_size = width;
	  mode_info->rsvd_mask_size = 0;
	}
    }
  else
    /* Just clear mode info block if it isn't a VESA mode.  */
    grub_memset (mode_info, 0, sizeof (*mode_info));

  return GRUB_ERR_NONE;
}

static grub_err_t
grub_video_vbe_init (void)
{
  grub_uint16_t *rm_vbe_mode_list;
  grub_uint16_t *p;
  grub_size_t vbe_mode_list_size;
  struct grub_vbe_info_block info_block;

  /* Check if there is adapter present.

     Firmware note: There has been a report that some cards store video mode
     list in temporary memory.  So we must first use vbe probe to get
     refreshed information to receive valid pointers and data, and then
     copy this information to somewhere safe.  */
  grub_vbe_probe (&info_block);
  if (grub_errno != GRUB_ERR_NONE)
    return grub_errno;

  /* Copy modelist to local memory.  */
  p = rm_vbe_mode_list = real2pm (info_block.video_mode_ptr);
  while(*p++ != 0xFFFF)
    ;

  vbe_mode_list_size = (grub_addr_t) p - (grub_addr_t) rm_vbe_mode_list;
  vbe_mode_list = grub_malloc (vbe_mode_list_size);
  if (! vbe_mode_list)
    return grub_errno;
  grub_memcpy (vbe_mode_list, rm_vbe_mode_list, vbe_mode_list_size);

  /* Adapter could be found, figure out initial video mode.  */
  grub_vbe_get_video_mode (&initial_vbe_mode);
  if (grub_errno != GRUB_ERR_NONE)
    {
      /* Free allocated resources.  */
      grub_free (vbe_mode_list);
      vbe_mode_list = NULL;

      return grub_errno;
    }

  /* Reset frame buffer.  */
  grub_memset (&framebuffer, 0, sizeof(framebuffer));

  return grub_video_fb_init ();
}

static grub_err_t
grub_video_vbe_fini (void)
{
  grub_vbe_status_t status;

  /* Restore old video mode.  */
  status = grub_vbe_bios_set_mode (initial_vbe_mode, 0);
  if (status != GRUB_VBE_STATUS_OK)
    /* TODO: Decide, is this something we want to do.  */
    return grub_errno;

  /* TODO: Free any resources allocated by driver.  */
  grub_free (vbe_mode_list);
  vbe_mode_list = NULL;

  /* TODO: destroy render targets.  */

  return grub_video_fb_fini ();
}

static grub_err_t
grub_video_vbe_setup (unsigned int width, unsigned int height,
                      unsigned int mode_type)
{
  grub_uint16_t *p;
  struct grub_vbe_mode_info_block vbe_mode_info;
  struct grub_vbe_mode_info_block best_vbe_mode_info;
  grub_uint32_t best_vbe_mode = 0;
  int depth;

  /* Decode depth from mode_type.  If it is zero, then autodetect.  */
  depth = (mode_type & GRUB_VIDEO_MODE_TYPE_DEPTH_MASK)
          >> GRUB_VIDEO_MODE_TYPE_DEPTH_POS;

  /* Walk thru mode list and try to find matching mode.  */
  for (p = vbe_mode_list; *p != 0xFFFF; p++)
    {
      grub_uint32_t vbe_mode = *p;

      grub_vbe_get_video_mode_info (vbe_mode, &vbe_mode_info);
      if (grub_errno != GRUB_ERR_NONE)
        {
          /* Could not retrieve mode info, retreat.  */
          grub_errno = GRUB_ERR_NONE;
          break;
        }

      if ((vbe_mode_info.mode_attributes & 0x001) == 0)
        /* If not available, skip it.  */
        continue;

      if ((vbe_mode_info.mode_attributes & 0x002) == 0)
        /* Not enough information.  */
        continue;

      if ((vbe_mode_info.mode_attributes & 0x008) == 0)
        /* Monochrome is unusable.  */
        continue;

      if ((vbe_mode_info.mode_attributes & 0x080) == 0)
        /* We support only linear frame buffer modes.  */
        continue;

      if ((vbe_mode_info.mode_attributes & 0x010) == 0)
        /* We allow only graphical modes.  */
        continue;

      if ((vbe_mode_info.memory_model != GRUB_VBE_MEMORY_MODEL_PACKED_PIXEL)
          && (vbe_mode_info.memory_model != GRUB_VBE_MEMORY_MODEL_DIRECT_COLOR))
        /* Not compatible memory model.  */
        continue;

      if ((vbe_mode_info.x_resolution != width)
          || (vbe_mode_info.y_resolution != height))
        /* Non matching resolution.  */
        continue;

      /* Check if user requested RGB or index color mode.  */
      if ((mode_type & GRUB_VIDEO_MODE_TYPE_COLOR_MASK) != 0)
        {
          if (((mode_type & GRUB_VIDEO_MODE_TYPE_INDEX_COLOR) != 0)
              && (vbe_mode_info.memory_model != GRUB_VBE_MEMORY_MODEL_PACKED_PIXEL))
            /* Requested only index color modes.  */
            continue;

          if (((mode_type & GRUB_VIDEO_MODE_TYPE_RGB) != 0)
              && (vbe_mode_info.memory_model != GRUB_VBE_MEMORY_MODEL_DIRECT_COLOR))
            /* Requested only RGB modes.  */
            continue;
        }

      /* If there is a request for specific depth, ignore others.  */
      if ((depth != 0) && (vbe_mode_info.bits_per_pixel != depth))
        continue;

      /* Select mode with most number of bits per pixel.  */
      if (best_vbe_mode != 0)
        if (vbe_mode_info.bits_per_pixel < best_vbe_mode_info.bits_per_pixel)
          continue;

      /* Save so far best mode information for later use.  */
      best_vbe_mode = vbe_mode;
      grub_memcpy (&best_vbe_mode_info, &vbe_mode_info, sizeof (vbe_mode_info));
    }

  /* Try to initialize best mode found.  */
  if (best_vbe_mode != 0)
    {
      grub_err_t err;
      /* If this fails, then we have mode selection heuristics problem,
         or adapter failure.  */
      /* grub_vbe_set_video_mode already sets active_vbe_mode_info. */
      grub_vbe_set_video_mode (best_vbe_mode, NULL);
      if (grub_errno != GRUB_ERR_NONE)
        return grub_errno;

      /* Fill mode info details.  */
      framebuffer.mode_info.width = active_vbe_mode_info.x_resolution;
      framebuffer.mode_info.height = active_vbe_mode_info.y_resolution;

      if (framebuffer.index_color_mode)
        framebuffer.mode_info.mode_type = GRUB_VIDEO_MODE_TYPE_INDEX_COLOR;
      else
        framebuffer.mode_info.mode_type = GRUB_VIDEO_MODE_TYPE_RGB;

      framebuffer.mode_info.bpp = active_vbe_mode_info.bits_per_pixel;
      framebuffer.mode_info.bytes_per_pixel = framebuffer.bytes_per_pixel;
      framebuffer.mode_info.pitch = framebuffer.bytes_per_scan_line;
      framebuffer.mode_info.number_of_colors = 256; /* TODO: fix me.  */
      framebuffer.mode_info.red_mask_size = active_vbe_mode_info.red_mask_size;
      framebuffer.mode_info.red_field_pos = active_vbe_mode_info.red_field_position;
      framebuffer.mode_info.green_mask_size = active_vbe_mode_info.green_mask_size;
      framebuffer.mode_info.green_field_pos = active_vbe_mode_info.green_field_position;
      framebuffer.mode_info.blue_mask_size = active_vbe_mode_info.blue_mask_size;
      framebuffer.mode_info.blue_field_pos = active_vbe_mode_info.blue_field_position;
      framebuffer.mode_info.reserved_mask_size = active_vbe_mode_info.rsvd_mask_size;
      framebuffer.mode_info.reserved_field_pos = active_vbe_mode_info.rsvd_field_position;

      framebuffer.mode_info.blit_format = grub_video_get_blit_format (&framebuffer.mode_info);

      err = grub_video_fb_create_render_target_from_pointer (&framebuffer.render_target, &framebuffer.mode_info, framebuffer.ptr);

      if (err)
	return err;

      err = grub_video_fb_set_active_render_target (framebuffer.render_target);

      if (err)
	return err;

      /* Copy default palette to initialize emulated palette.  */
      err = grub_video_fb_set_palette (0, GRUB_VIDEO_FBSTD_NUMCOLORS,
				       grub_video_fbstd_colors);
      return err;
    }

  /* Couldn't found matching mode.  */
  return grub_error (GRUB_ERR_UNKNOWN_DEVICE, "no matching mode found.");
}

static grub_err_t
grub_video_vbe_set_palette (unsigned int start, unsigned int count,
                            struct grub_video_palette_data *palette_data)
{
  if (framebuffer.index_color_mode)
    {
      /* TODO: Implement setting indexed color mode palette to hardware.  */
      //status = grub_vbe_bios_set_palette_data (sizeof (vga_colors)
      //                                         / sizeof (struct grub_vbe_palette_data),
      //                                         0,
      //                                         palette);

    }

  /* Then set color to emulated palette.  */

  return grub_video_fb_set_palette (start, count, palette_data);
}

static grub_err_t
grub_video_vbe_swap_buffers (void)
{
  /* TODO: Implement buffer swapping.  */
  return GRUB_ERR_NONE;
}

static grub_err_t
grub_video_vbe_set_active_render_target (struct grub_video_render_target *target)
{
  if (target == GRUB_VIDEO_RENDER_TARGET_DISPLAY)
      target = framebuffer.render_target;

  return grub_video_fb_set_active_render_target (target);
}

static grub_err_t
grub_video_vbe_get_info_and_fini (struct grub_video_mode_info *mode_info,
				  void **framebuf)
{
  grub_memcpy (mode_info, &(framebuffer.mode_info), sizeof (*mode_info));
  *framebuf = (char *) framebuffer.ptr;

  grub_free (vbe_mode_list);
  vbe_mode_list = NULL;

  grub_video_fb_fini ();

  return GRUB_ERR_NONE;
}


static struct grub_video_adapter grub_video_vbe_adapter =
  {
    .name = "VESA BIOS Extension Video Driver",

    .init = grub_video_vbe_init,
    .fini = grub_video_vbe_fini,
    .setup = grub_video_vbe_setup,
    .get_info = grub_video_fb_get_info,
    .get_info_and_fini = grub_video_vbe_get_info_and_fini,
    .set_palette = grub_video_vbe_set_palette,
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
    .swap_buffers = grub_video_vbe_swap_buffers,
    .create_render_target = grub_video_fb_create_render_target,
    .delete_render_target = grub_video_fb_delete_render_target,
    .set_active_render_target = grub_video_vbe_set_active_render_target,
    .get_active_render_target = grub_video_fb_get_active_render_target,

    .next = 0
  };

GRUB_MOD_INIT(video_i386_pc_vbe)
{
  grub_video_register (&grub_video_vbe_adapter);
}

GRUB_MOD_FINI(video_i386_pc_vbe)
{
  grub_video_unregister (&grub_video_vbe_adapter);
}
