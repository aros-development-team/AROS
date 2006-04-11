/* multiboot.h - multiboot header file. */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2003  Free Software Foundation, Inc.
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

#ifndef GRUB_MULTIBOOT_MACHINE_HEADER
#define GRUB_MULTIBOOT_MACHINE_HEADER 1

/* How many bytes from the start of the file we search for the header.  */
#define GRUB_MB_SEARCH                 8192

/* The magic field should contain this.  */
#define GRUB_MB_MAGIC                  0x1BADB002

/* This should be in %eax.  */
#define GRUB_MB_MAGIC2                 0x2BADB002

/* The bits in the required part of flags field we don't support.  */
#define GRUB_MB_UNSUPPORTED            0x0000fffc

/* Alignment of multiboot modules.  */
#define GRUB_MB_MOD_ALIGN              0x00001000

/* 
 * Flags set in the 'flags' member of the multiboot header.
 */

/* Align all boot modules on i386 page (4KB) boundaries.  */
#define GRUB_MB_PAGE_ALIGN		0x00000001

/* Must pass memory information to OS.  */
#define GRUB_MB_MEMORY_INFO		0x00000002

/* Must pass video information to OS.  */
#define GRUB_MB_VIDEO_MODE		0x00000004

/* This flag indicates the use of the address fields in the header.  */
#define GRUB_MB_AOUT_KLUDGE		0x00010000

/*
 *  Flags to be set in the 'flags' member of the multiboot info structure.
 */

/* is there basic lower/upper memory information? */
#define GRUB_MB_INFO_MEMORY		0x00000001
/* is there a boot device set? */
#define GRUB_MB_INFO_BOOTDEV		0x00000002
/* is the command-line defined? */
#define GRUB_MB_INFO_CMDLINE		0x00000004
/* are there modules to do something with? */
#define GRUB_MB_INFO_MODS		0x00000008

/* These next two are mutually exclusive */

/* is there a symbol table loaded? */
#define GRUB_MB_INFO_AOUT_SYMS		0x00000010
/* is there an ELF section header table? */
#define GRUB_MB_INFO_ELF_SHDR		0x00000020

/* is there a full memory map? */
#define GRUB_MB_INFO_MEM_MAP		0x00000040

/* Is there drive info?  */
#define GRUB_MB_INFO_DRIVE_INFO		0x00000080

/* Is there a config table?  */
#define GRUB_MB_INFO_CONFIG_TABLE	0x00000100

/* Is there a boot loader name?  */
#define GRUB_MB_INFO_BOOT_LOADER_NAME	0x00000200

/* Is there a APM table?  */
#define GRUB_MB_INFO_APM_TABLE		0x00000400

/* Is there video information?  */
#define GRUB_MB_INFO_VIDEO_INFO		0x00000800

#ifndef ASM_FILE

#include <grub/types.h>

struct grub_multiboot_header
{ 
  /* Must be GRUB_MB_MAGIC - see above.  */
  grub_uint32_t magic;

  /* Feature flags.  */
  grub_uint32_t flags;

  /* The above fields plus this one must equal 0 mod 2^32. */
  grub_uint32_t checksum;
  
  /* These are only valid if GRUB_MB_AOUT_KLUDGE is set.  */
  grub_uint32_t header_addr;
  grub_uint32_t load_addr;
  grub_uint32_t load_end_addr;
  grub_uint32_t bss_end_addr;
  grub_uint32_t entry_addr;

  /* These are only valid if GRUB_MB_VIDEO_MODE is set.  */
  grub_uint32_t mode_type;
  grub_uint32_t width;
  grub_uint32_t height;
  grub_uint32_t depth;
};

struct grub_multiboot_info
{
  /* MultiBoot info version number */
  grub_uint32_t flags;
  
  /* Available memory from BIOS */
  grub_uint32_t mem_lower;
  grub_uint32_t mem_upper;
  
  /* "root" partition */
  grub_uint32_t boot_device;
  
  /* Kernel command line */
  grub_uint32_t cmdline;
  
  /* Boot-Module list */
  grub_uint32_t mods_count;
  grub_uint32_t mods_addr;
  
  grub_uint32_t syms[4];
  
  /* Memory Mapping buffer */
  grub_uint32_t mmap_length;
  grub_uint32_t mmap_addr;
  
  /* Drive Info buffer */
  grub_uint32_t drives_length;
  grub_uint32_t drives_addr;
  
  /* ROM configuration table */
  grub_uint32_t config_table;
  
  /* Boot Loader Name */
  grub_uint32_t boot_loader_name;

  /* APM table */
  grub_uint32_t apm_table;

  /* Video */
  grub_uint32_t vbe_control_info;
  grub_uint32_t vbe_mode_info;
  grub_uint16_t vbe_mode;
  grub_uint16_t vbe_interface_seg;
  grub_uint16_t vbe_interface_off;
  grub_uint16_t vbe_interface_len;
};

struct grub_mod_list
{
  /* the memory used goes from bytes 'mod_start' to 'mod_end-1' inclusive */
  grub_uint32_t mod_start;
  grub_uint32_t mod_end;
  
  /* Module command line */
  grub_uint32_t cmdline;
  
  /* padding to take it to 16 bytes (must be zero) */
  grub_uint32_t pad;
};

#endif /* ! ASM_FILE */

#endif /* ! GRUB_MULTIBOOT_MACHINE_HEADER */
