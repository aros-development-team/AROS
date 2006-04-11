/* vbeinfo.c - command to list compatible VBE video modes.  */
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
#include <grub/machine/init.h>
#include <grub/machine/vbe.h>
#include <grub/mm.h>

static void *
real2pm (grub_vbe_farptr_t ptr)
{
  return (void *) ((((unsigned long) ptr & 0xFFFF0000) >> 12UL)
		   + ((unsigned long) ptr & 0x0000FFFF));
}

static grub_err_t
grub_cmd_vbeinfo (struct grub_arg_list *state __attribute__ ((unused)),
		  int argc __attribute__ ((unused)),
		  char **args __attribute__ ((unused)))
{
  struct grub_vbe_info_block controller_info;
  struct grub_vbe_mode_info_block mode_info_tmp;
  grub_uint32_t use_mode = GRUB_VBE_DEFAULT_VIDEO_MODE;
  grub_uint16_t *video_mode_list;
  grub_uint16_t *p;
  grub_uint16_t *saved_video_mode_list;
  grub_size_t video_mode_list_size;
  grub_err_t err;
  char *modevar;

  grub_printf ("List of compatible video modes:\n");

  err = grub_vbe_probe (&controller_info);
  if (err != GRUB_ERR_NONE)
    return err;

  /* Because the information on video modes is stored in a temporary place,
     it is better to copy it to somewhere safe.  */
  p = video_mode_list = real2pm (controller_info.video_mode_ptr);
  while (*p++ != 0xFFFF)
    ;
  
  video_mode_list_size = (grub_addr_t) p - (grub_addr_t) video_mode_list;
  saved_video_mode_list = grub_malloc (video_mode_list_size);
  if (! saved_video_mode_list)
    return grub_errno;

  grub_memcpy (saved_video_mode_list, video_mode_list, video_mode_list_size);
  
  /* Walk through all video modes listed.  */
  for (p = saved_video_mode_list; *p != 0xFFFF; p++)
    {
      const char *memory_model = 0;
      grub_uint32_t mode = (grub_uint32_t) *p;
      
      err = grub_vbe_get_video_mode_info (mode, &mode_info_tmp);
      if (err != GRUB_ERR_NONE)
	{
	  grub_errno = GRUB_ERR_NONE;
	  continue;
	}

      if ((mode_info_tmp.mode_attributes & 0x001) == 0)
	/* If not available, skip it.  */
	continue;

      if ((mode_info_tmp.mode_attributes & 0x002) == 0)
	/* Not enough information.  */
	continue;

      if ((mode_info_tmp.mode_attributes & 0x008) == 0)
	/* Monochrome is unusable.  */
	continue;

      if ((mode_info_tmp.mode_attributes & 0x080) == 0)
	/* We support only linear frame buffer modes.  */
	continue;

      if ((mode_info_tmp.mode_attributes & 0x010) == 0)
	/* We allow only graphical modes.  */
	continue;

      switch (mode_info_tmp.memory_model)
	{
	case 0x04:
	  memory_model = "Packed Pixel";
	  break;
	case 0x06:
	  memory_model = "Direct Color";
	  break;

	default:
	  break;
	}

      if (! memory_model)
	continue;

      grub_printf ("0x%03x: %d x %d x %d bpp (%s)\n",
		   mode,
                   mode_info_tmp.x_resolution,
                   mode_info_tmp.y_resolution,
                   mode_info_tmp.bits_per_pixel,
		   memory_model);
    }

  grub_free (saved_video_mode_list);
  
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

  grub_printf ("Configured VBE mode (vbe_mode) = 0x%03x\n", use_mode);

  return 0;
}

GRUB_MOD_INIT
{
  (void) mod;			/* To stop warning.  */
  grub_register_command ("vbeinfo",
                         grub_cmd_vbeinfo,
                         GRUB_COMMAND_FLAG_BOTH,
                         "vbeinfo",
                         "List compatible VESA BIOS extension video modes.",
                         0);
}

GRUB_MOD_FINI
{
  grub_unregister_command ("vbeinfo");
}
