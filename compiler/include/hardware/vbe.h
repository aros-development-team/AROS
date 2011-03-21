/*
    Copyright © 2002-2011, The AROS Development Team. All rights reserved.
    $Id$
 
    Desc: VESA BIOS information structures
    Lang: english
*/

#ifndef HARDWARE_VBE_H
#define HARDWARE_VBE_H

/* VBE controller information */
struct vbe_controller
{
    unsigned char  signature[4];	/* 'VESA' string				*/
    unsigned short version;		/* VBE version number				*/
    unsigned long  oem_string;		/* 16-bit far pointer to OEM string pointer	*/
    unsigned long  capabilities;	/* Capabilities flags				*/
    unsigned long  video_mode;		/* 16-bit far pointer to video mode table	*/
    unsigned short total_memory;	/* Total amount of VRAM				*/
    unsigned short oem_software_rev;	/* ROM version number				*/
    unsigned long  oem_vendor_name;	/* 16-bit far pointer				*/
    unsigned long  oem_product_name;	/* 16-bit far pointer to name string		*/
    unsigned long  oem_product_rev;	/* 16-bit far pointer to version string		*/
    unsigned char  reserved[222];	/* BIOS scratchpad				*/
    unsigned char  oem_data[256];	/* Strings can be copied here			*/
} __attribute__ ((packed));

/* VBE mode information.  */
struct vbe_mode 
{ 
    unsigned short mode_attributes;
    unsigned char  win_a_attributes;
    unsigned char  win_b_attributes;
    unsigned short win_granularity;
    unsigned short win_size;
    unsigned short win_a_segment;
    unsigned short win_b_segment;
    unsigned long  win_func;
    unsigned short bytes_per_scanline;

    /* >=1.2 */
    unsigned short x_resolution;
    unsigned short y_resolution;
    unsigned char  x_char_size;
    unsigned char  y_char_size;
    unsigned char  number_of_planes;
    unsigned char  bits_per_pixel;
    unsigned char  number_of_banks;
    unsigned char  memory_model;
    unsigned char  bank_size;
    unsigned char  number_of_image_pages;
    unsigned char  reserved0;

    /* direct color */
    unsigned char  red_mask_size;
    unsigned char  red_field_position;
    unsigned char  green_mask_size;
    unsigned char  green_field_position;
    unsigned char  blue_mask_size;
    unsigned char  blue_field_position;
    unsigned char  reserved_mask_size;
    unsigned char  reserved_field_position;
    unsigned char  direct_color_mode_info;

    /* >=2.0 */
    unsigned long  phys_base;
    unsigned long  reserved1;
    unsigned short reversed2;

    /* >=3.0 */
    unsigned short linear_bytes_per_scanline;
    unsigned char  banked_number_of_image_pages;
    unsigned char  linear_number_of_image_pages;
    unsigned char  linear_red_mask_size;
    unsigned char  linear_red_field_position;
    unsigned char  linear_green_mask_size;
    unsigned char  linear_green_field_position;
    unsigned char  linear_blue_mask_size;
    unsigned char  linear_blue_field_position;
    unsigned char  linear_reserved_mask_size;
    unsigned char  linear_reserved_field_position;
    unsigned long  max_pixel_clock;

    unsigned char reserved3[189];
} __attribute__ ((packed));

#endif
