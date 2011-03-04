/* memory.h - describe the memory map */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2002,2007,2010  Free Software Foundation, Inc.
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

#ifndef _GRUB_MACHINE_LBIO_HEADER
#define _GRUB_MACHINE_LBIO_HEADER      1

struct grub_linuxbios_table_header
{
  char signature[4];
  grub_uint32_t size;
};
typedef struct grub_linuxbios_table_header *grub_linuxbios_table_header_t;

struct grub_linuxbios_table_item
{
#define GRUB_LINUXBIOS_MEMBER_UNUSED		0x00
#define GRUB_LINUXBIOS_MEMBER_MEMORY		0x01
#define GRUB_LINUXBIOS_MEMBER_LINK              0x11
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

#endif
