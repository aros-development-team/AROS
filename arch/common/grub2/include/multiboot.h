/* multiboot.h - multiboot header file. */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2003,2007  Free Software Foundation, Inc.
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

#ifndef MULTIBOOT_HEADER
#define MULTIBOOT_HEADER 1

/* How many bytes from the start of the file we search for the header.  */
#define MULTIBOOT_SEARCH                 8192

/* The magic field should contain this.  */
#define MULTIBOOT_MAGIC                  0x1BADB002

/* This should be in %eax.  */
#define MULTIBOOT_MAGIC2                 0x2BADB002

/* The bits in the required part of flags field we don't support.  */
#define MULTIBOOT_UNSUPPORTED            0x0000fffc

/* Alignment of multiboot modules.  */
#define MULTIBOOT_MOD_ALIGN              0x00001000

/* 
 * Flags set in the 'flags' member of the multiboot header.
 */

/* Align all boot modules on i386 page (4KB) boundaries.  */
#define MULTIBOOT_PAGE_ALIGN		0x00000001

/* Must pass memory information to OS.  */
#define MULTIBOOT_MEMORY_INFO		0x00000002

/* Must pass video information to OS.  */
#define MULTIBOOT_VIDEO_MODE		0x00000004

/* This flag indicates the use of the address fields in the header.  */
#define MULTIBOOT_AOUT_KLUDGE		0x00010000

/*
 *  Flags to be set in the 'flags' member of the multiboot info structure.
 */

/* is there basic lower/upper memory information? */
#define MULTIBOOT_INFO_MEMORY		0x00000001
/* is there a boot device set? */
#define MULTIBOOT_INFO_BOOTDEV		0x00000002
/* is the command-line defined? */
#define MULTIBOOT_INFO_CMDLINE		0x00000004
/* are there modules to do something with? */
#define MULTIBOOT_INFO_MODS		0x00000008

/* These next two are mutually exclusive */

/* is there a symbol table loaded? */
#define MULTIBOOT_INFO_AOUT_SYMS		0x00000010
/* is there an ELF section header table? */
#define MULTIBOOT_INFO_ELF_SHDR		0x00000020

/* is there a full memory map? */
#define MULTIBOOT_INFO_MEM_MAP		0x00000040

/* Is there drive info?  */
#define MULTIBOOT_INFO_DRIVE_INFO		0x00000080

/* Is there a config table?  */
#define MULTIBOOT_INFO_CONFIG_TABLE	0x00000100

/* Is there a boot loader name?  */
#define MULTIBOOT_INFO_BOOT_LOADER_NAME	0x00000200

/* Is there a APM table?  */
#define MULTIBOOT_INFO_APM_TABLE		0x00000400

/* Is there video information?  */
#define MULTIBOOT_INFO_VIDEO_INFO		0x00000800

#endif /* ! MULTIBOOT_HEADER */
