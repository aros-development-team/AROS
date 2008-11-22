/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2002,2005,2006,2007  Free Software Foundation, Inc.
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

#ifndef GRUB_KERNEL_HEADER
#define GRUB_KERNEL_HEADER	1

#include <grub/types.h>
#include <grub/symbol.h>

/* The module header.  */
struct grub_module_header
{
  /* The type of object.  */
  grub_int8_t type;
  enum
  {
    OBJ_TYPE_ELF,
    OBJ_TYPE_MEMDISK,
  }  grub_module_header_types;

  /* The size of object (including this header).  */
  grub_target_size_t size;
};

/* "gmim" (GRUB Module Info Magic).  */
#define GRUB_MODULE_MAGIC 0x676d696d

struct grub_module_info
{
  /* Magic number so we know we have modules present.  */
  grub_uint32_t magic;
#if GRUB_TARGET_SIZEOF_VOID_P == 8
  grub_uint32_t padding;
#endif
  /* The offset of the modules.  */
  grub_target_off_t offset;
  /* The size of all modules plus this header.  */
  grub_target_size_t size;
};

extern grub_addr_t grub_arch_modules_addr (void);

extern void EXPORT_FUNC(grub_module_iterate) (int (*hook) (struct grub_module_header *));

/* The start point of the C code.  */
void grub_main (void);

/* The machine-specific initialization. This must initialize memory.  */
void grub_machine_init (void);

/* The machine-specific finalization.  */
void grub_machine_fini (void);

/* The machine-specific prefix initialization.  */
void grub_machine_set_prefix (void);

/* Register all the exported symbols. This is automatically generated.  */
void grub_register_exported_symbols (void);

#endif /* ! GRUB_KERNEL_HEADER */
