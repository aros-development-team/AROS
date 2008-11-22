/* multiboot.h - multiboot header file with grub definitions. */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2003,2007,2008  Free Software Foundation, Inc.
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

#ifndef GRUB_MULTIBOOT_HEADER
#define GRUB_MULTIBOOT_HEADER 1

#include <multiboot.h>

void grub_multiboot (int argc, char *argv[]);
void grub_module (int argc, char *argv[]);

#ifndef ASM_FILE

#include <grub/types.h>

struct grub_multiboot_header
{ 
  /* Must be MULTIBOOT_MAGIC - see above.  */
  grub_uint32_t magic;

  /* Feature flags.  */
  grub_uint32_t flags;

  /* The above fields plus this one must equal 0 mod 2^32. */
  grub_uint32_t checksum;
  
  /* These are only valid if MULTIBOOT_AOUT_KLUDGE is set.  */
  grub_uint32_t header_addr;
  grub_uint32_t load_addr;
  grub_uint32_t load_end_addr;
  grub_uint32_t bss_end_addr;
  grub_uint32_t entry_addr;

  /* These are only valid if MULTIBOOT_VIDEO_MODE is set.  */
  grub_uint32_t mode_type;
  grub_uint32_t width;
  grub_uint32_t height;
  grub_uint32_t depth;
};

struct grub_multiboot_info
{
  /* Multiboot info version number */
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

struct grub_multiboot_mmap_entry
{
  grub_uint32_t size;
  grub_uint64_t addr;
  grub_uint64_t len;
#define GRUB_MULTIBOOT_MEMORY_AVAILABLE		1
#define GRUB_MULTIBOOT_MEMORY_RESERVED		2
  grub_uint32_t type;
} __attribute__((packed));

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

#endif /* ! GRUB_MULTIBOOT_HEADER */
