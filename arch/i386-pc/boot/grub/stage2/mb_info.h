/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 1996  Erich Boleyn  <erich@uruk.org>
 *  Copyright (C) 2000  Free Software Foundation, Inc.
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

/*
 *  The structure type "mod_list" is used by the "multiboot_info" structure.
 */

struct mod_list
  {
    /* the memory used goes from bytes 'mod_start' to 'mod_end-1' inclusive */
    unsigned long mod_start;
    unsigned long mod_end;

    /* Module command line */
    unsigned long cmdline;

    /* padding to take it to 16 bytes (must be zero) */
    unsigned long pad;
  };


/*
 *  INT-15, AX=E820 style "AddressRangeDescriptor"
 *  ...with a "size" parameter on the front which is the structure size - 4,
 *  pointing to the next one, up until the full buffer length of the memory
 *  map has been reached.
 */

struct AddrRangeDesc
  {
    unsigned long size;
    unsigned long long BaseAddr;
    unsigned long long Length;
    unsigned long Type;

    /* unspecified optional padding... */
  };

/* usable memory "Type", all others are reserved.  */
#define MB_ARD_MEMORY       1


/*
 *  MultiBoot Info description
 *
 *  This is the struct passed to the boot image.  This is done by placing
 *  its address in the EAX register.
 */

struct multiboot_info
  {
    /* MultiBoot info version number */
    unsigned long flags;
 
    /* Available memory from BIOS */
    unsigned long mem_lower;
    unsigned long mem_upper;

    /* "root" partition */
    unsigned long boot_device;

    /* Kernel command line */
    unsigned long cmdline;

    /* Boot-Module list */
    unsigned long mods_count;
    unsigned long mods_addr;

    union
      {
	struct
	  {
	    /* (a.out) Kernel symbol table info */
	    unsigned long tabsize;
	    unsigned long strsize;
	    unsigned long addr;
	    unsigned long pad;
	  }
	a;

	struct
	  {
	    /* (ELF) Kernel section header table */
	    unsigned long num;
	    unsigned long size;
	    unsigned long addr;
	    unsigned long shndx;
	  }
	e;
      }
    syms;

    /* Memory Mapping buffer */
    unsigned long mmap_length;
    unsigned long mmap_addr;
  };

/*
 *  Flags to be set in the 'flags' parameter above
 */

/* is there basic lower/upper memory information? */
#define MB_INFO_MEMORY          0x1
/* is there a boot device set? */
#define MB_INFO_BOOTDEV         0x2
/* is the command-line defined? */
#define MB_INFO_CMDLINE         0x4
/* are there modules to do something with? */
#define MB_INFO_MODS            0x8

/* These next two are mutually exclusive */

/* is there a symbol table loaded? */
#define MB_INFO_AOUT_SYMS       0x10
/* is there an ELF section header table? */
#define MB_INFO_ELF_SHDR        0x20

/* is there a full memory map? */
#define MB_INFO_MEM_MAP         0x40

/*
 *  The following value must be present in the EAX register.
 */

#define MULTIBOOT_VALID         0x2BADB002
