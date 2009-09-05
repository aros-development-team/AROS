/* multiboot2.h - multiboot2 header file with grub definitions. */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2005,2007  Free Software Foundation, Inc.
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

#ifndef GRUB_MULTIBOOT2_HEADER
#define GRUB_MULTIBOOT2_HEADER 1

#include <grub/types.h>
#include <grub/err.h>
#include <grub/elf.h>

#ifndef GRUB_UTIL
typedef grub_uint32_t uint32_t;
typedef grub_uint64_t uint64_t;
#define __WORDSIZE GRUB_TARGET_WORDSIZE
#endif

struct multiboot_tag_header;

grub_err_t
grub_mb2_tag_alloc (grub_addr_t *addr, int key, grub_size_t len);

grub_err_t
grub_mb2_tags_arch_create (void);

void
grub_mb2_arch_boot (grub_addr_t entry, void *tags);

void
grub_mb2_arch_unload (struct multiboot_tag_header *tags);

grub_err_t
grub_mb2_arch_elf32_hook (Elf32_Phdr *phdr, grub_addr_t *addr, int *do_load);

grub_err_t
grub_mb2_arch_elf64_hook (Elf64_Phdr *phdr, grub_addr_t *addr, int *do_load);

grub_err_t
grub_mb2_arch_module_alloc (grub_size_t size, grub_addr_t *addr);

grub_err_t
grub_mb2_arch_module_free (grub_addr_t addr, grub_size_t size);

void
grub_multiboot2 (int argc, char *argv[]);

void
grub_module2 (int argc, char *argv[]);

#define for_each_tag(tag, tags) \
  for (tag = tags; \
       tag && tag->key != MULTIBOOT2_TAG_END; \
       tag = (struct multiboot_tag_header *)((char *)tag + tag->len))

#endif /* ! GRUB_MULTIBOOT2_HEADER */
