/*
 * $Id$
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#ifndef UBOOT_H_
#define UBOOT_H_

#include "support.h"

register void *global_data asm("r29");

/* 
 * This typedefs have to be replaced with proper definitions from 
 * U-Boot headers!
 */
typedef void cmd_tbl_t;

typedef unsigned long lbaint_t;

typedef struct block_dev_desc {
	int if_type;		/* type of the interface */
	int dev;		/* device number */
	unsigned char part_type;	/* partition type */
	unsigned char target;	/* target SCSI ID */
	unsigned char lun;	/* target LUN */
	unsigned char type;	/* device type */
	unsigned char removable;	/* removable device */
	unsigned char lba48;	/* device can use 48bit addr (ATA/ATAPI v7) */
	lbaint_t lba;		/* number of blocks */
	unsigned long blksz;	/* block size */
	unsigned char vendor[40 + 1];	/* IDE model, SCSI Vendor */
	unsigned char product[20 + 1];	/* IDE Serial no, SCSI product */
	unsigned char revision[8 + 1];	/* firmware revision */
	unsigned long (*block_read) (int dev,
				     unsigned long start,
				     lbaint_t blkcnt, void *buffer);
	unsigned long (*block_write) (int dev,
				      unsigned long start,
				      lbaint_t blkcnt, const void *buffer);
} block_dev_desc_t;

/* Interface types: */
#define IF_TYPE_UNKNOWN         0
#define IF_TYPE_IDE             1
#define IF_TYPE_SCSI            2
#define IF_TYPE_ATAPI           3
#define IF_TYPE_USB             4
#define IF_TYPE_DOC             5
#define IF_TYPE_MMC             6

/* Part types */
#define PART_TYPE_UNKNOWN       0x00
#define PART_TYPE_MAC           0x01
#define PART_TYPE_DOS           0x02
#define PART_TYPE_ISO           0x03
#define PART_TYPE_AMIGA         0x04

/* device types */
#define DEV_TYPE_UNKNOWN        0xff	/* not connected */
#define DEV_TYPE_HARDDISK       0x00	/* harddisk */
#define DEV_TYPE_TAPE           0x01	/* Tape */
#define DEV_TYPE_CDROM          0x05	/* CD-ROM */
#define DEV_TYPE_OPDISK         0x07	/* optical disk */
#define DEV_TYPE_NETBOOT        0x81	/* Netboot through TFTP */

typedef struct disk_partition {
	unsigned start;		/* # of first block in partition        */
	uint32_t size;		/* number of blocks in partition        */
	uint32_t blksz;		/* block size in bytes                  */
	uint8_t name[32];	/* partition name                       */
	uint8_t type[32];	/* string type description              */
} disk_partition_t;

typedef struct {
	node_t ush_link;
	uint16_t ush_bustype;
	uint16_t ush_already_scanned;
	block_dev_desc_t ush_device;
} *SCAN_HANDLE;

enum bustype {
	BUSTYPE_VIA_ATA,
	BUSTYPE_SCSI,
	BUSTYPE_USB,
	BUSTYPE_NET,
	BUSTYPE_FLOPPY,
	BUSTYPE_SIL_PARALLEL,
	BUSTYPE_SIL_SERIAL,
	BUSTYPE_SIL_4_SERIAL,

	BUSTYPE_NONE
};

#define NO_ERROR             0
#define ERROR_NO_MEMORY     -1
#define ERROR_OLD_SLB       -2
#define ERROR_OLD_UBOOT     -3
#define ERROR_NO_CONFIG     -4
#define ERROR_LOAD_ERROR    -5

#endif /*UBOOT_H_ */
