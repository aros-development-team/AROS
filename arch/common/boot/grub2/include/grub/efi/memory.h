/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2009  Free Software Foundation, Inc.
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

#ifndef GRUB_MEMORY_MACHINE_HEADER
#define GRUB_MEMORY_MACHINE_HEADER	1

#include <grub/err.h>
#include <grub/types.h>

#define GRUB_MMAP_REGISTER_BY_FIRMWARE  1

#define GRUB_MACHINE_MEMORY_AVAILABLE	1
#define GRUB_MACHINE_MEMORY_RESERVED	2
#define GRUB_MACHINE_MEMORY_ACPI	3
#define GRUB_MACHINE_MEMORY_NVS         4
#define GRUB_MACHINE_MEMORY_CODE        5
#define GRUB_MACHINE_MEMORY_MAX_TYPE 	5
  /* This one is special: it's used internally but is never reported
     by firmware. */
#define GRUB_MACHINE_MEMORY_HOLE 	6


grub_err_t EXPORT_FUNC(grub_machine_mmap_iterate)
(int NESTED_FUNC_ATTR (*hook) (grub_uint64_t, grub_uint64_t, grub_uint32_t));
grub_err_t grub_machine_mmap_register (grub_uint64_t start, grub_uint64_t size,
				       int type, int handle);
grub_err_t grub_machine_mmap_unregister (int handle);

grub_uint64_t grub_mmap_get_post64 (void);
grub_uint64_t grub_mmap_get_upper (void);
grub_uint64_t grub_mmap_get_lower (void);

#endif /* ! GRUB_MEMORY_MACHINE_HEADER */
