/*
    Copyright © 2002-2011, The AROS Development Team. All rights reserved.
    $Id$
 
    Desc: VESA BIOS information structures
    Lang: english
*/

#ifndef HARDWARE_VBE_H
#define HARDWARE_VBE_H

/*
 * VESA BIOS return code. Returned in AX register.
 * This comes in the lower half (AL) and indicates that the upper half
 * (AH) is valid.
 */
#define VBE_RC_SUPPORTED 0x004F

/* Upper half (contents of AH, shifted by 8) */
#define VBE_RC_OK	 0x0000	/* All went OK								*/
#define VBE_RC_FAIL	 0x0200 /* Call failed								*/
#define VBE_RC_NOHW	 0x0200 /* The given parameters are not supported by the hardware		*/
#define VBE_RC_NOMODE	 0X0300	/* The given parameters are not supported by the current video mode	*/

/* VBE controller information */
struct vbe_controller
{
    unsigned char  signature[4];	/* 'VESA' string				*/
    unsigned short version;		/* VBE version number				*/
    unsigned int   oem_string;		/* 16-bit far pointer to OEM string pointer	*/
    unsigned int   capabilities;	/* Capabilities flags, see below		*/
    unsigned int   video_mode;		/* 16-bit far pointer to video mode table	*/
    unsigned short total_memory;	/* Total amount of VRAM				*/
    unsigned short oem_software_rev;	/* ROM version number				*/
    unsigned int   oem_vendor_name;	/* 16-bit far pointer to hardware vendor string	*/
    unsigned int   oem_product_name;	/* 16-bit far pointer to name string		*/
    unsigned int   oem_product_rev;	/* 16-bit far pointer to version string		*/
    unsigned char  reserved[222];	/* BIOS scratchpad				*/
    unsigned char  oem_data[256];	/* Strings can be copied here			*/
} __attribute__ ((packed));

/*
 * A macro for transforming 16-bit far pointers (segment:offset)
 * into 32-bit linear addresses
 */
#define GET_FAR_PTR(x) ((void *)((x & 0xffff0000) >> 12) + (x & 0xffff))

/* Controller capabilities flags */
#define VC_PALETTE_WIDTH 0x0001	/* Can change palette width to 8			*/
#define VC_NO_VGA_HW	 0x0002 /* The hardware is not VGA-compatible			*/
#define VC_RAMDAC_BLANK	 0x0004	/* Use blank bit for function 09			*/
#define VC_STEREO	 0x0008	/* Stereoscopic signalling is supported			*/
#define VC_STEREO_EVC	 0x0010 /* EVC connector is used for stereoscopic signalling	*/

/* Mode number format for setting video mode */
#define VBE_MODE_NUMBER_MASK 0x01FF	/* Mask for actual mode number		*/
#define VBE_MODE_STANDARD    0x0100	/* This is VESA standard mode		*/
#define VBE_MODE_CUSTOM_RATE 0x0800	/* Custom refresh rate is supplied	*/
#define VBE_MODE_LINEAR_FB   0x4000	/* Select linear framebuffer mode	*/
#define VBE_MODE_KEEP	     0x8000	/* Do not clear video memory		*/

/* VBE mode information.  */
struct vbe_mode 
{ 
    unsigned short mode_attributes;	    /* Mode flags, see below				*/
    unsigned char  win_a_attributes;
    unsigned char  win_b_attributes;
    unsigned short win_granularity;
    unsigned short win_size;
    unsigned short win_a_segment;
    unsigned short win_b_segment;
    unsigned int   win_func;
    unsigned short bytes_per_scanline;	    /* Number of bytes per line				*/

    /* >=1.2 */
    unsigned short x_resolution;	    /* Number of pixels (or characters) per line	*/
    unsigned short y_resolution;	    /* Number of pixels (or characters) per column	*/
    unsigned char  x_char_size;
    unsigned char  y_char_size;
    unsigned char  number_of_planes;
    unsigned char  bits_per_pixel;	    /* Number of bits per pixel				*/
    unsigned char  number_of_banks;
    unsigned char  memory_model;	    /* Memory model type, see below			*/
    unsigned char  bank_size;
    unsigned char  number_of_image_pages;
    unsigned char  reserved0;

    /* direct color */
    unsigned char  red_mask_size;	    /* Number of bits for the color			*/
    unsigned char  red_field_position;	    /* LSB bit position of the color mask		*/
    unsigned char  green_mask_size;	    /* Number of bits in the color mask			*/
    unsigned char  green_field_position;
    unsigned char  blue_mask_size;
    unsigned char  blue_field_position;
    unsigned char  reserved_mask_size;
    unsigned char  reserved_field_position;
    unsigned char  direct_color_mode_info;  /* Direct color mode flags, see below		*/

    /* >=2.0 */
    unsigned int   phys_base;		    /* Linear framebuffer address (32-bit)		*/
    unsigned int   reserved1;
    unsigned short reserved2;

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
    unsigned int   max_pixel_clock;

    unsigned char reserved3[189];
} __attribute__ ((packed));

/* Video mode flags */
#define VM_SUPPORTED	0x0001 /* The mode is supported by the hardware			*/
#define VM_BIOSTTY	0x0004 /* The mode is supported by BIOS TTY functions		*/
#define VM_COLOR	0x0008 /* The mode is color; monochrome if unset		*/
#define VM_GRAPHICS	0x0010 /* The mode is graphical; text if unset			*/
#define VM_NO_VGA_HW	0x0020 /* VGA hardware registers are available for this mode	*/
#define VM_NO_VGA_MEM	0x0040 /* VGA memory window is available for this mode		*/
#define VM_LINEAR_FB	0x0080 /* Linear framebuffer is available for this mode		*/
#define VM_DBLSCAN	0x0200 /* Double-scan is available for this mode		*/
#define VM_LACE		0x0200 /* Interlace is available for this mode			*/
#define VM_TPLBUF	0x0400 /* Triple-buffering is supported for this mode		*/
#define VM_STEREO	0x0800 /* Stereoscopic display is supported for this mode	*/
#define VM_DUALDISP	0x1000 /* Dual display start address is supported		*/

/* Memory model types */
#define VMEM_TEXT     0x00 /* Text mode			*/
#define VMEM_CGA      0x01 /* CGA graphics		*/
#define VMEM_HERCULES 0x02 /* Hercules			*/
#define VMEM_PLANAR   0x03 /* Planar			*/
#define VMEM_PACKED   0x04 /* Packet pixel		*/
#define VMEM_NONCHAIN 0x05 /* Non-chained 4,256 color	*/
#define VMEM_RGB      0x06 /* Direct-color RGB		*/
#define VMEM_YUV      0x07 /* Direct-color YUV		*/

/* Direct color mode flags */
#define DAC_RAMP_PROGRAMMABLE 0x01 /* DAC ramp can be programmed		*/
#define USER_RESERVED_FIELD   0x02 /* Reserved fields are available for user	*/

#endif
