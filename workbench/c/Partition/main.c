/*
    Copyright � 2003-2004, The AROS Development Team. All rights reserved.
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

#define DEBUG 1
#include <aros/debug.h>

#include "args.h"

const char		old_device[] = "ide.device";

/*** Prototypes *************************************************************/
struct PartitionHandle *CreateRootTable(CONST_STRPTR device, LONG unit);
struct PartitionHandle *CreateMBRPartition(struct PartitionHandle *parent, ULONG lowcyl, ULONG highcyl);
struct PartitionHandle *CreateRDBPartition(struct PartitionHandle *parent, ULONG lowcyl, ULONG highcyl, CONST_STRPTR name, BOOL bootable);

/*** Functions **************************************************************/
int main(void)
{
	struct PartitionHandle *root   = NULL;
	TEXT                    choice = 'N';

	char		*use_device = &old_device;
	ULONG	use_unit = 0;

	D(bug("[c:partition] Checking Arguments\n"));
    
	if (!ReadArguments()) return RETURN_FAIL;

	D(bug("[c:partition] Arguments read\n"));
    
	if (ARG(DEVICE)!=NULL)
	{
		IPTR		arg_rettmp=0;
		arg_rettmp = (IPTR)ARG(DEVICE);
		use_device = (char *)arg_rettmp;
	}
	
	D(bug("[c:partition] Using %s\n", use_device));
	
	if (ARG(UNIT)!=NULL)
	{
		IPTR		*arg_rettmp=NULL;
		arg_rettmp = (IPTR *)ARG(UNIT);
		use_unit = *arg_rettmp;
	}
    
	D(bug("[c:partition] Unit %d\n",use_unit));
    
	if (ARG(QUIET) && !ARG(FORCE))
	{
		PutStr("ERROR: Cannot specify QUIET without FORCE.\n");
		return RETURN_FAIL;
	}
    
    if (!ARG(FORCE))
    {
        Printf("About to partition %s unit %d.\n", use_device, use_unit);
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
    
    D(bug("[c:partition] About to partition drive..\n"));
    
    if ((root = CreateRootTable(use_device, use_unit)) != NULL)
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
                                    rdbp1highcyl = mbrhighcyl;
            
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
    }
    
    /* Save to disk and deallocate */
    WritePartitionTable(root);
    ClosePartitionTable(root);
    CloseRootPartition(root);

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
        
        lowcyl = reserved / (parentDG.dg_Heads * parentDG.dg_TrackSectors) + 1;
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
    struct PartitionType    type        = {"DOS\1", 4};
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
        
        lowcyl = reserved / (parentDG.dg_Heads * parentDG.dg_TrackSectors) + 1;
    }
    
    if (highcyl == 0)
    {
        highcyl = parentDE.de_HighCyl;
    }
    
    CopyMem(&parentDE, &partitionDE, sizeof(struct DosEnvec));
    
    partitionDE.de_SizeBlock      = parentDG.dg_SectorSize >> 2;
D(bug("[c:partition]  SizeBlock %d\n",partitionDE.de_SizeBlock ));
    partitionDE.de_Surfaces       = parentDG.dg_Heads;
D(bug("[c:partition]  Surfaces %d\n",partitionDE.de_Surfaces));
    partitionDE.de_BlocksPerTrack = parentDG.dg_TrackSectors;
D(bug("[c:partition]  BlocksPerTrack %d\n",partitionDE.de_BlocksPerTrack));
    partitionDE.de_BufMemType     = parentDG.dg_BufMemType;
D(bug("[c:partition]  BufMemType %d\n",partitionDE.de_BufMemType));
    partitionDE.de_TableSize      = DE_DOSTYPE;
D(bug("[c:partition] TableSize %d\n",partitionDE.de_TableSize));
    partitionDE.de_Reserved       = 2;
D(bug("[c:partition] Reserved %d\n",partitionDE.de_Reserved));
    partitionDE.de_HighCyl        = highcyl;
D(bug("[c:partition] HighCyl %d\n",partitionDE.de_HighCyl));
    partitionDE.de_LowCyl         = lowcyl;
D(bug("[c:partition] LowCyl %d\n",partitionDE.de_LowCyl));
    partitionDE.de_NumBuffers     = 100;
    partitionDE.de_MaxTransfer    = 0xFFFFFF;
    partitionDE.de_Mask           = 0xFFFFFFFE;
            
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
