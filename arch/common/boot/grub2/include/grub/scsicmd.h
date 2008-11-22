/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2008  Free Software Foundation, Inc.
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

#ifndef	GRUB_SCSICMD_H
#define	GRUB_SCSICMD_H	1

#include <grub/types.h>

#define GRUB_SCSI_DEVTYPE_MASK	31
#define GRUB_SCSI_REMOVABLE_BIT	7
#define GRUB_SCSI_LUN_SHIFT	5

struct grub_scsi_inquiry
{
  grub_uint8_t opcode;
  grub_uint8_t lun;
  grub_uint16_t reserved;
  grub_uint16_t alloc_length;
  grub_uint8_t reserved2;
  grub_uint8_t pad[5];
} __attribute__((packed));

struct grub_scsi_inquiry_data
{
  grub_uint8_t devtype;
  grub_uint8_t rmb;
  grub_uint16_t reserved;
  grub_uint8_t length;
  grub_uint8_t reserved2[3];
  char vendor[8];
  char prodid[16];
  char prodrev[4];
} __attribute__((packed));

struct grub_scsi_read_capacity
{
  grub_uint8_t opcode;
  grub_uint8_t lun;
  grub_uint8_t reserved[8];
  grub_uint8_t pad[2];
} __attribute__((packed));

struct grub_scsi_read_capacity_data
{
  grub_uint32_t size;
  grub_uint32_t blocksize;
} __attribute__((packed));

struct grub_scsi_read10
{
  grub_uint8_t opcode;
  grub_uint8_t lun;
  grub_uint32_t lba;
  grub_uint8_t reserved;
  grub_uint16_t size;
  grub_uint8_t reserved2;
  grub_uint16_t pad;
} __attribute__((packed));

struct grub_scsi_read12
{
  grub_uint8_t opcode;
  grub_uint8_t lun;
  grub_uint32_t lba;
  grub_uint32_t size;
  grub_uint8_t reserved;
  grub_uint8_t control;
} __attribute__((packed));

struct grub_scsi_write10
{
  grub_uint8_t opcode;
  grub_uint8_t lun;
  grub_uint32_t lba;
  grub_uint8_t reserved;
  grub_uint16_t size;
  grub_uint8_t reserved2;
  grub_uint16_t pad;
} __attribute__((packed));

struct grub_scsi_write12
{
  grub_uint8_t opcode;
  grub_uint8_t lun;
  grub_uint32_t lba;
  grub_uint32_t size;
  grub_uint8_t reserved;
  grub_uint8_t control;
} __attribute__((packed));

typedef enum
  {
    grub_scsi_cmd_inquiry = 0x12,
    grub_scsi_cmd_read_capacity = 0x25,
    grub_scsi_cmd_read10 = 0x28,
    grub_scsi_cmd_write10 = 0x2a,
    grub_scsi_cmd_read12 = 0xa8,
    grub_scsi_cmd_write12 = 0xaa
  } grub_scsi_cmd_t;

typedef enum
  {
    grub_scsi_devtype_direct = 0x00,
    grub_scsi_devtype_cdrom = 0x05
  } grub_scsi_devtype_t;

#endif /* GRUB_SCSICMD_H */
