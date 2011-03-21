/*
    Copyright © 2002-2011, The AROS Development Team. All rights reserved.
    $Id$
 
    Desc: Multiboot information structures
    Lang: english
*/

#ifndef _MB_H
#define _MB_H

#include <aros/cpu.h> /* For __WORDSIZE */

#if (__WORDSIZE == 64)
#ifndef MULTIBOOT_64BIT

/*
 * Define this in your code if you want to build 32-bit code using full 64-bit pointers.
 * Useful for building pc-x86_64 bootstrap which runs in 32 bit mode.
 */
#define MULTIBOOT_64BIT

#endif
#endif

struct multiboot_header
{
    unsigned int magic;		/* MB_MAGIC	*/
    unsigned int flags;		/* See below	*/
    unsigned int chksum;

    unsigned int header_addr;
    unsigned int load_addr;
    unsigned int load_end_addr;
    unsigned int bss_end_addr;
    unsigned int entry_addr;

    /* Preferred video mode */
    unsigned int mode_type;
    unsigned int width;
    unsigned int height;
    unsigned int depth;
};

#define MB_MAGIC 0x1BADB002

/* Header flags */
#define MB_PAGE_ALIGN  0x00000001	/* Align modules to page boundary		*/
#define MB_MEMORY_INFO 0x00000002	/* We request memory information		*/
#define MB_VIDEO_MODE  0x00000004	/* We specify preferred video mode information	*/
#define MB_AOUT_KLUDGE 0x00010000

/* Structure passed from bootloader */
struct multiboot
{
    unsigned int  	flags;			/* See below					*/
    unsigned int  	mem_lower;		/* Low memory size				*/
    unsigned int  	mem_upper;		/* Upper memory size				*/
    unsigned int  	bootdev;		/* Boot device, int13h encoding			*/
    unsigned int  	cmdline;		/* 32-bit pointer to a command line string	*/
    unsigned int  	mods_count;		/* Number of modules				*/
    unsigned int  	mods_addr;		/* 32-bit pointer to module descriptors table	*/
    unsigned int  	elf_num;		/* Copy of ELF section header			*/
    unsigned int  	elf_size;
    unsigned int  	elf_addr;
    unsigned int  	elf_shndx;
    unsigned int  	mmap_length;		/* Length of memory map in bytes		*/
    unsigned int  	mmap_addr;		/* 32-bit pointer to memory map			*/
    unsigned int  	drives_length;		/* Size of drives table in bytes		*/
    unsigned int  	drives_addr;		/* 32-bit pointer to drives table		*/
    unsigned int  	config_table;		/* 32-bit pointer to ROM configuration table	*/
    unsigned int  	loader_name;		/* 32-bit pointer to bootloader name string	*/
    unsigned int  	apm_table;		/* 32-bit pointer to APM data table		*/
    unsigned int  	vbe_control_info;	/* 32-bit pointer to VESA controller descriptor	*/
    unsigned int  	vbe_mode_info;		/* 32-bit pointer to VESA mode descriptor	*/
    unsigned short	vbe_mode;		/* Current VESA video mode			*/
    unsigned short	vbe_if_seg;		/* VBE protected mode interface			*/
    unsigned short	vbe_if_off;
    unsigned short	vbe_if_len;
#ifdef MULTIBOOT_64BIT
    unsigned long long	framebuffer_addr;	/* Framebuffer address, 64-bit pointer		*/
#else
    unsigned int   framebuffer_addr_low;
    unsigned int   framebuffer_addr_high;
#define framebuffer_addr framebuffer_addr_low
#endif
    unsigned int   framebuffer_pitch;
    unsigned int   framebuffer_width;
    unsigned int   framebuffer_height;
    unsigned char  framebuffer_bpp;
    unsigned char  framebuffer_type;
    union
    {
    	struct
    	{
      	    unsigned int   framebuffer_palette_addr;
      	    unsigned short framebuffer_palette_num_colors;
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

/* flags */
#define MB_FLAGS_MEM	    1
#define MB_FLAGS_BOOTDEV    2
#define MB_FLAGS_CMDLINE    4
#define MB_FLAGS_MODS	    8
#define MB_FLAGS_AOUT	    16
#define MB_FLAGS_ELF	    32
#define MB_FLAGS_MMAP	    64
#define MB_FLAGS_DRIVES	    128
#define MB_FLAGS_CFGTBL	    256
#define MB_FLAGS_LDRNAME    512
#define MB_FLAGS_APMTBL	    1024
#define MB_FLAGS_GFX	    2048
#define MB_FLAGS_FB	    4096

/* framebuffer_type */
#define MB_FRAMEBUFFER_LUT  0
#define MB_FRAMEBUFFER_RGB  1
#define MB_FRAMEBUFFER_TEXT 2

#ifdef MULTIBOOT_64BIT

struct mb_mmap
{
    unsigned int       size;
    unsigned long long addr;
    unsigned long long len;
    unsigned int       type;
} __attribute((packed));

#else

struct mb_mmap
{
    unsigned int   size;
    unsigned int   addr_low;
    unsigned int   addr_high;
    unsigned int   len_low;
    unsigned int   len_high;
    unsigned int   type;
};

#define addr addr_low
#define len  len_low

#endif

#define MMAP_TYPE_RAM	    1	/* General purpose RAM */
#define MMAP_TYPE_RESERVED  2
#define MMAP_TYPE_ACPIDATA  3
#define MMAP_TYPE_ACPINVS   4

/* VBE controller information */
struct vbe_controller
{
    unsigned char signature[4];
    unsigned short version;
    unsigned long oem_string;
    unsigned long capabilities;
    unsigned long video_mode;
    unsigned short total_memory;
    unsigned short oem_software_rev;
    unsigned long oem_vendor_name;
    unsigned long oem_product_name;
    unsigned long oem_product_rev;
    unsigned char reserved[222];
    unsigned char oem_data[256];
} __attribute__ ((packed));

/* VBE mode information.  */
struct vbe_mode 
{ 
    unsigned short mode_attributes;
    unsigned char win_a_attributes;
    unsigned char win_b_attributes;
    unsigned short win_granularity;
    unsigned short win_size;
    unsigned short win_a_segment;
    unsigned short win_b_segment;
    unsigned long win_func;
    unsigned short bytes_per_scanline;

    /* >=1.2 */
    unsigned short x_resolution;
    unsigned short y_resolution;
    unsigned char x_char_size;
    unsigned char y_char_size;
    unsigned char number_of_planes;
    unsigned char bits_per_pixel;
    unsigned char number_of_banks;
    unsigned char memory_model;
    unsigned char bank_size;
    unsigned char number_of_image_pages;
    unsigned char reserved0;

    /* direct color */
    unsigned char red_mask_size;
    unsigned char red_field_position;
    unsigned char green_mask_size;
    unsigned char green_field_position;
    unsigned char blue_mask_size;
    unsigned char blue_field_position;
    unsigned char reserved_mask_size;
    unsigned char reserved_field_position;
    unsigned char direct_color_mode_info;

    /* >=2.0 */
    unsigned long phys_base;
    unsigned long reserved1;
    unsigned short reversed2;

    /* >=3.0 */
    unsigned short linear_bytes_per_scanline;
    unsigned char banked_number_of_image_pages;
    unsigned char linear_number_of_image_pages;
    unsigned char linear_red_mask_size;
    unsigned char linear_red_field_position;
    unsigned char linear_green_mask_size;
    unsigned char linear_green_field_position;
    unsigned char linear_blue_mask_size;
    unsigned char linear_blue_field_position;
    unsigned char linear_reserved_mask_size;
    unsigned char linear_reserved_field_position;
    unsigned long max_pixel_clock;

    unsigned char reserved3[189];
} __attribute__ ((packed));


/* Structure in RAM at 0x1000 */
struct arosmb {
    unsigned int   magic;		    /* Indicates if information is valid */
    unsigned int   flags;		    /* Copy of the multiboot flags */
    unsigned int   mem_lower;		    /* Amount of lowmem (Sub 1Mb) */
    unsigned int   mem_upper;		    /* Amount of upper memory */
    unsigned int   mmap_addr;		    /* Pointer to memory map */
    unsigned int   mmap_len;		    /* size of memory map */
    unsigned int   drives_addr;	    	    /* Pointer to drive information */
    unsigned int   drives_len;		    /* Size of drive information */
    char    	   ldrname[30];	    	    /* String of loadername */
    char    	   cmdline[200];	    /* Commandline */
    unsigned short vbe_mode;		    /* VBE mode */
    unsigned char  vbe_palette_width;	    /* VBE palette width */
    struct vbe_mode       vmi;              /* VBE mode information */
    struct vbe_controller vci;		    /* VBE controller information */
    unsigned long  acpirsdp;
    unsigned int   acpilength;
};

#define MBRAM_VALID	0x1337BABE

struct mb_drive
{
    unsigned int   size;
    unsigned char  number;
    unsigned char  mode;
    unsigned short cyls;
    unsigned char  heads;
    unsigned char  secs;
    unsigned short ports[10];		    /* Ugly, needs to be fixed */
};

/* Drive mode */
#define MB_MODE_CHS 0
#define MB_MODE_LBA 1

struct mb_module {	/* multiboot_mod_list - multiboot_module_t */
    unsigned int mod_start;	/* from bytes */
    unsigned int mod_end;	/* to 'mod_end-1' inclusive */
    unsigned int cmdline;	/* Module command line */
    unsigned int pad;		/* padding to take it to 16 bytes (must be zero) */
};

#endif /* _MB_H */
