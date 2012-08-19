/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef PARTITIONGPT_H
#define PARTITIONGPT_H

#include <libraries/uuid.h>

/* Type ID for legacy MBR */
#define MBRT_GPT 0xEE

/* GPT partition table header */
struct GPTHeader
{
    char   Signature[8];	/* ID signature						  */
    ULONG  Revision;		/* Revision number					  */
    ULONG  HeaderSize;		/* Size of this structure				  */
    ULONG  HeaderCRC32;		/* CRC32 of the header					  */
    ULONG  Reserved;
    UQUAD  CurrentBlock;	/* Number of block where this structure is placed	  */
    UQUAD  BackupBlock;		/* Number of block where backup partition table is placed */
    UQUAD  DataStart;		/* Number of the first usable block for data		  */
    UQUAD  DataEnd;		/* Number of the last usable block for data		  */
    uuid_t DiskID;		/* Disk unique ID					  */
    UQUAD  StartBlock;		/* Number of the first partition entry block		  */
    ULONG  NumEntries;		/* Number of partitions in the table			  */
    ULONG  EntrySize;		/* Size of one entry					  */
    ULONG  PartCRC32;		/* CRC32 of the partition entries array			  */
				/* The rest is reserved					  */
};

#define GPT_SIGNATURE	   "EFI PART"
#define GPT_MIN_HEADER_SIZE 92
#define GPT_MAX_HEADER_SIZE 512

struct GPTPartition
{
    uuid_t TypeID;	/* Partition type ID		*/
    uuid_t PartitionID;	/* Partition unique ID		*/
    UQUAD  StartBlock;	/* Number of the first block	*/
    UQUAD  EndBlock;	/* Number of the last block	*/
    ULONG  Flags0;	/* Flags			*/
    ULONG  Flags1;
    UBYTE  Name[72];	/* Name in UTF16-LE encoding	*/
};

/* Flags0 and Flags1, according to Microsoft(R) */
#define GPT_PF0_SYSTEM   (1 << 0)
#define GPT_PF1_READONLY (1 << 29)
#define GPT_PF1_HIDDEN   (1 << 30)
#define GPT_PF1_NOMOUNT  (1 << 31)

/* AROS-specific flags */
#define GPT_PF1_AROS_BOOTABLE (1 << 28)
#define GPT_PF1_AROS_BOOTPRI  0x000000FF	/* Mask for boot priority */

#endif /* PARTITIONGPT_H */
