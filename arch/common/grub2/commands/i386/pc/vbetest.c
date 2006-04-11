/* vbetest.c - command to test VESA BIOS Extension 2.0+ support.  */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2005  Free Software Foundation, Inc.
 *
 *  GRUB is free software; you can redistribute it and/or modify
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
 *  along with GRUB; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <grub/normal.h>
#include <grub/dl.h>
#include <grub/arg.h>
#include <grub/env.h>
#include <grub/misc.h>
#include <grub/term.h>
#include <grub/machine/init.h>
#include <grub/machine/vbe.h>
#include <grub/err.h>

static grub_err_t
grub_cmd_vbetest (struct grub_arg_list *state __attribute__ ((unused)),
		  int argc __attribute__ ((unused)),
		  char **args __attribute__ ((unused)))
{
  grub_err_t err;
  char *modevar;
  struct grub_vbe_mode_info_block mode_info;
  struct grub_vbe_info_block controller_info;
  grub_uint32_t use_mode = GRUB_VBE_DEFAULT_VIDEO_MODE;
  grub_uint32_t old_mode;
  grub_uint8_t *framebuffer = 0;
  grub_uint32_t bytes_per_scan_line = 0;
  unsigned char *ptr;
  int i;

  grub_printf ("Probing for VESA BIOS Extension ... ");

  /* Check if VESA BIOS exists.  */
  err = grub_vbe_probe (&controller_info);
  if (err != GRUB_ERR_NONE)
    return err;

  grub_printf ("found!\n");

  /* Dump out controller information.  */
  grub_printf ("VBE signature = %c%c%c%c\n",
	       controller_info.signature[0],
	       controller_info.signature[1],
	       controller_info.signature[2],
	       controller_info.signature[3]);

  grub_printf ("VBE version = %d.%d\n",
	       controller_info.version >> 8,
	       controller_info.version & 0xFF);
  grub_printf ("OEM string ptr = %08x\n",
	       controller_info.oem_string_ptr);
  grub_printf ("Total memory = %d\n",
	       controller_info.total_memory);

  err = grub_vbe_get_video_mode (&old_mode);
  grub_printf ("Get video mode err = %04x\n", err);

  if (err == GRUB_ERR_NONE)
    grub_printf ("Old video mode = %04x\n", old_mode);
  else
    grub_errno = GRUB_ERR_NONE;
  
  /* Check existence of vbe_mode environment variable.  */
  modevar = grub_env_get ("vbe_mode");
  if (modevar != 0)
    {
      unsigned long value;

      value = grub_strtoul (modevar, 0, 0);
      if (grub_errno == GRUB_ERR_NONE)
	use_mode = value;
      else
	grub_errno = GRUB_ERR_NONE;
    }

  err = grub_vbe_get_video_mode_info (use_mode, &mode_info);
  if (err != GRUB_ERR_NONE)
    return err;
  
  /* Dump out details about the mode being tested.  */
  grub_printf ("mode: 0x%03x\n",
               use_mode);
  grub_printf ("width : %d\n",
               mode_info.x_resolution);
  grub_printf ("height: %d\n",
               mode_info.y_resolution);
  grub_printf ("memory model: %02x\n",
               mode_info.memory_model);
  grub_printf ("bytes/scanline: %d\n",
               mode_info.bytes_per_scan_line);
  grub_printf ("bytes/scanline (lin): %d\n",
               mode_info.lin_bytes_per_scan_line);
  grub_printf ("base address: %08x\n",
               mode_info.phys_base_addr);
  grub_printf ("red mask/pos: %d/%d\n",
               mode_info.red_mask_size,
               mode_info.red_field_position);
  grub_printf ("green mask/pos: %d/%d\n",
               mode_info.green_mask_size,
               mode_info.green_field_position);
  grub_printf ("blue mask/pos: %d/%d\n",
               mode_info.blue_mask_size,
               mode_info.blue_field_position);

  grub_printf ("Press any key to continue.\n");

  grub_getkey ();

  /* Setup GFX mode.  */
  err = grub_vbe_set_video_mode (use_mode, &mode_info);
  if (err != GRUB_ERR_NONE)
    return err;

  /* Determine framebuffer address and how many bytes are in scan line.  */
  framebuffer = (grub_uint8_t *) mode_info.phys_base_addr;
  ptr = framebuffer;

  if (controller_info.version >= 0x300)
    {
      bytes_per_scan_line = mode_info.lin_bytes_per_scan_line;
    }
  else
    {
      bytes_per_scan_line = mode_info.bytes_per_scan_line;
    }

  /* Draw some random data to screen.  */
  for (i = 0; i < 256 * 256; i++)
    {
      ptr[i] = i & 0x0F;
    }

  /* Draw white line to screen.  */
  for (i = 0; i < 100; i++)
    {
      ptr[mode_info.bytes_per_scan_line * 50 + i] = 0x0F;
    }

  /* Draw another white line to screen.  */
  grub_memset (ptr + bytes_per_scan_line * 51, 0x0f, bytes_per_scan_line);

  grub_getkey ();

  /* Restore old video mode.  */
  grub_vbe_set_video_mode (old_mode, 0);

  return grub_errno;
}

GRUB_MOD_INIT
{
  (void) mod;			/* To stop warning.  */
  grub_register_command ("vbetest",
                         grub_cmd_vbetest,
                         GRUB_COMMAND_FLAG_BOTH,
                         "vbetest",
                         "Test VESA BIOS Extension 2.0+ support",
                         0);
}

GRUB_MOD_FINI
{
  grub_unregister_command ("vbetest");
}
