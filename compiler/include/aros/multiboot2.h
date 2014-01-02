/*
    Copyright © 2011-2012, The AROS Development Team. All rights reserved.
    $Id$
 
    Desc: Multiboot v2 information structures
    Lang: english
*/

#ifndef _AROS_MULTIBOOT2_H
#define _AROS_MULTIBOOT2_H

#include <aros/cpu.h> /* For __WORDSIZE */
#include <hardware/vbe.h>

#if (__WORDSIZE == 64)
#ifndef MULTIBOOT_64BIT

/*
 * Define this in your code if you want to build 32-bit code using full 64-bit pointers.
 * Useful for building pc-x86_64 bootstrap which runs in 32 bit mode.
 */
#define MULTIBOOT_64BIT

#endif
#endif

struct mb2_header
{
    ULONG magic;   /* MB2_MAGIC					    */
    ULONG arch;	   /* Architecture - see below			    */
    ULONG size;	   /* Total size of the header (including all tags) */
    ULONG checksum; /* Header checksum				    */
    /* Tags follow */
};

#define MB2_MAGIC 0xe85250d6

/* Architecture */
#define MB2_ARCH_I386   0
#define MB2_ARCH_MIPS32 4

struct mb2_header_tag
{
    UWORD type;  /* Tag ID			      */
    UWORD flags; /* Flags, see below		      */
    ULONG size;  /* Size of this tag, including data */
};

/* Tag IDs */
#define MB2_HEADER_TAG_END		   0
#define MB2_HEADER_TAG_INFORMATION_REQUEST 1
#define MB2_HEADER_TAG_ADDRESS		   2
#define MB2_HEADER_TAG_ENTRY_ADDRESS	   3
#define MB2_HEADER_TAG_CONSOLE_FLAGS	   4
#define MB2_HEADER_TAG_FRAMEBUFFER	   5
#define MB2_HEADER_TAG_MODULE_ALIGN	   6

/* Flags */
#define MBTF_OPTIONAL 1

struct mb2_header_tag_info_request
{
    UWORD type;		/* MB2_HEADER_TAG_INFORMATION_REQUEST */
    UWORD flags;
    ULONG size;
    ULONG requests[0];
};

struct mb2_header_tag_address
{
    UWORD type;		/* MB2_HEADER_TAG_ADDRESS */
    UWORD flags;
    ULONG size;
    ULONG header_addr;
    ULONG load_addr;
    ULONG load_end_addr;
    ULONG bss_end_addr;
};

struct mb2_header_tag_entry_address
{
    UWORD type;		/* MB2_HEADER_TAG_ENTRY_ADDRESS */
    UWORD flags;
    ULONG size;
    ULONG entry_addr;
};

struct mb2_header_tag_console_flags
{
    UWORD type;		/* MB2_HEADER_TAG_CONSOLE_FLAGS */
    UWORD flags;
    ULONG size;
    ULONG console_flags;
};

struct mb2_header_tag_framebuffer
{
    UWORD type;		/* MB2_HEADER_TAG_FRAMEBUFFER */
    UWORD flags;
    ULONG size;
    ULONG width;
    ULONG height;
    ULONG depth;
};

struct mb2_header_tag_module_align
{
    UWORD type;		/* MB2_HEADER_TAG_MODULE_ALIGN */
    UWORD flags;
    ULONG size;
    ULONG width;
    ULONG height;
    ULONG depth;
};

/* This value is passed in by the bootloader */
#define MB2_STARTUP_MAGIC 0x36d76289

/*
 * Multiboot v2 data is a UQUAD value representing overall length,
 * followed by a sequence of tags.
 * This is the generalized form of a tag.
 */
struct mb2_tag
{
    ULONG type;	/* ID				*/
    ULONG size;	/* Data size (including header)	*/
};

/* Known tag IDs */
#define MB2_TAG_END             0
#define MB2_TAG_CMDLINE         1
#define MB2_TAG_BOOTLOADER_NAME 2
#define MB2_TAG_MODULE          3
#define MB2_TAG_BASIC_MEMINFO   4
#define MB2_TAG_BOOTDEV         5
#define MB2_TAG_MMAP            6
#define MB2_TAG_VBE             7
#define MB2_TAG_FRAMEBUFFER     8
#define MB2_TAG_ELF_SECTIONS    9
#define MB2_TAG_APM             10
#define MB2_TAG_EFI32           11
#define MB2_TAG_EFI64           12
#define MB2_TAG_SMBIOS          13
#define MB2_TAG_ACPI_OLD        14
#define MB2_TAG_ACPI_NEW        15
#define MB2_TAG_NETWORK         16

struct mb2_tag_string
{
    ULONG type;
    ULONG size;
    BYTE  string[0];
};

struct mb2_tag_module
{
    ULONG type;
    ULONG size;
    ULONG mod_start;
    ULONG mod_end;
    BYTE  cmdline[0];
};

struct mb2_tag_basic_meminfo
{
    ULONG type;
    ULONG size;
    ULONG mem_lower;
    ULONG mem_upper;
};

struct mb2_tag_bootdev
{
    ULONG type;
    ULONG size;
    ULONG biosdev;
    ULONG slice;
    ULONG part;
};

struct mb2_mmap
{
#ifdef MULTIBOOT_64BIT
    UQUAD addr;	  /* Address and length, 64-bit */
    UQUAD len;
#else
    ULONG addr;
    ULONG addr_high;
    ULONG len;
    ULONG len_high;
#endif
    ULONG type;	 /* Entry type, see below	*/
    ULONG pad;	 /* Reserved			*/
};

/* Memory map entry types */
#define MMAP2_TYPE_RAM	     1	/* General purpose RAM  */
#define MMAP2_TYPE_RESERVED  2	/* System private areas */
#define MMAP2_TYPE_ACPIDATA  3  /* ACPI data structures */
#define MMAP2_TYPE_ACPINVS   4
#define MMAP2_TYPE_BAD	     5	/* Broken RAM		*/

struct mb2_tag_mmap
{
    ULONG type;
    ULONG size;
    ULONG entry_size;
    ULONG entry_version;
    struct mb2_mmap mmap[0];
};

struct mb2_tag_vbe
{
    ULONG type;
    ULONG size;

    UWORD vbe_mode;
    UWORD vbe_interface_seg;
    UWORD vbe_interface_off;
    UWORD vbe_interface_len;

    struct vbe_controller vbe_control_info;
    struct vbe_mode       vbe_mode_info;
};

struct mb2_tag_framebuffer_common
{
    ULONG type;			/* Tag ID				*/
    ULONG size;
#ifdef MULTIBOOT_64BIT
    UQUAD framebuffer_addr;	/* Framebuffer address, 64-bit pointer	*/
#else
    ULONG framebuffer_addr;
    ULONG framebuffer_addr_high;
#endif
    ULONG framebuffer_pitch;	/* Bytes per line			*/
    ULONG framebuffer_width;	/* Size in pixels or characters		*/
    ULONG framebuffer_height;
    UBYTE framebuffer_bpp;		/* Bits per pixel			*/
    UBYTE framebuffer_type;	/* See below				*/
    UWORD reserved;
};

/* Framebuffer types */
#define MB2_FRAMEBUFFER_LUT  0
#define MB2_FRAMEBUFFER_RGB  1
#define MB2_FRAMEBUFFER_TEXT 2

struct fb_color
{
    UBYTE red;
    UBYTE green;
    UBYTE blue;
};

struct mb2_tag_framebuffer
{
  struct mb2_tag_framebuffer_common common;

  union
  {
    struct
    {
        UWORD framebuffer_num_colors;
        struct fb_color framebuffer_palette[0];
    };
    struct
    {
        UBYTE framebuffer_red_field_position;
        UBYTE framebuffer_red_mask_size;
        UBYTE framebuffer_green_field_position;
        UBYTE framebuffer_green_mask_size;
        UBYTE framebuffer_blue_field_position;
        UBYTE framebuffer_blue_mask_size;
    };
  };
};

struct mb2_tag_elf_sections
{
    ULONG type;
    ULONG size;
    ULONG num;
    ULONG entsize;
    ULONG shndx;
    BYTE sections[0];
};

struct mb2_tag_apm
{
    ULONG type;
    ULONG size;
    UWORD version;
    UWORD cseg;
    ULONG offset;
    UWORD cseg_16;
    UWORD dseg;
    UWORD flags;
    UWORD cseg_len;
    UWORD cseg_16_len;
    UWORD dseg_len;
};

struct mb2_tag_efi32
{
    ULONG type;
    ULONG size;
    ULONG pointer;
};

struct mb2_tag_efi64
{
    ULONG type;
    ULONG size;
    UQUAD pointer;
};

struct mb2_tag_smbios
{
    ULONG type;
    ULONG size;
    UBYTE major;
    UBYTE minor;
    UBYTE reserved[6];
    UBYTE tables[0];
};

struct mb2_tag_old_acpi
{
    ULONG type;
    ULONG size;
    UBYTE rsdp[0];
};

struct mb2_tag_new_acpi
{
    ULONG type;
    ULONG size;
    UBYTE rsdp[0];
};

struct mb2_tag_network
{
    ULONG type;
    ULONG size;
    UBYTE dhcpack[0];
};

#endif
