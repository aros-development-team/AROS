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

#ifndef GRUB_MACHO_H
#define GRUB_MACHO_H 1
#include <grub/types.h>

/* Multi-architecture header. Always in big-endian. */
struct grub_macho_fat_header
{
  grub_uint32_t magic;
  grub_uint32_t nfat_arch;
} __attribute__ ((packed));
#define GRUB_MACHO_FAT_MAGIC 0xcafebabe

typedef grub_uint32_t grub_macho_cpu_type_t;
typedef grub_uint32_t grub_macho_cpu_subtype_t;

/* Architecture descriptor. Always in big-endian. */
struct grub_macho_fat_arch
{
  grub_macho_cpu_type_t cputype;
  grub_macho_cpu_subtype_t cpusubtype;
  grub_uint32_t offset;
  grub_uint32_t size;
  grub_uint32_t align;
} __attribute__ ((packed));

/* File header for 32-bit. Always in native-endian. */
struct grub_macho_header32
{
#define GRUB_MACHO_MAGIC32 0xfeedface
  grub_uint32_t magic;
  grub_macho_cpu_type_t cputype;
  grub_macho_cpu_subtype_t cpusubtype;
  grub_uint32_t filetype;
  grub_uint32_t ncmds;
  grub_uint32_t sizeofcmds;
  grub_uint32_t flags;
} __attribute__ ((packed));

/* File header for 64-bit. Always in native-endian. */
struct grub_macho_header64
{
#define GRUB_MACHO_MAGIC64 0xfeedfacf
  grub_uint32_t magic;
  grub_macho_cpu_type_t cputype;
  grub_macho_cpu_subtype_t cpusubtype;
  grub_uint32_t filetype;
  grub_uint32_t ncmds;
  grub_uint32_t sizeofcmds;
  grub_uint32_t flags;
  grub_uint32_t reserved;
} __attribute__ ((packed));

/* Common header of Mach-O commands. */
struct grub_macho_cmd
{
  grub_uint32_t cmd;
  grub_uint32_t cmdsize;
} __attribute__ ((packed));

typedef grub_uint32_t grub_macho_vmprot_t;

/* 32-bit segment command. */
struct grub_macho_segment32
{
#define GRUB_MACHO_CMD_SEGMENT32  1
  grub_uint32_t cmd;
  grub_uint32_t cmdsize;
  grub_uint8_t segname[16];
  grub_uint32_t vmaddr;
  grub_uint32_t vmsize;
  grub_uint32_t fileoff;
  grub_uint32_t filesize;
  grub_macho_vmprot_t maxprot;
  grub_macho_vmprot_t initprot;
  grub_uint32_t nsects;
  grub_uint32_t flags;
} __attribute__ ((packed));

/* 64-bit segment command. */
struct grub_macho_segment64
{
#define GRUB_MACHO_CMD_SEGMENT64  0x19
  grub_uint32_t cmd;
  grub_uint32_t cmdsize;
  grub_uint8_t segname[16];
  grub_uint64_t vmaddr;
  grub_uint64_t vmsize;
  grub_uint64_t fileoff;
  grub_uint64_t filesize;
  grub_macho_vmprot_t maxprot;
  grub_macho_vmprot_t initprot;
  grub_uint32_t nsects;
  grub_uint32_t flags;
} __attribute__ ((packed));

#define GRUB_MACHO_CMD_THREAD     5

struct grub_macho_lzss_header
{
  char magic[8];
#define GRUB_MACHO_LZSS_MAGIC "complzss"
  grub_uint32_t unused;
  grub_uint32_t uncompressed_size;
  grub_uint32_t compressed_size;
};

/* Convenience union. What do we need to load to identify the file type. */
union grub_macho_filestart
{
  struct grub_macho_fat_header fat;
  struct grub_macho_header32 thin32;
  struct grub_macho_header64 thin64;
  struct grub_macho_lzss_header lzss;
} __attribute__ ((packed));

#define GRUB_MACHO_LZSS_OFFSET 0x180

grub_size_t
grub_decompress_lzss (grub_uint8_t *dst, grub_uint8_t *dstend,
		      grub_uint8_t *src, grub_uint8_t *srcend);

#endif
