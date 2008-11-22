/* memory.h - describe the memory map */
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

#ifndef _GRUB_MEMORY_MACHINE_LB_HEADER
#define _GRUB_MEMORY_MACHINE_LB_HEADER      1

#include <grub/symbol.h>
#include <grub/i386/pc/memory.h>

#ifndef ASM_FILE
#include <grub/types.h>
#endif

#define GRUB_MEMORY_MACHINE_LOWER_USABLE		0x9fc00		/* 640 kiB - 1 kiB */

#define GRUB_MEMORY_MACHINE_UPPER_START			0x100000	/* 1 MiB */
#define GRUB_MEMORY_MACHINE_LOWER_SIZE			GRUB_MEMORY_MACHINE_UPPER_START

#ifndef ASM_FILE

struct grub_linuxbios_table_header
{
  char signature[4];
  grub_uint32_t size;
};
typedef struct grub_linuxbios_table_header *grub_linuxbios_table_header_t;

struct grub_linuxbios_table_item
{
#define GRUB_LINUXBIOS_MEMBER_UNUSED		0
#define GRUB_LINUXBIOS_MEMBER_MEMORY		1
  grub_uint32_t tag;
  grub_uint32_t size;
};
typedef struct grub_linuxbios_table_item *grub_linuxbios_table_item_t;

struct grub_linuxbios_mem_region
{
  grub_uint64_t addr;
  grub_uint64_t size;
#define GRUB_MACHINE_MEMORY_AVAILABLE		1
  grub_uint32_t type;
};
typedef struct grub_linuxbios_mem_region *mem_region_t;

void grub_machine_mmap_init (void);

void EXPORT_FUNC(grub_machine_mmap_iterate)
     (int NESTED_FUNC_ATTR (*hook) (grub_uint64_t, grub_uint64_t, grub_uint32_t));

#endif

#endif /* ! _GRUB_MEMORY_MACHINE_HEADER */
