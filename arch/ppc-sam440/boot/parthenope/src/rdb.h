/* rdb.h */

/* <project_name> -- <project_description>
 *
 * Copyright (C) 2006 - 2007
 *     Giuseppe Coviello <cjg@cruxppc.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef _RDB_H
#define _RDB_H

/* #ifndef EXTERNAL_TOOL */
#include "support.h"
#include "uboot.h"

struct RdbPartition {
	char *name;
	uint8_t disk;
	uint8_t partition;
	block_dev_desc_t *dev_desc;
	disk_partition_t *info;
};
/* #endif */

#define RDB_LOCATION_LIMIT 16

#define	IDNAME_RIGIDDISK	(uint32_t)0x5244534B	/* 'RDSK' */
#define IDNAME_BADBLOCK		(uint32_t)0x42414442	/* 'BADB' */
#define	IDNAME_PARTITION	(uint32_t)0x50415254	/* 'PART' */
#define IDNAME_FILESYSHEADER	(uint32_t)0x46534844	/* 'FSHD' */
#define IDNAME_LOADSEG		(uint32_t)0x4C534547	/* 'LSEG' */
#define IDNAME_BOOT		(uint32_t)0x424f4f54	/* 'BOOT' */
#define IDNAME_FREE		(uint32_t)0xffffffff

#define IS_FREE(blk) ((*(uint32_t *) (blk)) != IDNAME_RIGIDDISK		\
		      && (*(uint32_t *) (blk)) != IDNAME_BADBLOCK	\
		      && (*(uint32_t *) (blk)) != IDNAME_PARTITION	\
		      && (*(uint32_t *) (blk)) != IDNAME_FILESYSHEADER	\
		      && (*(uint32_t *) (blk)) != IDNAME_LOADSEG	\
		      && (*(uint32_t *) (blk)) != IDNAME_BOOT)

struct AmigaBlock {
	uint32_t amiga_ID;	/* Identifier 32 bit word */
	uint32_t amiga_SummedLongss;	/* Size of the structure for checksums */
	int32_t amiga_ChkSum;	/* Checksum of the structure */
};

struct RigidDiskBlock {
	uint32_t rdb_ID;	/* Identifier 32 bit word : 'RDSK' */
	uint32_t rdb_SummedLongs;	/* Size of the structure for checksums */
	int32_t rdb_ChkSum;	/* Checksum of the structure */
	uint32_t rdb_HostID;	/* SCSI Target ID of host, not really used */
	uint32_t rdb_BlockBytes;	/* Size of disk blocks */
	uint32_t rdb_Flags;	/* RDB Flags */
	/* block list heads */
	uint32_t rdb_BadBlockList;	/* Bad block list */
	uint32_t rdb_PartitionList;	/* Partition list */
	uint32_t rdb_FileSysHeaderList;	/* File system header list */
	uint32_t rdb_DriveInit;	/* Drive specific init code */
	uint32_t rdb_BootBlockList;	/* Amiga OS 4 Boot Blocks */
	uint32_t rdb_Reserved1[5];	/* Unused word, need to be set to $ffffffff */
	/* physical drive characteristics */
	uint32_t rdb_Cylinders;	/* Number of the cylinders of the drive */
	uint32_t rdb_Sectors;	/* Number of sectors of the drive */
	uint32_t rdb_Heads;	/* Number of heads of the drive */
	uint32_t rdb_Interleave;	/* Interleave */
	uint32_t rdb_Park;	/* Head parking cylinder */
	uint32_t rdb_Reserved2[3];	/* Unused word, need to be set to $ffffffff */
	uint32_t rdb_WritePreComp;	/* Starting cylinder of write precompensation */
	uint32_t rdb_ReducedWrite;	/* Starting cylinder of reduced write current */
	uint32_t rdb_StepRate;	/* Step rate of the drive */
	uint32_t rdb_Reserved3[5];	/* Unused word, need to be set to $ffffffff */
	/* logical drive characteristics */
	uint32_t rdb_RDBBlocksLo;	/* low block of range reserved for hardblocks */
	uint32_t rdb_RDBBlocksHi;	/* high block of range for these hardblocks */
	uint32_t rdb_LoCylinder;	/* low cylinder of partitionable disk area */
	uint32_t rdb_HiCylinder;	/* high cylinder of partitionable data area */
	uint32_t rdb_CylBlocks;	/* number of blocks available per cylinder */
	uint32_t rdb_AutoParkSeconds;	/* zero for no auto park */
	uint32_t rdb_HighRDSKBlock;	/* highest block used by RDSK */
	/* (not including replacement bad blocks) */
	uint32_t rdb_Reserved4;
	/* drive identification */
	char rdb_DiskVendor[8];
	char rdb_DiskProduct[16];
	char rdb_DiskRevision[4];
	char rdb_ControllerVendor[8];
	char rdb_ControllerProduct[16];
	char rdb_ControllerRevision[4];
	uint32_t rdb_Reserved5[10];
};

struct BootstrapCodeBlock {
	uint32_t bcb_ID;	/* 4 character identifier */
	uint32_t bcb_SummedLongs;	/* size of this checksummed structure */
	int32_t bcb_ChkSum;	/* block checksum (longword sum to zero) */
	uint32_t bcb_HostID;	/* SCSI Target ID of host */
	uint32_t bcb_Next;	/* block number of the next BootstrapCodeBlock */
	uint32_t bcb_LoadData[123];	/* binary data of the bootstrapper */
	/* note [123] assumes 512 byte blocks */
};

struct PartitionBlock {
	int32_t pb_ID;
	int32_t pb_SummedLongs;
	int32_t pb_ChkSum;
	uint32_t pb_HostID;
	int32_t pb_Next;
	uint32_t pb_Flags;
	uint32_t pb_Reserved1[2];
	uint32_t pb_DevFlags;
	char pb_DriveName[32];
	uint32_t pb_Reserved2[15];
	int32_t pb_Environment[17];
	uint32_t pb_EReserved[15];
};

struct AmigaPartitionGeometry {
	uint32_t apg_TableSize;
	uint32_t apg_SizeBlocks;
	uint32_t apg_Unused1;
	uint32_t apg_Surfaces;
	uint32_t apg_SectorPerBlock;
	uint32_t apg_BlockPerTrack;
	uint32_t apg_Reserved;
	uint32_t apg_Prealloc;
	uint32_t apg_Interleave;
	uint32_t apg_LowCyl;
	uint32_t apg_HighCyl;
	uint32_t apg_NumBuffers;
	uint32_t apg_BufMemType;
	uint32_t apg_MaxTransfer;
	uint32_t apg_Mask;
	int32_t apg_BootPriority;
	uint32_t apg_DosType;
	uint32_t apg_Baud;
	uint32_t apg_Control;
	uint32_t apg_BootBlocks;
};

void RdbPartitionTable_init(void);
struct RdbPartition *RdbPartitionTable_get(uint8_t disk, uint8_t partition);
struct RdbPartition *RdbPartitionTable_getbyname(char *name);
void RdbPartitionTable_dump(void);

#endif /* _RDB_H */
