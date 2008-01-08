/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2002,2003,2004,2007,2008  Free Software Foundation, Inc.
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

#ifndef GRUB_LOADER_CPU_HEADER
#define GRUB_LOADER_CPU_HEADER	1

#include <grub/types.h>
#include <grub/symbol.h>
#include <grub/multiboot.h>

extern grub_uint32_t EXPORT_VAR(grub_linux_prot_size);
extern char *EXPORT_VAR(grub_linux_tmp_addr);
extern char *EXPORT_VAR(grub_linux_real_addr);
extern grub_int32_t EXPORT_VAR(grub_linux_is_bzimage);
extern grub_addr_t EXPORT_VAR(grub_os_area_addr);
extern grub_size_t EXPORT_VAR(grub_os_area_size);

void EXPORT_FUNC(grub_linux_boot) (void) __attribute__ ((noreturn));

/* The asm part of the multiboot loader.  */
void EXPORT_FUNC(grub_multiboot_real_boot) (grub_addr_t entry, 
					    struct grub_multiboot_info *mbi) 
     __attribute__ ((noreturn));
void EXPORT_FUNC(grub_multiboot2_real_boot) (grub_addr_t entry,
                                             struct grub_multiboot_info *mbi)
     __attribute__ ((noreturn));

/* It is necessary to export these functions, because normal mode commands
   reuse rescue mode commands.  */
void grub_rescue_cmd_linux (int argc, char *argv[]);
void grub_rescue_cmd_initrd (int argc, char *argv[]);

#endif /* ! GRUB_LOADER_CPU_HEADER */
