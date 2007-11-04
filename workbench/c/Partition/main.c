/*
    Copyright © 2003-2007, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <utility/tagitem.h>
#include <libraries/partition.h>
#include <devices/trackdisk.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/partition.h>

#define DEBUG 0
#include <aros/debug.h>

#include "args.h"

#define MB (1024LL * 1024LL)
#define MIN_SIZE 50 /* Minimum disk space allowed in MBs */


/*** Prototypes *************************************************************/
static VOID FindLargestGap(struct PartitionHandle *root, ULONG *gapLowBlock,
    ULONG *gapHighBlock);
static VOID CheckGap(struct PartitionHandle *root, ULONG partLimitBlock,
    ULONG *gapLowBlock, ULONG *gapHighBlock);
static struct PartitionHandle *CreateMBRPartition(
    struct PartitionHandle *parent, ULONG lowcyl, ULONG highcyl, UBYTE id);
static struct PartitionHandle *CreateRDBPartition(
    struct PartitionHandle *parent, ULONG lowcyl, ULONG highcyl,
    CONST_STRPTR name, BOOL bootable);
static ULONG MBsToCylinders(ULONG size, struct DosEnvec *de);

/*** Functions **************************************************************/
int main(void)
{
    struct PartitionHandle *diskPart = NULL, *sysPart, *workPart,
        *root = NULL, *partition, *extPartition = NULL, *parent = NULL;
    TEXT choice = 'N';
    CONST_STRPTR device;
    LONG error = 0, sysSize = 0, workSize = 0, sysHighCyl, rootLowBlock = 1,
        rootHighBlock = 0, extLowBlock = 1, extHighBlock = 0, lowBlock,
        highBlock, lowCyl, highCyl, rootBlocks, extBlocks, reqHighCyl, unit;
    UWORD partCount = 0;

    /* Read and check arguments */
    if (!ReadArguments())
        error = IoErr();

    if (error == 0)
    {
        if (ARG(WORKSIZE) != (IPTR)NULL && ARG(SYSSIZE) == (IPTR)NULL)
        {
            PutStr("ERROR: Cannot specify WORKSIZE without SYSSIZE.\n");
            error = ERROR_REQUIRED_ARG_MISSING;
        }

        if (ARG(WORKSIZE) != (IPTR)NULL && ARG(MAXWORK))
        {
            PutStr("ERROR: Cannot specify both WORKSIZE and MAXWORK.\n");
            error = ERROR_TOO_MANY_ARGS;
        }

        if (ARG(QUIET) && !ARG(FORCE))
        {
            PutStr("ERROR: Cannot specify QUIET without FORCE.\n");
            error = ERROR_REQUIRED_ARG_MISSING;
        }
    }

    if (error == 0)
    {
        device = (CONST_STRPTR) ARG(DEVICE);
        unit   = *(LONG *)ARG(UNIT);
        if (ARG(SYSSIZE) != (IPTR)NULL)
            sysSize = *(LONG *)ARG(SYSSIZE);
        if (ARG(WORKSIZE) != (IPTR)NULL)
            workSize = *(LONG *)ARG(WORKSIZE);

        D(bug("[C:Partition] Using %s, unit %d\n", device, unit));
    
        if (!ARG(FORCE))
        {
            Printf("About to partition %s unit %d.\n", device, unit);
            if (ARG(WIPE))
                Printf("This will DESTROY ALL DATA on the drive!\n");
            Printf("Are you sure? (y/N)"); Flush(Output());

            Read(Input(), &choice, 1);
        }
        else
        {
            choice = 'y';
        }

        if (choice != 'y' && choice != 'Y') return RETURN_WARN;

        if (!ARG(QUIET))
        {
            Printf("Partitioning drive...");
            Flush(Output());
        }

        D(bug("[C:Partition] Partitioning drive...\n"));
    }

    if (error == 0)
    {
        if ((root = OpenRootPartition(device, unit)) == NULL)
        {
            error = ERROR_UNKNOWN;
            PutStr("*** Could not open root partition!\n");
        }
    }

    /* Destroy any existing partition table if requested */
    if (error == 0 && ARG(WIPE))
    {
        if (OpenPartitionTable(root) == 0)
            error = DestroyPartitionTable(root);
    }

    if (error == 0)
    {
        /* Open the existing partition table, or create it if none exists */
        if (OpenPartitionTable(root) != 0)
        {
            /* Create a root MBR partition table */
            if (CreatePartitionTable(root, PHPTT_MBR) != 0)
            {
                error = ERROR_UNKNOWN;
                PutStr("*** ERROR: Creating MBR partition table failed.\n");
                CloseRootPartition(root);
                root = NULL;
            }
        }
    }

    if (error == 0)
    {
        /* Find largest gap in root partition */
        FindLargestGap(root, &rootLowBlock, &rootHighBlock);

        /* Look for extended partition and count partitions */
        ForeachNode(&root->table->list, partition)
        {
            if (OpenPartitionTable(partition) == 0)
            {
                if (partition->table->type == PHPTT_EBR)
                    extPartition = partition;
                else
                    ClosePartitionTable(partition);
            }
            partCount++;
        }

        /* Create extended partition if it doesn't exist */
        if (extPartition == NULL)
        {
            lowCyl = (rootLowBlock - 1)
                / (LONG)(root->de.de_Surfaces * root->de.de_BlocksPerTrack)
                + 1;
            highCyl = (rootHighBlock + 1)
                / (root->de.de_Surfaces * root->de.de_BlocksPerTrack) - 1;
            extPartition =
                CreateMBRPartition(root, lowCyl, highCyl, 0x5);
            if (extPartition != NULL)
            {
                if (CreatePartitionTable(extPartition, PHPTT_EBR) != 0)
                {
                    PutStr("*** ERROR: Creating extended partition table failed.\n");
                    extPartition = NULL;
                }
                rootLowBlock = 1;
                rootHighBlock = 0;
                partCount++;
            }
        }
    }

    if (error == 0)
    {
        /* Find largest gap in extended partition */
        if (extPartition != NULL)
        {
            FindLargestGap(extPartition, &extLowBlock, &extHighBlock);
        }

        /* Choose whether to create primary or logical partition */
        rootBlocks = rootHighBlock - rootLowBlock + 1;
        extBlocks = extHighBlock - extLowBlock + 1;
        if (extBlocks > rootBlocks || partCount == 4)
        {
            parent = extPartition;
            lowBlock = extLowBlock;
            highBlock = extHighBlock;
        }
        else
        {
            parent = root;
            lowBlock = rootLowBlock;
            highBlock = rootHighBlock;
        }

        /* Decide geometry for RDB virtual disk */
        lowCyl = ((LONG)lowBlock - 1)
            / (LONG)(parent->de.de_Surfaces * parent->de.de_BlocksPerTrack) + 1;
        highCyl = (highBlock + 1)
            / (parent->de.de_Surfaces * parent->de.de_BlocksPerTrack) - 1;
        reqHighCyl = lowCyl + MBsToCylinders(sysSize, &parent->de)
            + MBsToCylinders(workSize, &parent->de);
        if (sysSize != 0 && (workSize != 0 || !ARG(MAXWORK))
            && reqHighCyl < highCyl)
            highCyl = reqHighCyl;
        if (reqHighCyl > highCyl
            || highCyl - lowCyl + 1 < MBsToCylinders(MIN_SIZE, &parent->de)
            && workSize == 0)
            error = ERROR_DISK_FULL;
    }

    if (error == 0)
    {
        /* Create RDB virtual disk */
        diskPart = CreateMBRPartition(parent, lowCyl, highCyl, 0x30);
        if (diskPart == NULL)
        {
            PutStr("*** ERROR: Not enough disk space.\n");
            error = ERROR_UNKNOWN;
        }
    }

    if (error == 0)
    {
        /* Create RDB partition table inside virtual-disk partition */
        error = CreatePartitionTable(diskPart, PHPTT_RDB);
        if (error != 0)
        {
            PutStr(
                "*** ERROR: Creating RDB partition table failed. Aborting.\n");
        }
    }

    if (error == 0)
    {
        sysHighCyl = MBsToCylinders(sysSize, &diskPart->de);

        /* Create partitions in the RDB table */
        sysPart = CreateRDBPartition(diskPart, 0, sysHighCyl, "DH0", TRUE);
        if (sysPart == NULL)
            error = ERROR_UNKNOWN;
        if (sysSize != 0
            && sysHighCyl < diskPart->de.de_HighCyl - diskPart->de.de_LowCyl)
        {
            workPart = CreateRDBPartition(diskPart, sysHighCyl + 1, 0, "DH1", FALSE);
            if (workPart == NULL)
                error = ERROR_UNKNOWN;
        }
    }

    if (error == 0)
    {
        /* Save to disk and deallocate */
        WritePartitionTable(diskPart);
        ClosePartitionTable(diskPart);

        if (parent == extPartition)
        {
            WritePartitionTable(extPartition);
            ClosePartitionTable(extPartition);
        }

        /* Save to disk and deallocate */
        WritePartitionTable(root);
        ClosePartitionTable(root);
    }

    if (root != NULL)
        CloseRootPartition(root);

    if (!ARG(QUIET)) Printf("done\n");

    PrintFault(error, NULL);
    return (error == 0) ? RETURN_OK : RETURN_FAIL;
}

/* Find the low and high cylinder values for the largest gap on the disk */
static VOID FindLargestGap(struct PartitionHandle *root, ULONG *gapLowBlock,
    ULONG *gapHighBlock)
{
    struct PartitionHandle *partition;
    LONG reservedBlocks = 0;
    ULONG partLimitBlock;

    /* Check gap between start of disk and first partition, or until end of
       disk if there are no partitions */
    GetPartitionTableAttrsTags(root, PTT_RESERVED, (IPTR) &reservedBlocks,
        TAG_DONE);
    partLimitBlock = reservedBlocks;
    CheckGap(root, partLimitBlock, gapLowBlock, gapHighBlock);

    /* Check gap between each partition and the next one (or end of disk for
       the last partition)*/
    ForeachNode(&root->table->list, partition)
    {
        partLimitBlock = (partition->de.de_HighCyl + 1)
            * partition->de.de_Surfaces
            * partition->de.de_BlocksPerTrack;
        CheckGap(root, partLimitBlock, gapLowBlock, gapHighBlock);
    }

    return;
}

/* Check if the gap between the end of the current partition and the start of
   the next one is bigger than the biggest gap previously found. If so, it is
   stored as the new biggest gap. */
static VOID CheckGap(struct PartitionHandle *root, ULONG partLimitBlock,
    ULONG *gapLowBlock, ULONG *gapHighBlock)
{
    struct PartitionHandle *nextPartition;
    ULONG lowBlock, nextLowBlock;

    /* Search partitions for next partition by disk position */
    nextLowBlock = (root->de.de_HighCyl - root->de.de_LowCyl + 1)
        * root->de.de_Surfaces * root->de.de_BlocksPerTrack; /* End of disk */
    ForeachNode(&root->table->list, nextPartition)
    {
        lowBlock = nextPartition->de.de_LowCyl
           * nextPartition->de.de_Surfaces
           * nextPartition->de.de_BlocksPerTrack;
        if (lowBlock >= partLimitBlock && lowBlock < nextLowBlock)
        {
            nextLowBlock = lowBlock;
        }
    }

    /* Check if the gap between the current pair of partitions is the
       biggest gap so far */
    if (nextLowBlock - partLimitBlock > *gapHighBlock - *gapLowBlock + 1)
    {
        *gapLowBlock = partLimitBlock;
        *gapHighBlock = nextLowBlock - 1;
    }

    return;
}

static struct PartitionHandle *CreateMBRPartition
(
    struct PartitionHandle *parent, ULONG lowcyl, ULONG highcyl, UBYTE id
)
{
    struct DriveGeometry    parentDG    = {0};
    struct DosEnvec         parentDE    = {0},
                            partitionDE = {0};
    struct PartitionType    type        = {{id},  1};
    struct PartitionHandle *partition;
    IPTR reserved, leadIn = 0;
    
    GetPartitionAttrsTags
    (
        parent, 
        
        PT_DOSENVEC, (IPTR) &parentDE, 
        PT_GEOMETRY, (IPTR) &parentDG,
        
        TAG_DONE
    );
    
    GetPartitionTableAttrsTags
    (
        parent,
        PTT_RESERVED, (IPTR) &reserved,
        PTT_MAXLEADIN, (IPTR) &leadIn,
        TAG_DONE
    );
        
    if (lowcyl == 0)
    {
        lowcyl = (reserved - 1) /
            (parentDG.dg_Heads * parentDG.dg_TrackSectors) + 1;
    }
    
    if (leadIn != 0)
    {
        lowcyl += (leadIn - 1) /
            (parentDG.dg_Heads * parentDG.dg_TrackSectors) + 1;
    }

    if (highcyl == 0)
    {
        highcyl = parentDE.de_HighCyl;
    }
    
    CopyMem(&parentDE, &partitionDE, sizeof(struct DosEnvec));
    
    partitionDE.de_SizeBlock = parentDG.dg_SectorSize >> 2;
    partitionDE.de_Reserved  = 2;
    partitionDE.de_HighCyl   = highcyl;
    partitionDE.de_LowCyl    = lowcyl;
    
    partition = AddPartitionTags
    (
        parent,
        PT_DOSENVEC, (IPTR) &partitionDE,
        PT_TYPE,     (IPTR) &type,
        TAG_DONE
    );
    
    return partition;
}

static struct PartitionHandle *CreateRDBPartition
(
    struct PartitionHandle *parent, ULONG lowcyl, ULONG highcyl,
    CONST_STRPTR name, BOOL bootable
)
{
    struct DriveGeometry    parentDG    = {0};
    struct DosEnvec         parentDE    = {0},
                            partitionDE = {0};
    struct PartitionType    type        = {"DOS\3", 4};
    struct PartitionHandle *partition;
        
    GetPartitionAttrsTags
    (
        parent, 
        
        PT_DOSENVEC, (IPTR) &parentDE, 
        PT_GEOMETRY, (IPTR) &parentDG,
        
        TAG_DONE
    );
    
    if (lowcyl == 0)
    {
        ULONG reserved;
        
        GetPartitionTableAttrsTags
        (
            parent, PTT_RESERVED, (IPTR) &reserved, TAG_DONE
        );
        
        lowcyl = (reserved - 1)
            / (parentDG.dg_Heads * parentDG.dg_TrackSectors) + 1;
    }
    
    if (highcyl == 0)
    {
        highcyl = parentDE.de_HighCyl - parentDE.de_LowCyl;
    }
    
    CopyMem(&parentDE, &partitionDE, sizeof(struct DosEnvec));
    
    partitionDE.de_SizeBlock      = parentDG.dg_SectorSize >> 2;
    partitionDE.de_Surfaces       = parentDG.dg_Heads;
    partitionDE.de_BlocksPerTrack = parentDG.dg_TrackSectors;
    partitionDE.de_BufMemType     = parentDG.dg_BufMemType;
    partitionDE.de_TableSize      = DE_DOSTYPE;
    partitionDE.de_Reserved       = 2;
    partitionDE.de_HighCyl        = highcyl;
    partitionDE.de_LowCyl         = lowcyl;
    partitionDE.de_NumBuffers     = 100;
    partitionDE.de_MaxTransfer    = 0xFFFFFF;
    partitionDE.de_Mask           = 0xFFFFFFFE;
            
    D(bug("[C:Partition] SizeBlock %d\n", partitionDE.de_SizeBlock ));
    D(bug("[C:Partition] Surfaces %d\n", partitionDE.de_Surfaces));
    D(bug("[C:Partition] BlocksPerTrack %d\n", partitionDE.de_BlocksPerTrack));
    D(bug("[C:Partition] BufMemType %d\n", partitionDE.de_BufMemType));
    D(bug("[C:Partition] TableSize %d\n", partitionDE.de_TableSize));
    D(bug("[C:Partition] Reserved %d\n", partitionDE.de_Reserved));
    D(bug("[C:Partition] HighCyl %d\n", partitionDE.de_HighCyl));
    D(bug("[C:Partition] LowCyl %d\n", partitionDE.de_LowCyl));

    partition = AddPartitionTags
    (
        parent,
        
        PT_DOSENVEC, (IPTR) &partitionDE,
        PT_TYPE,     (IPTR) &type,
        PT_NAME,     (IPTR) name,
        PT_BOOTABLE,        bootable,
        PT_AUTOMOUNT,       TRUE,
        
        TAG_DONE
    );
    
    return partition;
}

static ULONG MBsToCylinders(ULONG size, struct DosEnvec *de)
{
    UQUAD bytes;
    ULONG cyls = 0;

    if (size != 0)
    {
        bytes = size * MB;
        cyls = (bytes - 1) / (UQUAD)(de->de_SizeBlock * sizeof(ULONG))
            / de->de_BlocksPerTrack / de->de_Surfaces + 1;
    }
    return cyls;
}
