/* multiboot2.h - multiboot 2 header file. */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2007  Free Software Foundation, Inc.
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

#ifndef MULTIBOOT2_HEADER
#define MULTIBOOT2_HEADER 1

/* How many bytes from the start of the file we search for the header.  */
#define MULTIBOOT2_HEADER_SEARCH           8192

/* The magic field should contain this.  */
#define MULTIBOOT2_HEADER_MAGIC            0xe85250d6

/* Passed from the bootloader to the kernel.  */
#define MULTIBOOT2_BOOTLOADER_MAGIC        0x36d76289

/* Alignment of multiboot modules.  */
#define MULTIBOOT2_MOD_ALIGN               0x00001000

#ifndef ASM_FILE

#include "stdint.h"

/* XXX not portable? */
#if __WORDSIZE == 64
typedef uint64_t multiboot_word;
#else
typedef uint32_t multiboot_word;
#endif

struct multiboot_header
{
  uint32_t magic;
  uint32_t flags;
};

struct multiboot_tag_header
{
  uint32_t key;
  uint32_t len;
};

#define MULTIBOOT2_TAG_RESERVED1 0
#define MULTIBOOT2_TAG_RESERVED2 (~0)

#define MULTIBOOT2_TAG_START     1
struct multiboot_tag_start
{
  struct multiboot_tag_header header;
  multiboot_word size; /* Total size of all multiboot tags. */
};

#define MULTIBOOT2_TAG_NAME      2
struct multiboot_tag_name
{
  struct multiboot_tag_header header;
  char name[1];
};

#define MULTIBOOT2_TAG_MODULE    3
struct multiboot_tag_module
{
  struct multiboot_tag_header header;
  multiboot_word addr;
  multiboot_word size;
  char type[36];
  char cmdline[1];
};

#define MULTIBOOT2_TAG_MEMORY    4
struct multiboot_tag_memory
{
  struct multiboot_tag_header header;
  multiboot_word addr;
  multiboot_word size;
  multiboot_word type;
};

#define MULTIBOOT2_TAG_UNUSED    5
struct multiboot_tag_unused
{
  struct multiboot_tag_header header;
};

#define MULTIBOOT2_TAG_END       0xffff
struct multiboot_tag_end
{
  struct multiboot_tag_header header;
};

#endif /* ! ASM_FILE */

#endif /* ! MULTIBOOT2_HEADER */
