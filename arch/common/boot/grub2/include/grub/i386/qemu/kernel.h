/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2005,2006,2007,2008,2009  Free Software Foundation, Inc.
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

#ifndef GRUB_KERNEL_MACHINE_HEADER
#define GRUB_KERNEL_MACHINE_HEADER	1

/* The offset of GRUB_CORE_ENTRY_ADDR.  */
#define GRUB_KERNEL_MACHINE_CORE_ENTRY_ADDR	0x8

/* The offset of GRUB_KERNEL_IMAGE_SIZE.  */
#define GRUB_KERNEL_MACHINE_KERNEL_IMAGE_SIZE	0xc

/* The offset of GRUB_PREFIX.  */
#define GRUB_KERNEL_MACHINE_PREFIX		0x10

/* End of the data section. */
#define GRUB_KERNEL_MACHINE_DATA_END		0x50

#ifndef ASM_FILE

#include <grub/symbol.h>
#include <grub/types.h>

extern grub_addr_t grub_core_entry_addr;

/* The size of kernel image.  */
extern grub_int32_t grub_kernel_image_size;

/* The total size of module images following the kernel.  */
extern grub_int32_t grub_total_module_size;

/* The prefix which points to the directory where GRUB modules and its
   configuration file are located.  */
extern char grub_prefix[];

#endif /* ! ASM_FILE */

#endif /* ! GRUB_KERNEL_MACHINE_HEADER */
