/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.
*/

#ifndef PARTITIONGPT_H
#define PARTITIONGPT_H

#include <aros/macros.h>
#include <libraries/partition.h>
#include <libraries/uuid.h>

#include <string.h>

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
#define GPT_PF1_NOMOUNT  (1UL << 31)

/* AROS-specific flags */
#define GPT_PF1_AROS_BOOTABLE (1 << 28)
#define GPT_PF1_AROS_BOOTPRI  0x000000FF	/* Mask for boot priority */

/* In-memory partition handle, allocated with AllocVec() by both libraries */
struct GPTPartitionHandle
{
    struct PartitionHandle ph;  /* Public part                */
    ULONG entrySize;            /* Size of table entry        */
    ULONG position;            /* Stable index in the on-disk entry array */
    char name[37];              /* Name in ASCII              */
                                /* Actual table entry follows */
};

#define GPTH(ph) ((struct GPTPartitionHandle *)ph)

extern const uuid_t GPT_Type_Unused;
extern const uuid_t GPT_Type_AROS;

/*
 * Little-endian UUID conversion and comparison.
 * We can't use uuid.library here because it's not available during
 * system bootup. However, we are going to use it for generation.
 */
static inline void uuid_from_le(uuid_t *to, uuid_t *id)
{
    to->time_low            = AROS_LE2LONG(id->time_low);
    to->time_mid            = AROS_LE2WORD(id->time_mid);
    to->time_hi_and_version = AROS_LE2WORD(id->time_hi_and_version);

    /* Do not replace it with CopyMem(), gcc optimizes this nicely */
    memcpy(&to->clock_seq_hi_and_reserved, &id->clock_seq_hi_and_reserved, 8);
}

static inline void uuid_to_le(uuid_t *to, uuid_t *id)
{
    to->time_low            = AROS_LONG2LE(id->time_low);
    to->time_mid            = AROS_WORD2LE(id->time_mid);
    to->time_hi_and_version = AROS_WORD2LE(id->time_hi_and_version);

    /* Do not replace it with CopyMem(), gcc optimizes this nicely */
    memcpy(&to->clock_seq_hi_and_reserved, &id->clock_seq_hi_and_reserved, 8);
}

static inline BOOL uuid_cmp_le(uuid_t *leid, const uuid_t *id)
{
    if (AROS_LE2LONG(leid->time_low) != id->time_low)
        return FALSE;
    if (AROS_LE2WORD(leid->time_mid) != id->time_mid)
        return FALSE;
    if (AROS_LE2WORD(leid->time_hi_and_version) != id->time_hi_and_version)
        return FALSE;

    return !memcmp(&leid->clock_seq_hi_and_reserved, &id->clock_seq_hi_and_reserved, 8);
}

/* For AROS we put DOS Type ID into first four bytes of UUID (time_low), so we ignore them. */
static inline BOOL is_aros_uuid_le(uuid_t *leid)
{
    if (AROS_LE2WORD(leid->time_mid) != GPT_Type_AROS.time_mid)
        return 0;
    if (AROS_LE2WORD(leid->time_hi_and_version) != GPT_Type_AROS.time_hi_and_version)
        return 0;

    return !memcmp(&leid->clock_seq_hi_and_reserved, &GPT_Type_AROS.clock_seq_hi_and_reserved, 8);
}

#ifdef DEBUG_UUID

static inline void PRINT_LE_UUID(char *s, uuid_t *id)
{
    unsigned int i;

    bug("[GPT] %s UUID: 0x%08X-%04X-%04X-%02X%02X-", s,
        AROS_LE2LONG(id->time_low), AROS_LE2WORD(id->time_mid), AROS_LE2WORD(id->time_hi_and_version),
        id->clock_seq_hi_and_reserved, id->clock_seq_low);

    for (i = 0; i < sizeof(id->node); i++)
        bug("%02X", id->node[i]);

    RawPutChar('\n');
}

#else

#define PRINT_LE_UUID(s, id)

#endif

/* Shared by partition.library and partitionedit.library (partitiongpt_common.c) */
UQUAD gpt_le64(const UQUAD *q);
void GPT_PatchDosEnvec(struct Library *PartitionBase,
    struct PartitionHandle *ph, struct GPTPartition *p);
BOOL GPTEntryValid(struct GPTHeader *hdr, struct GPTPartition *p, BOOL limit32);

#endif /* PARTITIONGPT_H */
