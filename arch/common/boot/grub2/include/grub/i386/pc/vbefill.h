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

#ifndef GRUB_VBEFILL_MACHINE_HEADER
#define GRUB_VBEFILL_MACHINE_HEADER	1

/* NOTE: This header is private header for vbe driver and should not be used
   in other parts of the code.  */

struct grub_video_i386_vbeblit_info;

void
grub_video_i386_vbefill (struct grub_video_i386_vbeblit_info *dst,
                         grub_video_color_t color, int x, int y,
                         int width, int height);

void
grub_video_i386_vbefill_direct32 (struct grub_video_i386_vbeblit_info *dst,
                                  grub_video_color_t color,  int x, int y,
                                  int width, int height);

void
grub_video_i386_vbefill_direct24 (struct grub_video_i386_vbeblit_info *dst,
                                  grub_video_color_t color, int x, int y,
                                  int width, int height);

void
grub_video_i386_vbefill_direct16 (struct grub_video_i386_vbeblit_info *dst,
                                  grub_video_color_t color, int x, int y,
                                  int width, int height);

void
grub_video_i386_vbefill_direct8 (struct grub_video_i386_vbeblit_info *dst,
				 grub_video_color_t color, int x, int y,
				 int width, int height);

#endif /* ! GRUB_VBEFILL_MACHINE_HEADER */
