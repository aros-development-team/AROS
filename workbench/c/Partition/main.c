/*
    Copyright © 2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <utility/tagitem.h>
#include <libraries/partition.h>
#include <devices/trackdisk.h>

#include <proto/exec.h>
#include <proto/partition.h>

#include <string.h>
#include <stdlib.h>

#define DEBUG 0
#include <aros/debug.h>

#define SH_GLOBAL_SYSBASE TRUE
#define SH_GLOBAL_DOSBASE TRUE
#include <aros/shcommands.h>

AROS_SH2
(
    Partition, 0.1,
    AROS_SHA(BOOL, ,   FORCE, /S,   FALSE),
    AROS_SHA(BOOL, ,   QUIET, /S,   FALSE)
)
{
    AROS_SHCOMMAND_INIT
    
    struct Library         *PartitionBase = NULL;
    struct PartitionHandle *root    = NULL, /* root partition */
                           *mbrp    = NULL, /* MBR partition */
                           *rdbp    = NULL; /* RDB partition */
    struct PartitionType    mbrtype = {{0x30},  1},
                            rdbtype = {"DOS\1", 4};
    struct DosEnvec         tableDE,
                            partitionDE;
    struct DriveGeometry    tableDG;
    LONG                    rc;
    LONG                    reserved = 0;
    TEXT                    choice = 'N';
    
    if (SHArg(QUIET) && !SHArg(FORCE))
    {
        PutStr("ERROR: Cannot specify QUIET without FORCE.\n");
        return RETURN_FAIL;
    }
    
    PartitionBase = OpenLibrary("partition.library", 0);
    if (PartitionBase == NULL)
    {
        PutStr("ERROR: Could not open partition.library.\n");
        return RETURN_FAIL;
    }
    
    if (!SHArg(FORCE))
    {
        Printf("About to partition ide.device unit 0.\n");
        Printf("This will DESTROY ALL DATA on the drive!\n");
        Printf("Are you sure? (y/N)"); Flush(Output());
    
        Read(Input(), &choice, 1);
    }
    else
    {
        choice = 'y';
    }

    if (choice != 'y' && choice != 'Y') return RETURN_OK;
    
    if (!SHArg(QUIET))
    {
        Printf("Partitioning drive...");
        Flush(Output());
    }

    memset(&tableDG, 0, sizeof(struct DriveGeometry));
    
    /* Step 1: Destroy the existing partitiontable, if any exists */
    root = OpenRootPartition("ide.device", 0);
    DestroyPartitionTable(root);
    CloseRootPartition(root);
    
    /* Step 2: Create a root MBR partition table */
    root = OpenRootPartition("ide.device", 0);
    rc = CreatePartitionTable(root, PHPTT_MBR);
    if (rc != 0)
    {
        PutStr("*** ERROR: Creating partition table failed. Aborting.\n");
        return RETURN_FAIL; /* FIXME: take care of allocated resources... */
    }
    
    /* Step 3: Create a partition in the MBR table */
    memset(&tableDE, 0, sizeof(struct DosEnvec));
    memset(&partitionDE, 0, sizeof(struct DosEnvec));
    
    GetPartitionAttrsTags
    (
        root, 
        
        PT_DOSENVEC, (IPTR) &tableDE, 
        PT_GEOMETRY, (IPTR) &tableDG,
        
        TAG_DONE
    );
    GetPartitionTableAttrsTags(root, PTT_RESERVED, (IPTR) &reserved, TAG_DONE);
    
    CopyMem(&tableDE, &partitionDE, sizeof(struct DosEnvec));
    
    partitionDE.de_SizeBlock      = tableDG.dg_SectorSize >> 2;
    partitionDE.de_Reserved       = 2;
    partitionDE.de_HighCyl        = tableDE.de_HighCyl;
    partitionDE.de_LowCyl         = reserved 
                                  / (tableDG.dg_Heads * tableDG.dg_TrackSectors) 
                                  + 1;
    
    D(bug("* highcyl = %ld\n", partitionDE.de_HighCyl));
    D(bug("* lowcyl = %ld\n", partitionDE.de_LowCyl));
    D(bug("* table lowcyl = %ld\n", tableDE.de_LowCyl));
    D(bug("* table reserved = %ld\n", reserved));
        
    mbrp = AddPartitionTags
    (
        root,
        
        PT_DOSENVEC, (IPTR) &partitionDE,
        PT_TYPE,     (IPTR) &mbrtype,
        PT_POSITION,        0,
        PT_ACTIVE,          TRUE,
        
        TAG_DONE
    );
    
    /* Step 4: Create RDB partition table inside MBR partition */
    rc = CreatePartitionTable(mbrp, PHPTT_RDB);
    if (rc != 0)
    {
        PutStr("*** ERROR: Creating partition table failed. Aborting.\n");
        return RETURN_FAIL; /* FIXME: take care of allocated resources... */
    }
    
    /* Step 5: Create a partition in the RDB table */
    memset(&tableDE, 0, sizeof(struct DosEnvec));
    memset(&partitionDE, 0, sizeof(struct DosEnvec));
    
    GetPartitionAttrsTags
    (
        mbrp, 
        
        PT_DOSENVEC, (IPTR) &tableDE, 
        PT_GEOMETRY, (IPTR) &tableDG,
        
        TAG_DONE
    );
    GetPartitionTableAttrsTags(mbrp, PTT_RESERVED, (IPTR) &reserved, TAG_DONE);
    
    CopyMem(&tableDE, &partitionDE, sizeof(struct DosEnvec));
    
    partitionDE.de_SizeBlock      = tableDG.dg_SectorSize >> 2;
    partitionDE.de_Surfaces       = tableDG.dg_Heads;
    partitionDE.de_BlocksPerTrack = tableDG.dg_TrackSectors;
    partitionDE.de_BufMemType     = tableDG.dg_BufMemType;
    partitionDE.de_TableSize      = DE_DOSTYPE;
    partitionDE.de_Reserved       = 2;
    partitionDE.de_HighCyl        = tableDE.de_HighCyl;
    partitionDE.de_LowCyl         = reserved 
                                  / (tableDG.dg_Heads * tableDG.dg_TrackSectors) 
                                  + 1;
    partitionDE.de_NumBuffers     = 100;
    partitionDE.de_MaxTransfer    = 0xFFFFFF;
    partitionDE.de_Mask           = 0xFFFFFFFE;
        
    D(bug("* highcyl = %ld\n", partitionDE.de_HighCyl));
    D(bug("* lowcyl = %ld\n", partitionDE.de_LowCyl));
    D(bug("* table lowcyl = %ld\n", tableDE.de_LowCyl));
    D(bug("* table reserved = %ld\n", reserved));
    
    rdbp = AddPartitionTags
    (
        mbrp,
        
        PT_DOSENVEC, (IPTR) &partitionDE,
        PT_TYPE,     (IPTR) &rdbtype,
        PT_NAME,     (IPTR) "DH0",
        PT_BOOTABLE,        TRUE,
        PT_AUTOMOUNT,       TRUE,
        
        TAG_DONE
    );
    
    /* Step 6: Save to disk and deallocate */
    WritePartitionTable(mbrp);
    ClosePartitionTable(mbrp);
    WritePartitionTable(root);
    ClosePartitionTable(root);
    CloseRootPartition(root);

    if (!SHArg(QUIET)) Printf("done\n");
    
    CloseLibrary(PartitionBase);
    
    return RETURN_OK;

    AROS_SHCOMMAND_EXIT
}
