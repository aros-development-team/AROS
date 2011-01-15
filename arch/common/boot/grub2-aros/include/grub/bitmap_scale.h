/* bitmap_scale.h - Bitmap scaling functions. */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2008,2009  Free Software Foundation, Inc.
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

#ifndef GRUB_BITMAP_SCALE_HEADER
#define GRUB_BITMAP_SCALE_HEADER 1

#include <grub/err.h>
#include <grub/types.h>
#include <grub/bitmap_scale.h>

enum grub_video_bitmap_scale_method
{
  /* Choose the fastest interpolation algorithm.  */
  GRUB_VIDEO_BITMAP_SCALE_METHOD_FASTEST,
  /* Choose the highest quality interpolation algorithm.  */
  GRUB_VIDEO_BITMAP_SCALE_METHOD_BEST,

  /* Specific algorithms:  */
  /* Nearest neighbor interpolation.  */
  GRUB_VIDEO_BITMAP_SCALE_METHOD_NEAREST,
  /* Bilinear interpolation.  */
  GRUB_VIDEO_BITMAP_SCALE_METHOD_BILINEAR
};

grub_err_t
EXPORT_FUNC (grub_video_bitmap_create_scaled) (struct grub_video_bitmap **dst,
					       int dst_width, int dst_height,
					       struct grub_video_bitmap *src,
					       enum 
					       grub_video_bitmap_scale_method
					       scale_method);

#endif /* ! GRUB_BITMAP_SCALE_HEADER */
