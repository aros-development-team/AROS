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
#include <grub/video_fb.h>
#include <grub/types.h>
#include <grub/dl.h>
#include <grub/misc.h>
#include <grub/mm.h>
#include <grub/video.h>
#include <grub/machine/int.h>

static int vbe_detected = -1;

static struct grub_vbe_info_block controller_info;

/* Track last mode to support cards which fail on get_mode.  */
static grub_uint32_t last_set_mode = 3;

static struct
{
  struct grub_video_mode_info mode_info;

  grub_uint8_t *ptr;
} framebuffer;

static grub_uint32_t initial_vbe_mode;
static grub_uint16_t *vbe_mode_list;

static void *
real2pm (grub_vbe_farptr_t ptr)
{
  return (void *) ((((unsigned long) ptr & 0xFFFF0000) >> 12UL)
                   + ((unsigned long) ptr & 0x0000FFFF));
}

/* Call VESA BIOS 0x4f09 to set palette data, return status.  */
static grub_vbe_status_t 
grub_vbe_bios_set_palette_data (grub_uint32_t color_count,
				grub_uint32_t start_index,
				struct grub_vbe_palette_data *palette_data)
{
  struct grub_bios_int_registers regs;
  regs.eax = 0x4f09;
  regs.ebx = 0;
  regs.ecx = color_count;
  regs.edx = start_index;
  regs.es = (((grub_addr_t) palette_data) & 0xffff0000) >> 4;
  regs.edi = ((grub_addr_t) palette_data) & 0xffff;
  regs.flags = GRUB_CPU_INT_FLAGS_DEFAULT;
  grub_bios_interrupt (0x10, &regs);
  return regs.eax & 0xffff;
}

/* Call VESA BIOS 0x4f00 to get VBE Controller Information, return status.  */
grub_vbe_status_t 
grub_vbe_bios_get_controller_info (struct grub_vbe_info_block *ci)
{
  struct grub_bios_int_registers regs;
  /* Store *controller_info to %es:%di.  */
  regs.es = (((grub_addr_t) ci) & 0xffff0000) >> 4;
  regs.edi = ((grub_addr_t) ci) & 0xffff;
  regs.eax = 0x4f00;
  regs.flags = GRUB_CPU_INT_FLAGS_DEFAULT;
  grub_bios_interrupt (0x10, &regs);
  return regs.eax & 0xffff;
}

/* Call VESA BIOS 0x4f01 to get VBE Mode Information, return status.  */
grub_vbe_status_t 
grub_vbe_bios_get_mode_info (grub_uint32_t mode,
			     struct grub_vbe_mode_info_block *mode_info)
{
  struct grub_bios_int_registers regs;
  regs.eax = 0x4f01;
  regs.ecx = mode;
  /* Store *mode_info to %es:%di.  */
  regs.es = ((grub_addr_t) mode_info & 0xffff0000) >> 4;
  regs.edi = (grub_addr_t) mode_info & 0x0000ffff;
  regs.flags = GRUB_CPU_INT_FLAGS_DEFAULT;
  grub_bios_interrupt (0x10, &regs);
  return regs.eax & 0xffff;
}

/* Call VESA BIOS 0x4f02 to set video mode, return status.  */
static grub_vbe_status_t
grub_vbe_bios_set_mode (grub_uint32_t mode,
			struct grub_vbe_crtc_info_block *crtc_info)
{
  struct grub_bios_int_registers regs;

  regs.eax = 0x4f02;
  regs.ebx = mode;
  /* Store *crtc_info to %es:%di.  */
  regs.es = (((grub_addr_t) crtc_info) & 0xffff0000) >> 4;
  regs.edi = ((grub_addr_t) crtc_info) & 0xffff;
  regs.flags = GRUB_CPU_INT_FLAGS_DEFAULT;
  grub_bios_interrupt (0x10, &regs);

  return regs.eax & 0xffff;
}

/* Call VESA BIOS 0x4f03 to return current VBE Mode, return status.  */
grub_vbe_status_t 
grub_vbe_bios_get_mode (grub_uint32_t *mode)
{
  struct grub_bios_int_registers regs;

  regs.eax = 0x4f03;
  regs.flags = GRUB_CPU_INT_FLAGS_DEFAULT;
  grub_bios_interrupt (0x10, &regs);
  *mode = regs.ebx & 0xffff;

  return regs.eax & 0xffff;
}

grub_vbe_status_t 
grub_vbe_bios_getset_dac_palette_width (int set, int *dac_mask_size)
{
  struct grub_bios_int_registers regs;

  regs.eax = 0x4f08;
  regs.ebx = (*dac_mask_size & 0xff) >> 8;
  regs.ebx = set ? 1 : 0;
  regs.flags = GRUB_CPU_INT_FLAGS_DEFAULT;
  grub_bios_interrupt (0x10, &regs);
  *dac_mask_size = (regs.ebx >> 8) & 0xff;

  return regs.eax & 0xffff;
}

/* Call VESA BIOS 0x4f05 to set memory window, return status.  */
grub_vbe_status_t
grub_vbe_bios_set_memory_window (grub_uint32_t window,
				 grub_uint32_t position)
{
  struct grub_bios_int_registers regs;

  /* BL = window, BH = 0, Set memory window.  */
  regs.ebx = window & 0x00ff;
  regs.edx = position;
  regs.eax = 0x4f05;
  regs.flags = GRUB_CPU_INT_FLAGS_DEFAULT;
  grub_bios_interrupt (0x10, &regs);
  return regs.eax & 0xffff;
}

/* Call VESA BIOS 0x4f05 to return memory window, return status.  */
grub_vbe_status_t
grub_vbe_bios_get_memory_window (grub_uint32_t window,
				 grub_uint32_t *position)
{
  struct grub_bios_int_registers regs;

  regs.eax = 0x4f05;
  /* BH = 1, Get memory window. BL = window.  */
  regs.ebx = (window & 0x00ff) | 0x100;
  regs.flags = GRUB_CPU_INT_FLAGS_DEFAULT;
  grub_bios_interrupt (0x10, &regs);

  *position = regs.edx & 0xffff;
  return regs.eax & 0xffff;
}

/* Call VESA BIOS 0x4f06 to set scanline length (in bytes), return status.  */
grub_vbe_status_t 
grub_vbe_bios_set_scanline_length (grub_uint32_t length)
{
  struct grub_bios_int_registers regs;

  regs.ecx = length;
  regs.eax = 0x4f06;
  /* BL = 2, Set Scan Line in Bytes.  */
  regs.ebx = 0x0002;	
  regs.flags = GRUB_CPU_INT_FLAGS_DEFAULT;
  grub_bios_interrupt (0x10, &regs);
  return regs.eax & 0xffff;
}

/* Call VESA BIOS 0x4f06 to return scanline length (in bytes), return status.  */
grub_vbe_status_t 
grub_vbe_bios_get_scanline_length (grub_uint32_t *length)
{
  struct grub_bios_int_registers regs;

  regs.eax = 0x4f06;
  regs.ebx = 0x0001;
  regs.flags = GRUB_CPU_INT_FLAGS_DEFAULT;
  /* BL = 1, Get Scan Line Length (in bytes).  */
  grub_bios_interrupt (0x10, &regs);

  *length = regs.ebx & 0xffff;
  return regs.eax & 0xffff;
}

/* Call VESA BIOS 0x4f07 to set display start, return status.  */
static grub_vbe_status_t 
grub_vbe_bios_set_display_start (grub_uint32_t x, grub_uint32_t y)
{
  struct grub_bios_int_registers regs;

  /* Store x in %ecx.  */
  regs.ecx = x;
  regs.edx = y;
  regs.eax = 0x4f07;
  /* BL = 80h, Set Display Start during Vertical Retrace.  */
  regs.ebx = 0x0080;	
  regs.flags = GRUB_CPU_INT_FLAGS_DEFAULT;
  grub_bios_interrupt (0x10, &regs);
  return regs.eax & 0xffff;
}

/* Call VESA BIOS 0x4f07 to get display start, return status.  */
grub_vbe_status_t 
grub_vbe_bios_get_display_start (grub_uint32_t *x,
				 grub_uint32_t *y)
{
  struct grub_bios_int_registers regs;

  regs.eax = 0x4f07;
  /* BL = 1, Get Display Start.  */
  regs.ebx = 0x0001;
  regs.flags = GRUB_CPU_INT_FLAGS_DEFAULT;
  grub_bios_interrupt (0x10, &regs);

  *x = regs.ecx & 0xffff;
  *y = regs.edx & 0xffff;
  return regs.eax & 0xffff;
}

/* Call VESA BIOS 0x4f0a.  */
grub_vbe_status_t 
grub_vbe_bios_get_pm_interface (grub_uint16_t *segment, grub_uint16_t *offset,
				grub_uint16_t *length)
{
  struct grub_bios_int_registers regs;

  regs.eax = 0x4f0a;
  regs.ebx = 0x0000;
  regs.flags = GRUB_CPU_INT_FLAGS_DEFAULT;
  grub_bios_interrupt (0x10, &regs);

  if ((regs.eax & 0xffff) != GRUB_VBE_STATUS_OK)
    {
      *segment = 0;
      *offset = 0;
      *length = 0;
    }

  *segment = regs.es & 0xffff;
  *offset = regs.edi & 0xffff;
  *length = regs.ecx & 0xffff;
  return regs.eax & 0xffff;
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
      if (new_vbe_mode_info.memory_model != GRUB_VBE_MEMORY_MODEL_PACKED_PIXEL
	  && new_vbe_mode_info.memory_model
	  != GRUB_VBE_MEMORY_MODEL_DIRECT_COLOR)
	return grub_error (GRUB_ERR_NOT_IMPLEMENTED_YET,
			   "unsupported pixel format 0x%x",
			   new_vbe_mode_info.memory_model);
    }

  /* Get current mode.  */
  grub_vbe_get_video_mode (&old_vbe_mode);
  if (grub_errno != GRUB_ERR_NONE)
    return grub_errno;

  /* Try to set video mode.  */
  status = grub_vbe_bios_set_mode (vbe_mode, 0);
  if (status != GRUB_VBE_STATUS_OK)
    return grub_error (GRUB_ERR_BAD_DEVICE, "cannot set VBE mode %x", vbe_mode);
  last_set_mode = vbe_mode;

  if (vbe_mode < 0x100)
    {
      /* If this is not a VESA mode, guess address.  */
      framebuffer.ptr = (grub_uint8_t *) GRUB_MEMORY_MACHINE_VGA_ADDR;
    }
  else
    {
      framebuffer.ptr = (grub_uint8_t *) new_vbe_mode_info.phys_base_addr;
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

      /* If video mode is in indexed color, setup default VGA palette.  */
      if (vbe_mode < 0x100 || new_vbe_mode_info.memory_model
	  == GRUB_VBE_MEMORY_MODEL_PACKED_PIXEL)
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
  /* XXX: ATI cards don't support get_mode.  */
  if (status != GRUB_VBE_STATUS_OK)
    *mode = last_set_mode;

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
  grub_err_t err;

  /* Restore old video mode.  */
  if (last_set_mode != initial_vbe_mode)
    {
      status = grub_vbe_bios_set_mode (initial_vbe_mode, 0);
      if (status != GRUB_VBE_STATUS_OK)
	/* TODO: Decide, is this something we want to do.  */
	return grub_errno;
    }
  last_set_mode = initial_vbe_mode;

  /* TODO: Free any resources allocated by driver.  */
  grub_free (vbe_mode_list);
  vbe_mode_list = NULL;

  err = grub_video_fb_fini ();
  return err;
}

/*
  Set framebuffer render target page and display the proper page, based on
  `doublebuf_state.render_page' and `doublebuf_state.displayed_page',
  respectively.
*/
static grub_err_t
doublebuf_pageflipping_set_page (int page)
{
  /* Tell the video adapter to display the new front page.  */
  int display_start_line
    = framebuffer.mode_info.height * page;

  grub_vbe_status_t vbe_err =
    grub_vbe_bios_set_display_start (0, display_start_line);

  if (vbe_err != GRUB_VBE_STATUS_OK)
    return grub_error (GRUB_ERR_IO, "couldn't commit pageflip");

  return 0;
}

static void
vbe2videoinfo (grub_uint32_t mode,
	       const struct grub_vbe_mode_info_block *vbeinfo,
	       struct grub_video_mode_info *mode_info)
{
  mode_info->mode_number = mode;

  mode_info->width = vbeinfo->x_resolution;
  mode_info->height = vbeinfo->y_resolution;
  mode_info->mode_type = 0;
  switch (vbeinfo->memory_model)
    {
    case GRUB_VBE_MEMORY_MODEL_TEXT:
      mode_info->mode_type |= GRUB_VIDEO_MODE_TYPE_PURE_TEXT;
      break;

      /* CGA is basically 4-bit packed pixel.  */
    case GRUB_VBE_MEMORY_MODEL_CGA:
      mode_info->mode_type |= GRUB_VIDEO_MODE_TYPE_CGA;
    case GRUB_VBE_MEMORY_MODEL_PACKED_PIXEL:
      mode_info->mode_type |= GRUB_VIDEO_MODE_TYPE_INDEX_COLOR;
      break;

    case GRUB_VBE_MEMORY_MODEL_HERCULES:
      mode_info->mode_type |= GRUB_VIDEO_MODE_TYPE_HERCULES
	| GRUB_VIDEO_MODE_TYPE_1BIT_BITMAP;
      break;

      /* Non chain 4 is a special case of planar.  */
    case GRUB_VBE_MEMORY_MODEL_NONCHAIN4_256:
      mode_info->mode_type |= GRUB_VIDEO_MODE_TYPE_NONCHAIN4;
    case GRUB_VBE_MEMORY_MODEL_PLANAR:
      mode_info->mode_type |= GRUB_VIDEO_MODE_TYPE_PLANAR
	| GRUB_VIDEO_MODE_TYPE_INDEX_COLOR;
      break;

    case GRUB_VBE_MEMORY_MODEL_YUV:
      mode_info->mode_type |= GRUB_VIDEO_MODE_TYPE_YUV;
      break;
      
    case GRUB_VBE_MEMORY_MODEL_DIRECT_COLOR:
      mode_info->mode_type |= GRUB_VIDEO_MODE_TYPE_RGB;
      break;
    default:
      mode_info->mode_type |= GRUB_VIDEO_MODE_TYPE_UNKNOWN;
      break;
    }

  mode_info->bpp = vbeinfo->bits_per_pixel;
  /* Calculate bytes_per_pixel value.  */
  switch(vbeinfo->bits_per_pixel)
    {
    case 32:
      mode_info->bytes_per_pixel = 4;
      break;
    case 24:
      mode_info->bytes_per_pixel = 3;
      break;
    case 16:
      mode_info->bytes_per_pixel = 2;
      break;
    case 15:
      mode_info->bytes_per_pixel = 2;
      break;
    case 8:
      mode_info->bytes_per_pixel = 1;
      break;  
    case 4:
      mode_info->bytes_per_pixel = 0;
      break;  
    }

  if (controller_info.version >= 0x300)
    mode_info->pitch = vbeinfo->lin_bytes_per_scan_line;
  else
    mode_info->pitch = vbeinfo->bytes_per_scan_line;

  mode_info->number_of_colors = 256; /* TODO: fix me.  */
  mode_info->red_mask_size = vbeinfo->red_mask_size;
  mode_info->red_field_pos = vbeinfo->red_field_position;
  mode_info->green_mask_size = vbeinfo->green_mask_size;
  mode_info->green_field_pos = vbeinfo->green_field_position;
  mode_info->blue_mask_size = vbeinfo->blue_mask_size;
  mode_info->blue_field_pos = vbeinfo->blue_field_position;
  mode_info->reserved_mask_size = vbeinfo->rsvd_mask_size;
  mode_info->reserved_field_pos = vbeinfo->rsvd_field_position;

  mode_info->blit_format = grub_video_get_blit_format (mode_info);
}

static int
grub_video_vbe_iterate (int (*hook) (const struct grub_video_mode_info *info))
{
  grub_uint16_t *p;
  struct grub_vbe_mode_info_block vbe_mode_info;
  struct grub_video_mode_info mode_info;

  for (p = vbe_mode_list; *p != 0xFFFF; p++)
    {
      grub_vbe_get_video_mode_info (*p, &vbe_mode_info);
      if (grub_errno != GRUB_ERR_NONE)
        {
          /* Could not retrieve mode info, retreat.  */
          grub_errno = GRUB_ERR_NONE;
          break;
        }

      vbe2videoinfo (*p, &vbe_mode_info, &mode_info);
      if (hook (&mode_info))
	return 1;
    }
  return 0;
}

static grub_err_t
grub_video_vbe_setup (unsigned int width, unsigned int height,
                      unsigned int mode_type, unsigned int mode_mask)
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

      if (vbe_mode_info.bits_per_pixel != 8
	  && vbe_mode_info.bits_per_pixel != 15
	  && vbe_mode_info.bits_per_pixel != 16
	  && vbe_mode_info.bits_per_pixel != 24
	  && vbe_mode_info.bits_per_pixel != 32)
	/* Unsupported bitdepth . */
        continue;

      if (((vbe_mode_info.x_resolution != width)
	   || (vbe_mode_info.y_resolution != height)) && width != 0 && height != 0)
        /* Non matching resolution.  */
        continue;

      /* Check if user requested RGB or index color mode.  */
      if ((mode_mask & GRUB_VIDEO_MODE_TYPE_COLOR_MASK) != 0)
        {
	  unsigned my_mode_type = 0;

	  if (vbe_mode_info.memory_model == GRUB_VBE_MEMORY_MODEL_PACKED_PIXEL)
	    my_mode_type |= GRUB_VIDEO_MODE_TYPE_INDEX_COLOR;

	  if (vbe_mode_info.memory_model == GRUB_VBE_MEMORY_MODEL_DIRECT_COLOR)
	    my_mode_type |= GRUB_VIDEO_MODE_TYPE_RGB;

	  if ((my_mode_type & mode_mask
	       & (GRUB_VIDEO_MODE_TYPE_RGB | GRUB_VIDEO_MODE_TYPE_INDEX_COLOR))
	      != (mode_type & mode_mask
		  & (GRUB_VIDEO_MODE_TYPE_RGB
		     | GRUB_VIDEO_MODE_TYPE_INDEX_COLOR)))
	    continue;
        }

      /* If there is a request for specific depth, ignore others.  */
      if ((depth != 0) && (vbe_mode_info.bits_per_pixel != depth))
        continue;

      /* Select mode with most of "volume" (size of framebuffer in bits).  */
      if (best_vbe_mode != 0)
        if ((grub_uint64_t) vbe_mode_info.bits_per_pixel
	    * vbe_mode_info.x_resolution * vbe_mode_info.y_resolution
	    < (grub_uint64_t) best_vbe_mode_info.bits_per_pixel
	    * best_vbe_mode_info.x_resolution * best_vbe_mode_info.y_resolution)
          continue;

      /* Save so far best mode information for later use.  */
      best_vbe_mode = vbe_mode;
      grub_memcpy (&best_vbe_mode_info, &vbe_mode_info, sizeof (vbe_mode_info));
    }

  /* Try to initialize best mode found.  */
  if (best_vbe_mode != 0)
    {
      grub_err_t err;
      static struct grub_vbe_mode_info_block active_vbe_mode_info;
      /* If this fails, then we have mode selection heuristics problem,
         or adapter failure.  */
      grub_vbe_set_video_mode (best_vbe_mode, &active_vbe_mode_info);
      if (grub_errno != GRUB_ERR_NONE)
        return grub_errno;

      /* Fill mode info details.  */
      vbe2videoinfo (best_vbe_mode, &active_vbe_mode_info,
		     &framebuffer.mode_info);

      {
	/* Get video RAM size in bytes.  */
	grub_size_t vram_size = controller_info.total_memory << 16;
	grub_size_t page_size;        /* The size of a page in bytes.  */

	page_size = framebuffer.mode_info.pitch * framebuffer.mode_info.height;

	if (vram_size >= 2 * page_size)
	  err = grub_video_fb_setup (mode_type, mode_mask,
				     &framebuffer.mode_info,
				     framebuffer.ptr,
				     doublebuf_pageflipping_set_page,
				     framebuffer.ptr + page_size);
	else
	  err = grub_video_fb_setup (mode_type, mode_mask,
				     &framebuffer.mode_info,
				     framebuffer.ptr, 0, 0);
      }

      /* Copy default palette to initialize emulated palette.  */
      err = grub_video_fb_set_palette (0, GRUB_VIDEO_FBSTD_NUMCOLORS,
				       grub_video_fbstd_colors);
      return err;
    }

  /* Couldn't found matching mode.  */
  return grub_error (GRUB_ERR_UNKNOWN_DEVICE, "no matching mode found");
}

static grub_err_t
grub_video_vbe_set_palette (unsigned int start, unsigned int count,
                            struct grub_video_palette_data *palette_data)
{
  if (framebuffer.mode_info.mode_type == GRUB_VIDEO_MODE_TYPE_INDEX_COLOR)
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
grub_video_vbe_get_info_and_fini (struct grub_video_mode_info *mode_info,
				  void **framebuf)
{
  grub_free (vbe_mode_list);
  vbe_mode_list = NULL;
  return grub_video_fb_get_info_and_fini (mode_info, framebuf);
}

static void
grub_video_vbe_print_adapter_specific_info (void)
{
  grub_printf ("  VBE info:   version: %d.%d  OEM software rev: %d.%d\n",
	       controller_info.version >> 8,
               controller_info.version & 0xFF,
               controller_info.oem_software_rev >> 8,
               controller_info.oem_software_rev & 0xFF);

  /* The total_memory field is in 64 KiB units.  */
  grub_printf ("              total memory: %d KiB\n",
               (controller_info.total_memory << 16) / 1024);
}

static struct grub_video_adapter grub_video_vbe_adapter =
  {
    .name = "VESA BIOS Extension Video Driver",
    .id = GRUB_VIDEO_DRIVER_VBE,

    .prio = GRUB_VIDEO_ADAPTER_PRIO_FIRMWARE,

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
    .swap_buffers = grub_video_fb_swap_buffers,
    .create_render_target = grub_video_fb_create_render_target,
    .delete_render_target = grub_video_fb_delete_render_target,
    .set_active_render_target = grub_video_fb_set_active_render_target,
    .get_active_render_target = grub_video_fb_get_active_render_target,
    .iterate = grub_video_vbe_iterate,
    .print_adapter_specific_info = grub_video_vbe_print_adapter_specific_info,

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
