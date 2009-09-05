/* ata.h - ATA disk access.  */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2007, 2008, 2009 Free Software Foundation, Inc.
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

#ifndef GRUB_ATA_HEADER
#define GRUB_ATA_HEADER 1

#include <grub/misc.h>
#include <grub/symbol.h>
/* XXX: For now this only works on i386.  */
#include <grub/cpu/io.h>

typedef enum
  {
    GRUB_ATA_CHS,
    GRUB_ATA_LBA,
    GRUB_ATA_LBA48
  } grub_ata_addressing_t;

#define GRUB_ATA_REG_DATA	0
#define GRUB_ATA_REG_ERROR	1
#define GRUB_ATA_REG_FEATURES	1
#define GRUB_ATA_REG_SECTORS	2
#define GRUB_ATAPI_REG_IREASON	2
#define GRUB_ATA_REG_SECTNUM	3
#define GRUB_ATA_REG_CYLLSB	4
#define GRUB_ATA_REG_CYLMSB	5
#define GRUB_ATA_REG_LBALOW	3
#define GRUB_ATA_REG_LBAMID	4
#define GRUB_ATAPI_REG_CNTLOW	4
#define GRUB_ATA_REG_LBAHIGH	5
#define GRUB_ATAPI_REG_CNTHIGH	5
#define GRUB_ATA_REG_DISK	6
#define GRUB_ATA_REG_CMD	7
#define GRUB_ATA_REG_STATUS	7

#define GRUB_ATA_REG2_CONTROL	0

#define GRUB_ATA_STATUS_ERR	0x01
#define GRUB_ATA_STATUS_INDEX	0x02
#define GRUB_ATA_STATUS_ECC	0x04
#define GRUB_ATA_STATUS_DRQ	0x08
#define GRUB_ATA_STATUS_SEEK	0x10
#define GRUB_ATA_STATUS_WRERR	0x20
#define GRUB_ATA_STATUS_READY	0x40
#define GRUB_ATA_STATUS_BUSY	0x80

/* ATAPI interrupt reason values (I/O, D/C bits).  */
#define GRUB_ATAPI_IREASON_MASK     0x3
#define GRUB_ATAPI_IREASON_DATA_OUT 0x0
#define GRUB_ATAPI_IREASON_CMD_OUT  0x1
#define GRUB_ATAPI_IREASON_DATA_IN  0x2
#define GRUB_ATAPI_IREASON_ERROR    0x3

enum grub_ata_commands
  {
    GRUB_ATA_CMD_CHECK_POWER_MODE	= 0xe5,
    GRUB_ATA_CMD_IDENTIFY_DEVICE	= 0xec,
    GRUB_ATA_CMD_IDENTIFY_PACKET_DEVICE	= 0xa1,
    GRUB_ATA_CMD_IDLE			= 0xe3,
    GRUB_ATA_CMD_PACKET			= 0xa0,
    GRUB_ATA_CMD_READ_SECTORS		= 0x20,
    GRUB_ATA_CMD_READ_SECTORS_EXT	= 0x24,
    GRUB_ATA_CMD_SECURITY_FREEZE_LOCK	= 0xf5,
    GRUB_ATA_CMD_SET_FEATURES		= 0xef,
    GRUB_ATA_CMD_SLEEP			= 0xe6,
    GRUB_ATA_CMD_SMART			= 0xb0,
    GRUB_ATA_CMD_STANDBY_IMMEDIATE	= 0xe0,
    GRUB_ATA_CMD_WRITE_SECTORS		= 0x30,
    GRUB_ATA_CMD_WRITE_SECTORS_EXT	= 0x34,
  };

enum grub_ata_timeout_milliseconds
  {
    GRUB_ATA_TOUT_STD  =  1000,  /* 1s standard timeout.  */
    GRUB_ATA_TOUT_DATA = 10000   /* 10s DATA I/O timeout.  */
  };

struct grub_ata_device
{
  /* IDE port to use.  */
  int port;

  /* IO addresses on which the registers for this device can be
     found.  */
  int ioaddress;
  int ioaddress2;

  /* Two devices can be connected to a single cable.  Use this field
     to select device 0 (commonly known as "master") or device 1
     (commonly known as "slave").  */
  int device;

  /* Addressing methods available for accessing this device.  If CHS
     is only available, use that.  Otherwise use LBA, except for the
     high sectors.  In that case use LBA48.  */
  grub_ata_addressing_t addr;

  /* Sector count.  */
  grub_uint64_t size;

  /* CHS maximums.  */
  grub_uint16_t cylinders;
  grub_uint16_t heads;
  grub_uint16_t sectors_per_track;

  /* Set to 0 for ATA, set to 1 for ATAPI.  */
  int atapi;

  struct grub_ata_device *next;
};

grub_err_t EXPORT_FUNC(grub_ata_wait_not_busy) (struct grub_ata_device *dev,
                                                int milliseconds);
grub_err_t EXPORT_FUNC(grub_ata_wait_drq) (struct grub_ata_device *dev,
					   int rw, int milliseconds);
void EXPORT_FUNC(grub_ata_pio_read) (struct grub_ata_device *dev,
				     char *buf, grub_size_t size);

static inline void
grub_ata_regset (struct grub_ata_device *dev, int reg, int val)
{
  grub_outb (val, dev->ioaddress + reg);
}

static inline grub_uint8_t
grub_ata_regget (struct grub_ata_device *dev, int reg)
{
  return grub_inb (dev->ioaddress + reg);
}

static inline void
grub_ata_regset2 (struct grub_ata_device *dev, int reg, int val)
{
  grub_outb (val, dev->ioaddress2 + reg);
}

static inline grub_uint8_t
grub_ata_regget2 (struct grub_ata_device *dev, int reg)
{
  return grub_inb (dev->ioaddress2 + reg);
}

static inline grub_err_t
grub_ata_check_ready (struct grub_ata_device *dev)
{
  if (grub_ata_regget (dev, GRUB_ATA_REG_STATUS) & GRUB_ATA_STATUS_BUSY)
    return grub_ata_wait_not_busy (dev, GRUB_ATA_TOUT_STD);

  return GRUB_ERR_NONE;
}

#endif /* ! GRUB_ATA_HEADER */
