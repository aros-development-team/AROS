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
#include <stdio.h>
#include <stdlib.h>

#define DEBUG 0
#include <aros/debug.h>

int main()
{
    struct PartitionHandle *root = NULL,
                           *rdbp = NULL;
    struct DosEnvec         tableDE,
                            partitionDE;
    struct DriveGeometry    tableDG;
    LONG                    rc;
    LONG                    reserved = 0;
    
    memset(&tableDG, 0, sizeof(struct DriveGeometry));
    
    /* Step 1: Destroy the existing partitiontable, if any exists. */
    root = OpenRootPartition("ide.device", 0);
    DestroyPartitionTable(root);
    CloseRootPartition(root);
    
    /* Step 2: Create a root RDB partition table. */
    root = OpenRootPartition("ide.device", 0);
    rc = CreatePartitionTable(root, PHPTT_RDB);
    if (rc != 0)
    {
        printf("*** ERROR: Creating partition table failed. Aborting.\n");
        exit(RETURN_FAIL); /* FIXME: take care of allocated resources... */
    }
    
    /* Step 3: Create a RDB partition in the table. */
    memset(&tableDE, 0, sizeof(struct DosEnvec));
    memset(&partitionDE, 0, sizeof(struct DosEnvec));
    
    GetPartitionAttrsTags
    (
        root, 
        
        PT_DOSENVEC, &tableDE, 
        PT_GEOMETRY, &tableDG,
        
        TAG_DONE
    );
    GetPartitionTableAttrsTags(root, PTT_RESERVED, (IPTR) &reserved, TAG_DONE);
    
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
    partitionDE.de_DosType        = ID_FFS_DISK;
    
    D(bug("* highcyl = %ld\n", partitionDE.de_HighCyl));
    D(bug("* lowcyl = %ld\n", partitionDE.de_LowCyl));
    D(bug("* table lowcyl = %ld\n", tableDE.de_LowCyl));
    D(bug("* table reserved = %ld\n", reserved));
    
    rdbp = AddPartitionTags
    (
        root,
        
        PT_DOSENVEC, (IPTR) &partitionDE,
        PT_NAME,     (IPTR) "DH0",
        PT_BOOTABLE,        TRUE,
        PT_AUTOMOUNT,       TRUE,
        
        TAG_DONE
    );
    
    WritePartitionTable(root);
    ClosePartitionTable(root);
    CloseRootPartition(root);

    return RETURN_OK;
}
