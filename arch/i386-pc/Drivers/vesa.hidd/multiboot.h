#ifndef _MB_H
#define _MB_H

/*
    Copyright C 2002 AROS - The Amiga Research OS
    $Id$
 
    Desc: Multiboot information structures
    Lang: english
*/

#include <exec/types.h>

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

/* VBE controller information.  */
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
    struct vbe_controller vci;	    /* VBE controller information */
    struct vbe_mode vmi;            /* VBE mode information */
};

#endif /* _MB_H */
