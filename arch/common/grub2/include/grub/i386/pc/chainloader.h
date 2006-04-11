/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2004  Free Software Foundation, Inc.
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

#ifndef GRUB_CHAINLOADER_MACHINE_HEADER
#define GRUB_CHAINLOADER_MACHINE_HEADER	1

#include <grub/dl.h>

/* Common function for normal and rescue mode commands. */
typedef enum
  {
    GRUB_CHAINLOADER_FORCE = 0x1
  } grub_chainloader_flags_t;

void EXPORT_FUNC(grub_chainloader_cmd) (const char * file,
					grub_chainloader_flags_t flags);

#endif /* GRUB_CHAINLOADER_MACHINE_HEADER */
