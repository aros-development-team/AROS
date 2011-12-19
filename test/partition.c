#define DEBUG 0

#include <aros/debug.h>
#include <proto/exec.h>
#include <proto/partition.h>
#include <libraries/partition.h>
#include <utility/tagitem.h>

#include <stdio.h>
#include <stdlib.h>

struct PartitionBase *PartitionBase;

void PrintDE(struct DosEnvec *de, ULONG i) {
ULONG a;

    for (a=i;a;a--)
        printf("  ");
    printf("SizeBlock = %ld\n", de->de_SizeBlock<<2);
    for (a=i;a;a--)
        printf("  ");
    printf("Surfaces = %ld\n", de->de_Surfaces);
    for (a=i;a;a--)
        printf("  ");
    printf("BlocksPerTrack = %ld\n", de->de_BlocksPerTrack);
    for (a=i;a;a--)
        printf("  ");
    printf("LowCyl = %ld\n", de->de_LowCyl);
    for (a=i;a;a--)
        printf("  ");
    printf("HighCyl = %ld\n", de->de_HighCyl);
}

void PrintPInfo(struct PartitionHandle *ph, ULONG i) {
struct DosEnvec de;
UBYTE name[32];
LONG type = 0;
ULONG a;

    D(bug("Getting attrs for handle 0x%p\n", ph));

    GetPartitionAttrsTags
    (
        ph,
        PT_DOSENVEC, &de,
        PT_NAME, name,
        PT_TYPE, &type,
        TAG_DONE
    );

    D(bug("Got attrs for handle 0x%p\n", ph));    
    for (a=i;a;a--)
        printf("  ");
    printf("name: %s\n", name);
    for (a=i+1;a;a--)
        printf("  ");
    printf("type: %x\n", type);
    PrintDE(&de, i+1);
}

void PrintPartitions(struct PartitionHandle *root, ULONG i) {
struct PartitionHandle *ph;

    ph = (struct PartitionHandle *)root->table->list.lh_Head;
    while (ph->ln.ln_Succ)
    {
        D(bug("PartitionHandle 0x%p, code 0x%p\n", ph, PrintPInfo));

        PrintPInfo(ph, i);
        ph = (struct PartitionHandle *)ph->ln.ln_Succ;
    }
}

void PrintPartitionTable(struct PartitionHandle *root, ULONG i) {
struct DosEnvec de;
ULONG type = 0;
ULONG reserved = 0;
ULONG a;

    if (OpenPartitionTable(root)==0)
    {
        GetPartitionTableAttrsTags
        (
            root,
            PTT_TYPE, &type,
            PTT_RESERVED, &reserved,
            TAG_DONE
        );
        GetPartitionAttrsTags(root, PT_DOSENVEC, &de, TAG_DONE);
        for (a=i;a;a--)
            printf("  ");
        printf("Partition type is ");
        switch (type)
        {
        case PHPTT_UNKNOWN:
            printf("unknown\n");
            break;
        case PHPTT_RDB:
            printf("Rigid Disk Block\n");
            break;
        case PHPTT_MBR:
            printf("MBR -> PC\n");
            break;
        }
        for (a=i;a;a--)
            printf("  ");
        printf("reserved blocks: %d\n", reserved);
        PrintDE(&de,i);
        for (a=i;a;a--)
            printf("  ");
        printf("partitions:\n");
        PrintPartitions(root,i+1);
        ClosePartitionTable(root);
    }
    else
        printf("Couldn't read partition table\n");
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
            printf("got root handle of %s unit %d\n", device, unit);
            PrintPartitionTable(root, 0);
            CloseRootPartition(root);
        }
        else
            printf("No root handle\n");
        CloseLibrary((struct Library *)PartitionBase);
    }
    else
        printf("No partition.library\n");

    return 0;
}

