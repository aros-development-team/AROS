/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2009  Free Software Foundation, Inc.
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

#include <grub/env.h>
#include <grub/misc.h>
#include <grub/xnu.h>
#include <grub/mm.h>
#include <grub/cpu/xnu.h>
#include <grub/video_fb.h>

#define min(a,b) (((a) < (b)) ? (a) : (b))
#define max(a,b) (((a) > (b)) ? (a) : (b))

#define DEFAULT_VIDEO_MODE "1024x768x32,800x600x32,640x480x32"

static int NESTED_FUNC_ATTR video_hook (grub_video_adapter_t p __attribute__ ((unused)),
					struct grub_video_mode_info *info)
{
  if (info->mode_type & GRUB_VIDEO_MODE_TYPE_PURE_TEXT)
    return 0;

  return 1;
}

/* Setup video for xnu. */
grub_err_t
grub_xnu_set_video (struct grub_xnu_boot_params *params)
{
  struct grub_video_mode_info mode_info;
  int ret;
  int x,y;
  char *tmp, *modevar;
  void *framebuffer;
  grub_err_t err;

  modevar = grub_env_get ("gfxpayload");
  if (! modevar || *modevar == 0)
    err = grub_video_set_mode (DEFAULT_VIDEO_MODE, video_hook);
  else
    {
      tmp = grub_malloc (grub_strlen (modevar)
			 + sizeof (DEFAULT_VIDEO_MODE) + 1);
      if (! tmp)
	return grub_error (GRUB_ERR_OUT_OF_MEMORY,
			   "couldn't allocate temporary storag");
      grub_sprintf (tmp, "%s;" DEFAULT_VIDEO_MODE, modevar);
      err = grub_video_set_mode (tmp, video_hook);
      grub_free (tmp);
    }

  if (err)
    return err;

  ret = grub_video_get_info_and_fini (&mode_info, &framebuffer);
  if (ret)
    return grub_error (GRUB_ERR_IO, "couldn't retrieve video parameters");

  err = GRUB_ERR_NONE;
  x = mode_info.width - grub_xnu_bitmap->mode_info.width;
  x /= 2;
  y = mode_info.height - grub_xnu_bitmap->mode_info.height;
  y /= 2;
  err = grub_video_blit_bitmap (grub_xnu_bitmap,
				GRUB_VIDEO_BLIT_REPLACE,
				x > 0 ? x : 0,
				y > 0 ? y : 0,
				x < 0 ? -x : 0,
				y < 0 ? -y : 0,
				min (grub_xnu_bitmap->mode_info.width,
				     mode_info.width),
				min (grub_xnu_bitmap->mode_info.height,
				     mode_info.height));
  if (err)
    {
      grub_print_error ();
      grub_errno = GRUB_ERR_NONE;
      grub_xnu_bitmap = 0;
    }

  params->lfb_width = mode_info.width;
  params->lfb_height = mode_info.height;
  params->lfb_depth = mode_info.bpp;
  params->lfb_line_len = mode_info.pitch;

  params->lfb_base = PTR_TO_UINT32 (framebuffer);
  params->lfb_mode = grub_xnu_bitmap
    ? GRUB_XNU_VIDEO_SPLASH : GRUB_XNU_VIDEO_TEXT_IN_VIDEO;

  return GRUB_ERR_NONE;
}

