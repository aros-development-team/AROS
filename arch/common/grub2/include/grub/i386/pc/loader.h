/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2002,2003,2004  Free Software Foundation, Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef GRUB_LOADER_MACHINE_HEADER
#define GRUB_LOADER_MACHINE_HEADER	1

#include <grub/types.h>
#include <grub/symbol.h>
#include <grub/machine/multiboot.h>

extern grub_uint32_t EXPORT_VAR(grub_linux_prot_size);
extern char *EXPORT_VAR(grub_linux_tmp_addr);
extern char *EXPORT_VAR(grub_linux_real_addr);

void EXPORT_FUNC(grub_linux_boot_zimage) (void) __attribute__ ((noreturn));
void EXPORT_FUNC(grub_linux_boot_bzimage) (void) __attribute__ ((noreturn));

/* This is an asm part of the chainloader.  */
void EXPORT_FUNC(grub_chainloader_real_boot) (int drive, void *part_addr) __attribute__ ((noreturn));

/* The asm part of the multiboot loader.  */
void EXPORT_FUNC(grub_multiboot_real_boot) (grub_addr_t entry, 
					    struct grub_multiboot_info *mbi) 
     __attribute__ ((noreturn));

/* It is necessary to export these functions, because normal mode commands
   reuse rescue mode commands.  */
void grub_rescue_cmd_linux (int argc, char *argv[]);
void grub_rescue_cmd_initrd (int argc, char *argv[]);
void grub_rescue_cmd_multiboot (int argc, char *argv[]);
void grub_rescue_cmd_module (int argc, char *argv[]);

#endif /* ! GRUB_LOADER_MACHINE_HEADER */
