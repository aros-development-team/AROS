/*
    Copyright © 2003-2006, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <utility/tagitem.h>
#include <libraries/partition.h>
#include <devices/trackdisk.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/partition.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define DEBUG 0
#include <aros/debug.h>

#include "args.h"


/*** Prototypes *************************************************************/
struct PartitionHandle *CreateRootTable(CONST_STRPTR device, LONG unit);
struct PartitionHandle *CreateMBRPartition(struct PartitionHandle *parent, ULONG lowcyl, ULONG highcyl);
struct PartitionHandle *CreateRDBPartition(struct PartitionHandle *parent, ULONG lowcyl, ULONG highcyl, CONST_STRPTR name, BOOL bootable);

/*** Functions **************************************************************/
int main(void)
{
	struct PartitionHandle *root   = NULL;
	TEXT                    choice = 'N';
    CONST_STRPTR            device;
    ULONG                   unit;

    if (!ReadArguments())
    {
        PrintFault(IoErr(), NULL);
        return RETURN_FAIL;
    }

    device = (CONST_STRPTR) ARG(DEVICE);
    unit   = *(LONG *)ARG(UNIT);

    D(bug("[C:Partition] Using %s, unit %d\n", device, unit));
    
	if (ARG(QUIET) && !ARG(FORCE))
	{
		PutStr("ERROR: Cannot specify QUIET without FORCE.\n");
		return RETURN_FAIL;
	}
    
    if (!ARG(FORCE))
    {
        Printf("About to partition %s unit %d.\n", device, unit);
        Printf("This will DESTROY ALL DATA on the drive!\n");
        Printf("Are you sure? (y/N)"); Flush(Output());
    
        Read(Input(), &choice, 1);
    }
    else
    {
        choice = 'y';
    }

    if (choice != 'y' && choice != 'Y') return RETURN_OK;
    
    if (!ARG(QUIET))
    {
        Printf("Partitioning drive...");
        Flush(Output());
    }
    
    D(bug("[C:Partition] Partitioning drive...\n"));
    
    if ((root = CreateRootTable(device, unit)) != NULL)
    {
        CONST ULONG          TABLESIZE = 5 * 1024 * 1024;      
        CONST ULONG          DH0SIZE   = 1 * 1024 * 1024;
        struct DriveGeometry rootDG    = {0};
        ULONG                rootSize; /* Size in kB */
        
        GetPartitionAttrsTags(root, PT_GEOMETRY, (IPTR) &rootDG, TAG_DONE);
        rootSize = (rootDG.dg_TotalSectors / 1024) * rootDG.dg_SectorSize;

        /* See if the partition is 5 GB or larger */
        if (rootSize >= TABLESIZE)
        {
            /* 
                Only use the first 5 GB, first creating a 1 GB system
                partition (DH0) and then a 4 GB user partition (DH1).
            */
            
            struct PartitionHandle *mbrp    = NULL, /* MBR partition */
                                   *rdbp0   = NULL, /* First RDB partition */
                                   *rdbp1   = NULL; /* Second RDB partition */
            
            ULONG                   cylSize      = rootSize / rootDG.dg_Cylinders,
                                    mbrhighcyl   = (TABLESIZE / cylSize) - 1,
                                    rdbp0highcyl = (DH0SIZE / cylSize) - 1,
                                    rdbp1lowcyl  = rdbp0highcyl + 1,
                                    rdbp1highcyl;
            
            /* Create a partition in the MBR table */
            mbrp = CreateMBRPartition(root, 0, mbrhighcyl);
            
            /* Create RDB partition table inside MBR partition */
            if (CreatePartitionTable(mbrp, PHPTT_RDB) != 0)
            {
                PutStr("*** ERROR: Creating partition table failed. Aborting.\n");
                return RETURN_FAIL; /* FIXME: take care of allocated resources... */
            }
            
            /* Create a partition in the RDB table */
            rdbp0 = CreateRDBPartition(mbrp, 0, rdbp0highcyl, "DH0", TRUE); // FIXME: error check
            rdbp1highcyl = mbrhighcyl - mbrp->de.de_LowCyl;
            rdbp1 = CreateRDBPartition(mbrp, rdbp1lowcyl, rdbp1highcyl, "DH1", FALSE); // FIXME: error check
            
            /* Save to disk and deallocate */
            WritePartitionTable(mbrp);
            ClosePartitionTable(mbrp);
        }
        else
        {
            /* 
                Use the entire partition.
            */
            
            struct PartitionHandle *mbrp = NULL, /* MBR partition */
                                   *rdbp = NULL; /* RDB partition */
            
            /* Create a partition in the MBR table */
            mbrp = CreateMBRPartition(root, 0, 0);
            
            /* Create RDB partition table inside MBR partition */
            if (CreatePartitionTable(mbrp, PHPTT_RDB) != 0)
            {
                PutStr("*** ERROR: Creating partition table failed. Aborting.\n");
                return RETURN_FAIL; /* FIXME: take care of allocated resources... */
            }
            
            /* Create a partition in the RDB table */
            rdbp = CreateRDBPartition(mbrp, 0, 0, "DH0", TRUE); // FIXME: error check
            
            /* Save to disk and deallocate */
            WritePartitionTable(mbrp);
            ClosePartitionTable(mbrp);
        }
    
        /* Save to disk and deallocate */
        WritePartitionTable(root);
        ClosePartitionTable(root);
        CloseRootPartition(root);
    }

    if (!ARG(QUIET)) Printf("done\n");
        
    return RETURN_OK;
}

struct PartitionHandle *CreateRootTable(CONST_STRPTR device, LONG unit)
{
    struct PartitionHandle *root;
    
    if ((root = OpenRootPartition(device, unit)) != NULL)
    {
        /* Destroy the existing partitiontable, if any exists */
        DestroyPartitionTable(root);
        
        /* Create a root MBR partition table */
        if (CreatePartitionTable(root, PHPTT_MBR) != 0)       
        {
            PutStr("*** ERROR: Creating partition table failed.\n");
            CloseRootPartition(root);
            root = NULL;
        }
    }
    else
    {
        PutStr("*** Could not open root partition!\n");
    }
    
    return root;
}

struct PartitionHandle *CreateMBRPartition
(
    struct PartitionHandle *parent, ULONG lowcyl, ULONG highcyl
)
{
    struct DriveGeometry    parentDG    = {0};
    struct DosEnvec         parentDE    = {0},
                            partitionDE = {0};
    struct PartitionType    type        = {{0x30},  1}; /* AROS RDB */
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
        
        lowcyl = (reserved - 1) /
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
        PT_POSITION,        0,
        PT_ACTIVE,          TRUE,
        
        TAG_DONE
    );
    
    return partition;
}

struct PartitionHandle *CreateRDBPartition
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
