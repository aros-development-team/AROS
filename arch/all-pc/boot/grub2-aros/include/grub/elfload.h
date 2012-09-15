/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2003, 2004, 2005, 2006, 2007  Free Software Foundation, Inc.
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

#ifndef GRUB_ELFLOAD_HEADER
#define GRUB_ELFLOAD_HEADER	1

#include <grub/err.h>
#include <grub/elf.h>
#include <grub/file.h>
#include <grub/symbol.h>
#include <grub/types.h>

struct grub_elf_file
{
  grub_file_t file;
  union {
    Elf64_Ehdr ehdr64;
    Elf32_Ehdr ehdr32;
  } ehdr;
  void *phdrs;
};
typedef struct grub_elf_file *grub_elf_t;

typedef grub_err_t (*grub_elf32_load_hook_t)
  (Elf32_Phdr *phdr, grub_addr_t *addr, int *load);
typedef grub_err_t (*grub_elf64_load_hook_t)
  (Elf64_Phdr *phdr, grub_addr_t *addr, int *load);

grub_elf_t grub_elf_open (const char *);
grub_elf_t grub_elf_file (grub_file_t file, const char *filename);
grub_err_t grub_elf_close (grub_elf_t);

int grub_elf_is_elf32 (grub_elf_t);
grub_size_t grub_elf32_size (grub_elf_t,
			     const char *filename,
			     Elf32_Addr *, grub_uint32_t *);
grub_err_t grub_elf32_load (grub_elf_t, const char *filename,
			    grub_elf32_load_hook_t, grub_addr_t *,
			    grub_size_t *);

int grub_elf_is_elf64 (grub_elf_t);
grub_size_t grub_elf64_size (grub_elf_t,
			     const char *filename,
			     Elf64_Addr *, grub_uint64_t *);
grub_err_t grub_elf64_load (grub_elf_t, const char *filename,
			    grub_elf64_load_hook_t, grub_addr_t *,
			    grub_size_t *);
grub_err_t
grub_elf32_phdr_iterate (grub_elf_t elf,
			 const char *filename,
			 int NESTED_FUNC_ATTR (*hook) (grub_elf_t, Elf32_Phdr *, void *),
			 void *hook_arg);
grub_err_t
grub_elf64_phdr_iterate (grub_elf_t elf,
			 const char *filename,
			 int NESTED_FUNC_ATTR (*hook) (grub_elf_t, Elf64_Phdr *, void *),
			 void *hook_arg);

#endif /* ! GRUB_ELFLOAD_HEADER */
