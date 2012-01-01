/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

*/

#include <proto/utility.h>

#include "partition_support.h"
#include "platform.h"
#include "debug.h"

/*****************************************************************************

    NAME */
#include <utility/tagitem.h>
#include <libraries/partition.h>

   AROS_LH2(LONG, GetPartitionAttrs,

/*  SYNOPSIS */
   AROS_LHA(struct PartitionHandle *, ph,       A1),
   AROS_LHA(const struct TagItem *, taglist,    A2),

/*  LOCATION */
   struct Library *, PartitionBase, 15, Partition)

/*  FUNCTION
    get attributes of a partition

    INPUTS
    ph      - PartitionHandle
    taglist - list of attributes, unknown tags are ignored:

	PT_GEOMETRY   - struct DriveGeometry *	; Fill in DriveGeometry structure
        PT_DOSENVEC   - struct DosEnvec *	; Fill in DosEnvec structure
        PT_TYPE       - struct PartitionType *  ; Get partition type
        PT_POSITION   - ULONG *           	; Get position (entry number) of partition within its table.
        					; Returns -1 is there's no table (e. g. if used on disk root)
        PT_ACTIVE     - LONG *           	; Get value of "active" flag (PC-MBR specific)
        PT_BOOTABLE   - LONG *			; Get value of "bootable" flag
        PT_AUTOMOUNT  - LONG *			; Get value of "automount" flag
        PT_NAME       - STRPTR    		; Get name of partition (max 31 Bytes + NULL-byte)
        PT_STARTBLOCK - ULONG *			; Get number of starting block for the partition (V2)
        PT_ENDBLOCK   - ULONG *			; Get number of ending block for the partition (V2)

    RESULT
    	Currently reserved, always zero.

    NOTES
	Nested partition tables (e. g. RDB subpartitions on PC MBR drive) are treated as virtual disks.
	In this case start and end block numbers are relative to the beginning of the virtual disk
	(which is represented by parent partition containing the RDB itself), not absolute numbers.
	The same applies to DriveGeomerty and geometry-related fields in DosEnvec structure.

	Note that geometry data can be stored on disk in the partition table ifself (RDB for example), and
	this way it can not match physical device's geometry (for example, if the disk was partitioned on
	another operating system which used virtual geometry). In this case you might need to adjust these
	data in order to mount the file system correctly (if absolute start/end blocks are not
	cylinder-aligned).

	Starting from V2, partition.library always provides default values for all attributes, even for those
	not listed as readable in QueryPartitionAttrs() results.

    EXAMPLE

    BUGS

    SEE ALSO
    	SetPartitionAttrs()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    LONG (*getPartitionAttr)(struct Library *, struct PartitionHandle *, struct TagItem *) = NULL;

    struct TagItem *tag;

    if (ph->root)
    {
	struct PTFunctionTable *handler = ph->root->table->handler;

	getPartitionAttr = handler->getPartitionAttr;
    }

    while ((tag = NextTagItem((struct TagItem **)&taglist)))
    {
    	ULONG sup;

	/* If we have partition handler, call its function first */
        if (getPartitionAttr)
            sup = getPartitionAttr(PartitionBase, ph, tag);
        else
            sup = FALSE;

	if (!sup)
	{
	    struct PartitionHandle *list_ph = NULL;

	    /*
	     * No handler (root partition) or the handler didn't process the attribute.
	     * Return defaults.
	     */
            switch (tag->ti_Tag)
            {
            case PT_GEOMETRY:
                CopyMem(&ph->dg, (APTR)tag->ti_Data, sizeof(struct DriveGeometry));
                break;

            case PT_DOSENVEC:
                CopyMem(&ph->de, (APTR)tag->ti_Data, sizeof(struct DosEnvec));
                break;
                
	    case PT_TYPE:
	    	/* We have no type semantics */
		PTYPE(tag->ti_Data)->id_len = 0;
		break;

	    case PT_LEADIN:
	    case PT_ACTIVE:
	    case PT_BOOTABLE:
	    case PT_AUTOMOUNT:
	    	*((ULONG *)tag->ti_Data) = 0;
	    	break;

	    case PT_POSITION:
  		D(bug("[GetPartitionAttrs] PT_POSITION(0x%p)\n", ph));

		if (ph->root)
		{
	            ULONG i = 0;

		    D(bug("[GetPartitionAttrs] Parent table 0x%p\n", ph->root->table));

		    ForeachNode(&ph->root->table->list, list_ph)
		    {
		    	D(bug("[GetPartitionAttrs] Child handle 0x%p\n", list_ph));

	            	if (list_ph == ph)
        	    	{
			    *((ULONG *)tag->ti_Data) = i;
			    break;
			}
            	    	i++;
            	    }
            	}

		/* If nothing was found, return -1 (means "not applicable") */
            	if (list_ph != ph)
            	    *((ULONG *)tag->ti_Data) = -1;

            	break;

	    case PT_NAME:
	        if (ph->ln.ln_Name)
	        {
	            strncpy((STRPTR)tag->ti_Data, ph->ln.ln_Name, 31);
	            /* Make sure that name is NULL-terminated */
	            ((STRPTR)tag->ti_Data)[31] = 0;
	        }
	        else
	    	    ((STRPTR)tag->ti_Data)[0] = 0;
	    	break;

	    case PT_STARTBLOCK:
	    	*((ULONG *)tag->ti_Data) = ph->de.de_LowCyl * ph->de.de_Surfaces * ph->de.de_BlocksPerTrack;
	    	break;

	    case PT_ENDBLOCK:
	    	*((ULONG *)tag->ti_Data) = (ph->de.de_HighCyl + 1) * ph->de.de_Surfaces * ph->de.de_BlocksPerTrack - 1;
	    	break;
            }
        }
    }

    return 0;

    AROS_LIBFUNC_EXIT
}
