/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2002,2007  Free Software Foundation, Inc.
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

#ifndef GRUB_BOOT_HEADER
#define GRUB_BOOT_HEADER	1

#define GRUB_BOOT_VERSION_MAJOR	4
#define GRUB_BOOT_VERSION_MINOR	0
#define GRUB_BOOT_VERSION	((GRUB_BOOT_VERSION_MINOR << 8) \
					| GRUB_BOOT_VERSION_MAJOR)

#endif /* ! GRUB_BOOT_HEADER */
