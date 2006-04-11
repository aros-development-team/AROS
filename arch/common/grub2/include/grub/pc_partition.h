/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 1999,2000,2001,2002,2004  Free Software Foundation, Inc.
 *
 *  GRUB is free software; you can redistribute it and/or modify
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
 *  along with GRUB; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef GRUB_PC_PARTITION_HEADER
#define GRUB_PC_PARTITION_HEADER	1

#include <grub/symbol.h>
#include <grub/types.h>
#include <grub/err.h>

/* The signature.  */
#define GRUB_PC_PARTITION_SIGNATURE		0xaa55

/* This is not a flag actually, but used as if it were a flag.  */
#define GRUB_PC_PARTITION_TYPE_HIDDEN_FLAG	0x10

/* DOS partition types.  */
#define GRUB_PC_PARTITION_TYPE_NONE		0
#define GRUB_PC_PARTITION_TYPE_FAT12		1
#define GRUB_PC_PARTITION_TYPE_FAT16_LT32M	4
#define GRUB_PC_PARTITION_TYPE_EXTENDED		5
#define GRUB_PC_PARTITION_TYPE_FAT16_GT32M	6
#define GRUB_PC_PARTITION_TYPE_FAT32		0xb
#define GRUB_PC_PARTITION_TYPE_FAT32_LBA	0xc
#define GRUB_PC_PARTITION_TYPE_FAT16_LBA	0xe
#define GRUB_PC_PARTITION_TYPE_WIN95_EXTENDED	0xf
#define GRUB_PC_PARTITION_TYPE_EZD		0x55
#define GRUB_PC_PARTITION_TYPE_MINIX		0x80
#define GRUB_PC_PARTITION_TYPE_LINUX_MINIX	0x81
#define GRUB_PC_PARTITION_TYPE_EXT2FS		0x83
#define GRUB_PC_PARTITION_TYPE_LINUX_EXTENDED	0x85
#define GRUB_PC_PARTITION_TYPE_VSTAFS		0x9e
#define GRUB_PC_PARTITION_TYPE_FREEBSD		0xa5
#define GRUB_PC_PARTITION_TYPE_OPENBSD		0xa6
#define GRUB_PC_PARTITION_TYPE_NETBSD		0xa9
#define GRUB_PC_PARTITION_TYPE_LINUX_RAID	0xfd

/* Constants for BSD disk label.  */
#define GRUB_PC_PARTITION_BSD_LABEL_SECTOR	1
#define GRUB_PC_PARTITION_BSD_LABEL_MAGIC	0x82564557
#define GRUB_PC_PARTITION_BSD_MAX_ENTRIES	8

/* BSD partition types.  */
#define GRUB_PC_PARTITION_BSD_TYPE_UNUSED	0
#define GRUB_PC_PARTITION_BSD_TYPE_SWAP		1
#define GRUB_PC_PARTITION_BSD_TYPE_V6		2
#define GRUB_PC_PARTITION_BSD_TYPE_V7		3
#define GRUB_PC_PARTITION_BSD_TYPE_SYSV		4
#define GRUB_PC_PARTITION_BSD_TYPE_V71K		5
#define GRUB_PC_PARTITION_BSD_TYPE_V8		6
#define GRUB_PC_PARTITION_BSD_TYPE_BSDFFS	7
#define GRUB_PC_PARTITION_BSD_TYPE_MSDOS	8
#define GRUB_PC_PARTITION_BSD_TYPE_BSDLFS	9
#define GRUB_PC_PARTITION_BSD_TYPE_OTHER	10
#define GRUB_PC_PARTITION_BSD_TYPE_HPFS		11
#define GRUB_PC_PARTITION_BSD_TYPE_ISO9660	12
#define GRUB_PC_PARTITION_BSD_TYPE_BOOT		13

/* FreeBSD-specific types.  */
#define GRUB_PC_PARTITION_FREEBSD_TYPE_VINUM	14
#define GRUB_PC_PARTITION_FREEBSD_TYPE_RAID	15
#define GRUB_PC_PARTITION_FREEBSD_TYPE_JFS2	21

/* NetBSD-specific types.  */
#define	GRUB_PC_PARTITION_NETBSD_TYPE_ADOS	14
#define	GRUB_PC_PARTITION_NETBSD_TYPE_HFS	15
#define	GRUB_PC_PARTITION_NETBSD_TYPE_FILECORE	16
#define	GRUB_PC_PARTITION_NETBSD_TYPE_EXT2FS	17
#define	GRUB_PC_PARTITION_NETBSD_TYPE_NTFS	18
#define	GRUB_PC_PARTITION_NETBSD_TYPE_RAID	19
#define	GRUB_PC_PARTITION_NETBSD_TYPE_CCD	20
#define	GRUB_PC_PARTITION_NETBSD_TYPE_JFS2	21
#define	GRUB_PC_PARTITION_NETBSD_TYPE_APPLEUFS	22

/* OpenBSD-specific types.  */
#define	GRUB_PC_PARTITION_OPENBSD_TYPE_ADOS	14
#define	GRUB_PC_PARTITION_OPENBSD_TYPE_HFS	15
#define	GRUB_PC_PARTITION_OPENBSD_TYPE_FILECORE	16
#define	GRUB_PC_PARTITION_OPENBSD_TYPE_EXT2FS	17
#define	GRUB_PC_PARTITION_OPENBSD_TYPE_NTFS	18
#define	GRUB_PC_PARTITION_OPENBSD_TYPE_RAID	19

/* The BSD partition entry.  */
struct grub_pc_partition_bsd_entry
{
  grub_uint32_t size;
  grub_uint32_t offset;
  grub_uint32_t fragment_size;
  grub_uint8_t fs_type;
  grub_uint8_t fs_fragments;
  grub_uint16_t fs_cylinders;
} __attribute__ ((packed));

/* The BSD disk label. Only define members useful for GRUB.  */
struct grub_pc_partition_disk_label
{
  grub_uint32_t magic;
  grub_uint8_t padding[128];
  grub_uint32_t magic2;
  grub_uint16_t checksum;
  grub_uint16_t num_partitions;
  grub_uint32_t boot_size;
  grub_uint32_t superblock_size;
  struct grub_pc_partition_bsd_entry entries[GRUB_PC_PARTITION_BSD_MAX_ENTRIES];
} __attribute__ ((packed));

/* The partition entry.  */
struct grub_pc_partition_entry
{
  /* If active, 0x80, otherwise, 0x00.  */
  grub_uint8_t flag;

  /* The head of the start.  */
  grub_uint8_t start_head;

  /* (S | ((C >> 2) & 0xC0)) where S is the sector of the start and C
     is the cylinder of the start. Note that S is counted from one.  */
  grub_uint8_t start_sector;

  /* (C & 0xFF) where C is the cylinder of the start.  */
  grub_uint8_t start_cylinder;

  /* The partition type.  */
  grub_uint8_t type;

  /* The end versions of start_head, start_sector and start_cylinder,
     respectively.  */
  grub_uint8_t end_head;
  grub_uint8_t end_sector;
  grub_uint8_t end_cylinder;

  /* The start sector. Note that this is counted from zero.  */
  grub_uint32_t start;

  /* The length in sector units.  */
  grub_uint32_t length;
} __attribute__ ((packed));

/* The structure of MBR.  */
struct grub_pc_partition_mbr
{
  /* The code area (actually, including BPB).  */
  grub_uint8_t code[446];

  /* Four partition entries.  */
  struct grub_pc_partition_entry entries[4];

  /* The signature 0xaa55.  */
  grub_uint16_t signature;
} __attribute__ ((packed));


struct grub_pc_partition
{
    /* The DOS partition number.  */
  int dos_part;

  /* The BSD partition number (a == 0).  */
  int bsd_part;

  /* The DOS partition type.  */
  int dos_type;

  /* The BSD partition type.  */
  int bsd_type;

  /* The offset of the extended partition.  */
  unsigned long ext_offset;
};

static inline int
grub_pc_partition_is_empty (int type)
{
  return (type == GRUB_PC_PARTITION_TYPE_NONE);
}

static inline int
grub_pc_partition_is_extended (int type)
{
  return (type == GRUB_PC_PARTITION_TYPE_EXTENDED
	  || type == GRUB_PC_PARTITION_TYPE_WIN95_EXTENDED
	  || type == GRUB_PC_PARTITION_TYPE_LINUX_EXTENDED);
}

static inline int
grub_pc_partition_is_bsd (int type)
{
  return (type == GRUB_PC_PARTITION_TYPE_FREEBSD
	  || type == GRUB_PC_PARTITION_TYPE_OPENBSD
	  || type == GRUB_PC_PARTITION_TYPE_NETBSD);
}

#endif /* ! GRUB_PC_PARTITION_HEADER */
