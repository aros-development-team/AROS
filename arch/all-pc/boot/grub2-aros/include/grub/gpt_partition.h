/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2002,2005,2006,2007,2008  Free Software Foundation, Inc.
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

#ifndef GRUB_GPT_PARTITION_HEADER
#define GRUB_GPT_PARTITION_HEADER	1

#include <grub/types.h>
#include <grub/partition.h>

struct grub_gpt_part_type
{
  grub_uint32_t data1;
  grub_uint16_t data2;
  grub_uint16_t data3;
  grub_uint8_t data4[8];
} __attribute__ ((aligned(8)));
typedef struct grub_gpt_part_type grub_gpt_part_type_t;

#define GRUB_GPT_PARTITION_TYPE_EMPTY \
  { 0x0, 0x0, 0x0, \
    { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 } \
  }

#define GRUB_GPT_PARTITION_TYPE_BIOS_BOOT \
  { grub_cpu_to_le32_compile_time (0x21686148), \
      grub_cpu_to_le16_compile_time (0x6449), \
      grub_cpu_to_le16_compile_time (0x6e6f),	       \
    { 0x74, 0x4e, 0x65, 0x65, 0x64, 0x45, 0x46, 0x49 } \
  }

#define GRUB_GPT_PARTITION_TYPE_LDM \
  { grub_cpu_to_le32_compile_time (0x5808C8AAU),\
      grub_cpu_to_le16_compile_time (0x7E8F), \
      grub_cpu_to_le16_compile_time (0x42E0),	       \
	{ 0x85, 0xD2, 0xE1, 0xE9, 0x04, 0x34, 0xCF, 0xB3 }	\
  }

struct grub_gpt_header
{
  grub_uint8_t magic[8];
  grub_uint32_t version;
  grub_uint32_t headersize;
  grub_uint32_t crc32;
  grub_uint32_t unused1;
  grub_uint64_t primary;
  grub_uint64_t backup;
  grub_uint64_t start;
  grub_uint64_t end;
  grub_uint8_t guid[16];
  grub_uint64_t partitions;
  grub_uint32_t maxpart;
  grub_uint32_t partentry_size;
  grub_uint32_t partentry_crc32;
} GRUB_PACKED;

struct grub_gpt_partentry
{
  grub_gpt_part_type_t type;
  grub_uint8_t guid[16];
  grub_uint64_t start;
  grub_uint64_t end;
  grub_uint64_t attrib;
  char name[72];
} GRUB_PACKED;

grub_err_t
grub_gpt_partition_map_iterate (grub_disk_t disk,
				grub_partition_iterate_hook_t hook,
				void *hook_data);


#endif /* ! GRUB_GPT_PARTITION_HEADER */
