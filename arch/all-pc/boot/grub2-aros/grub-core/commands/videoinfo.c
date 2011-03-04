/* videoinfo.c - command to list video modes.  */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2005,2007,2008,2009,2010  Free Software Foundation, Inc.
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
#include <grub/dl.h>
#include <grub/env.h>
#include <grub/misc.h>
#include <grub/mm.h>
#include <grub/command.h>
#include <grub/i18n.h>

static unsigned height, width, depth; 

static int
hook (const struct grub_video_mode_info *info)
{
  if (height && width && (info->width != width || info->height != height))
    return 0;

  if (depth && info->bpp != depth)
    return 0;

  if (info->mode_number == GRUB_VIDEO_MODE_NUMBER_INVALID)
    grub_printf ("        ");
  else
    grub_printf ("  0x%03x ", info->mode_number);
  grub_printf ("%4d x %4d x %2d  ", info->width, info->height, info->bpp);

  if (info->mode_type & GRUB_VIDEO_MODE_TYPE_PURE_TEXT)
    grub_printf ("Text-only ");
  /* Show mask and position details for direct color modes.  */
  if (info->mode_type & GRUB_VIDEO_MODE_TYPE_RGB)
    grub_printf ("Direct, mask: %d/%d/%d/%d  pos: %d/%d/%d/%d",
		 info->red_mask_size,
		 info->green_mask_size,
		 info->blue_mask_size,
		 info->reserved_mask_size,
		 info->red_field_pos,
		 info->green_field_pos,
		 info->blue_field_pos,
		 info->reserved_field_pos);
  if (info->mode_type & GRUB_VIDEO_MODE_TYPE_INDEX_COLOR)
    grub_printf ("Packed ");
  if (info->mode_type & GRUB_VIDEO_MODE_TYPE_YUV)
    grub_printf ("YUV ");
  if (info->mode_type & GRUB_VIDEO_MODE_TYPE_PLANAR)
    grub_printf ("Planar ");
  if (info->mode_type & GRUB_VIDEO_MODE_TYPE_HERCULES)
    grub_printf ("Hercules ");
  if (info->mode_type & GRUB_VIDEO_MODE_TYPE_CGA)
    grub_printf ("CGA ");
  if (info->mode_type & GRUB_VIDEO_MODE_TYPE_NONCHAIN4)
    grub_printf ("Non-chain 4 ");
  if (info->mode_type & GRUB_VIDEO_MODE_TYPE_1BIT_BITMAP)
    grub_printf ("Monochrome ");
  if (info->mode_type & GRUB_VIDEO_MODE_TYPE_UNKNOWN)
    grub_printf ("Unknown ");

  grub_printf ("\n");

  return 0;
}

static grub_err_t
grub_cmd_videoinfo (grub_command_t cmd __attribute__ ((unused)),
		    int argc, char **args)
{
  grub_video_adapter_t adapter;
  grub_video_driver_id_t id;

  height = width = depth = 0;
  if (argc)
    {
      char *ptr;
      ptr = args[0];
      width = grub_strtoul (ptr, &ptr, 0);
      if (grub_errno)
	return grub_errno;
      if (*ptr != 'x')
	return grub_error (GRUB_ERR_BAD_ARGUMENT, "invalid mode specification");
      ptr++;
      height = grub_strtoul (ptr, &ptr, 0);
      if (grub_errno)
	return grub_errno;
      if (*ptr == 'x')
	{
	  ptr++;
	  depth = grub_strtoul (ptr, &ptr, 0);
	  if (grub_errno)
	    return grub_errno;
	}
    }

#ifdef GRUB_MACHINE_PCBIOS
  if (grub_strcmp (cmd->name, "vbeinfo") == 0)
    grub_dl_load ("vbe");
#endif

  id = grub_video_get_driver_id ();

  grub_printf ("List of supported video modes:\n");
  grub_printf ("Legend: P=Packed pixel, D=Direct color, "
	       "mask/pos=R/G/B/reserved\n");

  FOR_VIDEO_ADAPTERS (adapter)
  {
    grub_printf ("Adapter '%s':\n", adapter->name);

    if (!adapter->iterate)
      {
	grub_printf ("  No info available\n");
	continue;
      }

    if (adapter->id != id)
      {
	if (adapter->init ())
	  {
	    grub_printf ("  Failed\n");
	    grub_errno = GRUB_ERR_NONE;
	    continue;
	  }
      }

    if (adapter->print_adapter_specific_info)
      adapter->print_adapter_specific_info ();

    adapter->iterate (hook);

    if (adapter->id != id)
      {
	if (adapter->fini ())
	  {
	    grub_errno = GRUB_ERR_NONE;
	    continue;
	  }
      }
  }
  return GRUB_ERR_NONE;
}

static grub_command_t cmd;
#ifdef GRUB_MACHINE_PCBIOS
static grub_command_t cmd_vbe;
#endif

GRUB_MOD_INIT(videoinfo)
{
  cmd = grub_register_command ("videoinfo", grub_cmd_videoinfo, "[WxH[xD]]",
			       N_("List available video modes. If "
				     "resolution is given show only modes"
				     " matching it."));
#ifdef GRUB_MACHINE_PCBIOS
  cmd_vbe = grub_register_command ("vbeinfo", grub_cmd_videoinfo, "[WxH[xD]]",
				   N_("List available video modes. If "
				      "resolution is given show only modes"
				      " matching it."));
#endif
}

GRUB_MOD_FINI(videoinfo)
{
  grub_unregister_command (cmd);
#ifdef GRUB_MACHINE_PCBIOS
  grub_unregister_command (cmd_vbe);
#endif
}

