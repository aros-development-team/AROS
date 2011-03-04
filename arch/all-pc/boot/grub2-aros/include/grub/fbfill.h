/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2006,2007,2008  Free Software Foundation, Inc.
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

#ifndef GRUB_FBFILL_HEADER
#define GRUB_FBFILL_HEADER	1

/* NOTE: This header is private header for fb driver and should not be used
   in other parts of the code.  */

struct grub_video_fbblit_info;

struct grub_video_fbrender_target
{
  /* Copy of the screen's mode info structure, except that width, height and
     mode_type has been re-adjusted to requested render target settings.  */
  struct grub_video_mode_info mode_info;

  struct
  {
    unsigned int x;
    unsigned int y;
    unsigned int width;
    unsigned int height;
  } viewport;

  /* Indicates whether the data has been allocated by us and must be freed
     when render target is destroyed.  */
  int is_allocated;

  /* Pointer to data.  Can either be in video card memory or in local host's
     memory.  */
  grub_uint8_t *data;
};

void
grub_video_fbfill (struct grub_video_fbblit_info *dst,
		   grub_video_color_t color, int x, int y,
		   int width, int height);

void
grub_video_fbfill_direct32 (struct grub_video_fbblit_info *dst,
			    grub_video_color_t color,  int x, int y,
			    int width, int height);

void
grub_video_fbfill_direct24 (struct grub_video_fbblit_info *dst,
			    grub_video_color_t color, int x, int y,
			    int width, int height);

void
grub_video_fbfill_direct16 (struct grub_video_fbblit_info *dst,
			    grub_video_color_t color, int x, int y,
			    int width, int height);

void
grub_video_fbfill_direct8 (struct grub_video_fbblit_info *dst,
			   grub_video_color_t color, int x, int y,
			   int width, int height);

#endif /* ! GRUB_FBFILL_HEADER */
