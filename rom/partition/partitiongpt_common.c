/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc: GPT helpers shared by partition.library and ptedit.library
*/

#include <exec/memory.h>
#include <libraries/partition.h>
#include <proto/exec.h>
#include <proto/partition.h>

#include <string.h>

#include <linklibs/fatbpb.h>
#include "partition_support.h"
#include "partition_types.h"
#include "partitiongpt.h"
#include "platform.h"
#include "debug.h"

UQUAD __attribute__((noinline)) gpt_le64(const UQUAD *q)
{
    const ULONG *l = (const ULONG *)q;
    return ((UQUAD)AROS_LE2LONG(l[1]) << 32) | AROS_LE2LONG(l[0]);
}

const uuid_t GPT_Type_Unused = MAKE_UUID(0x00000000, 0x0000, 0x0000, 0x0000, 0x000000000000ULL);
static const uuid_t GPT_Type_MSBasicData = MAKE_UUID(0xEBD0A0A2, 0xB9E5, 0x4433, 0x87C0, 0x68B6B72699C7ULL);

/*
 * This is a bit special.
 * The first four bytes (time_low) hold DOS Type ID (for simple mapping),
 * so we set them to zero here. We ignore it during comparison.
 * I hope this won't create any significant problems. Even if some ID ever collides, it will
 * unlikely collide with existing DOSTypes being used, so it can be blacklisted then.
 */
const uuid_t GPT_Type_AROS   = MAKE_UUID(0x00000000, 0xBB67, 0x46C5, 0xAA4A, 0xF502CA018E5EULL);

static ULONG GPT_ProbeFAT(struct Library *PartitionBase,
    struct PartitionHandle *ph, struct GPTPartition *p)
{
    UQUAD start = gpt_le64(&(p->StartBlock));
    UQUAD end = gpt_le64(&(p->EndBlock));
    UQUAD blocks;
    ULONG block_size = ph->de.de_SizeBlock << 2;
    ULONG type = 0;
    UBYTE *block;

    if (block_size < 512 || end < start || end == ~0ULL)
        return 0;

    blocks = end - start + 1;
    if (blocks > ~0ULL / block_size)
        return 0;

    block = AllocMem(block_size, MEMF_ANY);
    if (block)
    {
        struct FATBPBInfo bpb;

        if (ReadPartitionDataQ(ph, block, block_size, 0) == 0 &&
            FAT_ParseBPB(block, block_size, blocks * block_size, &bpb))
            type = bpb.dos_type;

        FreeMem(block, block_size);
    }

    return type;
}

#define GPT_UpdateGeometry PartitionUpdateGeometry

void GPT_PatchDosEnvec(struct Library *PartitionBase,
    struct PartitionHandle *ph, struct GPTPartition *p)
{
    ULONG type   = 0;
    LONG bootpri = 0;
    BOOL rdb = uuid_cmp_le(&p->TypeID, &GPT_Type_AROS_RDB);

    if (is_aros_uuid_le(&p->TypeID) && !rdb)
    {
        type    = AROS_LE2LONG(p->TypeID.time_low);
        /* This casting is needed for proper sign expansion */
        bootpri = (BYTE)(AROS_LE2LONG(p->Flags1) & GPT_PF1_AROS_BOOTPRI);
    }
    else
    {
        const struct TypeMapping *m;

        for (m = PartTypes; m->DOSType; m++)
        {
            if (m->uuid && uuid_cmp_le(&p->TypeID, m->uuid))
            {
                type = m->DOSType;
                break;
            }
        }

        if (type == 0 && uuid_cmp_le(&p->TypeID, &GPT_Type_MSBasicData))
            type = GPT_ProbeFAT(PartitionBase, ph, p);
    }
    
    /* RDB containers use block-based geometry, including when reopened.
     * The child RDB carries its own geometry independently of the disk. */
    if (rdb &&
        gpt_le64(&(p->EndBlock)) < 0xffffffffULL)
    {
        ph->de.de_Surfaces = 1;
        ph->de.de_BlocksPerTrack = 1;
        ph->de.de_LowCyl = gpt_le64(&(p->StartBlock));
        ph->de.de_HighCyl = gpt_le64(&(p->EndBlock));
    }
    setDosType(&ph->de, type);
    ph->de.de_BootPri = bootpri;
    GPT_UpdateGeometry(ph);
}

/* Entry lies within the usable area. With limit32, it must also be
 * representable by the 32-bit DosEnvec interface. */
BOOL GPTEntryValid(struct GPTHeader *hdr, struct GPTPartition *p, BOOL limit32)
{
    UQUAD start = gpt_le64(&p->StartBlock), end = gpt_le64(&p->EndBlock);

    return (!limit32 || end < 0xffffffffULL) && end >= start &&
        start >= gpt_le64(&hdr->DataStart) && end <= gpt_le64(&hdr->DataEnd);
}
