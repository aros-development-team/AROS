/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 0

#include <aros/debug.h>
#include <proto/exec.h>
#include <proto/partition.h>
#include <libraries/partition.h>
#include <utility/tagitem.h>

#include <stdio.h>
#include <stdlib.h>

struct PartitionBase *PartitionBase;

LONG PrintPartitionTable(struct PartitionHandle *root, ULONG i);

static UQUAD getStartBlock(struct PartitionHandle *ph)
{
    ULONG ret = 0;

    while (ph)
    {
        UQUAD start;

        GetPartitionAttrsTags(ph, PT_STARTBLOCK, &start, TAG_DONE);
        ret += start;
        ph = ph->root;
    }

    return ret;
}        

void PrintDE(struct DosEnvec *de, ULONG i) {
ULONG a;

    for (a=i;a;a--)
        printf("  ");
    printf("SizeBlock      = %ld\n", de->de_SizeBlock<<2);
    for (a=i;a;a--)
        printf("  ");
    printf("Surfaces       = %ld\n", de->de_Surfaces);
    for (a=i;a;a--)
        printf("  ");
    printf("BlocksPerTrack = %ld\n", de->de_BlocksPerTrack);
    for (a=i;a;a--)
        printf("  ");
    printf("LowCyl         = %ld\n", de->de_LowCyl);
    for (a=i;a;a--)
        printf("  ");
    printf("HighCyl        = %ld\n", de->de_HighCyl);
}

void PrintPInfo(struct PartitionHandle *ph, ULONG i)
{
    struct DosEnvec de;
    UBYTE name[32];
    struct PartitionType type;
    ULONG a;
    UQUAD start, end, abs;

    GetPartitionAttrsTags(ph,
                          PT_DOSENVEC  , &de,
                          PT_NAME      , name,
                          PT_TYPE      , &type,
                          PT_STARTBLOCK, &start,
                          PT_ENDBLOCK  , &end,
                          TAG_DONE);

    for (a=i;a;a--)
        printf("  ");
    printf("name: %s\n", name);
    for (a=i+1;a;a--)
        printf("  ");

    printf("type:");
    for (a = 0; a < type.id_len; a++)
        printf(" %02X", type.id[a]);
    printf("\n");

    PrintDE(&de, i+1);

    for (a = i + 1; a; a--)
        printf("  ");
    printf("StartBlock     = %llu\n", (unsigned long long)start);
    for (a = i + 1; a; a--)
        printf("  ");    
    printf("EndBlock       = %llu\n", (unsigned long long)end);

    abs = getStartBlock(ph->root);
    for (a = i + 1; a; a--)
        printf("  ");
    printf("Abs StartBlock = %llu\n", (unsigned long long)(start + abs));
    for (a = i + 1; a; a--)
        printf("  ");    
    printf("Abs EndBlock   = %llu\n", (unsigned long long)(end + abs));

    PrintPartitionTable(ph, i + 1);
}

void PrintPartitions(struct PartitionHandle *root, ULONG i) {
struct PartitionHandle *ph;

    ph = (struct PartitionHandle *)root->table->list.lh_Head;
    while (ph->ln.ln_Succ)
    {
        D(printf("PartitionHandle 0x%p\n", ph));
        PrintPInfo(ph, i);
        ph = (struct PartitionHandle *)ph->ln.ln_Succ;
    }
}

LONG PrintPartitionTable(struct PartitionHandle *root, ULONG i)
{
    struct DosEnvec de;
    ULONG type = 0;
    ULONG reserved = 0;
    ULONG a;

    a = OpenPartitionTable(root);
    if (a)
        return a;

    GetPartitionTableAttrsTags(root,
                               PTT_TYPE    , &type,
                               PTT_RESERVED, &reserved,
                               TAG_DONE);
    GetPartitionAttrsTags(root, PT_DOSENVEC, &de, TAG_DONE);

    for (a=i;a;a--)
        printf("  ");
    printf("Partition table type is ");
    switch (type)
    {
    case PHPTT_RDB:
        printf("Rigid Disk Block\n");
        break;

    case PHPTT_MBR:
        printf("MBR -> PC\n");
        break;

    case PHPTT_EBR:
        printf("EBR -> PC\n");
        break;

    default:
        printf("unknown\n");
        break;
    }

    for (a=i;a;a--)
        printf("  ");
    printf("reserved blocks: %d\n", (int)reserved);

    PrintDE(&de,i);
    for (a=i;a;a--)
        printf("  ");
    printf("partitions:\n");
    PrintPartitions(root,i+1);
    ClosePartitionTable(root);

    return 0;
}

int main(int argc, char **argv)
{
    struct PartitionHandle *root;
    char *device = "fdsk.device";
    ULONG unit = 1;

    if (argc > 2)
    {
        device = argv[1];
        unit = atoi(argv[2]);
    }

    PartitionBase = (struct PartitionBase *)OpenLibrary("partition.library", 1);
    if (PartitionBase)
    {
        root = OpenRootPartition(device, unit);
        if (root)
        {
            printf("got root handle of %s unit %d\n", device, (int)unit);

            if (PrintPartitionTable(root, 0))
                printf("Couldn't read partition table\n");

            CloseRootPartition(root);
        }
        else
            printf("No root handle for %s unit %d\n", device, (int)unit);
        CloseLibrary((struct Library *)PartitionBase);
    }
    else
        printf("No partition.library\n");

    return 0;
}

