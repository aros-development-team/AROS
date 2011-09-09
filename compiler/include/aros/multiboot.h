/*
    Copyright © 2002-2011, The AROS Development Team. All rights reserved.
    $Id$
 
    Desc: Multiboot information structures
    Lang: english
*/

#ifndef _MB_H
#define _MB_H

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

/* This is passed in by the bootloader */
#define MB_STARTUP_MAGIC 0x2BADB002

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
    unsigned int	framebuffer_addr;
    unsigned int   	framebuffer_addr_high;
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

struct mb_mmap
{
    unsigned int       size;	  /* Entry size (not including this field) */
#ifdef MULTIBOOT_64BIT
    unsigned long long addr;	  /* Full 64-bit address and length	   */
    unsigned long long len;
#else
    unsigned int       addr;
    unsigned int       addr_high;
    unsigned int       len;
    unsigned int       len_high;
#endif
    unsigned int       type;	 /* Entry type, see below		   */
} __attribute((packed));

/* Memory map entry types */
#define MMAP_TYPE_RAM	    1	/* General purpose RAM */
#define MMAP_TYPE_RESERVED  2
#define MMAP_TYPE_ACPIDATA  3
#define MMAP_TYPE_ACPINVS   4

/* Disk drive information from PC BIOS */
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

/* Modules list */
struct mb_module
{
    unsigned int mod_start;	/* from bytes */
    unsigned int mod_end;	/* to 'mod_end-1' inclusive */
    unsigned int cmdline;	/* Module command line */
    unsigned int pad;		/* padding to take it to 16 bytes (must be zero) */
};

#endif /* _MB_H */
