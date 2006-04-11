/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2002, 2005  Free Software Foundation, Inc.
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

#ifndef GRUB_KERNEL_HEADER
#define GRUB_KERNEL_HEADER	1

#include <grub/types.h>

/* The module header.  */
struct grub_module_header
{
  /* The offset of object code.  */
  grub_host_off_t offset;
  /* The size of object code plus this header.  */
  grub_host_size_t size;
};

/* "gmim" (GRUB Module Info Magic).  */
#define GRUB_MODULE_MAGIC 0x676d696d

struct grub_module_info
{
  /* Magic number so we know we have modules present.  */
  grub_uint32_t magic;
  /* The offset of the modules.  */
  grub_host_off_t offset;
  /* The size of all modules plus this header.  */
  grub_host_size_t size;
};

extern grub_addr_t grub_arch_modules_addr (void);

/* The start point of the C code.  */
void grub_main (void);

/* The machine-specific initialization. This must initialize memory.  */
void grub_machine_init (void);

/* The machine-specific finalization.  */
void grub_machine_fini (void);

/* Register all the exported symbols. This is automatically generated.  */
void grub_register_exported_symbols (void);

#endif /* ! GRUB_KERNEL_HEADER */
