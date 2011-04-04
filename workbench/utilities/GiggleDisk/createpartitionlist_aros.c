
/*
** $Id$
**
** (c) 1998-2011 Guido Mersmann
** (c) 2011 The AROS Development Team.
*/

/*************************************************************************/

#define SOURCENAME "createpartitionlist_aros.c"
#define NODEBUG  /* turns off debug of this file */

#include <internal/debug.h>
#include <internal/memory.h>

#include <proto/exec.h>
#include <proto/partition.h>

#include "functions.h"
#include "header.h"
#include "mbr.h"
#include "partitionentry.h"
#include "rdb.h"
#include "readargs.h"
#include "readwrite.h"
#include "sprintf.h"
#include "version.h"

/*************************************************************************/

/* /// Create_Partition_List()
**
*/

/*************************************************************************/

ULONG Create_PartitionList( void )
{
    ULONG result = 0;
    
    if (!OpenPartitionTable(device.root))
    {
        struct PartitionTableHandler *table = device.root->table;
        struct PartitionHandle *part;

    	for (part = (struct PartitionHandle *)&table->list.lh_Head; part->ln.ln_Succ;
    	     part = (struct PartitionHandle *)part->ln.ln_Succ)
    	{
    	    struct PartitionEntry *pe;

    	    pe = Memory_AllocVec(sizeof(struct PartitionEntry));
	    if (pe)
	    {
	        struct PartitionType ptype;
	        ULONG automount = 0;
	        struct TagItem partTags[] =
	        {
	            {PT_AUTOMOUNT, (IPTR)&automount	},
    	    	    {PT_DOSENVEC , (IPTR)&pe->ENV	},
    	    	    {PT_NAME     , (IPTR)&pe->DriveName },
    	    	    {PT_TYPE	 , (IPTR)&ptype		},
    	    	    {TAG_DONE    , 0			}
    	    	};

		AddTail(&partitionlist, &pe->Node);

		ptype.id_len = 0;
		if (GetPartitionAttrs(part, partTags))
		{
		    result = MSG_ERROR_UnableToReadPartitionTable;
		    break;
		}

		switch (ptype.id_len)
		{
		case 1:
		    pe->PartitionType = ptype.id[0];
		    break;

		case 4:
		    pe->PartitionType = *(ULONG *)ptype.id;
		    break;

		default:
		    /* Sanity check */
		    result = MSG_ERROR_UnableToReadPartitionTable;
		    break;
		}

		pe->SectorSize	    = pe->ENV.de_SizeBlock << 2;
		pe->StartCylinder   = pe->ENV.de_LowCyl;
		pe->EndCylinder	    = pe->ENV.de_HighCyl;
		pe->FirstSector	    = pe->ENV.de_SecOrg;
		pe->NumberOfSectors = (pe->EndCylinder - pe->StartCylinder + 1) * pe->ENV.de_Surfaces * pe->ENV.de_BlocksPerTrack;
		pe->PartitionSize   = pe->NumberOfSectors * pe->SectorSize;
                pe->Device          = readargs_array[ARG_DEVICE];
                pe->Unit            = readargs_array[ARG_UNIT];
                pe->StackSize	    = 16384;
                pe->Flags	    = automount ? 0 : PBFF_NOMOUNT;
	    }
	    else
	    {
	    	result = MSG_ERROR_NotEnoughMemory;
	    	break;
	    }
    	}
    }

    if (!result && !(partitionlist.lh_Head->ln_Succ))
        result = MSG_ERROR_NothingUseful;

    return result;
}
/* \\\ */
