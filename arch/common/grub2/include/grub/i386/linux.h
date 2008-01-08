/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 1999,2000,2001,2002,2003,2004,2007  Free Software Foundation, Inc.
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

#ifndef GRUB_LINUX_MACHINE_HEADER
#define GRUB_LINUX_MACHINE_HEADER	1

#define GRUB_LINUX_MAGIC_SIGNATURE	0x53726448      /* "HdrS" */
#define GRUB_LINUX_DEFAULT_SETUP_SECTS	4
#define GRUB_LINUX_FLAG_CAN_USE_HEAP	0x80
#define GRUB_LINUX_INITRD_MAX_ADDRESS	0x37FFFFFF
#define GRUB_LINUX_MAX_SETUP_SECTS	64
#define GRUB_LINUX_BOOT_LOADER_TYPE	0x72
#define GRUB_LINUX_HEAP_END_OFFSET	(0x9000 - 0x200)

#define GRUB_LINUX_BZIMAGE_ADDR		0x100000
#define GRUB_LINUX_ZIMAGE_ADDR		0x10000
#define GRUB_LINUX_OLD_REAL_MODE_ADDR	0x90000
#define GRUB_LINUX_SETUP_STACK		0x9000

#define GRUB_LINUX_FLAG_BIG_KERNEL	0x1

/* Linux's video mode selection support. Actually I hate it!  */
#define GRUB_LINUX_VID_MODE_NORMAL	0xFFFF
#define GRUB_LINUX_VID_MODE_EXTENDED	0xFFFE
#define GRUB_LINUX_VID_MODE_ASK		0xFFFD

#define GRUB_LINUX_CL_OFFSET		0x9000
#define GRUB_LINUX_CL_END_OFFSET	0x90FF
#define GRUB_LINUX_SETUP_MOVE_SIZE	0x9100
#define GRUB_LINUX_CL_MAGIC		0xA33F

#define GRUB_LINUX_EFI_SIGNATURE	\
  ('E' << 24 | 'F' << 16 | 'I' << 8 | 'L')

#ifndef ASM_FILE

/* For the Linux/i386 boot protocol version 2.03.  */
struct linux_kernel_header
{ 
  grub_uint8_t code1[0x0020];
  grub_uint16_t cl_magic;		/* Magic number 0xA33F */
  grub_uint16_t cl_offset;		/* The offset of command line */
  grub_uint8_t code2[0x01F1 - 0x0020 - 2 - 2];
  grub_uint8_t setup_sects;		/* The size of the setup in sectors */
  grub_uint16_t root_flags;		/* If the root is mounted readonly */
  grub_uint16_t syssize;		/* obsolete */
  grub_uint16_t swap_dev;		/* obsolete */
  grub_uint16_t ram_size;		/* obsolete */
  grub_uint16_t vid_mode;		/* Video mode control */
  grub_uint16_t root_dev;		/* Default root device number */
  grub_uint16_t boot_flag;		/* 0xAA55 magic number */
  grub_uint16_t jump;			/* Jump instruction */
  grub_uint32_t header;			/* Magic signature "HdrS" */
  grub_uint16_t version;		/* Boot protocol version supported */
  grub_uint32_t realmode_swtch;		/* Boot loader hook */
  grub_uint16_t start_sys;		/* The load-low segment (obsolete) */
  grub_uint16_t kernel_version;		/* Points to kernel version string */
  grub_uint8_t type_of_loader;		/* Boot loader identifier */
  grub_uint8_t loadflags;		/* Boot protocol option flags */
  grub_uint16_t setup_move_size;	/* Move to high memory size */
  grub_uint32_t code32_start;		/* Boot loader hook */
  grub_uint32_t ramdisk_image;		/* initrd load address */
  grub_uint32_t ramdisk_size;		/* initrd size */
  grub_uint32_t bootsect_kludge;	/* obsolete */
  grub_uint16_t heap_end_ptr;		/* Free memory after setup end */
  grub_uint16_t pad1;			/* Unused */
  char *cmd_line_ptr;			/* Points to the kernel command line */
  grub_uint32_t initrd_addr_max;        /* Highest address for initrd */
} __attribute__ ((packed));

/* Boot parameters for Linux based on 2.6.12. This is used by the setup
   sectors of Linux, and must be simulated by GRUB on EFI, because
   the setup sectors depend on BIOS.  */
struct linux_kernel_params
{
  grub_uint8_t video_cursor_x;		/* 0 */
  grub_uint8_t video_cursor_y;

  grub_uint16_t ext_mem;		/* 2 */
  
  grub_uint16_t video_page;		/* 4 */
  grub_uint8_t video_mode;		/* 6 */
  grub_uint8_t video_width;		/* 7 */
  
  grub_uint8_t padding1[0xa - 0x8];
  
  grub_uint16_t video_ega_bx;		/* a */
  
  grub_uint8_t padding2[0xe - 0xc];
  
  grub_uint8_t video_height;		/* e */
  grub_uint8_t have_vga;		/* f */
  grub_uint16_t font_size;		/* 10 */
  
  grub_uint16_t lfb_width;		/* 12 */
  grub_uint16_t lfb_height;		/* 14 */
  grub_uint16_t lfb_depth;		/* 16 */
  grub_uint32_t lfb_base;		/* 18 */
  grub_uint32_t lfb_size;		/* 1c */

  grub_uint16_t cl_magic;		/* 20 */
  grub_uint16_t cl_offset;

  grub_uint16_t lfb_line_len;		/* 24 */
  grub_uint8_t red_mask_size;		/* 26 */
  grub_uint8_t red_field_pos;
  grub_uint8_t green_mask_size;
  grub_uint8_t green_field_pos;
  grub_uint8_t blue_mask_size;
  grub_uint8_t blue_field_pos;
  grub_uint8_t reserved_mask_size;
  grub_uint8_t reserved_field_pos;
  grub_uint16_t vesapm_segment;		/* 2e */
  grub_uint16_t vesapm_offset;		/* 30 */
  grub_uint16_t lfb_pages;		/* 32 */
  grub_uint16_t vesa_attrib;		/* 34 */

  grub_uint8_t padding3[0x40 - 0x36];

  grub_uint16_t apm_version;		/* 40 */
  grub_uint16_t apm_code_segment;	/* 42 */
  grub_uint32_t apm_entry;		/* 44 */
  grub_uint16_t apm_16bit_code_segment;	/* 48 */
  grub_uint16_t apm_data_segment;	/* 4a */
  grub_uint16_t apm_flags;		/* 4c */
  grub_uint32_t apm_code_len;		/* 4e */
  grub_uint16_t apm_data_len;		/* 52 */
  
  grub_uint8_t padding4[0x60 - 0x54];
  
  grub_uint32_t ist_signature;		/* 60 */
  grub_uint32_t ist_command;		/* 64 */
  grub_uint32_t ist_event;		/* 68 */
  grub_uint32_t ist_perf_level;		/* 6c */

  grub_uint8_t padding5[0x80 - 0x70];
  
  grub_uint8_t hd0_drive_info[0x10];	/* 80 */
  grub_uint8_t hd1_drive_info[0x10];	/* 90 */
  grub_uint16_t rom_config_len;		/* a0 */

  grub_uint8_t padding6[0x1c0 - 0xa2];
  
  grub_uint32_t efi_signature;		/* 1c0 */
  grub_uint32_t efi_system_table;	/* 1c4 */
  grub_uint32_t efi_mem_desc_size;	/* 1c8 */
  grub_uint32_t efi_mem_desc_version;	/* 1cc */
  grub_uint32_t efi_mmap;		/* 1d0 */
  grub_uint32_t efi_mmap_size;		/* 1d4 */
  
  grub_uint8_t padding7[0x1e0 - 0x1d8];
  
  grub_uint32_t alt_mem;		/* 1e0 */
  
  grub_uint8_t padding8[0x1e8 - 0x1e4];
  
  grub_uint32_t mmap_size;		/* 1e8 */

  grub_uint8_t padding9[0x1ff - 0x1ec];
  
  grub_uint8_t ps_mouse;		/* 1ff */
} __attribute__ ((packed));
#endif /* ! ASM_FILE */

#endif /* ! GRUB_LINUX_MACHINE_HEADER */
