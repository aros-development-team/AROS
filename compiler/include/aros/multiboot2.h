/*
    Copyright © 2011, The AROS Development Team. All rights reserved.
    $Id: multiboot.h 37698 2011-03-21 07:05:51Z sonic $
 
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
    unsigned int magic;	   /* MB2_MAGIC					    */
    unsigned int arch;	   /* Architecture - see below			    */
    unsigned int size;	   /* Total size of the header (including all tags) */
    unsigned int checksum; /* Header checksum				    */
    /* Tags follow */
};

#define MB2_MAGIC 0xe85250d6

/* Architecture */
#define MB2_ARCH_I386   0
#define MB2_ARCH_MIPS32 4

struct mb2_header_tag
{
    unsigned short type;  /* Tag ID			      */
    unsigned short flags; /* Flags, see below		      */
    unsigned int   size;  /* Size of this tag, including data */
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
    unsigned short type;		/* MB2_HEADER_TAG_INFORMATION_REQUEST */
    unsigned short flags;
    unsigned int   size;
    unsigned int   requests[0];
};

struct mb2_header_tag_address
{
    unsigned short type;		/* MB2_HEADER_TAG_ADDRESS */
    unsigned short flags;
    unsigned int   size;
    unsigned int   header_addr;
    unsigned int   load_addr;
    unsigned int   load_end_addr;
    unsigned int   bss_end_addr;
};

struct mb2_header_tag_entry_address
{
    unsigned short type;		/* MB2_HEADER_TAG_ENTRY_ADDRESS */
    unsigned short flags;
    unsigned int   size;
    unsigned int   entry_addr;
};

struct mb2_header_tag_console_flags
{
    unsigned short type;		/* MB2_HEADER_TAG_CONSOLE_FLAGS */
    unsigned short flags;
    unsigned int   size;
    unsigned int   console_flags;
};

struct mb2_header_tag_framebuffer
{
    unsigned short type;		/* MB2_HEADER_TAG_FRAMEBUFFER */
    unsigned short flags;
    unsigned int   size;
    unsigned int   width;
    unsigned int   height;
    unsigned int   depth;
};

struct mb2_header_tag_module_align
{
    unsigned short type;		/* MB2_HEADER_TAG_MODULE_ALIGN */
    unsigned short flags;
    unsigned int   size;
    unsigned int   width;
    unsigned int   height;
    unsigned int   depth;
};

/* This value is passed in by the bootloader */
#define MB2_STARTUP_MAGIC 0x36d76289

/*
 * Multiboot v2 data is an UQUAD value representing overall length,
 * followed by a sequence of tags.
 * This is the generalized form of a tag.
 */
struct mb2_tag
{
    unsigned int type;	/* ID				*/
    unsigned int size;	/* Data size (including header)	*/
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
    unsigned int type;
    unsigned int size;
    char         string[0];
};

struct mb2_tag_module
{
    unsigned int type;
    unsigned int size;
    unsigned int mod_start;
    unsigned int mod_end;
    char         cmdline[0];
};

struct mb2_tag_basic_meminfo
{
    unsigned int type;
    unsigned int size;
    unsigned int mem_lower;
    unsigned int mem_upper;
};

struct mb2_tag_bootdev
{
    unsigned int type;
    unsigned int size;
    unsigned int biosdev;
    unsigned int slice;
    unsigned int part;
};

struct mb2_mmap
{
#ifdef MULTIBOOT_64BIT
    unsigned long long addr;	  /* Address end length, 64-bit */
    unsigned long long len;
#else
    unsigned int       addr;
    unsigned int       addr_high;
    unsigned int       len;
    unsigned int       len_high;
#endif
    unsigned int       type;	 /* Entry type, see below	*/
    unsigned int       pad;	 /* Reserved			*/
};

/* Memory map entry types */
#define MMAP2_TYPE_RAM	     1	/* General purpose RAM  */
#define MMAP2_TYPE_RESERVED  2	/* System private areas */
#define MMAP2_TYPE_ACPIDATA  3  /* ACPI data structures */
#define MMAP2_TYPE_ACPINVS   4
#define MMAP2_TYPE_BAD	     5	/* Broken RAM		*/

struct mb2_tag_mmap
{
    unsigned int    type;
    unsigned int    size;
    unsigned int    entry_size;
    unsigned int    entry_version;
    struct mb2_mmap mmap[0];
};

struct mb2_tag_vbe
{
    unsigned int   type;
    unsigned int   size;

    unsigned short vbe_mode;
    unsigned short vbe_interface_seg;
    unsigned short vbe_interface_off;
    unsigned short vbe_interface_len;

    struct vbe_controller vbe_control_info;
    struct vbe_mode       vbe_mode_info;
};

struct mb2_tag_framebuffer_common
{
    unsigned int       type;			/* Tag ID				*/
    unsigned int       size;
#ifdef MULTIBOOT_64BIT
    unsigned long long framebuffer_addr;	/* Framebuffer address, 64-bit pointer	*/
#else
    unsigned int       framebuffer_addr;
    unsigned int       framebuffer_addr_high;
#endif
    unsigned int       framebuffer_pitch;	/* Bytes per line			*/
    unsigned int       framebuffer_width;	/* Size in pixels or characters		*/
    unsigned int       framebuffer_height;
    unsigned char      framebuffer_bpp;		/* Bits per pixel			*/
    unsigned char      framebuffer_type;	/* See below				*/
    unsigned short     reserved;
};

/* Framebuffer types */
#define MB2_FRAMEBUFFER_LUT  0
#define MB2_FRAMEBUFFER_RGB  1
#define MB2_FRAMEBUFFER_TEXT 2

struct fb_color
{
    unsigned char red;
    unsigned char green;
    unsigned char blue;
};

struct mb2_tag_framebuffer
{
  struct mb2_tag_framebuffer_common common;

  union
  {
    struct
    {
        unsigned short framebuffer_num_colors;
        struct fb_color framebuffer_palette[0];
    };
    struct
    {
        unsigned char framebuffer_red_field_position;
        unsigned char framebuffer_red_mask_size;
        unsigned char framebuffer_green_field_position;
        unsigned char framebuffer_green_mask_size;
        unsigned char framebuffer_blue_field_position;
        unsigned char framebuffer_blue_mask_size;
    };
  };
};

struct mb2_tag_elf_sections
{
    unsigned int type;
    unsigned int size;
    unsigned int num;
    unsigned int entsize;
    unsigned int shndx;
    char sections[0];
};

struct mb2_tag_apm
{
    unsigned int   type;
    unsigned int   size;
    unsigned short version;
    unsigned short cseg;
    unsigned int   offset;
    unsigned short cseg_16;
    unsigned short dseg;
    unsigned short flags;
    unsigned short cseg_len;
    unsigned short cseg_16_len;
    unsigned short dseg_len;
};

struct mb2_tag_efi32
{
    unsigned int type;
    unsigned int size;
    unsigned int pointer;
};

struct mb2_tag_efi64
{
    unsigned int       type;
    unsigned int       size;
    unsigned long long pointer;
};

struct mb2_tag_smbios
{
    unsigned int  type;
    unsigned int  size;
    unsigned char major;
    unsigned char minor;
    unsigned char reserved[6];
    unsigned char tables[0];
};

struct mb2_tag_old_acpi
{
    unsigned int  type;
    unsigned int  size;
    unsigned char rsdp[0];
};

struct mb2_tag_new_acpi
{
    unsigned int  type;
    unsigned int  size;
    unsigned char rsdp[0];
};

struct mb2_tag_network
{
    unsigned int  type;
    unsigned int  size;
    unsigned char dhcpack[0];
};

#endif
