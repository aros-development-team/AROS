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

#ifndef GRUB_CPU_XNU_H
#define GRUB_CPU_XNU_H 1

#include <grub/err.h>

#define GRUB_XNU_PAGESIZE 4096
typedef grub_uint32_t grub_xnu_ptr_t;

struct grub_xnu_boot_params
{
  grub_uint16_t verminor;
  grub_uint16_t vermajor;
  /* Command line passed to xnu. */
  grub_uint8_t cmdline[1024];

  /* Later are the same as EFI's get_memory_map (). */
  grub_xnu_ptr_t efi_mmap;
  grub_uint32_t efi_mmap_size;
  grub_uint32_t efi_mem_desc_size;
  grub_uint32_t efi_mem_desc_version;

  /* Later are video parameters. */
  grub_xnu_ptr_t lfb_base;
#define GRUB_XNU_VIDEO_SPLASH 1
#define GRUB_XNU_VIDEO_TEXT_IN_VIDEO 2
  grub_uint32_t lfb_mode;
  grub_uint32_t lfb_line_len;
  grub_uint32_t lfb_width;
  grub_uint32_t lfb_height;
  grub_uint32_t lfb_depth;

  /* Pointer to device tree and its len. */
  grub_xnu_ptr_t devtree;
  grub_uint32_t devtreelen;

  /* First used address by kernel or boot structures. */
  grub_xnu_ptr_t heap_start;
  /* Last used address by kernel or boot structures minus previous value. */
  grub_uint32_t heap_size;

  /* First memory page containing runtime code or data. */
  grub_uint32_t efi_runtime_first_page;
  /* First memory page containing runtime code or data minus previous value. */
  grub_uint32_t efi_runtime_npages;
  grub_uint32_t efi_system_table;
  /* Size of grub_efi_uintn_t in bits. */
  grub_uint8_t efi_uintnbits;
} __attribute__ ((packed));
#define GRUB_XNU_BOOTARGS_VERMINOR 4
#define GRUB_XNU_BOOTARGS_VERMAJOR 1

extern grub_uint32_t grub_xnu_entry_point;
extern grub_uint32_t grub_xnu_stack;
extern grub_uint32_t grub_xnu_arg1;
extern char grub_xnu_cmdline[1024];
grub_err_t grub_xnu_boot (void);
grub_err_t grub_cpu_xnu_fill_devicetree (void);
grub_err_t grub_xnu_set_video (struct grub_xnu_boot_params *bootparams_relloc);
extern grub_uint32_t grub_xnu_heap_will_be_at;
extern grub_uint8_t grub_xnu_launcher_start[];
extern grub_uint8_t grub_xnu_launcher_end[];
#endif
