/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2005  Free Software Foundation, Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <grub/err.h>
#include <grub/machine/memory.h>
#include <grub/machine/vga.h>
#include <grub/machine/vbe.h>
#include <grub/machine/console.h>
#include <grub/term.h>
#include <grub/types.h>
#include <grub/dl.h>
#include <grub/misc.h>
#include <grub/normal.h>
#include <grub/font.h>
#include <grub/arg.h>
#include <grub/mm.h>
#include <grub/env.h>

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
static int index_color_mode = 0;

static struct grub_vbe_info_block controller_info;
static struct grub_vbe_mode_info_block active_mode_info;

static grub_uint32_t active_mode = 0;

static grub_uint8_t *framebuffer = 0;
static grub_uint32_t bytes_per_scan_line = 0;

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
      status = grub_vbe_get_controller_info (vbe_ib);
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
  if (grub_vbe_probe (0) != GRUB_ERR_NONE)
    return grub_errno;

  /* Try to get mode info.  */
  if (grub_vbe_get_video_mode_info (mode, &active_mode_info) != GRUB_ERR_NONE)
    return grub_errno;

  /* For all VESA BIOS modes, force linear frame buffer.  */
  if (mode >= 0x100)
    {
      /* We only want linear frame buffer modes.  */
      mode |= 1 << 14;

      /* Determine frame buffer pixel format.  */
      switch (active_mode_info.memory_model)
	{
	case 0x04:
	  index_color_mode = 1;
	  break;
	  
	case 0x06:
	  index_color_mode = 0;
	  break;
	  
	default:
	  return grub_error (GRUB_ERR_NOT_IMPLEMENTED_YET,
			     "unsupported pixel format 0x%x",
			     active_mode_info.memory_model);
	}
    }

  /* Get current mode.  */
  if (grub_vbe_get_video_mode (&old_mode) != GRUB_ERR_NONE)
    return grub_errno;
  
  /* Try to set video mode.  */
  status = grub_vbe_set_mode (mode, 0);
  if (status != 0x004F)
    return grub_error (GRUB_ERR_BAD_DEVICE, "cannot set VBE mode %x", mode);

  /* Save information for later usage.  */
  active_mode = mode;

  if (mode < 0x100)
    {
      /* If this is not a VESA mode, guess address.  */
      framebuffer = (grub_uint8_t *) 0xA0000;
      index_color_mode = 1;
    }
  else
    {
      framebuffer = (grub_uint8_t *) active_mode_info.phys_base_addr;

      if (controller_info.version >= 0x300)
	bytes_per_scan_line = active_mode_info.lin_bytes_per_scan_line;
      else
	bytes_per_scan_line = active_mode_info.bytes_per_scan_line;
    }

  /* If video mode is in indexed color, setup default VGA palette.  */
  if (index_color_mode)
    {
      struct grub_vbe_palette_data *palette
	= (struct grub_vbe_palette_data *) GRUB_MEMORY_MACHINE_SCRATCH_ADDR;

      /* Make sure that the BIOS can reach the palette.  */
      grub_memcpy (palette, vga_colors, sizeof (vga_colors));
      status = grub_vbe_set_palette_data (sizeof (vga_colors) 
                                          / sizeof (struct grub_vbe_palette_data), 
                                          0, 
                                          palette);

      /* For now, ignore the status. Not sure if this is fatal.  */
#if 0
      if (status != 0x004F)
	{
	  grub_vbe_set_mode (old_mode, 0);
	  return grub_error (GRUB_ERR_BAD_DEVICE,
			     "cannot set the default VGA palette");
	}
#endif
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
  if (grub_vbe_probe (0) != GRUB_ERR_NONE)
    return grub_errno;

  /* Try to query current mode from VESA BIOS.  */
  status = grub_vbe_get_mode (mode);
  if (status != 0x004F)
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
  if (grub_vbe_probe (0) != GRUB_ERR_NONE)
    return grub_errno;

  /* If mode is not VESA mode, skip mode info query.  */
  if (mode >= 0x100)
    {
      /* Try to get mode info from VESA BIOS.  */
      status = grub_vbe_get_mode_info (mode, mi_tmp);
      if (status != 0x004F)
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

void
grub_vbe_set_pixel_rgb (grub_uint32_t x,
			grub_uint32_t y,
                        grub_uint8_t red,
                        grub_uint8_t green,
                        grub_uint8_t blue)
{
  grub_uint32_t value;

  if (x >= active_mode_info.x_resolution)
    return;

  if (y >= active_mode_info.y_resolution)
    return;

  if (controller_info.version >= 0x300)
    {
      red >>= 8 - active_mode_info.lin_red_mask_size;
      green >>= 8 - active_mode_info.lin_green_mask_size;
      blue >>= 8 - active_mode_info.lin_blue_mask_size;

      value = red << active_mode_info.lin_red_field_position;
      value |= green << active_mode_info.lin_green_field_position;
      value |= blue << active_mode_info.lin_blue_field_position;
    }
  else
    {
      red >>= 8 - active_mode_info.red_mask_size;
      green >>= 8 - active_mode_info.green_mask_size;
      blue >>= 8 - active_mode_info.blue_mask_size;

      value = red << active_mode_info.red_field_position;
      value |= green << active_mode_info.green_field_position;
      value |= blue << active_mode_info.blue_field_position;
    }

  switch (active_mode_info.bits_per_pixel)
    {
    case 32:
      {
	grub_uint32_t *ptr = (grub_uint32_t *) (framebuffer
						+ y * bytes_per_scan_line
						+ x * 4);
	
	*ptr = value;
      }
      break;
      
    case 24:
      {
	grub_uint8_t *ptr = (grub_uint8_t *) (framebuffer
					      + y * bytes_per_scan_line
					      + x * 3);
	grub_uint8_t *ptr2 = (grub_uint8_t *) &value;
	
	ptr[0] = ptr2[0];
	ptr[1] = ptr2[1];
	ptr[2] = ptr2[2];
      }
      break;

    case 16:
    case 15:
      {
	grub_uint16_t *ptr = (grub_uint16_t *) (framebuffer
						+ y * bytes_per_scan_line
						+ x * 2);
	
	*ptr = (grub_uint16_t) (value & 0xFFFF);
      }
      break;

    default:
      break;
    }
}

void
grub_vbe_set_pixel_index (grub_uint32_t x,
                          grub_uint32_t y,
                          grub_uint8_t color)
{
  if (x >= active_mode_info.x_resolution)
    return;

  if (y >= active_mode_info.y_resolution)
    return;

  if (index_color_mode == 1)
    {
      grub_uint8_t *ptr = (grub_uint8_t *) (framebuffer
					    + y * bytes_per_scan_line
					    + x);

      *ptr = color;
    }
  else
    {
      color &= 0x0F;
      
      if (color < 16)
	{
	  grub_vbe_set_pixel_rgb (x,
                                  y,
                                  vga_colors[color].red,
                                  vga_colors[color].green,
                                  vga_colors[color].blue);
	}
      else
	{
	  grub_vbe_set_pixel_rgb (x,
                                  y,
                                  0,
                                  0,
                                  0);
	}
    }
}
