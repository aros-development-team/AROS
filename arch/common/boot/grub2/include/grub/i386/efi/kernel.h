/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2002,2003,2007  Free Software Foundation, Inc.
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

#ifndef GRUB_MACHINE_KERNEL_HEADER
#define GRUB_MACHINE_KERNEL_HEADER   1

/* The prefix which points to the directory where GRUB modules and its
   configuration file are located.  */
extern char grub_prefix[];

/* The offset of GRUB_PREFIX.  */
#define GRUB_KERNEL_MACHINE_PREFIX		0x8

/* End of the data section. */
#define GRUB_KERNEL_MACHINE_DATA_END		0x50

#endif /* ! GRUB_MACHINE_KERNEL_HEADER */

