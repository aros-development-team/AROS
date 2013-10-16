/*
    Copyright © 2003-2013, The AROS Development Team. All rights reserved.
    $Id$
*/

/******************************************************************************

    NAME

        Partition

    SYNOPSIS

        DEVICE, UNIT/N, SYSSIZE/K/N, SYSTYPE/K, SYSNAME/K, WORKSIZE/K/N,
        MAXWORK/S, WORKTYPE/K, WORKNAME/K, WIPE/S, FORCE/S, QUIET/S, RDB/S

    LOCATION

        C:

    FUNCTION

        Partition creates either one or two AROS partitions on a given drive.
        Existing partitions will be kept unless the WIPE option is specified
        (or a serious bug occurs, for which we take no responsibility).
        Partitions created by this command must be formatted before they can
        be used.

        By default, a single SFS System partition is created using the
        largest amount of free space possible. A smaller size can be chosen
        using the SYSSIZE argument. To also create a Work partition, either
        WORKSIZE or MAXWORK must additionally be specified. The WORKSIZE
        argument allows the size of the Work partition to be specified,
        while setting the MAXWORK switch makes the Work partition as large
        as possible.

        The filesystems used by the System and Work partitions may be
        specified using the SYSTYPE and WORKTYPE arguments respectively.
        The available options are "SFS" (Smart Filesystem, the default), and
        "FFSIntl" (the traditional so-called Fast Filesystem).

        The DOS device names used for the System and Work partitions may be
        specified using the SYSNAME and WORKNAME arguments respectively.
        By default, these are DH0 and DH1.

        If you wish to use only AROS on the drive you run this command on,
        you can specify the WIPE option, which destroys all existing
        partitions on the drive. Be very careful with this option: it
        deletes all other operating systems and data on the drive, and could
        be disastrous if the wrong drive is accidentally partitioned.

        If the drive does not already contain an extended partition, one is
        created using the largest available region of free space. The AROS
        partitions are then created as a logical partition within. This is
        in order to make the addition of further partitions easier.

    INPUTS

	DEVICE -- Device driver name (ata.device by default)
	UNIT -- The drive's unit number (0 by default, which is the primary
            master when using ata.device)
	SYSSIZE -- The System (boot) partition size in megabytes.
	SYSTYPE -- The file system to use for the system partition, either
            "SFS" (the default) or "FFSIntl".
	SYSNAME -- The name to use for the system partition (defaults to DH0).
	WORKSIZE -- The Work (secondary) partition size in megabytes. To use
            this option, SYSSIZE must also be specified.
	MAXWORK -- Make the Work partition as large as possible. To use this
            option, SYSSIZE must also be specified.
	WORKTYPE -- The file system to use for the work partition, either
            "SFS" (the default) or "FFSIntl".
	WORKNAME -- The name to use for the work partition (defaults to DH1).
	WIPE -- Destroy all other partitions on the drive, including those
            used by other operating systems (CAUTION!).
	FORCE -- Do not ask for confirmation before partitioning the drive.
        QUIET -- Do not print any output. This option can only be used when
            FORCE is also specified.
        RDB -- Create only RDB partitions, no MBR or EBR partitions will be
            created.

    RESULT

        Standard DOS error codes.

    NOTES

        Using HDToolBox instead of this command may sometimes be safer, as
        it shows where partitions will be created on the drive before
        changes are written to disk. However, HDToolBox can be unreliable.

    EXAMPLE

        Partition ata.device 1 SYSSIZE 200 MAXWORK

    BUGS

    SEE ALSO
	
	Sys:System/Format, SFSFormat

******************************************************************************/

#include <utility/tagitem.h>
#include <libraries/partition.h>
#include <devices/trackdisk.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/partition.h>
#include <proto/utility.h>

#define DEBUG 0
#include <aros/debug.h>

#include "args.h"

#define MB (1024LL * 1024LL)
#define MIN_SIZE 50 /* Minimum disk space allowed in MBs */
#define MAX_FFS_SIZE (4L * 1024)
#define MAX_SFS_SIZE (124L * 1024)
#define MAX_SIZE(A) (((A) == &sfs0) ? MAX_SFS_SIZE : MAX_FFS_SIZE)

const TEXT version_string[] = "$VER: Partition 41.6 (15.10.2013)";

static const struct PartitionType dos3 = { "DOS\3", 4 };
#if AROS_BIG_ENDIAN
static const struct PartitionType sfs0 = { "SFS\0", 4 };
#else
/* atm, SFS is BE on AROS */
static const struct PartitionType sfs0 = { "SFS\0", 4 };
#endif


/*** Prototypes *************************************************************/
static VOID FindLargestGap(struct PartitionHandle *root, QUAD *gapLowBlock,
    QUAD *gapHighBlock);
static VOID CheckGap(struct PartitionHandle *root, QUAD partLimitBlock,
    QUAD *gapLowBlock, QUAD *gapHighBlock);
static struct PartitionHandle *CreateMBRPartition(
    struct PartitionHandle *parent, ULONG lowcyl, ULONG highcyl, UBYTE id);
static struct PartitionHandle *CreateRDBPartition(
    struct PartitionHandle *parent, ULONG lowcyl, ULONG highcyl,
    CONST_STRPTR name, BOOL bootable, const struct PartitionType *type);
static ULONG MBsToCylinders(ULONG size, struct DosEnvec *de);
static LONG RecurviseDestroyPartitions(struct PartitionHandle *root);

/*** Functions **************************************************************/
int main(void)
{
    struct PartitionHandle *diskPart = NULL, *sysPart, *workPart,
        *root = NULL, *partition, *extPartition = NULL, *parent = NULL;
    TEXT choice = 'N';
    CONST_STRPTR device;
    const struct PartitionType *sysType = NULL, *workType = NULL;
    LONG error = 0, sysSize = 0, workSize = 0, sysHighCyl, lowCyl, highCyl,
        reqHighCyl, unit, hasActive = FALSE;
    QUAD rootLowBlock = 1, rootHighBlock = 0, extLowBlock = 1,
        extHighBlock = 0, lowBlock, highBlock, rootBlocks, extBlocks;
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

        if (ARG(SYSTYPE) != (IPTR)NULL &&
            Stricmp((CONST_STRPTR)ARG(SYSTYPE), "FFSIntl") != 0 &&
            Stricmp((CONST_STRPTR)ARG(SYSTYPE), "SFS") != 0)
        {
            PutStr("ERROR: SYSTYPE must be either 'FFSIntl' or 'SFS'.\n");
            error = ERROR_REQUIRED_ARG_MISSING;
        }

        if (ARG(WORKTYPE) != (IPTR)NULL &&
            Stricmp((CONST_STRPTR)ARG(WORKTYPE), "FFSIntl") != 0 &&
            Stricmp((CONST_STRPTR)ARG(WORKTYPE), "SFS") != 0)
        {
            PutStr("ERROR: WORKTYPE must be either 'FFSIntl' or 'SFS'.\n");
            error = ERROR_REQUIRED_ARG_MISSING;
        }
    }

    if (error == 0)
    {
        /* Get DOSType for each partition */
        if (ARG(SYSTYPE) == (IPTR)NULL ||
            Stricmp((CONST_STRPTR)ARG(SYSTYPE), "SFS") != 0)
            sysType = &dos3;
        else
            sysType = &sfs0;

        if (ARG(WORKTYPE) == (IPTR)NULL ||
            Stricmp((CONST_STRPTR)ARG(WORKTYPE), "FFSIntl") != 0)
            workType = &sfs0;
        else
            workType = &dos3;

        device = (CONST_STRPTR) ARG(DEVICE);
        unit   = *(LONG *)ARG(UNIT);
        if (ARG(SYSSIZE) != (IPTR)NULL)
            sysSize = *(LONG *)ARG(SYSSIZE);
        if (ARG(WORKSIZE) != (IPTR)NULL)
            workSize = *(LONG *)ARG(WORKSIZE);

        D(bug("[C:Partition] Using %s, unit %ld\n", device, unit));
    
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
        D(bug("[C:Partition] Open root partition\n"));
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
        {
            D(bug("[C:Partition] WIPE partitions\n"));
            error = RecurviseDestroyPartitions(root);
        }
    }

    if (error == 0)
    {
        /* Open the existing partition table, or create it if none exists */
        D(bug("[C:Partition] Opening existing root partition table..."));
        if (OpenPartitionTable(root) != 0)
        {
            D(bug("failed\n[C:Partition] Creating %s root partition table\n",
                  ARG(RDB) ? "RDB" : "MBR"));

            ULONG type = ARG(RDB) ? PHPTT_RDB : PHPTT_MBR;
            /* Create a root partition table */
            if (CreatePartitionTable(root, type) != 0)
            {
                error = ERROR_UNKNOWN;
                PutStr("*** ERROR: Creating root partition table failed.\n");
                CloseRootPartition(root);
                root = NULL;
            }
        }
        else
            D(bug("ok\n"));
    }

    if (error == 0)
    {
        /* Find largest gap in root partition */
        FindLargestGap(root, &rootLowBlock, &rootHighBlock);

        if (!ARG(RDB))
        {
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

                if (!hasActive)
                    GetPartitionAttrsTags(partition,
                        PT_ACTIVE, (IPTR) &hasActive, TAG_DONE);

                partCount++;
            }

            /* Create extended partition if it doesn't exist */
            if (extPartition == NULL)
            {
                D(bug("[C:Partition] Creating EBR partition\n"));
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

        /* Convert block range to cylinders */
        lowCyl = (lowBlock - 1)
            / (LONG)(parent->de.de_Surfaces * parent->de.de_BlocksPerTrack) + 1;
        highCyl = (highBlock + 1)
            / (LONG)(parent->de.de_Surfaces * parent->de.de_BlocksPerTrack) - 1;

        /* Ensure neither partition is too large for its filesystem */
        if ((sysSize == 0 &&
            highCyl - lowCyl + 1 >
            MBsToCylinders(MAX_SIZE(sysType), &parent->de)) ||
            sysSize > MAX_SIZE(sysType))
            sysSize = MAX_SIZE(sysType);
        if ((ARG(MAXWORK) &&
            (highCyl - lowCyl + 1)
            - MBsToCylinders(sysSize, &parent->de) + 1 >
            MBsToCylinders(MAX_SIZE(workType), &parent->de)) ||
            workSize > MAX_SIZE(workType))
            workSize = MAX_SIZE(workType);

        /* Decide geometry for RDB virtual disk */
        reqHighCyl = lowCyl + MBsToCylinders(sysSize, &parent->de)
            + MBsToCylinders(workSize, &parent->de);
        if (sysSize != 0 && (workSize != 0 || !ARG(MAXWORK))
            && reqHighCyl < highCyl)
            highCyl = reqHighCyl;
        if (reqHighCyl > highCyl
            || (highCyl - lowCyl + 1 < MBsToCylinders(MIN_SIZE, &parent->de)
            && workSize == 0))
            error = ERROR_DISK_FULL;
    }

    if (error == 0 && !ARG(RDB))
    {
        /* Create RDB virtual disk */
        D(bug("[C:Partition] Creating RDB virtual disk\n"));
        diskPart = CreateMBRPartition(parent, lowCyl, highCyl, 0x30);
        if (diskPart == NULL)
        {
            PutStr("*** ERROR: Not enough disk space.\n");
            error = ERROR_UNKNOWN;
        }
    }

    if (error == 0 && !ARG(RDB))
    {
        /* Create RDB partition table inside virtual-disk partition */
        D(bug("[C:Partition] Creating RDB partition table\n"));
        error = CreatePartitionTable(diskPart, PHPTT_RDB);
        if (error != 0)
        {
            PutStr(
                "*** ERROR: Creating RDB partition table failed. Aborting.\n");
        }
    }

    if (error == 0)
    {
        if (ARG(RDB))
        {
            diskPart = parent;

            /* Create partitions in the RDB table */
            sysHighCyl = sysSize != 0
                        ? lowCyl + MBsToCylinders(sysSize, &diskPart->de)
                        : highCyl;

            /* Create System partition (defaults to FFSIntl) */
            sysPart = CreateRDBPartition(diskPart, lowCyl, sysHighCyl, (APTR)ARG(SYSNAME),
                TRUE, sysType);
            if (sysPart == NULL)
                error = ERROR_UNKNOWN;

            if (error == 0
                && workSize != 0
                && sysHighCyl < diskPart->de.de_HighCyl - diskPart->de.de_LowCyl)
            {
                /* Create Work partition (defaults to SFS) */
                workPart = CreateRDBPartition(diskPart, sysHighCyl + 1, highCyl,
                    (APTR)ARG(WORKNAME), FALSE, workType);
                if (workPart == NULL)
                    error = ERROR_UNKNOWN;
            }
        }
        else
        {
            /* Create partitions in the RDB table */
            sysHighCyl = MBsToCylinders(sysSize, &diskPart->de);

            /* Create System partition (defaults to FFSIntl) */
            sysPart = CreateRDBPartition(diskPart, 0, sysHighCyl, (APTR)ARG(SYSNAME),
                TRUE, sysType);
            if (sysPart == NULL)
                error = ERROR_UNKNOWN;

            if (sysSize != 0
                && sysHighCyl < diskPart->de.de_HighCyl - diskPart->de.de_LowCyl)
            {
                /* Create Work partition (defaults to SFS) */
                workPart = CreateRDBPartition(diskPart, sysHighCyl + 1, 0,
                    (APTR)ARG(WORKNAME), FALSE, workType);
                if (workPart == NULL)
                    error = ERROR_UNKNOWN;
            }
        }
    }

    if (error == 0)
    {
        /* If MBR has no active partition, make extended partition active to
           prevent broken BIOSes from treating disk as unbootable */
        if (!hasActive && !ARG(RDB))
            SetPartitionAttrsTags(extPartition, PT_ACTIVE, TRUE, TAG_DONE);

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

    if (!ARG(QUIET) && error == 0) Printf("Partitioning completed\n");

    PrintFault(error, NULL);
    return (error == 0) ? RETURN_OK : RETURN_FAIL;
}

/* Find the low and high cylinder values for the largest gap on the disk */
static VOID FindLargestGap(struct PartitionHandle *root, QUAD *gapLowBlock,
    QUAD *gapHighBlock)
{
    struct PartitionHandle *partition;
    LONG reservedBlocks = 0;
    QUAD partLimitBlock;

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
static VOID CheckGap(struct PartitionHandle *root, QUAD partLimitBlock,
    QUAD *gapLowBlock, QUAD *gapHighBlock)
{
    struct PartitionHandle *nextPartition;
    QUAD lowBlock, nextLowBlock;

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
    ULONG maxcyl;
    
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

    maxcyl = parentDE.de_HighCyl - parentDE.de_LowCyl;
    if (highcyl == 0 || highcyl > maxcyl)
    {
        highcyl = maxcyl;
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
    CONST_STRPTR name, BOOL bootable, const struct PartitionType *type
)
{
    struct DriveGeometry    parentDG    = {0};
    struct DosEnvec         parentDE    = {0},
                            partitionDE = {0};
    struct PartitionHandle *partition;
    ULONG maxcyl;
        
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
    
    maxcyl = parentDE.de_HighCyl - parentDE.de_LowCyl;
    if (highcyl == 0 || highcyl > maxcyl)
    {
        highcyl = maxcyl;
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

    D(bug("[C:Partition] SizeBlock %ld\n", partitionDE.de_SizeBlock ));
    D(bug("[C:Partition] Surfaces %ld\n", partitionDE.de_Surfaces));
    D(bug("[C:Partition] BlocksPerTrack %ld\n", partitionDE.de_BlocksPerTrack));
    D(bug("[C:Partition] BufMemType %ld\n", partitionDE.de_BufMemType));
    D(bug("[C:Partition] TableSize %ld\n", partitionDE.de_TableSize));
    D(bug("[C:Partition] Reserved %ld\n", partitionDE.de_Reserved));
    D(bug("[C:Partition] HighCyl %ld\n", partitionDE.de_HighCyl));
    D(bug("[C:Partition] LowCyl %ld\n", partitionDE.de_LowCyl));

    partition = AddPartitionTags
    (
        parent,
        
        PT_DOSENVEC, (IPTR) &partitionDE,
        PT_TYPE,     (IPTR) type,
        PT_NAME,     (IPTR) name,
        PT_BOOTABLE,        bootable,
        PT_AUTOMOUNT,       TRUE,
        
        TAG_DONE
    );
    
    return partition;
}

/* Convert a size in megabytes into a cylinder count. The figure returned
   is rounded down to avoid breaching maximum filesystem sizes. */
static ULONG MBsToCylinders(ULONG size, struct DosEnvec *de)
{
    UQUAD bytes;
    ULONG cyls = 0;

    if (size != 0)
    {
        bytes = size * MB;
        cyls = bytes / (UQUAD)(de->de_SizeBlock * sizeof(ULONG))
            / de->de_BlocksPerTrack / de->de_Surfaces;
    }
    return cyls;
}

#define ZEROBUFFSIZE    4096

/* Go through whole partition tree and WIPE each and every entry */
static LONG RecurviseDestroyPartitions(struct PartitionHandle *root)
{
    LONG error = 0;

    if (root->table)
    {
        struct PartitionHandle *partition;
        ForeachNode(&root->table->list, partition)
        {
            if (OpenPartitionTable(partition) == 0)
            {
                error += RecurviseDestroyPartitions(partition);
            }
            else
            {
                APTR buffer = AllocMem(ZEROBUFFSIZE, MEMF_CLEAR);
                /* Damage first blocks of partition */
                WritePartitionDataQ(partition, buffer, ZEROBUFFSIZE, 0);
                FreeMem(buffer, ZEROBUFFSIZE);
            }
        }
    }

    error += DestroyPartitionTable(root);

    return error;
}
