/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2002,2003  Free Software Foundation, Inc.
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

#ifndef KERNEL_MACHINE_HEADER
#define KERNEL_MACHINE_HEADER	1

/* The offset of GRUB_TOTAL_MODULE_SIZE.  */
#define GRUB_KERNEL_MACHINE_TOTAL_MODULE_SIZE	0x8

/* The offset of GRUB_KERNEL_IMAGE_SIZE.  */
#define GRUB_KERNEL_MACHINE_KERNEL_IMAGE_SIZE	0xc

/* The offset of GRUB_COMPRESSED_SIZE.  */
#define GRUB_KERNEL_MACHINE_COMPRESSED_SIZE	0x10

/* The offset of GRUB_INSTALL_DOS_PART.  */
#define GRUB_KERNEL_MACHINE_INSTALL_DOS_PART	0x14

/* The offset of GRUB_INSTALL_BSD_PART.  */
#define GRUB_KERNEL_MACHINE_INSTALL_BSD_PART	0x18

/* The offset of GRUB_PREFIX.  */
#define GRUB_KERNEL_MACHINE_PREFIX		0x1c

/* The size of the first region which won't be compressed.  */
#define GRUB_KERNEL_MACHINE_RAW_SIZE		0x4A0

#ifndef ASM_FILE

#include <grub/types.h>

/* The DOS partition number of the installed partition.  */
extern grub_int32_t grub_install_dos_part;

/* The BSD partition number of the installed partition.  */
extern grub_int32_t grub_install_bsd_part;

/* The prefix which points to the directory where GRUB modules and its
   configuration file are located.  */
extern char grub_prefix[];

/* The boot BIOS drive number.  */
extern grub_int32_t grub_boot_drive;

/* The end address of the kernel.  */
extern grub_addr_t grub_end_addr;

#endif /* ! ASM_FILE */

#endif /* ! KERNEL_MACHINE_HEADER */
