/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2005,2006,2007,2008  Free Software Foundation, Inc.
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

#include <grub/err.h>
#include <grub/machine/memory.h>
#include <grub/machine/vbe.h>
#include <grub/machine/vbeblit.h>
#include <grub/machine/vbefill.h>
#include <grub/machine/vbeutil.h>
#include <grub/types.h>
#include <grub/dl.h>
#include <grub/misc.h>
#include <grub/font.h>
#include <grub/mm.h>
#include <grub/video.h>
#include <grub/bitmap.h>

/* Specify "standard" VGA palette, some video cards may
   need this and this will also be used when using RGB modes.  */
static struct grub_vbe_palette_data vga_colors[16] =
  {
    // {B, G, R, A}
    {0x00, 0x00, 0x00, 0x00}, // 0 = black
    {0xA8, 0x00, 0x00, 0x00}, // 1 = blue
    {0x00, 0xA8, 0x00, 0x00}, // 2 = green
    {0xA8, 0xA8, 0x00, 0x00}, // 3 = cyan
    {0x00, 0x00, 0xA8, 0x00}, // 4 = red
    {0xA8, 0x00, 0xA8, 0x00}, // 5 = magenta
    {0x00, 0x54, 0xA8, 0x00}, // 6 = brown
    {0xA8, 0xA8, 0xA8, 0x00}, // 7 = ligth gray

    {0x54, 0x54, 0x54, 0x00}, // 8 = dark gray
    {0xFE, 0x54, 0x54, 0x00}, // 9 = bright blue
    {0x54, 0xFE, 0x54, 0x00}, // 10 = bright green
    {0xFE, 0xFE, 0x54, 0x00}, // 11 = bright cyan
    {0x54, 0x54, 0xFE, 0x00}, // 12 = bright red
    {0xFE, 0x54, 0xFE, 0x00}, // 13 = bright magenta
    {0x54, 0xFE, 0xFE, 0x00}, // 14 = yellow
    {0xFE, 0xFE, 0xFE, 0x00}  // 15 = white
  };

static int vbe_detected = -1;

static struct grub_vbe_info_block controller_info;
static struct grub_vbe_mode_info_block active_mode_info;

static struct
{
  struct grub_video_render_target render_target;

  unsigned int bytes_per_scan_line;
  unsigned int bytes_per_pixel;
  grub_uint32_t active_mode;
  grub_uint8_t *ptr;
  int index_color_mode;
  struct grub_video_palette_data palette[256];
} framebuffer;

static struct grub_video_render_target *render_target;
static grub_uint32_t initial_mode;
static grub_uint32_t mode_in_use = 0x55aa;
static grub_uint16_t *mode_list;

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
      if (status == 0x004F)
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
grub_vbe_set_video_mode (grub_uint32_t mode,
                         struct grub_vbe_mode_info_block *mode_info)
{
  grub_vbe_status_t status;
  grub_uint32_t old_mode;

  /* Make sure that VBE is supported.  */
  grub_vbe_probe (0);
  if (grub_errno != GRUB_ERR_NONE)
    return grub_errno;

  /* Try to get mode info.  */
  grub_vbe_get_video_mode_info (mode, &active_mode_info);
  if (grub_errno != GRUB_ERR_NONE)
    return grub_errno;

  /* For all VESA BIOS modes, force linear frame buffer.  */
  if (mode >= 0x100)
    {
      /* We only want linear frame buffer modes.  */
      mode |= 1 << 14;

      /* Determine frame buffer pixel format.  */
      switch (active_mode_info.memory_model)
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
                             active_mode_info.memory_model);
        }
    }

  /* Get current mode.  */
  grub_vbe_get_video_mode (&old_mode);
  if (grub_errno != GRUB_ERR_NONE)
    return grub_errno;

  /* Try to set video mode.  */
  status = grub_vbe_bios_set_mode (mode, 0);
  if (status != GRUB_VBE_STATUS_OK)
    return grub_error (GRUB_ERR_BAD_DEVICE, "cannot set VBE mode %x", mode);

  /* Save information for later usage.  */
  framebuffer.active_mode = mode;

  if (mode < 0x100)
    {
      /* If this is not a VESA mode, guess address.  */
      framebuffer.ptr = (grub_uint8_t *) 0xA0000;
      framebuffer.index_color_mode = 1;
    }
  else
    {
      framebuffer.ptr = (grub_uint8_t *) active_mode_info.phys_base_addr;

      if (controller_info.version >= 0x300)
        framebuffer.bytes_per_scan_line = active_mode_info.lin_bytes_per_scan_line;
      else
        framebuffer.bytes_per_scan_line = active_mode_info.bytes_per_scan_line;
    }

  /* Calculate bytes_per_pixel value.  */
  switch(active_mode_info.bits_per_pixel)
    {
    case 32: framebuffer.bytes_per_pixel = 4; break;
    case 24: framebuffer.bytes_per_pixel = 3; break;
    case 16: framebuffer.bytes_per_pixel = 2; break;
    case 15: framebuffer.bytes_per_pixel = 2; break;
    case 8: framebuffer.bytes_per_pixel = 1; break;
    default:
      grub_vbe_bios_set_mode (old_mode, 0);
      return grub_error (GRUB_ERR_BAD_DEVICE, 
                         "cannot set VBE mode %x",
                         mode);
      break;
    }

  /* If video mode is in indexed color, setup default VGA palette.  */
  if (framebuffer.index_color_mode)
    {
      struct grub_vbe_palette_data *palette
        = (struct grub_vbe_palette_data *) GRUB_MEMORY_MACHINE_SCRATCH_ADDR;

      /* Make sure that the BIOS can reach the palette.  */
      grub_memcpy (palette, vga_colors, sizeof (vga_colors));
      status = grub_vbe_bios_set_palette_data (sizeof (vga_colors) 
                                               / sizeof (struct grub_vbe_palette_data), 
                                               0, 
                                               palette);

      /* Just ignore the status.  */
    }

  /* Copy mode info for caller.  */
  if (mode_info)
    grub_memcpy (mode_info, &active_mode_info, sizeof (*mode_info));

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
    }
  else
    /* Just clear mode info block if it isn't a VESA mode.  */
    grub_memset (mode_info, 0, sizeof (*mode_info));

  return GRUB_ERR_NONE;
}

grub_uint8_t *
grub_video_vbe_get_video_ptr (struct grub_video_i386_vbeblit_info *source,
                              grub_uint32_t x, grub_uint32_t y)
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

static grub_err_t
grub_video_vbe_init (void)
{
  grub_uint16_t *rm_mode_list;
  grub_uint16_t *p;
  grub_size_t mode_list_size;
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
  p = rm_mode_list = real2pm (info_block.video_mode_ptr);
  while(*p++ != 0xFFFF)
    ;

  mode_list_size = (grub_addr_t) p - (grub_addr_t) rm_mode_list;
  mode_list = grub_malloc (mode_list_size);
  if (! mode_list)
    return grub_errno;
  grub_memcpy (mode_list, rm_mode_list, mode_list_size);

  /* Adapter could be found, figure out initial video mode.  */
  grub_vbe_get_video_mode (&initial_mode);
  if (grub_errno != GRUB_ERR_NONE)
    {
      /* Free allocated resources.  */
      grub_free (mode_list);
      mode_list = 0;

      return grub_errno;
    }

  /* Reset frame buffer and render target variables.  */
  grub_memset (&framebuffer, 0, sizeof(framebuffer));
  render_target = &framebuffer.render_target;

  return GRUB_ERR_NONE;
}

static grub_err_t
grub_video_vbe_fini (void)
{
  grub_vbe_status_t status;

  /* Restore old video mode.  */
  status = grub_vbe_bios_set_mode (initial_mode, 0);
  if (status != GRUB_VBE_STATUS_OK)
    /* TODO: Decide, is this something we want to do.  */
    return grub_errno;

  /* TODO: Free any resources allocated by driver.  */
  grub_free (mode_list);
  mode_list = 0;

  /* TODO: destroy render targets.  */

  /* Return success to caller.  */
  return GRUB_ERR_NONE;
}

static grub_err_t
grub_video_vbe_setup (unsigned int width, unsigned int height,
                      unsigned int mode_type)
{
  grub_uint16_t *p;
  struct grub_vbe_mode_info_block mode_info;
  struct grub_vbe_mode_info_block best_mode_info;
  grub_uint32_t best_mode = 0;
  int depth;
  unsigned int i;

  /* Decode depth from mode_type.  If it is zero, then autodetect.  */
  depth = (mode_type & GRUB_VIDEO_MODE_TYPE_DEPTH_MASK) 
          >> GRUB_VIDEO_MODE_TYPE_DEPTH_POS;

  /* Walk thru mode list and try to find matching mode.  */
  for (p = mode_list; *p != 0xFFFF; p++)
    {
      grub_uint32_t mode = *p;

      grub_vbe_get_video_mode_info (mode, &mode_info);
      if (grub_errno != GRUB_ERR_NONE)
        {
          /* Could not retrieve mode info, retreat.  */
          grub_errno = GRUB_ERR_NONE;
          break;
        }

      if ((mode_info.mode_attributes & 0x001) == 0)
        /* If not available, skip it.  */
        continue;

      if ((mode_info.mode_attributes & 0x002) == 0)
        /* Not enough information.  */
        continue;

      if ((mode_info.mode_attributes & 0x008) == 0)
        /* Monochrome is unusable.  */
        continue;

      if ((mode_info.mode_attributes & 0x080) == 0)
        /* We support only linear frame buffer modes.  */
        continue;

      if ((mode_info.mode_attributes & 0x010) == 0)
        /* We allow only graphical modes.  */
        continue;

      if ((mode_info.memory_model != GRUB_VBE_MEMORY_MODEL_PACKED_PIXEL)
          && (mode_info.memory_model != GRUB_VBE_MEMORY_MODEL_DIRECT_COLOR))
        /* Not compatible memory model.  */
        continue;

      if ((mode_info.x_resolution != width)
          || (mode_info.y_resolution != height))
        /* Non matching resolution.  */
        continue;

      /* Check if user requested RGB or index color mode.  */
      if ((mode_type & GRUB_VIDEO_MODE_TYPE_COLOR_MASK) != 0)
        {
          if (((mode_type & GRUB_VIDEO_MODE_TYPE_INDEX_COLOR) != 0)
              && (mode_info.memory_model != GRUB_VBE_MEMORY_MODEL_PACKED_PIXEL))
            /* Requested only index color modes.  */
            continue;

          if (((mode_type & GRUB_VIDEO_MODE_TYPE_RGB) != 0)
              && (mode_info.memory_model != GRUB_VBE_MEMORY_MODEL_DIRECT_COLOR))
            /* Requested only RGB modes.  */
            continue;
        }

      /* If there is a request for specific depth, ignore others.  */
      if ((depth != 0) && (mode_info.bits_per_pixel != depth))
        continue;

      /* Select mode with most number of bits per pixel.  */
      if (best_mode != 0)
        if (mode_info.bits_per_pixel < best_mode_info.bits_per_pixel)
          continue;

      /* Save so far best mode information for later use.  */
      best_mode = mode;
      grub_memcpy (&best_mode_info, &mode_info, sizeof (mode_info));
    }

  /* Try to initialize best mode found.  */
  if (best_mode != 0)
    {
      /* If this fails, then we have mode selection heuristics problem, 
         or adapter failure.  */
      grub_vbe_set_video_mode (best_mode, &active_mode_info);
      if (grub_errno != GRUB_ERR_NONE)
        return grub_errno;

      /* Now we are happily in requested video mode.  Cache some info
         in order to fasten later operations.  */
      mode_in_use = best_mode;

      /* Reset render target to framebuffer one.  */
      render_target = &framebuffer.render_target;

      /* Fill mode info details in framebuffer's render target.  */
      render_target->mode_info.width = active_mode_info.x_resolution;
      render_target->mode_info.height = active_mode_info.y_resolution;

      if (framebuffer.index_color_mode)
        render_target->mode_info.mode_type = GRUB_VIDEO_MODE_TYPE_INDEX_COLOR;
      else
        render_target->mode_info.mode_type = GRUB_VIDEO_MODE_TYPE_RGB;

      render_target->mode_info.bpp = active_mode_info.bits_per_pixel;
      render_target->mode_info.bytes_per_pixel = framebuffer.bytes_per_pixel;
      render_target->mode_info.pitch = framebuffer.bytes_per_scan_line;
      render_target->mode_info.number_of_colors = 256; /* TODO: fix me.  */
      render_target->mode_info.red_mask_size = active_mode_info.red_mask_size;
      render_target->mode_info.red_field_pos = active_mode_info.red_field_position;
      render_target->mode_info.green_mask_size = active_mode_info.green_mask_size;
      render_target->mode_info.green_field_pos = active_mode_info.green_field_position;
      render_target->mode_info.blue_mask_size = active_mode_info.blue_mask_size;
      render_target->mode_info.blue_field_pos = active_mode_info.blue_field_position;
      render_target->mode_info.reserved_mask_size = active_mode_info.rsvd_mask_size;
      render_target->mode_info.reserved_field_pos = active_mode_info.rsvd_field_position;

      render_target->mode_info.blit_format = grub_video_get_blit_format (&render_target->mode_info);

      /* Reset viewport to match new mode.  */
      render_target->viewport.x = 0;
      render_target->viewport.y = 0;
      render_target->viewport.width = active_mode_info.x_resolution;
      render_target->viewport.height = active_mode_info.y_resolution;

      /* Set framebuffer pointer and mark it as non allocated.  */
      render_target->is_allocated = 0;
      render_target->data = framebuffer.ptr;

      /* Copy default palette to initialize emulated palette.  */
      for (i = 0; 
           i < (sizeof (vga_colors) 
                / sizeof (struct grub_vbe_palette_data));
           i++)
        {
          framebuffer.palette[i].r = vga_colors[i].red;
          framebuffer.palette[i].g = vga_colors[i].green;
          framebuffer.palette[i].b = vga_colors[i].blue;
          framebuffer.palette[i].a = 0xFF;
        }

      return GRUB_ERR_NONE;
    }

  /* Couldn't found matching mode.  */
  return grub_error (GRUB_ERR_UNKNOWN_DEVICE, "no matching mode found.");
}

static grub_err_t
grub_video_vbe_get_info (struct grub_video_mode_info *mode_info)
{
  /* Copy mode info from active render target.  */
  grub_memcpy (mode_info, &render_target->mode_info, 
               sizeof (struct grub_video_mode_info));

  return GRUB_ERR_NONE;
}

static grub_err_t
grub_video_vbe_set_palette (unsigned int start, unsigned int count,
                            struct grub_video_palette_data *palette_data)
{
  unsigned int i;

  if (framebuffer.index_color_mode)
    {
      /* TODO: Implement setting indexed color mode palette to hardware.  */
      //status = grub_vbe_bios_set_palette_data (sizeof (vga_colors) 
      //                                         / sizeof (struct grub_vbe_palette_data), 
      //                                         0, 
      //                                         palette);

    }

  /* Then set color to emulated palette.  */
  for (i = 0; (i < count) && ((i + start) < 256); i++)
    framebuffer.palette[start + i] = palette_data[i];

  return GRUB_ERR_NONE;
}

static grub_err_t
grub_video_vbe_get_palette (unsigned int start, unsigned int count,
                            struct grub_video_palette_data *palette_data)
{
  unsigned int i;

  /* Assume that we know everything from index color palette.  */
  for (i = 0; (i < count) && ((i + start) < 256); i++)
    palette_data[i] = framebuffer.palette[start + i];

  return GRUB_ERR_NONE;
}

static grub_err_t
grub_video_vbe_set_viewport (unsigned int x, unsigned int y,
                             unsigned int width, unsigned int height)
{
  /* Make sure viewport is withing screen dimensions.  If viewport was set
     to be out of the region, mark its size as zero.  */
  if (x > active_mode_info.x_resolution)
    {
      x = 0;
      width = 0;
    }

  if (y > active_mode_info.y_resolution)
    {
      y = 0;
      height = 0;
    }

  if (x + width > active_mode_info.x_resolution)
    width = active_mode_info.x_resolution - x;

  if (y + height > active_mode_info.y_resolution)
    height = active_mode_info.y_resolution - y;

  render_target->viewport.x = x;
  render_target->viewport.y = y;
  render_target->viewport.width = width;
  render_target->viewport.height = height;

  return GRUB_ERR_NONE;
}

static grub_err_t
grub_video_vbe_get_viewport (unsigned int *x, unsigned int *y,
                             unsigned int *width, unsigned int *height)
{
  if (x) *x = render_target->viewport.x;
  if (y) *y = render_target->viewport.y;
  if (width) *width = render_target->viewport.width;
  if (height) *height = render_target->viewport.height;

  return GRUB_ERR_NONE;
}

static grub_video_color_t
grub_video_vbe_map_color (grub_uint32_t color_name)
{
  /* TODO: implement color theme mapping code.  */

  if (color_name < 256)
    {
      if ((render_target->mode_info.mode_type
           & GRUB_VIDEO_MODE_TYPE_INDEX_COLOR) != 0)
        return color_name;
      else
        {
          grub_video_color_t color;

          color = grub_video_vbe_map_rgb (framebuffer.palette[color_name].r,
                                          framebuffer.palette[color_name].g,
                                          framebuffer.palette[color_name].b);

          return color;
        }
    }

  return 0;
}

grub_video_color_t
grub_video_vbe_map_rgb (grub_uint8_t red, grub_uint8_t green,
                        grub_uint8_t blue)
{
  if ((render_target->mode_info.mode_type 
       & GRUB_VIDEO_MODE_TYPE_INDEX_COLOR) != 0)
    {
      int minindex = 0;
      int delta = 0;
      int tmp;
      int val;
      int i;

      /* Find best matching color.  */
      for (i = 0; i < 256; i++)
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

grub_video_color_t
grub_video_vbe_map_rgba (grub_uint8_t red, grub_uint8_t green,
                         grub_uint8_t blue, grub_uint8_t alpha)
{
  if ((render_target->mode_info.mode_type 
       & GRUB_VIDEO_MODE_TYPE_INDEX_COLOR) != 0)
    /* No alpha available in index color modes, just use
       same value as in only RGB modes.  */
    return grub_video_vbe_map_rgb (red, green, blue);
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

grub_err_t grub_video_vbe_unmap_color (grub_video_color_t color,
                                       grub_uint8_t *red, grub_uint8_t *green,
                                       grub_uint8_t *blue, grub_uint8_t *alpha)
{
  struct grub_video_i386_vbeblit_info target_info;

  target_info.mode_info = &render_target->mode_info;
  target_info.data = render_target->data;

  grub_video_vbe_unmap_color_int (&target_info, color, red, green, blue, alpha);
  
  return GRUB_ERR_NONE;
}

void
grub_video_vbe_unmap_color_int (struct grub_video_i386_vbeblit_info * source,
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

static grub_err_t
grub_video_vbe_fill_rect (grub_video_color_t color, int x, int y,
                          unsigned int width, unsigned int height)
{
  struct grub_video_i386_vbeblit_info target;

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

  /* Use vbeblit_info to encapsulate rendering.  */
  target.mode_info = &render_target->mode_info;
  target.data = render_target->data;

  /* Try to figure out more optimized version.  */
  if (target.mode_info->blit_format == GRUB_VIDEO_BLIT_FORMAT_R8G8B8A8)
    {
      grub_video_i386_vbefill_R8G8B8A8 (&target, color, x, y, 
                                        width, height);
      return GRUB_ERR_NONE;
    }

  if (target.mode_info->blit_format == GRUB_VIDEO_BLIT_FORMAT_R8G8B8)
    {
      grub_video_i386_vbefill_R8G8B8 (&target, color, x, y,
                                      width, height);
      return GRUB_ERR_NONE;
    }

  if (target.mode_info->blit_format == GRUB_VIDEO_BLIT_FORMAT_INDEXCOLOR)
    {
      grub_video_i386_vbefill_index (&target, color, x, y,
                                     width, height);
      return GRUB_ERR_NONE;
    }

  /* No optimized version found, use default (slow) filler.  */
  grub_video_i386_vbefill (&target, color, x, y, width, height);

  return GRUB_ERR_NONE;
}

// TODO: Remove this method and replace with bitmap based glyphs
static grub_err_t
grub_video_vbe_blit_glyph (struct grub_font_glyph * glyph,
                           grub_video_color_t color, int x, int y)
{
  struct grub_video_i386_vbeblit_info target;
  unsigned int width;
  unsigned int charwidth;
  unsigned int height;
  unsigned int i;
  unsigned int j;
  unsigned int x_offset = 0;
  unsigned int y_offset = 0;

  /* Make sure there is something to do.  */
  if (x >= (int)render_target->viewport.width)
    return GRUB_ERR_NONE;

  if (y >= (int)render_target->viewport.height)
    return GRUB_ERR_NONE;

  /* Calculate glyph dimensions.  */
  width = ((glyph->width + 7) / 8) * 8;
  charwidth = width;
  height = glyph->height;

  if (x + (int)width < 0)
    return GRUB_ERR_NONE;

  if (y + (int)height < 0)
    return GRUB_ERR_NONE;

  /* Do not allow drawing out of viewport.  */
  if (x < 0)
    {
      width += x;
      x_offset = (unsigned int)-x;
      x = 0;
    }
  if (y < 0)
    {
      height += y;
      y_offset = (unsigned int)-y;
      y = 0;
    }

  if ((x + width) > render_target->viewport.width)
    width = render_target->viewport.width - x;
  if ((y + height) > render_target->viewport.height)
    height = render_target->viewport.height - y;

  /* Add viewport offset.  */
  x += render_target->viewport.x;
  y += render_target->viewport.y;

  /* Use vbeblit_info to encapsulate rendering.  */
  target.mode_info = &render_target->mode_info;
  target.data = render_target->data;

  /* Draw glyph.  */
  for (j = 0; j < height; j++)
    for (i = 0; i < width; i++)
      if ((glyph->bitmap[((i + x_offset) / 8) 
                         + (j + y_offset) * (charwidth / 8)] 
           & (1 << ((charwidth - (i + x_offset) - 1) % 8))))
        set_pixel (&target, x+i, y+j, color);

  return GRUB_ERR_NONE;
}

/* NOTE: This function assumes that given coordinates are within bounds of 
   handled data.  */
static void
common_blitter (struct grub_video_i386_vbeblit_info *target,
                struct grub_video_i386_vbeblit_info *source,
                enum grub_video_blit_operators oper, int x, int y,
                unsigned int width, unsigned int height,
                int offset_x, int offset_y)
{
  if (oper == GRUB_VIDEO_BLIT_REPLACE)
    {
      /* Try to figure out more optimized version for replace operator.  */
      if (source->mode_info->blit_format == GRUB_VIDEO_BLIT_FORMAT_R8G8B8A8)
        {
          if (target->mode_info->blit_format == GRUB_VIDEO_BLIT_FORMAT_R8G8B8A8)
            {
              grub_video_i386_vbeblit_R8G8B8X8_R8G8B8X8 (target, source,
                                                         x, y, width, height,
                                                         offset_x, offset_y);
              return;
            }

          if (target->mode_info->blit_format == GRUB_VIDEO_BLIT_FORMAT_R8G8B8)
            {
              grub_video_i386_vbeblit_R8G8B8_R8G8B8X8 (target, source,
                                                       x, y, width, height,
                                                       offset_x, offset_y);
              return;
            }

          if (target->mode_info->blit_format == GRUB_VIDEO_BLIT_FORMAT_INDEXCOLOR)
            {
              grub_video_i386_vbeblit_index_R8G8B8X8 (target, source,
                                                      x, y, width, height,
                                                      offset_x, offset_y);
              return;
            }
        }

      if (source->mode_info->blit_format == GRUB_VIDEO_BLIT_FORMAT_R8G8B8)
        {
          if (target->mode_info->blit_format == GRUB_VIDEO_BLIT_FORMAT_R8G8B8A8)
            {
              grub_video_i386_vbeblit_R8G8B8A8_R8G8B8 (target, source,
                                                       x, y, width, height,
                                                       offset_x, offset_y);
              return;
            }

          if (target->mode_info->blit_format == GRUB_VIDEO_BLIT_FORMAT_R8G8B8)
            {
              grub_video_i386_vbeblit_R8G8B8_R8G8B8 (target, source,
                                                     x, y, width, height,
                                                     offset_x, offset_y);
              return;
            }

          if (target->mode_info->blit_format == GRUB_VIDEO_BLIT_FORMAT_INDEXCOLOR)
            {
              grub_video_i386_vbeblit_index_R8G8B8 (target, source,
                                                    x, y, width, height,
                                                    offset_x, offset_y);
              return;
            }
        }

      if (source->mode_info->blit_format == GRUB_VIDEO_BLIT_FORMAT_INDEXCOLOR)
        {
          if (target->mode_info->blit_format == GRUB_VIDEO_BLIT_FORMAT_INDEXCOLOR)
            {
              grub_video_i386_vbeblit_index_index (target, source,
                                                   x, y, width, height,
                                                   offset_x, offset_y);
              return;
            }
        }

      /* No optimized replace operator found, use default (slow) blitter.  */
      grub_video_i386_vbeblit_replace (target, source, x, y, width, height,
                                       offset_x, offset_y);
    }
  else
    {
      /* Try to figure out more optimized blend operator.  */
      if (source->mode_info->blit_format == GRUB_VIDEO_BLIT_FORMAT_R8G8B8A8)
      {
        if (target->mode_info->blit_format == GRUB_VIDEO_BLIT_FORMAT_R8G8B8A8)
        {
          grub_video_i386_vbeblit_R8G8B8A8_R8G8B8A8 (target, source,
              x, y, width, height,
              offset_x, offset_y);
          return;
        }

        if (target->mode_info->blit_format == GRUB_VIDEO_BLIT_FORMAT_R8G8B8)
        {
          grub_video_i386_vbeblit_R8G8B8_R8G8B8A8 (target, source,
              x, y, width, height,
              offset_x, offset_y);
          return;
        }

        if (target->mode_info->blit_format == GRUB_VIDEO_BLIT_FORMAT_INDEXCOLOR)
        {
          grub_video_i386_vbeblit_index_R8G8B8A8 (target, source,
              x, y, width, height,
              offset_x, offset_y);
          return;
        }
      }

      if (source->mode_info->blit_format == GRUB_VIDEO_BLIT_FORMAT_R8G8B8)
      {
        if (target->mode_info->blit_format == GRUB_VIDEO_BLIT_FORMAT_R8G8B8A8)
        {
          grub_video_i386_vbeblit_R8G8B8A8_R8G8B8 (target, source,
              x, y, width, height,
              offset_x, offset_y);
          return;
        }

        if (target->mode_info->blit_format == GRUB_VIDEO_BLIT_FORMAT_R8G8B8)
        {
          grub_video_i386_vbeblit_R8G8B8_R8G8B8 (target, source,
              x, y, width, height,
              offset_x, offset_y);
          return;
        }

        if (target->mode_info->blit_format == GRUB_VIDEO_BLIT_FORMAT_INDEXCOLOR)
        {
          grub_video_i386_vbeblit_index_R8G8B8 (target, source,
              x, y, width, height,
              offset_x, offset_y);
          return;
        }
      }

      if (source->mode_info->blit_format == GRUB_VIDEO_BLIT_FORMAT_INDEXCOLOR)
      {
        if (target->mode_info->blit_format == GRUB_VIDEO_BLIT_FORMAT_INDEXCOLOR)
        {
          grub_video_i386_vbeblit_index_index (target, source,
                                               x, y, width, height,
                                               offset_x, offset_y);
          return;
        }
      }

      /* No optimized blend operation found, use default (slow) blitter.  */
      grub_video_i386_vbeblit_blend (target, source, x, y, width, height,
                                     offset_x, offset_y);
    }
}

static grub_err_t
grub_video_vbe_blit_bitmap (struct grub_video_bitmap *bitmap,
                            enum grub_video_blit_operators oper, int x, int y,
                            int offset_x, int offset_y,
                            unsigned int width, unsigned int height)
{
  struct grub_video_i386_vbeblit_info source;
  struct grub_video_i386_vbeblit_info target;

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

  /* Use vbeblit_info to encapsulate rendering.  */
  source.mode_info = &bitmap->mode_info;
  source.data = bitmap->data;
  target.mode_info = &render_target->mode_info;
  target.data = render_target->data;

  /* Do actual blitting.  */
  common_blitter (&target, &source, oper, x, y, width, height,
                  offset_x, offset_y);

  return GRUB_ERR_NONE;
}

static grub_err_t
grub_video_vbe_blit_render_target (struct grub_video_render_target *source,
                                   enum grub_video_blit_operators oper,
                                   int x, int y, int offset_x, int offset_y,
                                   unsigned int width, unsigned int height)
{
  struct grub_video_i386_vbeblit_info source_info;
  struct grub_video_i386_vbeblit_info target_info;

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

  /* Use vbeblit_info to encapsulate rendering.  */
  source_info.mode_info = &source->mode_info;
  source_info.data = source->data;
  target_info.mode_info = &render_target->mode_info;
  target_info.data = render_target->data;

  /* Do actual blitting.  */
  common_blitter (&target_info, &source_info, oper, x, y, width, height, 
                  offset_x, offset_y);

  return GRUB_ERR_NONE;
}

static grub_err_t
grub_video_vbe_scroll (grub_video_color_t color, int dx, int dy)
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
      struct grub_video_i386_vbeblit_info target;
      grub_uint8_t *src;
      grub_uint8_t *dst;
      int j;

      target.mode_info = &render_target->mode_info;
      target.data = render_target->data;

      for (j = 0; j < height; j++)
        {
          dst = grub_video_vbe_get_video_ptr (&target, dst_x, dst_y + j);
          src = grub_video_vbe_get_video_ptr (&target, src_x, src_y + j);
          grub_memmove (dst, src,
                        width * target.mode_info->bytes_per_pixel);
        }
    }

  /* 4. Fill empty space with specified color.  In this implementation
     there might be colliding areas but at the moment there is no need
     to optimize this.  */

  /* 4a. Fill top & bottom parts.  */
  if (dy > 0)
    grub_video_vbe_fill_rect (color, 0, 0, render_target->viewport.width, dy);
  else if (dy < 0)
    {
      if (render_target->viewport.height < grub_abs (dy))
        dy = -render_target->viewport.height;

      grub_video_vbe_fill_rect (color, 0, render_target->viewport.height + dy,
                                render_target->viewport.width, -dy);
    }

  /* 4b. Fill left & right parts.  */
  if (dx > 0)
    grub_video_vbe_fill_rect (color, 0, 0, 
                              dx, render_target->viewport.height);
  else if (dx < 0)
    {
      if (render_target->viewport.width < grub_abs (dx))
        dx = -render_target->viewport.width;

      grub_video_vbe_fill_rect (color, render_target->viewport.width + dx, 0,
                                -dx, render_target->viewport.height);
    }

  return GRUB_ERR_NONE;
}

static grub_err_t 
grub_video_vbe_swap_buffers (void)
{
  /* TODO: Implement buffer swapping.  */
  return GRUB_ERR_NONE;
}

static grub_err_t
grub_video_vbe_create_render_target (struct grub_video_render_target **result,
                                     unsigned int width, unsigned int height,
                                     unsigned int mode_type __attribute__ ((unused)))
{
  struct grub_video_render_target *target;
  unsigned int size;

  /* Validate arguments.  */
  if ((! result)
      || (width == 0)
      || (height == 0))
    return grub_error (GRUB_ERR_BAD_ARGUMENT,
                       "invalid argument given.");

  /* Allocate memory for render target.  */
  target = grub_malloc (sizeof (struct grub_video_render_target));
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
  target->mode_info.number_of_colors = 256; /* Emulated palette.  */
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

static grub_err_t
grub_video_vbe_delete_render_target (struct grub_video_render_target *target)
{
  /* If there is no target, then just return without error.  */
  if (! target)
    return GRUB_ERR_NONE;

  /* TODO: Delist render target fron render target list.  */

  /* If this is software render target, free it's memory.  */
  if (target->is_allocated)
    grub_free (target->data);

  /* Free render target.  */
  grub_free (target);

  return GRUB_ERR_NONE;
}

static grub_err_t
grub_video_vbe_set_active_render_target (struct grub_video_render_target *target)
{
  if (target == GRUB_VIDEO_RENDER_TARGET_FRONT_BUFFER)
    {
      render_target = &framebuffer.render_target;

      return GRUB_ERR_NONE;
    }

  if (target == GRUB_VIDEO_RENDER_TARGET_BACK_BUFFER)
    return grub_error (GRUB_ERR_NOT_IMPLEMENTED_YET, 
                       "double buffering not implemented yet.");

  if (! target->data)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, 
                       "invalid render target given.");

  render_target = target;

  return GRUB_ERR_NONE;
}

static grub_err_t
grub_video_vbe_get_active_render_target (struct grub_video_render_target **target)
{
  *target = render_target;
  
  return GRUB_ERR_NONE;
}

static struct grub_video_adapter grub_video_vbe_adapter =
  {
    .name = "VESA BIOS Extension Video Driver",

    .init = grub_video_vbe_init,
    .fini = grub_video_vbe_fini,
    .setup = grub_video_vbe_setup,
    .get_info = grub_video_vbe_get_info,
    .set_palette = grub_video_vbe_set_palette,
    .get_palette = grub_video_vbe_get_palette,
    .set_viewport = grub_video_vbe_set_viewport,
    .get_viewport = grub_video_vbe_get_viewport,
    .map_color = grub_video_vbe_map_color,
    .map_rgb = grub_video_vbe_map_rgb,
    .map_rgba = grub_video_vbe_map_rgba,
    .unmap_color = grub_video_vbe_unmap_color,
    .fill_rect = grub_video_vbe_fill_rect,
    .blit_glyph = grub_video_vbe_blit_glyph,
    .blit_bitmap = grub_video_vbe_blit_bitmap,
    .blit_render_target = grub_video_vbe_blit_render_target,
    .scroll = grub_video_vbe_scroll,
    .swap_buffers = grub_video_vbe_swap_buffers,
    .create_render_target = grub_video_vbe_create_render_target,
    .delete_render_target = grub_video_vbe_delete_render_target,
    .set_active_render_target = grub_video_vbe_set_active_render_target,
    .get_active_render_target = grub_video_vbe_get_active_render_target,

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
