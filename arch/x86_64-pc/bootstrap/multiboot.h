#ifndef _MB_H
#define _MB_H

/*
    Copyright C 2002 AROS - The Amiga Research OS
    $Id: multiboot.h 16879 2003-03-27 10:06:40Z jogr0326 $
 
    Desc: Multiboot information structures
    Lang: english
*/


/* Structure passed from bootloader */
struct multiboot {
    unsigned int   flags;
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
    unsigned int   mem_lower;
    unsigned int   mem_upper;
    unsigned int   bootdev;
    char *  cmdline;
    unsigned int   mods_count;
    unsigned int   mods_addr;
    unsigned int   elf_num;
    unsigned int   elf_size;
    unsigned int   elf_addr;
    unsigned int   elf_shndx;
    unsigned int   mmap_length;
    unsigned int   mmap_addr;
    unsigned int   drives_length;
    unsigned int   drives_addr;
    unsigned int   config_table;
    char *loader_name;
    unsigned int   apm_table;
    unsigned int   vbe_control_info;
    unsigned int   vbe_mode_info;
    unsigned short   vbe_mode;
    unsigned short   vbe_if_seg;
    unsigned short   vbe_if_off;
    unsigned short   vbe_if_len;
};

struct mb_module {
    unsigned long   mod_start;
    unsigned long   mod_end;
    unsigned long   string;
    unsigned long   reserved;
};

struct mb_mmap {
    unsigned int   size;
    unsigned int   addr_low;
    unsigned int   addr_high;
    unsigned int   len_low;
    unsigned int   len_high;
    unsigned int   type;
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
    unsigned int   magic;		    /* Indicates if information is valid */
#define MBRAM_VALID	0x1337BABE
    unsigned int   flags;		    /* Copy of the multiboot flags */
    unsigned int   mem_lower;		    /* Amount of lowmem (Sub 1Mb) */
    unsigned int   mem_upper;		    /* Amount of upper memory */
    unsigned int   mmap_addr;		    /* Pointer to memory map */
    unsigned int   mmap_len;		    /* size of memory map */
    unsigned int   drives_addr;	    /* Pointer to drive information */
    unsigned int   drives_len;		    /* Size of drive information */
    char    ldrname[30];	    /* String of loadername */
    char    cmdline[200];	    /* Commandline */
    unsigned short   vbe_mode;		    /* VBE mode */
    struct vbe_mode vmi;            /* VBE mode information */
    struct vbe_controller vci;      /* VBE controller information */
};

struct mb_drive {
    unsigned int   size;
    unsigned char   number;
    unsigned char   mode;
#define MB_MODE_CHS 0
#define MB_MODE_LBA 1
    unsigned short   cyls;
    unsigned char   heads;
    unsigned char   secs;
    unsigned short   ports[10];		    /* Ugly, needs to be fixed */
};

#endif /* _MB_H */
