#ifndef _MB_H
#define _MB_H

/*
    Copyright C 2002 AROS - The Amiga Research OS
    $Id$
 
    Desc: Multiboot information structures
    Lang: english
*/

#include <exec/types.h>


/* Structure passed from bootloader */
struct multiboot {
    ULONG   flags;
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
    ULONG   mem_lower;
    ULONG   mem_upper;
    ULONG   bootdev;
    STRPTR  cmdline;
    ULONG   mods_count;
    ULONG   mods_addr;
    ULONG   elf_num;
    ULONG   elf_size;
    ULONG   elf_addr;
    ULONG   elf_shndx;
    ULONG   mmap_length;
    ULONG   mmap_addr;
    ULONG   drives_length;
    ULONG   drives_addr;
    ULONG   config_table;
    STRPTR  loader_name;
    ULONG   apm_table;
    ULONG   vbe_control_info;
    ULONG   vbe_mode_info;
    UWORD   vbe_mode;
    UWORD   vbe_if_seg;
    UWORD   vbe_if_off;
    UWORD   vbe_if_len;
};

struct mb_mmap {
    ULONG   size;
    ULONG   addr_low;
    ULONG   addr_high;
    ULONG   len_low;
    ULONG   len_high;
    ULONG   type;
#define MMAP_TYPE_RAM	    1
#define MMAP_TYPE_RESERVED  2
#define MMAP_TYPE_ACPIDATA  3
#define MMAP_TYPE_ACPINVS   4
};

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
    ULONG   magic;		    /* Indicates if information is valid */
#define MBRAM_VALID	0x1337BABE
    ULONG   flags;		    /* Copy of the multiboot flags */
    ULONG   mem_lower;		    /* Amount of lowmem (Sub 1Mb) */
    ULONG   mem_upper;		    /* Amount of upper memory */
    ULONG   mmap_addr;		    /* Pointer to memory map */
    ULONG   mmap_len;		    /* size of memory map */
    ULONG   drives_addr;	    /* Pointer to drive information */
    ULONG   drives_len;		    /* Size of drive information */
    char    ldrname[30];	    /* String of loadername */
    char    cmdline[200];	    /* Commandline */
    UWORD   vbe_mode;		    /* VBE mode */
    UBYTE   vbe_palette_width;	    /* VBE palette width */
    struct vbe_mode vmi;            /* VBE mode information */
    struct vbe_controller vci;      /* VBE controller information */
};

struct mb_drive {
    ULONG   size;
    UBYTE   number;
    UBYTE   mode;
#define MB_MODE_CHS 0
#define MB_MODE_LBA 1
    UWORD   cyls;
    UBYTE   heads;
    UBYTE   secs;
    UWORD   ports[10];		    /* Ugly, needs to be fixed */
};

#endif /* _MB_H */
