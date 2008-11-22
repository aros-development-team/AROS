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

#ifndef GRUB_VBEBLIT_MACHINE_HEADER
#define GRUB_VBEBLIT_MACHINE_HEADER	1

/* NOTE: This header is private header for vbe driver and should not be used
   in other parts of the code.  */

struct grub_video_i386_vbeblit_info;

void
grub_video_i386_vbeblit_replace (struct grub_video_i386_vbeblit_info *dst,
                                 struct grub_video_i386_vbeblit_info *src,
                                 int x, int y, int width, int height,
                                 int offset_x, int offset_y);

void
grub_video_i386_vbeblit_replace_directN (struct grub_video_i386_vbeblit_info *dst,
					 struct grub_video_i386_vbeblit_info *src,
					 int x, int y, int width, int height,
					 int offset_x, int offset_y);

void
grub_video_i386_vbeblit_replace_BGRX8888_RGBX8888 (struct grub_video_i386_vbeblit_info *dst,
						   struct grub_video_i386_vbeblit_info *src,
						   int x, int y, int width, int height,
						   int offset_x, int offset_y);

void
grub_video_i386_vbeblit_replace_BGRX8888_RGB888 (struct grub_video_i386_vbeblit_info *dst,
						 struct grub_video_i386_vbeblit_info *src,
						 int x, int y,
						 int width, int height,
						 int offset_x, int offset_y);

void
grub_video_i386_vbeblit_replace_BGR888_RGBX8888 (struct grub_video_i386_vbeblit_info *dst,
						 struct grub_video_i386_vbeblit_info *src,
						 int x, int y,
						 int width, int height,
						 int offset_x, int offset_y);

void
grub_video_i386_vbeblit_replace_BGR888_RGB888 (struct grub_video_i386_vbeblit_info *dst,
					       struct grub_video_i386_vbeblit_info *src,
					       int x, int y,
					       int width, int height,
					       int offset_x, int offset_y);

void
grub_video_i386_vbeblit_replace_RGBX8888_RGB888 (struct grub_video_i386_vbeblit_info *dst,
						 struct grub_video_i386_vbeblit_info *src,
						 int x, int y,
						 int width, int height,
						 int offset_x, int offset_y);

void
grub_video_i386_vbeblit_replace_RGB888_RGBX8888 (struct grub_video_i386_vbeblit_info *dst,
						 struct grub_video_i386_vbeblit_info *src,
						 int x, int y,
						 int width, int height,
						 int offset_x, int offset_y);

void
grub_video_i386_vbeblit_replace_index_RGBX8888 (struct grub_video_i386_vbeblit_info *dst,
						struct grub_video_i386_vbeblit_info *src,
						int x, int y,
						int width, int height,
						int offset_x, int offset_y);

void
grub_video_i386_vbeblit_replace_index_RGB888 (struct grub_video_i386_vbeblit_info *dst,
					      struct grub_video_i386_vbeblit_info *src,
					      int x, int y, int width, int height,
					      int offset_x, int offset_y);

void
grub_video_i386_vbeblit_blend (struct grub_video_i386_vbeblit_info *dst,
                               struct grub_video_i386_vbeblit_info *src,
                               int x, int y, int width, int height,
                               int offset_x, int offset_y);

void
grub_video_i386_vbeblit_blend_BGRA8888_RGBA8888 (struct grub_video_i386_vbeblit_info *dst,
						 struct grub_video_i386_vbeblit_info *src,
						 int x, int y,
						 int width, int height,
						 int offset_x, int offset_y);

void
grub_video_i386_vbeblit_blend_BGR888_RGBA8888 (struct grub_video_i386_vbeblit_info *dst,
					       struct grub_video_i386_vbeblit_info *src,
					       int x, int y,
					       int width, int height,
					       int offset_x, int offset_y);

void
grub_video_i386_vbeblit_blend_RGBA8888_RGBA8888 (struct grub_video_i386_vbeblit_info *dst,
						 struct grub_video_i386_vbeblit_info *src,
						 int x, int y,
						 int width, int height,
						 int offset_x, int offset_y);

void
grub_video_i386_vbeblit_blend_RGB888_RGBA8888 (struct grub_video_i386_vbeblit_info *dst,
					       struct grub_video_i386_vbeblit_info *src,
					       int x, int y,
					       int width, int height,
					       int offset_x, int offset_y);

void
grub_video_i386_vbeblit_blend_index_RGBA8888 (struct grub_video_i386_vbeblit_info *dst,
					      struct grub_video_i386_vbeblit_info *src,
					      int x, int y,
					      int width, int height,
					      int offset_x, int offset_y);

#endif /* ! GRUB_VBEBLIT_MACHINE_HEADER */
