/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2005  Free Software Foundation, Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef GRUB_VBE_MACHINE_HEADER
#define GRUB_VBE_MACHINE_HEADER	1

#include <grub/symbol.h>
#include <grub/types.h>
#include <grub/err.h>

/* Default video mode to be used.  */
#define GRUB_VBE_DEFAULT_VIDEO_MODE     0x101

/* Note:
 
   Please refer to VESA BIOS Extension 3.0 Specification for more descriptive
   meanings of following structures and how they should be used.
  
   I have tried to maintain field name comatibility against specification 
   while following naming convetions used in GRUB.  */

typedef grub_uint32_t grub_vbe_farptr_t;
typedef grub_uint32_t grub_vbe_physptr_t;
typedef grub_uint32_t grub_vbe_status_t;

struct grub_vbe_info_block
{
  grub_uint8_t signature[4];
  grub_uint16_t version;

  grub_vbe_farptr_t oem_string_ptr;
  grub_uint32_t capabilities;
  grub_vbe_farptr_t video_mode_ptr;
  grub_uint16_t total_memory;

  grub_uint16_t oem_software_rev;
  grub_vbe_farptr_t oem_vendor_name_ptr;
  grub_vbe_farptr_t oem_product_name_ptr;
  grub_vbe_farptr_t oem_product_rev_ptr;

  grub_uint8_t reserved[222];

  grub_uint8_t oem_data[256];
} __attribute__ ((packed));

struct grub_vbe_mode_info_block
{
  /* Mandory information for all VBE revisions.  */
  grub_uint16_t mode_attributes;
  grub_uint8_t win_a_attributes;
  grub_uint8_t win_b_attributes;
  grub_uint16_t win_granularity;
  grub_uint16_t win_size;
  grub_uint16_t win_a_segment;
  grub_uint16_t win_b_segment;
  grub_vbe_farptr_t win_func_ptr;
  grub_uint16_t bytes_per_scan_line;

  /* Mandory information for VBE 1.2 and above.  */
  grub_uint16_t x_resolution;
  grub_uint16_t y_resolution;
  grub_uint8_t x_char_size;
  grub_uint8_t y_char_size;
  grub_uint8_t number_of_planes;
  grub_uint8_t bits_per_pixel;
  grub_uint8_t number_of_banks;
  grub_uint8_t memory_model;
  grub_uint8_t bank_size;
  grub_uint8_t number_of_image_pages;
  grub_uint8_t reserved;

  /* Direct Color fields (required for direct/6 and YUV/7 memory models).  */
  grub_uint8_t red_mask_size;
  grub_uint8_t red_field_position;
  grub_uint8_t green_mask_size;
  grub_uint8_t green_field_position;
  grub_uint8_t blue_mask_size;
  grub_uint8_t blue_field_position;
  grub_uint8_t rsvd_mask_size;
  grub_uint8_t rsvd_field_position;
  grub_uint8_t direct_color_mode_info;

  /* Mandory information for VBE 2.0 and above.  */
  grub_vbe_physptr_t phys_base_addr;
  grub_uint32_t reserved2;
  grub_uint16_t reserved3;

  /* Mandory information for VBE 3.0 and above.  */
  grub_uint16_t lin_bytes_per_scan_line;
  grub_uint8_t bnk_number_of_image_pages;
  grub_uint8_t lin_number_of_image_pages;
  grub_uint8_t lin_red_mask_size;
  grub_uint8_t lin_red_field_position;
  grub_uint8_t lin_green_mask_size;
  grub_uint8_t lin_green_field_position;
  grub_uint8_t lin_blue_mask_size;
  grub_uint8_t lin_blue_field_position;
  grub_uint8_t lin_rsvd_mask_size;
  grub_uint8_t lin_rsvd_field_position;
  grub_uint32_t max_pixel_clock;

  /* Reserved field to make structure to be 256 bytes long, VESA BIOS 
     Extension 3.0 Specification says to reserve 189 bytes here but 
     that doesn't make structure to be 256 bytes. So additional one is 
     added here.  */
  grub_uint8_t reserved4[189 + 1];
} __attribute__ ((packed));

struct grub_vbe_crtc_info_block
{
  grub_uint16_t horizontal_total;
  grub_uint16_t horizontal_sync_start;
  grub_uint16_t horizontal_sync_end;
  grub_uint16_t vertical_total;
  grub_uint16_t vertical_sync_start;
  grub_uint16_t vertical_sync_end;
  grub_uint8_t flags;
  grub_uint32_t pixel_clock;
  grub_uint16_t refresh_rate;
  grub_uint8_t reserved[40];
} __attribute__ ((packed));

struct grub_vbe_palette_data
{
  grub_uint8_t blue;
  grub_uint8_t green;
  grub_uint8_t red;
  grub_uint8_t aligment;
} __attribute__ ((packed));

/* Prototypes for kernel real mode thunks.  */

/* Call VESA BIOS 0x4f00 to get VBE Controller Information, return status.  */
grub_vbe_status_t EXPORT_FUNC(grub_vbe_get_controller_info) (struct grub_vbe_info_block *controller_info);

/* Call VESA BIOS 0x4f01 to get VBE Mode Information, return status.  */
grub_vbe_status_t EXPORT_FUNC(grub_vbe_get_mode_info) (grub_uint32_t mode,
                                                       struct grub_vbe_mode_info_block *mode_info);

/* Call VESA BIOS 0x4f02 to set video mode, return status.  */
grub_vbe_status_t EXPORT_FUNC(grub_vbe_set_mode) (grub_uint32_t mode,
                                                  struct grub_vbe_crtc_info_block *crtc_info);

/* Call VESA BIOS 0x4f03 to return current VBE Mode, return status.  */
grub_vbe_status_t EXPORT_FUNC(grub_vbe_get_mode) (grub_uint32_t *mode);

/* Call VESA BIOS 0x4f05 to set memory window, return status.  */
grub_vbe_status_t EXPORT_FUNC(grub_vbe_set_memory_window) (grub_uint32_t window,
                                                           grub_uint32_t position);

/* Call VESA BIOS 0x4f05 to return memory window, return status.  */
grub_vbe_status_t EXPORT_FUNC(grub_vbe_get_memory_window) (grub_uint32_t window,
                                                           grub_uint32_t *position);

/* Call VESA BIOS 0x4f06 to set scanline length (in bytes), return status.  */
grub_vbe_status_t EXPORT_FUNC(grub_vbe_set_scanline_length) (grub_uint32_t length);

/* Call VESA BIOS 0x4f06 to return scanline length (in bytes), return status.  */
grub_vbe_status_t EXPORT_FUNC(grub_vbe_get_scanline_length) (grub_uint32_t *length);

/* Call VESA BIOS 0x4f07 to set display start, return status.  */
grub_vbe_status_t EXPORT_FUNC(grub_vbe_set_display_start) (grub_uint32_t x,
                                                           grub_uint32_t y);

/* Call VESA BIOS 0x4f07 to get display start, return status.  */
grub_vbe_status_t EXPORT_FUNC(grub_vbe_get_display_start) (grub_uint32_t *x,
                                                           grub_uint32_t *y);

/* Call VESA BIOS 0x4f09 to set palette data, return status.  */
grub_vbe_status_t EXPORT_FUNC(grub_vbe_set_palette_data) (grub_uint32_t color_count,
                                                          grub_uint32_t start_index,
                                                          struct grub_vbe_palette_data *palette_data);

/* Prototypes for helper functions.  */

grub_err_t grub_vbe_probe (struct grub_vbe_info_block *info_block);
grub_err_t grub_vbe_set_video_mode (grub_uint32_t mode,
                                    struct grub_vbe_mode_info_block *mode_info);
grub_err_t grub_vbe_get_video_mode (grub_uint32_t *mode);
grub_err_t grub_vbe_get_video_mode_info (grub_uint32_t mode,
                                         struct grub_vbe_mode_info_block *mode_info);
void grub_vbe_set_pixel_rgb (grub_uint32_t x,
                             grub_uint32_t y,
                             grub_uint8_t red,
                             grub_uint8_t green,
                             grub_uint8_t blue);
void grub_vbe_set_pixel_index (grub_uint32_t x,
                               grub_uint32_t y,
                               grub_uint8_t color);

#endif /* ! GRUB_VBE_MACHINE_HEADER */
