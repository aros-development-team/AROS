/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2009  Free Software Foundation, Inc.
 *
 *  GRUB is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  GRUB is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GRUB.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef GRUB_ACPI_HEADER
#define GRUB_ACPI_HEADER	1

#include <grub/types.h>
#include <grub/err.h>

struct grub_acpi_rsdp_v10
{
  grub_uint8_t signature[8];
  grub_uint8_t checksum;
  grub_uint8_t oemid[6];
  grub_uint8_t revision;
  grub_uint32_t rsdt_addr;
} __attribute__ ((packed));

struct grub_acpi_rsdp_v20
{
  struct grub_acpi_rsdp_v10 rsdpv1;
  grub_uint32_t length;
  grub_uint64_t xsdt_addr;
  grub_uint8_t checksum;
  grub_uint8_t reserved[3];
} __attribute__ ((packed));

struct grub_acpi_table_header
{
  grub_uint8_t signature[4];
  grub_uint32_t length;
  grub_uint8_t revision;
  grub_uint8_t checksum;
  grub_uint8_t oemid[6];
  grub_uint8_t oemtable[8];
  grub_uint32_t oemrev;
  grub_uint8_t creator_id[4];
  grub_uint32_t creator_rev;
} __attribute__ ((packed));

struct grub_acpi_fadt
{
  struct grub_acpi_table_header hdr;
  grub_uint32_t facs_addr;
  grub_uint32_t dsdt_addr;
  grub_uint8_t somefields1[88];
  grub_uint64_t facs_xaddr;
  grub_uint64_t dsdt_xaddr;
  grub_uint8_t somefields2[96];
} __attribute__ ((packed));

struct grub_acpi_rsdp_v10 *grub_acpi_get_rsdpv1 (void);
struct grub_acpi_rsdp_v20 *grub_acpi_get_rsdpv2 (void);
struct grub_acpi_rsdp_v10 *grub_machine_acpi_get_rsdpv1 (void);
struct grub_acpi_rsdp_v20 *grub_machine_acpi_get_rsdpv2 (void);
grub_uint8_t grub_byte_checksum (void *base, grub_size_t size);

grub_err_t grub_acpi_create_ebda (void);

#endif /* ! GRUB_ACPI_HEADER */
