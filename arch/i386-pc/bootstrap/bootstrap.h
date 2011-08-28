/*
 * bootstrap.h
 *
 *  Created on: Dec 1, 2009
 *      Author: misc
 */

#ifndef BOOTSTRAP_H_
#define BOOTSTRAP_H_

#include <aros/kernel.h>
#include <sys/types.h>

#if DEBUG
#define D(x) x
#else
#define D(x) /* eps */
#endif

#define BOOT_STACK_SIZE 65536
#define BOOT_TMP_SIZE	 524288

/*
 * Multiboot stuff
 */
typedef struct {
    unsigned int   magic;
    unsigned int   flags;
    unsigned int   chksum;
} multiboot_header;

#define MB_MAGIC    0x1BADB002
#define MB_FLAGS    0x00000003


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
    unsigned int    mod_start;
    unsigned int    mod_end;
    unsigned int    string;
    unsigned int    reserved;
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
    unsigned int  oem_string;
    unsigned int  capabilities;
    unsigned int  video_mode;
    unsigned short total_memory;
    unsigned short oem_software_rev;
    unsigned int  oem_vendor_name;
    unsigned int oem_product_name;
    unsigned int oem_product_rev;
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
    unsigned int win_func;
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
    unsigned int phys_base;
    unsigned int reserved1;
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
    unsigned int max_pixel_clock;

    unsigned char reserved3[189];
} __attribute__ ((packed));


#define rdcr(reg) \
    ({ long val; asm volatile("mov %%" #reg ",%0":"=r"(val)); val; })

#define wrcr(reg, val) \
    do { asm volatile("mov %0,%%" #reg::"r"(val)); } while(0)

#define _STR(x) # x
#define STR(x) _STR(x)

#define MAX_BSS_SECTIONS	256

size_t mem_avail();
size_t mem_used();

#endif /* BOOTSTRAP_H_ */
