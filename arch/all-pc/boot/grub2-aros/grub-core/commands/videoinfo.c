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

GRUB_MOD_LICENSE ("GPLv3+");

struct hook_ctx
{
  unsigned height, width, depth; 
  struct grub_video_mode_info *current_mode;
};

static int
hook (const struct grub_video_mode_info *info, void *hook_arg)
{
  struct hook_ctx *ctx = hook_arg;

  if (ctx->height && ctx->width && (info->width != ctx->width || info->height != ctx->height))
    return 0;

  if (ctx->depth && info->bpp != ctx->depth)
    return 0;

  if (info->mode_number == GRUB_VIDEO_MODE_NUMBER_INVALID)
    grub_printf ("        ");
  else
    {
      if (ctx->current_mode && info->mode_number == ctx->current_mode->mode_number)
	grub_printf ("*");
      else
	grub_printf (" ");
      grub_printf (" 0x%03x ", info->mode_number);
    }
  grub_printf ("%4d x %4d x %2d (%4d)  ", info->width, info->height, info->bpp,
	       info->pitch);

  if (info->mode_type & GRUB_VIDEO_MODE_TYPE_PURE_TEXT)
    grub_xputs (_("Text-only "));
  /* Show mask and position details for direct color modes.  */
  if (info->mode_type & GRUB_VIDEO_MODE_TYPE_RGB)
    /* TRANSLATORS: "Direct color" is a mode when the color components
       are written dirrectly into memory.  */
    grub_printf_ (N_("Direct color, mask: %d/%d/%d/%d  pos: %d/%d/%d/%d"),
		  info->red_mask_size,
		  info->green_mask_size,
		  info->blue_mask_size,
		  info->reserved_mask_size,
		  info->red_field_pos,
		  info->green_field_pos,
		  info->blue_field_pos,
		  info->reserved_field_pos);
  if (info->mode_type & GRUB_VIDEO_MODE_TYPE_INDEX_COLOR)
    /* TRANSLATORS: In "paletted color" mode you write the index of the color
       in the palette. Synonyms include "packed pixel".  */
    grub_xputs (_("Paletted "));
  if (info->mode_type & GRUB_VIDEO_MODE_TYPE_YUV)
    grub_xputs (_("YUV "));
  if (info->mode_type & GRUB_VIDEO_MODE_TYPE_PLANAR)
    /* TRANSLATORS: "Planar" is the video memory where you have to write
       in several different banks "plans" to control the different color
       components of the same pixel.  */
    grub_xputs (_("Planar "));
  if (info->mode_type & GRUB_VIDEO_MODE_TYPE_HERCULES)
    grub_xputs (_("Hercules "));
  if (info->mode_type & GRUB_VIDEO_MODE_TYPE_CGA)
    grub_xputs (_("CGA "));
  if (info->mode_type & GRUB_VIDEO_MODE_TYPE_NONCHAIN4)
    /* TRANSLATORS: Non-chain 4 is a 256-color planar
       (unchained) video memory mode.  */
    grub_xputs (_("Non-chain 4 "));
  if (info->mode_type & GRUB_VIDEO_MODE_TYPE_1BIT_BITMAP)
    grub_xputs (_("Monochrome "));
  if (info->mode_type & GRUB_VIDEO_MODE_TYPE_UNKNOWN)
    grub_xputs (_("Unknown video mode "));

  grub_xputs ("\n");

  return 0;
}

static void
print_edid (struct grub_video_edid_info *edid_info)
{
  unsigned int edid_width, edid_height;

  if (grub_video_edid_checksum (edid_info))
    {
      grub_puts_ (N_("  EDID checksum invalid"));
      grub_errno = GRUB_ERR_NONE;
      return;
    }

  grub_printf_ (N_("  EDID version: %u.%u\n"),
		edid_info->version, edid_info->revision);
  if (grub_video_edid_preferred_mode (edid_info, &edid_width, &edid_height)
	== GRUB_ERR_NONE)
    grub_printf_ (N_("    Preferred mode: %ux%u\n"), edid_width, edid_height);
  else
    {
      grub_printf_ (N_("    No preferred mode available\n"));
      grub_errno = GRUB_ERR_NONE;
    }
}

static grub_err_t
grub_cmd_videoinfo (grub_command_t cmd __attribute__ ((unused)),
		    int argc, char **args)
{
  grub_video_adapter_t adapter;
  grub_video_driver_id_t id;
  struct hook_ctx ctx;

  ctx.height = ctx.width = ctx.depth = 0;
  if (argc)
    {
      char *ptr;
      ptr = args[0];
      ctx.width = grub_strtoul (ptr, &ptr, 0);
      if (grub_errno)
	return grub_errno;
      if (*ptr != 'x')
	return grub_error (GRUB_ERR_BAD_ARGUMENT,
			   N_("invalid video mode specification `%s'"),
			   args[0]);
      ptr++;
      ctx.height = grub_strtoul (ptr, &ptr, 0);
      if (grub_errno)
	return grub_errno;
      if (*ptr == 'x')
	{
	  ptr++;
	  ctx.depth = grub_strtoul (ptr, &ptr, 0);
	  if (grub_errno)
	    return grub_errno;
	}
    }

#ifdef GRUB_MACHINE_PCBIOS
  if (grub_strcmp (cmd->name, "vbeinfo") == 0)
    grub_dl_load ("vbe");
#endif

  id = grub_video_get_driver_id ();

  grub_puts_ (N_("List of supported video modes:"));
  grub_puts_ (N_("Legend: mask/position=red/green/blue/reserved"));

  FOR_VIDEO_ADAPTERS (adapter)
  {
    struct grub_video_mode_info info;
    struct grub_video_edid_info edid_info;

    grub_printf_ (N_("Adapter `%s':\n"), adapter->name);

    if (!adapter->iterate)
      {
	grub_puts_ (N_("  No info available"));
	continue;
      }

    ctx.current_mode = NULL;

    if (adapter->id == id)
      {
	if (grub_video_get_info (&info) == GRUB_ERR_NONE)
	  ctx.current_mode = &info;
	else
	  /* Don't worry about errors.  */
	  grub_errno = GRUB_ERR_NONE;
      }
    else
      {
	if (adapter->init ())
	  {
	    grub_puts_ (N_("  Failed to initialize video adapter"));
	    grub_errno = GRUB_ERR_NONE;
	    continue;
	  }
      }

    if (adapter->print_adapter_specific_info)
      adapter->print_adapter_specific_info ();

    adapter->iterate (hook, &ctx);

    if (adapter->get_edid && adapter->get_edid (&edid_info) == GRUB_ERR_NONE)
      print_edid (&edid_info);
    else
      grub_errno = GRUB_ERR_NONE;

    ctx.current_mode = NULL;

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
  cmd = grub_register_command ("videoinfo", grub_cmd_videoinfo,
			       /* TRANSLATORS: "x" has to be entered in,
				  like an identifier, so please don't
				  use better Unicode codepoints.  */
			       N_("[WxH[xD]]"),
			       N_("List available video modes. If "
				     "resolution is given show only modes"
				     " matching it."));
#ifdef GRUB_MACHINE_PCBIOS
  cmd_vbe = grub_register_command ("vbeinfo", grub_cmd_videoinfo,
				   /* TRANSLATORS: "x" has to be entered in,
				      like an identifier, so please don't
				      use better Unicode codepoints.  */
				   N_("[WxH[xD]]"),
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

