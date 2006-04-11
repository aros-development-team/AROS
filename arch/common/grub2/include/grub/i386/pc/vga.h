/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2003  Free Software Foundation, Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
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
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef GRUB_VGA_MACHINE_HEADER
#define GRUB_VGA_MACHINE_HEADER	1

#include <grub/symbol.h>

/* Set the video mode to MODE and return the previous mode.  */
unsigned char EXPORT_FUNC(grub_vga_set_mode) (unsigned char mode);

/* Return a pointer to the ROM font table.  */
unsigned char *EXPORT_FUNC(grub_vga_get_font) (void);

#endif /* ! GRUB_VGA_MACHINE_HEADER */
