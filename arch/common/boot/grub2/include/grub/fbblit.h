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

#ifndef GRUB_FBBLIT_HEADER
#define GRUB_FBBLIT_HEADER	1

/* NOTE: This header is private header for fb driver and should not be used
   in other parts of the code.  */

struct grub_video_fbblit_info;

void
grub_video_fbblit_replace (struct grub_video_fbblit_info *dst,
			   struct grub_video_fbblit_info *src,
			   int x, int y, int width, int height,
			   int offset_x, int offset_y);

void
grub_video_fbblit_replace_directN (struct grub_video_fbblit_info *dst,
				   struct grub_video_fbblit_info *src,
				   int x, int y, int width, int height,
				   int offset_x, int offset_y);

void
grub_video_fbblit_replace_BGRX8888_RGBX8888 (struct grub_video_fbblit_info *dst,
					     struct grub_video_fbblit_info *src,
					     int x, int y, int width, int height,
					     int offset_x, int offset_y);

void
grub_video_fbblit_replace_BGRX8888_RGB888 (struct grub_video_fbblit_info *dst,
					   struct grub_video_fbblit_info *src,
					   int x, int y,
					   int width, int height,
					   int offset_x, int offset_y);

void
grub_video_fbblit_replace_BGR888_RGBX8888 (struct grub_video_fbblit_info *dst,
					   struct grub_video_fbblit_info *src,
					   int x, int y,
					   int width, int height,
					   int offset_x, int offset_y);

void
grub_video_fbblit_replace_BGR888_RGB888 (struct grub_video_fbblit_info *dst,
					 struct grub_video_fbblit_info *src,
					 int x, int y,
					 int width, int height,
					 int offset_x, int offset_y);

void
grub_video_fbblit_replace_RGBX8888_RGB888 (struct grub_video_fbblit_info *dst,
					   struct grub_video_fbblit_info *src,
					   int x, int y,
					   int width, int height,
					   int offset_x, int offset_y);

void
grub_video_fbblit_replace_RGB888_RGBX8888 (struct grub_video_fbblit_info *dst,
					   struct grub_video_fbblit_info *src,
					   int x, int y,
					   int width, int height,
					   int offset_x, int offset_y);

void
grub_video_fbblit_replace_index_RGBX8888 (struct grub_video_fbblit_info *dst,
					  struct grub_video_fbblit_info *src,
					  int x, int y,
					  int width, int height,
					  int offset_x, int offset_y);

void
grub_video_fbblit_replace_index_RGB888 (struct grub_video_fbblit_info *dst,
					struct grub_video_fbblit_info *src,
					int x, int y, int width, int height,
					int offset_x, int offset_y);

void
grub_video_fbblit_blend (struct grub_video_fbblit_info *dst,
			 struct grub_video_fbblit_info *src,
			 int x, int y, int width, int height,
			 int offset_x, int offset_y);

void
grub_video_fbblit_blend_BGRA8888_RGBA8888 (struct grub_video_fbblit_info *dst,
					   struct grub_video_fbblit_info *src,
					   int x, int y,
					   int width, int height,
					   int offset_x, int offset_y);

void
grub_video_fbblit_blend_BGR888_RGBA8888 (struct grub_video_fbblit_info *dst,
					 struct grub_video_fbblit_info *src,
					 int x, int y,
					 int width, int height,
					 int offset_x, int offset_y);

void
grub_video_fbblit_blend_RGBA8888_RGBA8888 (struct grub_video_fbblit_info *dst,
					   struct grub_video_fbblit_info *src,
					   int x, int y,
					   int width, int height,
					   int offset_x, int offset_y);

void
grub_video_fbblit_blend_RGB888_RGBA8888 (struct grub_video_fbblit_info *dst,
					 struct grub_video_fbblit_info *src,
					 int x, int y,
					 int width, int height,
					 int offset_x, int offset_y);

void
grub_video_fbblit_blend_index_RGBA8888 (struct grub_video_fbblit_info *dst,
					struct grub_video_fbblit_info *src,
					int x, int y,
					int width, int height,
					int offset_x, int offset_y);

void
grub_video_fbblit_replace_32bit_1bit (struct grub_video_fbblit_info *dst,
				      struct grub_video_fbblit_info *src,
				      int x, int y,
				      int width, int height,
				      int offset_x, int offset_y);

void
grub_video_fbblit_replace_24bit_1bit (struct grub_video_fbblit_info *dst,
				      struct grub_video_fbblit_info *src,
				      int x, int y,
				      int width, int height,
				      int offset_x, int offset_y);

void
grub_video_fbblit_replace_16bit_1bit (struct grub_video_fbblit_info *dst,
				      struct grub_video_fbblit_info *src,
				      int x, int y,
				      int width, int height,
				      int offset_x, int offset_y);

void
grub_video_fbblit_replace_8bit_1bit (struct grub_video_fbblit_info *dst,
				     struct grub_video_fbblit_info *src,
				     int x, int y,
				     int width, int height,
				     int offset_x, int offset_y);

void
grub_video_fbblit_blend_XXXA8888_1bit (struct grub_video_fbblit_info *dst,
				       struct grub_video_fbblit_info *src,
				       int x, int y,
				       int width, int height,
				       int offset_x, int offset_y);

void
grub_video_fbblit_blend_XXX888_1bit (struct grub_video_fbblit_info *dst,
				       struct grub_video_fbblit_info *src,
				       int x, int y,
				       int width, int height,
				       int offset_x, int offset_y);

void
grub_video_fbblit_blend_XXX565_1bit (struct grub_video_fbblit_info *dst,
				     struct grub_video_fbblit_info *src,
				     int x, int y,
				     int width, int height,
				     int offset_x, int offset_y);
#endif /* ! GRUB_FBBLIT_HEADER */
